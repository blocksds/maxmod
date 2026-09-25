// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021-2026, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <maxmod_headless.h>
#include <mm_mas.h>
#include <mm_msl.h>

#include "core/effect.h"
#include "core/mas.h"
#include "core/mixer.h"
#include "core/player_types.h"
#include "headless/mixer.h"

// Address of soundbank in memory/rom
msl_head *mp_solution;

// Number of modules in sound bank
mm_word mmModuleCount;

// Number of samples in sound bank
mm_word mmSampleCount;

// Pointer to buffer allocated by mmInitDefault()
static mm_addr mm_init_default_buffer = NULL;

// This is set to true when Maxmod is initialized
static bool mm_initialized = false;

// Initialize maxmod
// TODO: Make this public
static bool mmInit(mm_headless_system *setup)
{
    mp_solution = setup->soundbank;

    mmSampleCount = mp_solution->head_data.sampleCount;
    mmModuleCount = mp_solution->head_data.moduleCount;

    mm_achannels = setup->active_channels;
    mm_pchannels = setup->module_channels;
    mm_num_mch = setup->mod_channel_count;
    mm_num_ach = setup->mix_channel_count;

    if ((mm_num_mch > 256) || (mm_num_ach > 256))
        return false;

    mmMixerInit(setup); // Initialize software mixer

    mm_ch_mask = (1U << mm_num_ach) - 1;

    mmSetModuleVolume(0x400);
    mmSetJingleVolume(0x400);
    mmSetEffectsVolume(0x400);

    mmSetModuleTempo(0x400);

    mmSetModulePitch(0x400);

    mmResetEffects();

    mm_initialized = true;

    return true;
}

bool mmInitDefault(mm_addr soundbank, mm_word number_of_channels, mm_word sample_rate)
{
    if (number_of_channels > 256)
        return false;

    // Allocate buffer
    size_t size_of_channel = sizeof(mm_module_channel) + sizeof(mm_active_channel) + sizeof(mm_mixer_channel);
    size_t size_of_buffer = number_of_channels * size_of_channel;

    mm_init_default_buffer = calloc(1, size_of_buffer);
    if (mm_init_default_buffer == NULL)
        return false;

    // Split up buffer
    mm_addr module_channels, active_channels, mixing_channels;

    module_channels = mm_init_default_buffer;
    active_channels = (mm_addr)(((uintptr_t)module_channels) + (number_of_channels * sizeof(mm_module_channel)));
    mixing_channels = (mm_addr)(((uintptr_t)active_channels) + (number_of_channels * sizeof(mm_active_channel)));

    mm_headless_system setup =
    {
        .sample_rate = sample_rate,
        .mod_channel_count = number_of_channels,
        .mix_channel_count = number_of_channels,
        .module_channels = module_channels,
        .active_channels = active_channels,
        .mixing_channels = mixing_channels,
        .soundbank = soundbank
    };

    if (!mmInit(&setup))
    {
        free(mm_init_default_buffer);
        return false;
    }

    return true;
}

bool mmEnd(void)
{
    mm_initialized = false;

    mmMixerEnd();

    if (mm_init_default_buffer)
    {
        free(mm_init_default_buffer);
        mm_init_default_buffer = NULL;
    }

    return true;
}

// This updates the player and writes "total_samples" to "buffer" (stereo)
void mmFrame(mm_addr buffer, mm_word total_samples)
{
    if (!mm_initialized)
        return;

    // Update effects

    mmUpdateEffects();

    // Update sub layer
    // Sub layer has 60hz accuracy

    // TODO: Ignore sub layer for now, we need to rework how GBA handles the sub
    // layer and do it the same way here.
    //mppUpdateSub();

    // Update main layer and mix samples.
    // Main layer is sample-accurate.

    // Copy channels
    mpp_channels = mm_pchannels;

    // Copy #channels
    mpp_nchannels = mm_num_mch;

    // layer=0 (main)
    mpp_clayer = MM_MAIN;

    // Copy layer pointer
    mpp_layerp = &mmLayerMain; // mpp_layerA

    // Check if main layer is active.
    // Skip processing if disabled (and just mix samples)
    if (mpp_layerp->isplaying == 0)
    {
        // Main layer isn't active, mix full amount
        mmMixerMix(buffer, total_samples);
        return;
    }

    mm_sword remaining_samples = total_samples;
    mm_byte *destination = buffer;

    while (1)
    {
        // Note that the tick rate may change in mppProcessTick()
        const mm_sword samples_per_tick = mpp_layerp->tickrate;

        mm_sword samples_to_next_tick = samples_per_tick - mpp_layerp->sampcount;

        if (samples_to_next_tick <= 0)
        {
            mppProcessTick();
            mpp_layerp->sampcount = 0;
            continue;
        }

        if (samples_to_next_tick > remaining_samples)
        {
            mmMixerMix(destination, remaining_samples);
            mpp_layerp->sampcount += remaining_samples;
            break;
        }

        mmMixerMix(destination, samples_to_next_tick);
        remaining_samples -=samples_to_next_tick;
        destination += samples_to_next_tick * 2;

        mpp_layerp->sampcount = 0;

        mppProcessTick();
    }
}

mm_word mmGetModuleCount(void)
{
    return mmModuleCount;
}

mm_word mmGetSampleCount(void)
{
    return mmSampleCount;
}
