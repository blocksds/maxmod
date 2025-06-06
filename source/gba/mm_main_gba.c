// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#include "maxmod.h"

#include "mm_main.h"
#include "mm_mixer.h"
#include "mm_mas.h"
#include "mm_mas_arm.h"
#include "mm_effect.h"
#include "mp_mas_structs.h"

#include "mm_mixer_gba.h"

// Pointer to a user function to be called during the vblank irq
mm_voidfunc mm_vblank_function;

// Address of soundbank in memory/rom
mm_addr mp_solution;

// Initialize maxmod
void mmInit(mm_gba_system* setup)
{
    mp_solution = setup->soundbank;

    mm_achannels = setup->active_channels;
    mm_pchannels = (mm_addr)setup->module_channels;
    mm_num_mch = (mm_word)setup->mod_channel_count;
    mm_num_ach = (mm_word)setup->mix_channel_count;

    mmMixerInit(setup); // Initialize software/hardware mixer

    mm_ch_mask = (1U << mm_num_ach) - 1;

    mmSetModuleVolume(0x400);
    mmSetJingleVolume(0x400);
    mmSetEffectsVolume(0x400);

    mmSetModuleTempo(0x400);

    mmSetModulePitch(0x400);

    mmResetEffects();
}

// Set function to be called during the vblank IRQ
void mmSetVBlankHandler(mm_voidfunc function)
{
    mm_vblank_function = function;
}

// Work routine, user _must_ call this every frame.
void mmFrame(void)
{
    // Update effects

    mmUpdateEffects();

    // Update sub layer
    // Sub layer has 60hz accuracy

    mppUpdateSub();

    // Update main layer and mix samples.
    // Main layer is sample-accurate.

    // Copy channels
    mpp_channels = mm_pchannels;

    // Copy #channels
    mpp_nchannels = mm_num_mch;

    // layer=0 (main)
    mpp_clayer = 0;

    // Copy layer pointer
    mpp_layerp = &mmLayerMain; // mpp_layerA

    // Check if main layer is active.
    // Skip processing if disabled (and just mix samples)
    if (mpp_layerp->isplaying == 0)
    {
        // Main layer isn't active, mix full amount

        // PROF_START
        mmMixerMix(mm_mixlen);
        // PROF_END 0

        return;
    }

    // mixlen is divisible by 2
    int remaining_len = mm_mixlen;

    while (1)
    {
        // Get samples/tick
        int sample_num = mpp_layerp->tickrate;

        // Get sample count
        int sampcount = mpp_layerp->sampcount;

        // Calc tickrate-counter
        sample_num -= sampcount;

        if (sample_num < 0)
            sample_num = 0;

        if (sample_num >= remaining_len)
            break; // Mix remaining samples

        // Mix and process tick

        // Reset sample counter
        mpp_layerp->sampcount = 0;

        // subtract from #samples to mix
        remaining_len -= sample_num;

        // PROF_START
        mmMixerMix(sample_num); // mix samples
        // PROF_END 0

        mppProcessTick();
    }

    // Add samples remaining to SAMPCOUNT and mix more samples

    mpp_layerp->sampcount += remaining_len;

    // PROF_START
    mmMixerMix(remaining_len);
    // PROF_END 0
}
