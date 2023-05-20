// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

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
#include "multiplatform_defs.h"
#include "useful_qualifiers.h"

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
mm_byte mmFifoPositionRQ;
mm_byte mmFifoPositionWQ;

mm_word mmFifoChannel;

static void mmReceiveValue32(uint32_t, void*);
static void mmReceiveDatamsg(int, void*);
static mm_word ReadNFifoBytes(mm_word*, int);
static void ProcessNextMessage(mm_word*);

// ARM7 Communication Setup
ARM_TARGET void mmSetupComms(mm_word channel) {
    mmFifoChannel = channel;
    
    // Reset FIFO
    mmFifoPositionRQ = 0;
    mmFifoPositionWQ = 0;
    
    // setup datamsg handler first
    // (first to not miss INIT message!)
    fifoSetDatamsgHandler(channel, mmReceiveDatamsg, 0);
    
    // setup value32 handler
    fifoSetValue32Handler(channel, mmReceiveValue32, 0);
}

// Default maxmod message Value32 receiving code
// Should be unused...?!
// It was also bugged because param3 was the message size as well!
static ARM_TARGET void mmReceiveValue32(uint32_t value32, void* UNUSED(userdata)) {    
    mm_byte message_type = (value32 >> 16) % NUM_MESSAGE_KINDS;
    
    // The mod operations aren't really needed since it's a byte,
    // but they can help change this in the future!
    mmFifo[mmFifoPositionWQ++] = message_type;
    mmFifoPositionWQ %= FIFO_SIZE;
    
    mm_byte num_parameters = (value32 >> (16 + NUM_MESSAGE_KINDS_BITS)) & MAX_NUM_PARAMS;
    
    for(int i = 0; i < num_parameters; i++) {
        // Edited with the if, the asm one was bugged
        if(i < 2)
            mmFifo[mmFifoPositionWQ++] = (value32 >> (i * 8)) & 0xFF;
        else
            mmFifo[mmFifoPositionWQ++] = (value32 >> ((i+1) * 8)) & 0xFF;
        mmFifoPositionWQ %= FIFO_SIZE;
    }
}

// Datamsg handler
static ARM_TARGET void mmReceiveDatamsg(int bytes, void* UNUSED(userdata)) {
    // Don't smash the stack!!!
    if(bytes > MAX_DATAMSG_SIZE)
        return;

    mm_byte datamsg_stack[MAX_DATAMSG_SIZE];

    fifoGetDatamsg(mmFifoChannel, bytes, datamsg_stack);
    
    int size = datamsg_stack[0];
    
    // Don't read bad data!!!
    if((size > (MAX_DATAMSG_SIZE - 1)) || (size > (bytes - 1)))
        return;
    
    for(int i = 0; i < size; i++) {
        mmFifo[mmFifoPositionWQ++] = datamsg_stack[1 + i];
        mmFifoPositionWQ %= FIFO_SIZE;
    }

    if(!mmIsInitialized())
        mmProcessComms();
}

// Give ARM9 some data
ARM_TARGET mm_bool mmSendUpdateToARM9(void) {
    uint32_t value = 0;
    
    // Signal that it's active
    if(mmLayerMain.isplaying)
        value = 1 << 16;
    
    value |= mm_sfx_clearmask;
    
    mm_sfx_clearmask = 0;
    
    return fifoSendValue32(mmFifoChannel, value);
}

// Give ARM9 some data
ARM_TARGET mm_bool mmARM9msg(mm_word value) {
    return fifoSendValue32(mmFifoChannel, value);
}

// Process messages waiting in the fifo
ARM_TARGET void mmProcessComms(void) {
    mmSuspendIRQ_t();
    
    mm_word curr_pos_r = mmFifoPositionRQ;
    mm_word curr_pos_w = mmFifoPositionWQ;
    
    mmFifoPositionRQ = mmFifoPositionWQ;
    
    mmRestoreIRQ_t();
    
    while(curr_pos_w != curr_pos_r)
        ProcessNextMessage(&curr_pos_r);
}

// Read X bytes from the FIFO
static ARM_TARGET mm_word ReadNFifoBytes(mm_word* curr_pos_r, int n_bytes) {
    mm_word value = 0;
    for(int i = 0; i < n_bytes; i++) {
        value |= mmFifo[*curr_pos_r] << (8 * i);
        *curr_pos_r = ((*curr_pos_r) + 1) % FIFO_SIZE;
    }
    return value;
}

