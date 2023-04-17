// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

/****************************************************************************
 *                                                          __              *
 *                ____ ___  ____ __  ______ ___  ____  ____/ /              *
 *               / __ `__ \/ __ `/ |/ / __ `__ \/ __ \/ __  /               *
 *              / / / / / / /_/ />  </ / / / / / /_/ / /_/ /                *
 *             /_/ /_/ /_/\__,_/_/|_/_/ /_/ /_/\____/\__,_/                 *
 *                                                                          *
 *               Nintendo DS & Gameboy Advance Sound System                 *
 *                                                                          *
 ****************************************************************************/

/******************************************************************************
 *
 * Definitions
 *
 ******************************************************************************/

#include "maxmod.h"
#include "mm_main.h"
#include "mp_mas_structs.h"
#include "mm_main_ds.h"
#include "mm_comms.h"
#include "mm_comms7.h"
#include "mm_mas.h"
#include "mm_effect.h"
#include "mp_defs.h"
#include "libnds_defs.h"

#if defined(SYS_GBA)
#include "mm_main_gba.h"
#include "mm_mixer_gba.h"
#elif defined(SYS_NDS)
#include "mm_main_ds.h"
#include "mm_mixer_ds.h"
#include "mm_mixer_super.h"
#endif

void mmInit7(void);
void mmInitialize(mm_bool);
mm_word mmEventForwarder(mm_word, mm_word);
void StopActiveChannel(mm_word);

#define NUM_CHANNELS 32

#define INITIALIZED_VALUE 42
#define BASE_VOLUME 0x400
#define BASE_TEMPO 0x400
#define BASE_PITCH 0x400
#define ALL_CHANNELS_UNLOCK 0xFFFF

#define TIMER_0_IRQ (1<<3)

extern mm_byte mm_mixing_mode;

mm_module_channel mm_rds_pchannels[NUM_CHANNELS];
mm_active_channel mm_rds_achannels[NUM_CHANNELS];

// Returns true if the system is ready for playback
mm_bool mmIsInitialized(void) {
    return mmInitialized == INITIALIZED_VALUE;
}

void mmInitialize(mm_bool do_initialize) {
    if(do_initialize)
        mmInitialized = INITIALIZED_VALUE;
    else
        mmInitialized = 0;
}

// Unlock audio channels so they can be used by the sequencer.
void mmUnlockChannels(mm_word mask) {
#ifdef SYS_NDS
    // Can NOT unlock channels in mode b
    if(mm_mixing_mode == 1)
        return;
#endif
    mm_ch_mask |= mask;
}

// Lock audio channels to prevent the sequencer from using them.
void mmLockChannels(mm_word mask) {
    mm_ch_mask &= ~mask;

    // It stops them, even if they were already locked
    for(int i = 0; i < NUM_CHANNELS; i++)
        if(mask & (1 << i))
            StopActiveChannel(i);
}

// Stop active channel
void StopActiveChannel(mm_word index) {
    mm_module_channel *channels;
    mm_word num_channels;

#ifdef SYS_NDS
    mm_mixer_channel *mix_ch = &mm_mix_channels[index];
#endif
#ifdef SYS_GBA
    mm_mixer_channel *mix_ch = &mm_mixchannels[index];
#endif

#ifdef SYS_GBA
    mix_ch->src = -1;
#endif
#ifdef SYS_NDS
    mix_ch->samp_cnt = 0;
    mix_ch->vol = 0;
    mix_ch->cvol = 0;
#endif

    *((mm_word*)(REG_SOUNDXCNT+index)) = 0;

    mm_active_channel *act_ch = &mm_achannels[index];
    mm_byte prev_flags = act_ch->flags;
    act_ch->flags = 0;
    act_ch->type = 0;
    
    if(prev_flags & MCAF_EFFECT)
        return;
    
    if(prev_flags & MCAF_SUB) {
        channels = &mm_schannels[0];
        num_channels = MP_SCHANNELS;
    }
    else {
        channels = mm_pchannels;
        num_channels = mm_num_mch;
    }

    // This was 99.9999% bugged, as it did not iterate, but just checked the same index.
    for(mm_word i = 0; i < num_channels; i++) {
        if(channels[i].alloc == index) {
            channels[i].alloc = 255;
            return;
        }
    }
}

// Install ARM7 system
void mmInstall(int fifo_channel) {
    mmInitialize(0);
    mmSetupComms(fifo_channel);
}

void mmInit7(void) {
    // WHAT IS THIS REFERENCING?!
    irqSet(TIMER_0_IRQ, mmFrame);
    irqEnable(TIMER_0_IRQ);
    
    // Init volumes
    mmSetModuleVolume(BASE_VOLUME);
    mmSetJingleVolume(BASE_VOLUME);
    mmSetEffectsVolume(BASE_VOLUME);
    
    mmInitialize(1);
    
    mmUnlockChannels(ALL_CHANNELS_UNLOCK);
    
    // Setup channel addresses
    mm_achannels = mm_rds_achannels;
    mm_pchannels = mm_rds_pchannels;
    mm_num_mch = NUM_CHANNELS;
    mm_num_ach = NUM_CHANNELS;
    
    mmSetModuleTempo(BASE_TEMPO);
    mmSetModulePitch(BASE_PITCH);
    
    mmResetEffects();
    
    // Setup Mixer
    mmMixerInit();
    
    // Forward events
    mmSetEventHandler(mmEventForwarder);

    // Why is this done twice?!
    mmInitialize(1);
}

void mmFrame(void) {
    if(mmIsInitialized()) {
        REG_IME = 1;
        mmUpdateEffects(); // update sound effects
        mmPulse(); // update module playback
        mmMixerMix(); // update audio
        mmSendUpdateToARM9();
    }
    mmProcessComms();
}

// Forward event to arm9
mm_word mmEventForwarder(mm_word msg, mm_word param) {
    return mmARM9msg(msg | (param << 8) | (1 << 20));
}

// Load sound bank address
void mmGetSoundBank(mm_word n_songs, mm_addr bank) {
    mmModuleCount = n_songs;
    mmModuleBank = bank;
    // What is this * 4 from? Is it a sizeof?!
    mmSampleBank = bank + (n_songs * 4);
    mmInit7();
}
