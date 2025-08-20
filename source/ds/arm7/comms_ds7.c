// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdint.h>

#include <nds.h>

#include <maxmod7.h>
#include <mm_mas.h>
#include <mm_types.h>

#include "core/effect.h"
#include "core/mas.h"
#include "core/player_types.h"
#include "ds/arm7/comms_ds7.h"
#include "ds/arm7/main_ds7.h"
#include "ds/common/comm_messages.h"
#include "ds/common/stream.h"

/***********************************************************************
 * Value32 format
 *
 * [cc] [mm] [bb] [aa]
 *
 * [mm] : ppmmmmmm, p = parameters, m = message type
 * [aa] : argument1, use if p >= 1
 * [bb] : argument2, use if p >= 2
 * [cc] : argument3, use if p == 3
 ***********************************************************************/

/***********************************************************************
 * Datamsg format
 *
 * First byte: Length of data
 * Following bytes: data
 ***********************************************************************/

#define FIFO_MAXMOD 7
#define FIFO_SIZE 256

#define MAX_DATAMSG_SIZE 32

#define NUM_MESSAGE_KINDS_BITS 6
#define NUM_MESSAGE_KINDS (1 << NUM_MESSAGE_KINDS_BITS)

#define MAX_NUM_PARAMS ((1 << (8 - NUM_MESSAGE_KINDS_BITS)) - 1)

// FIFO queue for messages from ARM9
mm_byte mmFifo[FIFO_SIZE];

// Read/Write queue positions in the FIFO
// These must be able to contain FIFO_SIZE
volatile mm_byte mmFifoPositionRQ;
volatile mm_byte mmFifoPositionWQ;

static mm_word mmFifoChannel;

static void mmReceiveDatamsg(int, void*);
static void ProcessNextMessage(void);

// ARM7 Communication Setup
ARM_CODE void mmSetupComms(mm_word channel)
{
    mmFifoChannel = channel;

    // Reset FIFO
    mmFifoPositionRQ = 0;
    mmFifoPositionWQ = 0;

    // setup datamsg handler first
    // (first to not miss INIT message!)
    fifoSetDatamsgHandler(channel, mmReceiveDatamsg, 0);
}

// Datamsg handler
static ARM_CODE void mmReceiveDatamsg(int bytes, void *userdata)
{
    (void)userdata;

    // Don't smash the stack!!!
    if (bytes > MAX_DATAMSG_SIZE)
        return;

    mm_byte datamsg_stack[MAX_DATAMSG_SIZE];

    fifoGetDatamsg(mmFifoChannel, bytes, datamsg_stack);

    int size = datamsg_stack[0];

    // Don't read bad data!!!
    if ((size > (MAX_DATAMSG_SIZE - 1)) || (size > (bytes - 1)))
        return;

    for (int i = 0; i < size; i++)
    {
        mmFifo[mmFifoPositionWQ++] = datamsg_stack[1 + i];
        mmFifoPositionWQ %= FIFO_SIZE;
    }

    // If Maxmod hasn't been fully initialized we need to handle received
    // messages to initialize it.
    //
    // If it has been initialized we need to be careful to not handle any
    // commands while Maxmod is working, so we can't handle them here, we handle
    // them in mmFrame() instead.
    if (!mmIsInitialized())
        mmProcessComms();
}

// Give ARM9 some data
mm_bool mmARM9msg(mm_byte cmd, mm_word value)
{
    return fifoSendValue32(mmFifoChannel, (cmd << 20) | value);
}

// Give ARM9 some data
void mmSendUpdateToARM9(void)
{
    uint32_t value = 0;

    // Send pattern and row, but not the tick. The tick changes too fast,
    // sometimes more than once in one frame. Rows and patterns don't usually
    // behave like that, only in some extreme situations.
    value |= (mmGetPositionRow() << 8) | mmGetPosition();

    // Tell the ARM9 if there is a module or a jingle playing
    if (mmLayerMain.isplaying)
        value |= 1 << 16;
    if (mmLayerSub.isplaying)
        value |= 1 << 17;

    mmARM9msg(MSG_ARM7_UPDATE, value);
}

