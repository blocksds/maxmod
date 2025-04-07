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

mm_word mm_sfx_mastervolume; // 0 to 1024

// Struct that holds information about a sfx being played
typedef struct {
    mm_byte channel; // Value stored: channel index + 1 (0 means disabled)
    mm_byte counter; // Taken from mm_sfx_counter
} mm_sfx_channel_state;

mm_sfx_channel_state mm_sfx_channels[channelCount];

extern mm_word mm_sfx_bitmask; // Channels in use
extern mm_word mm_sfx_clearmask;

// Counter that increments every time a new effect is played
mm_byte mm_sfx_counter;

// Test handle and return mixing channel index
static int mme_get_channel_index(mm_sfxhand handle)
{
    mm_byte channel = handle & 0xFF;

    if (channel == 0)
        return -1;

    if (channel > channelCount)
        return -1;

    // Check if instances match

    mm_sfx_channel_state *state = &mm_sfx_channels[channel - 1];

    if (state->counter != (handle >> 8))
        return -1;

    return state->channel - 1;
}

// Clear channel entry and bitmask
static void mme_clear_channel(int channel)
{
    // Clear channel effect

    mm_sfx_channels[channel].counter = 0;
    mm_sfx_channels[channel].channel = 0;

    // Clear effect bitmask

    mm_word bit_flag = 1U << channel;

    mm_sfx_clearmask |= bit_flag;
    mm_sfx_bitmask &= ~bit_flag;
}

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

// Set effect panning (0..255)
void mmEffectPanning(mm_sfxhand handle, mm_byte panning)
{
    int channel = mme_get_channel_index(handle);

    if (channel < 0)
        return;

    mmMixerSetPan(channel, panning);
}

// Indicates if a sound effect is active or not
mm_bool mmEffectActive(mm_sfxhand handle)
{
    int channel = mme_get_channel_index(handle);

    if (channel < 0)
        return 0; // false

    return 1; // true
}

// Set effect volume (0..255)
void mmEffectVolume(mm_sfxhand handle, mm_word volume)
{
    int channel = mme_get_channel_index(handle);

    if (channel < 0)
        return;

#if defined(SYS_GBA)
    int shift = 10;
#elif defined(SYS_NDS)
    int shift = 2;
#endif

    volume = (volume * mm_sfx_mastervolume) >> shift;

    mmMixerSetVolume(channel, volume);
}

// Set effect playback rate
void mmEffectRate(mm_sfxhand handle, mm_word rate)
{
    int channel = mme_get_channel_index(handle);

    if (channel < 0)
        return;

    mmMixerSetFreq(channel, rate);
}

// Scale sampling rate by 6.10 factor
void mmEffectScaleRate(mm_sfxhand handle, mm_word factor)
{
    int channel = mme_get_channel_index(handle);

    if (channel < 0)
        return;

    mmMixerMulFreq(channel, factor);
}

// Stop sound effect
mm_word mmEffectCancel(mm_sfxhand handle)
{
    int channel = mme_get_channel_index(handle);

    if (channel < 0)
        return 0;

    // Free achannel
    mm_active_channel *ch = &mm_achannels[channel];

    ch->type = ACHN_BACKGROUND;
    ch->fvol = 0; // Clear volume for channel allocator

    mme_clear_channel((handle & 0xFF) - 1);

    mmMixerSetVolume(channel, 0); // Zero voice volume

    return 1;
}

// Release sound effect (allow interruption)
void mmEffectRelease(mm_sfxhand handle)
{
    int channel = mme_get_channel_index(handle);

    if (channel < 0)
        return;

    // Release achannel
    mm_active_channel *ch = &mm_achannels[channel];

    ch->type = ACHN_BACKGROUND;

    mme_clear_channel((handle & 0xFF) - 1);
}
