// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021-2026, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_HEADLESS_MAIN_H
#define MM_HEADLESS_MAIN_H

#include <mm_msl.h>

// Headless setup information, passed to mmInit().
//
// This contains pointer to the internal structs used by Maxmod. It doesn't
// need to point to any temporary mixing buffer, only channel states and the
// soundbank being used.
typedef struct t_mmheadlesssystem
{
    // Software mixing rate. Higher values offer better quality at expense of a
    // larger CPU and memory load.
    mm_word     sample_rate;

    // This is the amount of module channels there will be. It must be greater
    // or equal to the largest channel number used by your modules (notice: NOT
    // virtual channel number).
    mm_word     mod_channel_count;

    // Number of mixing channels. Higher numbers offer better polyphony at
    // expense of larger memory footprint and CPU load.
    mm_word     mix_channel_count;

    // Pointer to module channel buffer, this can be placed in EWRAM. Size of
    // buffer must be ``MM_SIZEOF_MODCH * mod_channel_count`` bytes.
    mm_addr     module_channels;

    // Pointer to active channel buffer, this can be placed in EWRAM. Size of
    // buffer must be ``MM_SIZEOF_ACTCH * mix_channel_count`` bytes.
    mm_addr     active_channels;

    // Pointer to mixing channel buffer, this can be placed in EWRAM. Size of
    // buffer must be ``MM_SIZEOF_MIXCH * mix_channel_count`` bytes.
    mm_addr     mixing_channels;

    // Pointer to your soundbank file in RAM.
    mm_addr     soundbank;

} mm_headless_system;

// Address of soundbank in memory/rom
extern msl_head *mp_solution;

#endif // MM_HEADLESS_MAIN_H