static void mmSendHandleToARM9(mm_sfxhand handle)
{
    // We use the address handler to send SFX handles because the Value32
    // handler is already used (and it is used in interrupts).
    //
    // The address handler is unused and it's good enough to send a 16-bit
    // value like a SFX hnadle.
    fifoSendAddress(mmFifoChannel, (void *)(0x2000000 | handle));
}

// Process messages waiting in the fifo
void mmProcessComms(void)
{
    while (mmFifoPositionRQ != mmFifoPositionWQ)
        ProcessNextMessage();
}

// Read X bytes from the FIFO
static ARM_CODE mm_word ReadNFifoBytes(int n_bytes)
{
    mm_word value = 0;

    for (int i = 0; i < n_bytes; i++)
    {
        value |= mmFifo[mmFifoPositionRQ++] << (8 * i);
        mmFifoPositionRQ %= FIFO_SIZE;
    }

    return value;
}

// Do the actual processing with a switch
static ARM_CODE void ProcessNextMessage(void)
{
    enum mm_message_ids msg_id = (enum mm_message_ids)ReadNFifoBytes(1);

    // If Maxmod hasn't been initialized the only command allowed is MSG_BANK to
    // initialize Maxmod.
    if (!mmIsInitialized())
    {
        if (msg_id != MSG_BANK)
            libndsCrash("Maxmod: Bad init command");
    }

    switch (msg_id)
    {
        case MSG_BANK:
        {
            mm_word n_songs = ReadNFifoBytes(2);
            mm_addr bank = (mm_addr)ReadNFifoBytes(4);
            mm_word n_samples = ReadNFifoBytes(2);
            mmGetMemoryBank(n_songs, n_samples, bank);
            break;
        }
        case MSG_SELCHAN:
        {
            mm_byte lock = ReadNFifoBytes(1);
            mm_word mask = ReadNFifoBytes(4);
            if (lock)
                mmLockChannels(mask);
            else
                mmUnlockChannels(mask);
            break;
        }
        case MSG_START:
        {
            mm_word id = ReadNFifoBytes(2);
            mm_pmode mode = (mm_pmode)ReadNFifoBytes(1);
            mm_layer_type layer = ReadNFifoBytes(1);

            if (layer == MM_MAIN)
                mmStart(id, mode);
            else
                mmJingleStart(id, mode);

            break;
        }
        case MSG_PAUSE:
        {
            mm_layer_type layer = ReadNFifoBytes(1);

            if (layer == MM_MAIN)
                mmPause();
            else
                mmJinglePause();

            break;
        }
        case MSG_RESUME:
        {
            mm_layer_type layer = ReadNFifoBytes(1);

            if (layer == MM_MAIN)
                mmResume();
            else
                mmJingleResume();

            break;
        }
        case MSG_STOP:
        {
            mm_layer_type layer = ReadNFifoBytes(1);

            if (layer == MM_MAIN)
                mmStop();
            else
                mmJingleStop();

            break;
        }
        case MSG_POSITION:
        {
            mm_byte position = ReadNFifoBytes(1);
            mm_byte row = ReadNFifoBytes(1);

            mmSetPositionEx(position, row);
            break;
        }
        case MSG_MASTERVOL:
        {
            mm_hword volume = ReadNFifoBytes(2);
            mm_layer_type layer = ReadNFifoBytes(1);

            if (layer == MM_MAIN)
                mmSetModuleVolume(volume);
            else
                mmSetJingleVolume(volume);

            break;
        }
        case MSG_MASTERTEMPO:
            mmSetModuleTempo(ReadNFifoBytes(2));
            break;
        case MSG_MASTERPITCH:
            mmSetModulePitch(ReadNFifoBytes(2));
            break;
        case MSG_MASTEREFFECTVOL:
            mmSetEffectsVolume(ReadNFifoBytes(2));
            break;
        case MSG_OPENSTREAM:
        {
            mm_addr wavebuffer = (mm_addr)ReadNFifoBytes(4);
            mm_word clks = ReadNFifoBytes(2);
            mm_word len = ReadNFifoBytes(2);
            mm_stream_formats format = ReadNFifoBytes(1);
            mmStreamBegin(wavebuffer, clks, len, format);
            mmARM9msg(MSG_ARM7_STREAM_READY, 0);
            break;
        }
        case MSG_CLOSESTREAM:
        {
            mmStreamEnd();
            mmARM9msg(MSG_ARM7_STREAM_READY, 0);
            break;
        }
        case MSG_SELECTMODE:
            mmSelectMode((mm_mode_enum)ReadNFifoBytes(1));
            break;
        case MSG_EFFECT:
        {
            mm_sfxhand handle = mmEffect(ReadNFifoBytes(2));
            mmSendHandleToARM9(handle);
            break;
        }
        case MSG_EFFECTVOL:
        {
            mm_sfxhand handle = (mm_sfxhand)ReadNFifoBytes(2);
            mm_byte volume = ReadNFifoBytes(1);
            mmEffectVolume(handle, volume);
            break;
        }
        case MSG_EFFECTPAN:
        {
            mm_sfxhand handle = (mm_sfxhand)ReadNFifoBytes(2);
            mm_byte panning = ReadNFifoBytes(1);
            mmEffectPanning(handle, panning);
            break;
        }
        case MSG_EFFECTRATE:
        {
            mm_sfxhand handle = (mm_sfxhand)ReadNFifoBytes(2);
            mm_hword rate = ReadNFifoBytes(2);
            mmEffectRate(handle, rate);
            break;
        }
        case MSG_EFFECTMULRATE:
        {
            mm_sfxhand handle = (mm_sfxhand)ReadNFifoBytes(2);
            mm_hword factor = ReadNFifoBytes(2);
            mmEffectScaleRate(handle, factor);
            break;
        }
        case MSG_EFFECTOPT:
        {
            mm_word cmd = ReadNFifoBytes(1);
            mm_sfxhand handle = (mm_sfxhand)ReadNFifoBytes(2);

            if (cmd == 0)
                mmEffectCancel(handle);
            else if (cmd == 1)
                mmEffectRelease(handle);

            // Ignore other commands. The base module went into locking... Don't do it?!
            break;
        }
        case MSG_EFFECTEX:
        {
            mm_sound_effect sfx;
            sfx.sample = (mm_ds_sample *)ReadNFifoBytes(4);
            sfx.rate = ReadNFifoBytes(2);
            sfx.handle = (mm_sfxhand)ReadNFifoBytes(2);
            sfx.volume = ReadNFifoBytes(1);
            sfx.panning = ReadNFifoBytes(1);

            mm_sfxhand handle = mmEffectEx(&sfx);
            mmSendHandleToARM9(handle);
            break;
        }
        case MSG_REVERBENABLE:
            mmReverbEnable();
            break;
        case MSG_REVERBDISABLE:
            mmReverbDisable();
            break;
        case MSG_REVERBCFG:
        {
            mm_reverb_cfg cfg;
            cfg.flags = ReadNFifoBytes(2);
            cfg.memory = (mm_addr)ReadNFifoBytes(4);
            cfg.delay = ReadNFifoBytes(2);
            cfg.rate = ReadNFifoBytes(2);
            cfg.feedback = ReadNFifoBytes(2);
            cfg.panning = ReadNFifoBytes(1);

            mmReverbConfigure(&cfg);
            break;
        }
        case MSG_REVERBSTART:
            mmReverbStart((mm_reverbch)ReadNFifoBytes(1));
            break;
        case MSG_REVERBSTOP:
            mmReverbStop((mm_reverbch)ReadNFifoBytes(1));
            break;
        case MSG_EFFECTCANCELALL:
            mmEffectCancelAll();
            break;
        default:
            break;
    }
}
