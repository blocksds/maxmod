// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "maxmod9.h"
#include "mm_main9.h"
#include "mm_flusher.h"
#include "mp_defs.h"
#include "mp_format_mas.h"
#include "mm_mas.h"
#include "mm_comm_messages_ds.h"
#include "mm_comms.h"
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

#define EFFECT_CHANNELS 16
#define MAX_PARAM_WORDS 4
#define NO_HANDLES_AVAILABLE 0

// TODO: Perhaps it would be better to migrate to the number of bytes directly...???
static const mm_byte message_num_params[] = {
    0,
    3,
    3,
    0,
    0,
    0,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    0,
    0,
    1,
    0,
    3,
    3,
    0,
    0,
    3,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    0
};

static void SendSimpleExt(mm_hword, mm_byte, enum message_ids);
static void SendSimple(mm_word, enum message_ids);
static void SendString(mm_word*, int);
static mm_sfxhand mmValidateEffectHandle(mm_sfxhand);
static mm_sfxhand mmCreateEffectHandle(void);
static void mmReceiveMessage(uint32_t, void*);

// Fifo channel to use for communications
mm_word mmFifoChannel;

mm_word sfx_bitmask;

mm_byte sfx_instances[EFFECT_CHANNELS];

// ARM9 Communication Setup
void mmSetupComms(mm_word channel) {
    mmFifoChannel = channel;
    fifoSetValue32Handler(channel, mmReceiveMessage, 0);
}

// Send data via Value32...?, wrapper
static void SendSimpleExt(mm_hword data, mm_byte ext, enum message_ids msg_id) {
    SendSimple(data | (ext << 24), msg_id);
}

// Send data via Value32...? Apparently, it was switched to Datamsg???
static void SendSimple(mm_word data, enum message_ids msg_id) {
    mm_word buffer[MAX_PARAM_WORDS];
    mm_byte num_params = 0;
    
    // Determine number of parameters
    if(msg_id < sizeof(message_num_params))
        num_params = message_num_params[msg_id];
    
    // Store in buffer
    buffer[0] = (data << 16) | (msg_id << 8) | (num_params + 1);
    buffer[1] = data >> 24;
    
    // Send. If they're three bytes, do two transfers
    if(num_params > 2)
        SendString(buffer, 2);
    else
        SendString(buffer, 1);
}

// Send data via Datamsg
static void SendString(mm_word* values, int num_words) {
    fifoSendDatamsg(mmFifoChannel, num_words * sizeof(mm_word), (unsigned char*)values);
}

// Send a soundbank to ARM7
void mmSendBank(mm_word num_songs, mm_addr bank_addr) {
    mm_word buffer[MAX_PARAM_WORDS];
    
    buffer[0] = (num_songs << 16) | (MSG_BANK << 8) | (7);
    buffer[1] = (mm_word)bank_addr;
    
    SendString(buffer, 2);
}

// Lock channels to prevent use by maxmod
void mmLockChannels(mm_word bitmask) {
    mm_word buffer[MAX_PARAM_WORDS];
    
    buffer[0] = (bitmask << 24) | (1 << 16) | (MSG_SELCHAN << 8) | (6);
    buffer[1] = (bitmask >> 8);
    
    SendString(buffer, 2);
}

// Unlock channels to allow use by maxmod
void mmUnlockChannels(mm_word bitmask) {
    mm_word buffer[MAX_PARAM_WORDS];
    
    buffer[0] = (bitmask << 24) | (0 << 16) | (MSG_SELCHAN << 8) | (6);
    buffer[1] = (bitmask >> 8);
    
    SendString(buffer, 2);
}

// Start module playback
void mmStart(mm_word module_ID, mm_pmode mode) {
    mmActiveStatus = 1;
    SendSimpleExt(module_ID, mode, MSG_START);
}

// Pause module playback
void mmPause(void) {
    SendSimple(0, MSG_PAUSE);
}

// Resume module playback
void mmResume(void) {
    SendSimple(0, MSG_RESUME);
}

// Stop module playback
void mmStop(void) {
    SendSimple(0, MSG_STOP);
}

// Set playback position
void mmPosition(mm_word position) {
    SendSimple(position, MSG_POSITION);
}

// Start jingle
void mmJingle(mm_word module_ID) {
    SendSimple(module_ID, MSG_STARTSUB);
}

