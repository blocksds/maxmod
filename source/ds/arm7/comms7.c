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
#include "core/format_mas.h"
#include "core/mas_structs.h"
#include "ds/arm7/comms7.h"
#include "ds/arm7/main7.h"
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

mm_word mmFifoChannel;

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

    if (!mmIsInitialized())
        mmProcessComms();
}

// Give ARM9 some data
mm_bool mmARM9msg(mm_byte cmd, mm_word value)
{
    return fifoSendValue32(mmFifoChannel, (cmd << 20) | value);
}

// Give ARM9 some data
mm_bool mmSendUpdateToARM9(void)
{
    uint32_t value = 0;

    // Signal that it's active
    if (mmLayerMain.isplaying)
        value = 1 << 16;

    value |= mm_sfx_clearmask;

    mm_sfx_clearmask = 0;

    return mmARM9msg(MSG_ARM7_UPDATE, value);
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

    switch (msg_id)
    {
        case MSG_BANK:
        {
            mm_word n_songs = ReadNFifoBytes(2);
            mm_addr bank = (mm_addr)ReadNFifoBytes(4);
            mm_word n_samples = ReadNFifoBytes(2);
            mmGetSoundBank(n_songs, n_samples, bank);
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
            mmStart(id, mode);
            break;
        }
        case MSG_PAUSE:
            mmPause();
            break;
        case MSG_RESUME:
            mmResume();
            break;
        case MSG_STOP:
            mmStop();
            break;
        case MSG_POSITION:
            mmPosition(ReadNFifoBytes(1));
            break;
        case MSG_STARTSUB:
            mmJingle(ReadNFifoBytes(2));
            break;
        case MSG_MASTERVOL:
            mmSetModuleVolume(ReadNFifoBytes(2));
            break;
        case MSG_MASTERVOLSUB:
            mmSetJingleVolume(ReadNFifoBytes(2));
            break;
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
            mm_sound_effect sfx;
            sfx.id = ReadNFifoBytes(2);
            sfx.rate = 0x400;
            sfx.handle = (mm_sfxhand)ReadNFifoBytes(2);
            sfx.volume = 0xFF;
            sfx.panning = 0x80;
            mmEffectEx(&sfx);
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
            mmEffectEx(&sfx);
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

            if (cfg.flags & MMRF_MEMORY)
                cfg.memory = (mm_addr)ReadNFifoBytes(4);

            if (cfg.flags & MMRF_DELAY)
                cfg.delay = ReadNFifoBytes(2);

            if (cfg.flags & MMRF_RATE)
                cfg.rate = ReadNFifoBytes(2);

            if (cfg.flags & MMRF_FEEDBACK)
                cfg.feedback = ReadNFifoBytes(2);

            if (cfg.flags & MMRF_PANNING)
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
