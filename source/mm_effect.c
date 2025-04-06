// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#include "maxmod.h"

#include "mm_mas_arm.h"
#include "mm_mas.h"
#include "mm_mixer.h"
#include "mm_msl.h"
#include "mp_mas_structs.h"

#if defined(SYS_GBA)
#include "mm_main_gba.h"
#include "mm_mixer_gba.h"
#include "mp_mixer_gba.h"
#elif defined(SYS_NDS)
#include "mm_main_ds.h"
#include "mm_mixer_ds.h"
#include "mp_mixer_ds.h"
#endif

#define channelCount    16
#define releaseLevel    200

extern mm_word mm_sfx_mastervolume; // 0 to 1024

// Struct that holds information about a sfx being played
typedef struct {
    mm_byte channel; // Value stored: channel index + 1 (0 means disabled)
    mm_byte counter; // Taken from mm_sfx_counter
} mm_sfx_channel_state;

extern mm_sfx_channel_state mm_sfx_channels[channelCount];

extern mm_word mm_sfx_bitmask; // Channels in use
extern mm_word mm_sfx_clearmask;

// Counter that increments every time a new effect is played
extern mm_byte mm_sfx_counter;

void mmResetEffects(void)
{
    for (int i = 0; i < channelCount; i++)
    {
        mm_sfx_channels[i].counter = 0;
        mm_sfx_channels[i].channel = 0;
    }

    mm_sfx_bitmask = 0;
}

// Play sound effect with default parameters
mm_sfxhand mmEffect(mm_word sample_ID)
{
    mm_sound_effect effect =
    {
        .id = sample_ID,
        .rate = 1 << 10,
        .handle = 0,
        .volume = 255,
        .panning = 128
    };

    return mmEffectEx(&effect);
}

// Set master volume scale, 0->1024
void mmSetEffectsVolume(mm_word volume)
{
    if (volume > (1 << 10))
        volume = 1 << 10;

    mm_sfx_mastervolume = volume;
}