// Do the actual processing with a switch
static ARM_TARGET void ProcessNextMessage(mm_word* curr_pos_r) {
    enum message_ids msg_id = (enum message_ids)ReadNFifoBytes(curr_pos_r, 1);
    mm_word tmp_cmd = 0;
    mm_sound_effect tmp_sfx;
    mm_reverb_cfg tmp_reverb_cfg;
    
    switch(msg_id) {
        case MSG_BANK:
            mmGetSoundBank(ReadNFifoBytes(curr_pos_r, 2), (mm_addr)ReadNFifoBytes(curr_pos_r, 4));
            break;
        case MSG_SELCHAN:
            if(ReadNFifoBytes(curr_pos_r, 1))
                mmLockChannels(ReadNFifoBytes(curr_pos_r, 4));
            else
                mmUnlockChannels(ReadNFifoBytes(curr_pos_r, 4));
            break;
        case MSG_START:
            mmStart(ReadNFifoBytes(curr_pos_r, 2), (mm_pmode)ReadNFifoBytes(curr_pos_r, 1));
            break;
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
            mmPosition(ReadNFifoBytes(curr_pos_r, 1));
            break;
        case MSG_STARTSUB:
            mmJingle(ReadNFifoBytes(curr_pos_r, 2));
            break;
        case MSG_MASTERVOL:
            mmSetModuleVolume(ReadNFifoBytes(curr_pos_r, 2));
            break;
        case MSG_MASTERVOLSUB:
            mmSetJingleVolume(ReadNFifoBytes(curr_pos_r, 2));
            break;
        case MSG_MASTERTEMPO:
            mmSetModuleTempo(ReadNFifoBytes(curr_pos_r, 2));
            break;
        case MSG_MASTERPITCH:
            mmSetModulePitch(ReadNFifoBytes(curr_pos_r, 2));
            break;
        case MSG_MASTEREFFECTVOL:
            mmSetEffectsVolume(ReadNFifoBytes(curr_pos_r, 2));
            break;
        case MSG_OPENSTREAM:
            mmStreamBegin(ReadNFifoBytes(curr_pos_r, 4), ReadNFifoBytes(curr_pos_r, 2), ReadNFifoBytes(curr_pos_r, 2), ReadNFifoBytes(curr_pos_r, 1));
            break;
        case MSG_CLOSESTREAM:
            mmStreamEnd();
            break;
        case MSG_SELECTMODE:
            mmSelectMode((mm_mode_enum)ReadNFifoBytes(curr_pos_r, 1));
            break;
        case MSG_EFFECT:
            tmp_sfx.id = ReadNFifoBytes(curr_pos_r, 2);
            tmp_sfx.rate = 0x400;
            tmp_sfx.handle = (mm_sfxhand)ReadNFifoBytes(curr_pos_r, 2);
            tmp_sfx.volume = 0xFF;
            tmp_sfx.panning = 0x80;
            mmEffectEx(&tmp_sfx);
            break;
        case MSG_EFFECTVOL:
            mmEffectVolume((mm_sfxhand)ReadNFifoBytes(curr_pos_r, 2), ReadNFifoBytes(curr_pos_r, 1));
            break;
        case MSG_EFFECTPAN:
            mmEffectPanning((mm_sfxhand)ReadNFifoBytes(curr_pos_r, 2), ReadNFifoBytes(curr_pos_r, 1));
            break;
        case MSG_EFFECTRATE:
            mmEffectRate((mm_sfxhand)ReadNFifoBytes(curr_pos_r, 2), ReadNFifoBytes(curr_pos_r, 2));
            break;
        case MSG_EFFECTMULRATE:
            mmEffectScaleRate((mm_sfxhand)ReadNFifoBytes(curr_pos_r, 2), ReadNFifoBytes(curr_pos_r, 2));
            break;
        case MSG_EFFECTOPT:
            tmp_cmd = ReadNFifoBytes(curr_pos_r, 1);
            if(tmp_cmd == 0)
                mmEffectCancel((mm_sfxhand)ReadNFifoBytes(curr_pos_r, 2));
            else if(tmp_cmd == 1)
                mmEffectRelease((mm_sfxhand)ReadNFifoBytes(curr_pos_r, 2));
            else
                // Ignore this!!! The base module went into locking... Don't do it?!
                ReadNFifoBytes(curr_pos_r, 2);
            break;
        case MSG_EFFECTEX:
            tmp_sfx.sample = (mm_ds_sample*)ReadNFifoBytes(curr_pos_r, 4);
            tmp_sfx.rate = ReadNFifoBytes(curr_pos_r, 2);
            tmp_sfx.handle = (mm_sfxhand)ReadNFifoBytes(curr_pos_r, 2);
            tmp_sfx.volume = ReadNFifoBytes(curr_pos_r, 1);
            tmp_sfx.panning = ReadNFifoBytes(curr_pos_r, 1);
            mmEffectEx(&tmp_sfx);
            break;
        case MSG_REVERBENABLE:
            mmReverbEnable();
            break;
        case MSG_REVERBDISABLE:
            mmReverbDisable();
            break;
        case MSG_REVERBCFG:
            tmp_reverb_cfg.flags = ReadNFifoBytes(curr_pos_r, 2);

            if(tmp_reverb_cfg.flags & MMRF_MEMORY)
                tmp_reverb_cfg.memory = (mm_addr)ReadNFifoBytes(curr_pos_r, 4);

            if(tmp_reverb_cfg.flags & MMRF_DELAY)
                tmp_reverb_cfg.delay = ReadNFifoBytes(curr_pos_r, 2);

            if(tmp_reverb_cfg.flags & MMRF_RATE)
                tmp_reverb_cfg.rate = ReadNFifoBytes(curr_pos_r, 2);

            if(tmp_reverb_cfg.flags & MMRF_FEEDBACK)
                tmp_reverb_cfg.feedback = ReadNFifoBytes(curr_pos_r, 2);

            if(tmp_reverb_cfg.flags & MMRF_PANNING)
                tmp_reverb_cfg.panning = ReadNFifoBytes(curr_pos_r, 1);

            mmReverbConfigure(&tmp_reverb_cfg);
            break;
        case MSG_REVERBSTART:
            mmReverbStart((mm_reverbch)ReadNFifoBytes(curr_pos_r, 1));
            break;
        case MSG_REVERBSTOP:
            mmReverbStop((mm_reverbch)ReadNFifoBytes(curr_pos_r, 1));
            break;
        case MSG_EFFECTCANCELALL:
            mmEffectCancelAll();
            break;
        default:
            break;
    }
}