// Set module volume
void mmSetModuleVolume(mm_word vol) {
    SendSimple(vol, MSG_MASTERVOL);
}

// Set jingle volume
void mmSetJingleVolume(mm_word vol) {
    SendSimple(vol, MSG_MASTERVOLSUB);
}

// Set master tempo
void mmSetModuleTempo(mm_word tempo) {
    SendSimple(tempo, MSG_MASTERTEMPO);
}

// Set master pitch
void mmSetModulePitch(mm_word pitch) {
    SendSimple(pitch, MSG_MASTERPITCH);
}

// Set master effect volume
void mmSetEffectsVolume(mm_word vol) {
    SendSimple(vol, MSG_MASTEREFFECTVOL);
}

// Select audio mode
void mmSelectMode(mm_mode_enum mode) {
    SendSimple(mode, MSG_SELECTMODE);
}

// Open audio stream
void mmStreamBegin(mm_word wave, mm_hword clks, mm_hword len, mm_byte format) {
    mm_word buffer[MAX_PARAM_WORDS];
    
    buffer[0] = (wave << 16) | (MSG_OPENSTREAM << 8) | (10);
    buffer[1] = (wave >> 16) | (clks << 16);
    buffer[2] = len | (format << 16);
    
    SendString(buffer, 3);
}

// Close audio stream
void mmStreamEnd(void) {
    SendSimple(0, MSG_CLOSESTREAM);
}

// Returns same handle, or a newer valid handle
static mm_sfxhand mmValidateEffectHandle(mm_sfxhand handle) {
    mm_byte instance_num = (handle & 0xFF) - 1;

    if((instance_num < EFFECT_CHANNELS) && (sfx_instances[instance_num] == ((handle >> 8) & 0xFF)))
        return handle;
    
    return mmCreateEffectHandle();
}

// Return effect handle
// NO_HANDLES_AVAILABLE = no channels available
static mm_sfxhand mmCreateEffectHandle(void) {
    int i = 0;
    
    // Search for free channel
    for(; i < EFFECT_CHANNELS; i++)
        if(!((sfx_bitmask>>i) & 1))
            break;
    
    if(i >= EFFECT_CHANNELS)
        return NO_HANDLES_AVAILABLE;

    // Disable IRQ, cannot be interrupted by sfx update
    mm_hword old_ime = REG_IME;
    REG_IME = 0;
    
    // Set bit
    sfx_bitmask |= 1 << i;
    
    // Re-enable IRQ
    REG_IME = old_ime;

    sfx_instances[i] += 1;
    
    return (sfx_instances[i] << 8) | (i + 1);
}

// Play sound effect, default parameters
mm_sfxhand mmEffect(mm_word sample_ID) {
    mm_word buffer[MAX_PARAM_WORDS];

    mm_sfxhand handle = mmCreateEffectHandle();
    
    if(handle != NO_HANDLES_AVAILABLE) {
        buffer[0] = (sample_ID << 16) | (MSG_EFFECT << 8) | (5);
        buffer[1] = handle;
        
        SendString(buffer, 2);
    }

    return handle;
}

// Set effect volume
// volume should be a byte... :/
void mmEffectVolume(mm_sfxhand handle, mm_word volume) {
    SendSimpleExt(handle, volume, MSG_EFFECTVOL);
}

// Set effect panning
void mmEffectPanning(mm_sfxhand handle, mm_byte panning) {
    SendSimpleExt(handle, panning, MSG_EFFECTPAN);
}

// Set effect playback rate
void mmEffectRate(mm_sfxhand handle, mm_word rate) {
    mm_word buffer[MAX_PARAM_WORDS];
    
    buffer[0] = (handle << 16) | (MSG_EFFECTRATE << 8) | (5);
    buffer[1] = rate;
    
    SendString(buffer, 2);
}

// Scale effect playback rate by some factor
void mmEffectScaleRate(mm_sfxhand handle, mm_word factor) {
    mm_word buffer[MAX_PARAM_WORDS];
    
    buffer[0] = (handle << 16) | (MSG_EFFECTMULRATE << 8) | (5);
    buffer[1] = factor;
    
    SendString(buffer, 2);
}

