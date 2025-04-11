// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdint.h>

#include <nds.h>

#include "maxmod7.h"
#include "mm_main7.h"
#include "mm_comms7.h"
#include "mm_main.h"
#include "mm_main_ds.h"
#include "mp_defs.h"
#include "mp_format_mas.h"
#include "mp_mas_structs.h"
#include "mm_effect.h"
#include "mm_mas.h"
#include "mm_stream.h"
#include "mm_comm_messages_ds.h"
#include "mm_types.h"

/*****************************************************************************************************************************

[value] represents a 1 byte value
[[value]] represents a 2 byte value
[[[value]]] represents a 3 byte value
[[[[value]]]] represents a 4 byte value
... represents data with a variable length

message table:
-----------------------------------------------------------------------------------------------------------------
message			size	parameters			desc
-----------------------------------------------------------------------------------------------------------------
0: BANK			6	[[#songs]] [[[mm_bank]]]	get sound bank
1: SELCHAN		4	[[bitmask]] [cmd]		select channels
2: START		4	[[id]] [mode] 			start module
3: PAUSE		1	---				pause module
4: RESUME		1	---				resume module
5: STOP			1	---				stop module
6: POSITION		2	[position]			set playback position
7: STARTSUB		3	[[id]]				start submodule
8: MASTERVOL		3	[[volume]]			set master volume
9: MASTERVOLSUB		3	[[volume]]			set master volume for sub module
A: MASTERTEMPO		3	[[tempo]]			set master tempo
B: MASTERPITCH		3	[[pitch]]			set master pitch
C: MASTEREFFECTVOL	3	[[volume]]			set master effect volume, bbaa= volume
D: OPENSTREAM		10	[[[[wave]]]] [[clks]] [[len]] [format]    open audio stream
E: CLOSESTREAM		1	---				close audio stream
F: SELECTMODE		2	[mode]				select audio mode

10: EFFECT		5	[[id]] [[handle]]		play effect, default params
11: EFFECTVOL		4	[[handle]] [volume]		set effect volume
12: EFFECTPAN		4	[[handle]] [panning]		set effect panning
13: EFFECTRATE		5	[[handle]] [[rate]]		set effect pitch
14: EFFECTMULRATE	5	[[handle]] [[factor]]		scale effect pitch
15: EFFECTOPT		4	[[handle]] [options]		set effect options
16: EFFECTEX		11	[[[[sample/id]]]] [[rate]] [[handle]] [vol] [pan] play effect, full params
17: ---			-	---				---

18: REVERBENABLE	1	---				enable reverb
19: REVERBDISABLE	1	---				disable reverb
1A: REVERBCFG		3..14	[[flags]] : [[[[memory]]]] [[delay]] [[rate]] [[feedback]] [panning]
1B: REVERBSTART		1	[channels]			start reverb
1C: REVERBSTOP		1	[channels]			stop reverb

1D: EFFECTCANCELALL	1	---				cancel all effects

1E->3F: Reserved
******************************************************************************************************************************/

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
    enum message_ids msg_id = (enum message_ids)ReadNFifoBytes(1);

    switch (msg_id)
    {
        case MSG_BANK:
        {
            mm_word n_songs = ReadNFifoBytes(2);
            mm_addr bank = (mm_addr)ReadNFifoBytes(4);
            mmGetSoundBank(n_songs, bank);
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