// Release sound effect
void mmEffectRelease(mm_sfxhand handle) {
    mm_word buffer[MAX_PARAM_WORDS];
    
    buffer[0] = (handle << 24) | (1 << 16) | (MSG_EFFECTOPT << 8) | (4);
    buffer[1] = (handle >> 8);
    
    SendString(buffer, 2);
}

// Stop sound effect
void mmEffectCancel(mm_sfxhand handle) {
    mm_word buffer[MAX_PARAM_WORDS];
    
    buffer[0] = (handle << 24) | (0 << 16) | (MSG_EFFECTOPT << 8) | (4);
    buffer[1] = (handle >> 8);
    
    SendString(buffer, 2);
}

// Play sound effect, parameters supplied
mm_sfxhand mmEffectEx(mm_sound_effect* sound) {
    mm_word buffer[MAX_PARAM_WORDS];
    mm_sfxhand handle = sound->handle;

    if(handle == NO_HANDLES_AVAILABLE)
        handle = mmCreateEffectHandle();
    else
        handle = mmValidateEffectHandle(handle);

    if(handle == NO_HANDLES_AVAILABLE)
        return handle;
    
    buffer[0] = (((mm_word)sound->sample) << 16) | (MSG_EFFECTEX << 8) | (11);
    buffer[1] = (sound->rate << 16) | (((mm_word)sound->sample) >> 16);
    buffer[2] = (sound->panning << 24) | (sound->volume << 16) | (handle & 0xFFFF);
    
    SendString(buffer, 3);
    
    return handle;
}

// Cancel all sound effects
void mmEffectCancelAll(void) {
    SendSimple(0, MSG_EFFECTCANCELALL);
}

// Enable reverb system
void mmReverbEnable(void) {
    SendSimple(0, MSG_REVERBENABLE);
}

// Disable reverb system
void mmReverbDisable(void) {
    SendSimple(0, MSG_REVERBDISABLE);
}

// Configure reverb system
// TODO: Maybe migrate to a more easy-to-mantain config?
void mmReverbConfigure(mm_reverb_cfg* config) {
    mm_word buffer[MAX_PARAM_WORDS];
    mm_byte* buffer_byte = (mm_byte*)buffer;

    size_t bytes = 4;

    if(config->flags & MMRF_MEMORY) {
        for(size_t i = 0; i < sizeof(config->memory); i++)
            buffer_byte[bytes + i] = (((mm_word)config->memory) >> (8 * i)) & 0xFF;
        bytes += sizeof(config->memory);
    }

    if(config->flags & MMRF_DELAY) {
        for(size_t i = 0; i < sizeof(config->delay); i++)
            buffer_byte[bytes + i] = (config->delay >> (8 * i)) & 0xFF;
        bytes += sizeof(config->delay);
    }

    if(config->flags & MMRF_RATE) {
        for(size_t i = 0; i < sizeof(config->rate); i++)
            buffer_byte[bytes + i] = (config->rate >> (8 * i)) & 0xFF;
        bytes += sizeof(config->rate);
    }

    if(config->flags & MMRF_FEEDBACK) {
        for(size_t i = 0; i < sizeof(config->feedback); i++)
            buffer_byte[bytes + i] = (config->feedback >> (8 * i)) & 0xFF;
        bytes += sizeof(config->feedback);
    }

    if(config->flags & MMRF_PANNING) {
        for(size_t i = 0; i < sizeof(config->panning); i++)
            buffer_byte[bytes + i] = (config->panning >> (8 * i)) & 0xFF;
        bytes += sizeof(config->panning);
    }

    buffer[0] = (config->flags << 16) | (MSG_REVERBCFG << 8) | (bytes - 1);

    SendString(buffer, (bytes + 3) >> 2);
}

// Enable reverb output
void mmReverbStart(mm_reverbch channels) {
    SendSimple(channels, MSG_REVERBSTART);
}

// Disable reverb output
void mmReverbStop(mm_reverbch channels) {
    SendSimple(channels, MSG_REVERBSTOP);
}

// Default maxmod message receiving code
static void mmReceiveMessage(uint32_t value32, void* UNUSED(userdata)) {
    if((value32 >> 20) == 1) {
        if(mmCallback != NULL)
            mmCallback(value32 & 0xFF, (value32 >> 8) & 0xFF);
    }
    else {
        sfx_bitmask &= ~(value32 & 0xFFFF);
        mmActiveStatus = (value32 >> 16) & 1;
    }
}
