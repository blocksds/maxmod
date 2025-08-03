// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#if defined(SYS_GBA)
#include <maxmod.h>
#elif defined(SYS_NDS7)
#include <maxmod7.h>
#endif

#include <mm_mas.h>
#include <mm_msl.h>

#include "core/effect.h"
#include "core/mas.h"
#include "core/mixer.h"
#include "core/mas_structs.h"
#if defined(SYS_GBA)
#include "gba/main_gba.h"
#include "gba/mixer.h"
#elif defined(SYS_NDS7)
#include "ds/arm7/main_ds7.h"
#include "ds/arm7/mixer.h"
#endif

#define releaseLevel    200

static mm_word mm_sfx_mastervolume; // 0 to 1024

// Struct that holds information about a sfx being played
typedef struct {
    mm_byte mix_channel; // mixer channel index + 1 (0 means disabled)
    mm_byte counter; // Taken from mm_sfx_counter
} mm_sfx_channel_state;

static mm_sfx_channel_state mm_sfx_channels[EFFECT_CHANNELS];

static mm_word mm_sfx_bitmask; // Channels in use

#ifdef SYS_NDS7
// The ARM9 has a variable called "sfx_bitmask" that keeps track of which
// channels are playing sound effects. This is done so that the ARM9 can create
// handles and allocate channels without the ARM7 being involved in it.
//
// In the ARM7, "mm_sfx_clearmask" is used to keep track of the channels that
// were playing sound effects but have stopped playing them in the last update.
// This variable is sent to the ARM9 every frame, and it is used to clear bits
// in "sfx_bitmask".
mm_word mm_sfx_clearmask;
#endif

// Counter that increments every time a new effect is played
static mm_byte mm_sfx_counter;

// Test handle and return mixing channel index
static int mme_get_mix_channel_index(mm_sfxhand handle)
{
    int sfx_channel = (handle & 0xFF) - 1;
    mm_byte handle_counter = handle >> 8;

    if (sfx_channel < 0)
        return -1;

    if (sfx_channel >= EFFECT_CHANNELS)
        return -1;

    mm_sfx_channel_state *state = &mm_sfx_channels[sfx_channel];

    // Check if counters match
    if (state->counter != handle_counter)
        return -1;

    return state->mix_channel - 1;
}

// Clear sfx channel entry and bitmask
static void mme_clear_sfx_channel(int sfx_channel)
{
    // Clear channel effect

    mm_sfx_channels[sfx_channel].counter = 0;
    mm_sfx_channels[sfx_channel].mix_channel = 0;

    // Clear effect bitmask

    mm_word bit_flag = 1U << sfx_channel;

#ifdef SYS_NDS7
    mm_sfx_clearmask |= bit_flag;
#endif
    mm_sfx_bitmask &= ~bit_flag;
}

// This is only meant to be used by Maxmod internally when initializing the
// sound engine.
void mmResetEffects(void)
{
    for (int i = 0; i < EFFECT_CHANNELS; i++)
    {
        mm_sfx_channels[i].counter = 0;
        mm_sfx_channels[i].mix_channel = 0;
    }

    mm_sfx_bitmask = 0;
}

// Return index to free effect channel. If no channels are free, it returns -1.
static int mme_get_free_sfx_channel(void)
{
    // Look for the first clear bit
    for (int i = 0; i < EFFECT_CHANNELS; i++)
    {
        if (mm_sfx_bitmask & (1 << i))
            continue;

        return i;
    }

    return -1;
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

// Play sound effect with specified parameters
mm_sfxhand mmEffectEx(mm_sound_effect *sound)
{
    if (sound->id >= mmGetSampleCount())
        return 0;

    int sfx_channel = -1;
    int mix_channel = NO_CHANNEL_AVAILABLE;
    mm_byte sfx_count;

    // Reuse or create new SFX handle
    // ------------------------------

    bool reused_handle = false;

    if (sound->handle != 0)
    {
        // If there is a provided handle, check if it's valid
        mix_channel = mme_get_mix_channel_index(sound->handle);
        if (mix_channel >= 0)
        {
            // It's valid, reuse the old mixer channel, as well as the sfx
            // channel and count.
            sfx_channel = (sound->handle & 0xFF) - 1;
            sfx_count = sound->handle >> 8;

            reused_handle = true;
        }
    }

    if (!reused_handle)
    {
        // No reused handle, generate a new one

        sfx_channel = mme_get_free_sfx_channel();
        if (sfx_channel < 0)
            return 0;

        // Allocate new mixer channel
        mix_channel = mmAllocChannel();
        if (mix_channel == NO_CHANNEL_AVAILABLE)
            return 0;

        sfx_count = mm_sfx_counter;

        mm_sfx_counter++;
    }

    // Generate new handle and register SFX information
    // ------------------------------------------------

    mm_sfxhand handle = (sfx_count << 8) | (sfx_channel + 1);

    mm_sfx_channels[sfx_channel].mix_channel = mix_channel + 1;
    mm_sfx_channels[sfx_channel].counter = sfx_count;

    // Mark sfx channel as used
    mm_sfx_bitmask |= 1U << sfx_channel;

    // Setup active channel
    // --------------------

    mm_active_channel *act_ch = &mm_achannels[mix_channel];

    act_ch->fvol = releaseLevel;

    if (handle == 0)
        act_ch->type = ACHN_BACKGROUND;
    else
        act_ch->type = ACHN_CUSTOM;

    act_ch->flags = MCAF_EFFECT;

    // Setup mixer channel
    // -------------------

#if defined(SYS_GBA)

    mm_mixer_channel *mix_ch = &mm_mixchannels[mix_channel];

    // Set sample data address

    msl_head *head = mp_solution;
    mm_word sample_offset = (mm_word)head->sampleTable[sound->id & 0xFFFF];
    mm_byte *sample_addr = ((mm_byte *)mp_solution) + sample_offset;
    mm_mas_gba_sample *sample = (mm_mas_gba_sample *)(sample_addr + sizeof(mm_mas_prefix));

    mix_ch->src = (mm_word)(&(sample->data[0]));

    // set pitch to original * pitch
    mix_ch->freq = (sound->rate * sample->default_frequency) >> (10 - 2);

    // reset read position
    mix_ch->read = 0;

    mix_ch->vol = (sound->volume * mm_sfx_mastervolume) >> 10;
    mix_ch->pan = sound->panning;

#elif defined(SYS_NDS7)

    mm_mixer_channel *mix_ch = &mm_mix_channels[mix_channel];

    // Set sample data address

    mm_word source = (mm_word)sound->sample;

    if (source < 0x10000) // If external sample, skip this
    {
        // This is using an ID number

        source = mmSampleBank[source];

        source &= 0xFFFFFF; // Mask out counter value

        if (source == 0)
        {
            mix_ch->key_on = 0;
            mix_ch->samp = 0;
            mix_ch->tpan = 0;
            return 0;
        }

        source += 8; // TODO: Remove magic number
        source += 0x2000000;
    }

    mix_ch->key_on = 0;
    mix_ch->samp = source;

    mm_mas_ds_sample *sample = (mm_mas_ds_sample *)source;

    // Set pitch to original * pitch
    mix_ch->freq = (sound->rate * sample->default_frequency) >> 10;

    // Clear sample offset
    mix_ch->read = 0;

    // Set panning | start bit
    mix_ch->tpan = sound->panning >> 1;
    mix_ch->key_on = 1;

    // Set volume
    mix_ch->vol = (sound->volume * mm_sfx_mastervolume) >> 2;

#endif

    return handle;
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
    int mix_channel = mme_get_mix_channel_index(handle);
    if (mix_channel < 0)
        return;

    mmMixerSetPan(mix_channel, panning);
}

// Indicates if a sound effect is active or not
mm_bool mmEffectActive(mm_sfxhand handle)
{
    int mix_channel = mme_get_mix_channel_index(handle);
    if (mix_channel < 0)
        return 0;

    return 1;
}

// Set effect volume (0..255)
void mmEffectVolume(mm_sfxhand handle, mm_word volume)
{
    int mix_channel = mme_get_mix_channel_index(handle);
    if (mix_channel < 0)
        return;

#if defined(SYS_GBA)
    int shift = 10;
#elif defined(SYS_NDS7)
    int shift = 2;
#endif

    volume = (volume * mm_sfx_mastervolume) >> shift;

    mmMixerSetVolume(mix_channel, volume);
}

// Set effect playback rate
void mmEffectRate(mm_sfxhand handle, mm_word rate)
{
    int mix_channel = mme_get_mix_channel_index(handle);
    if (mix_channel < 0)
        return;

    mmMixerSetFreq(mix_channel, rate);
}

// Scale sampling rate by 6.10 factor
void mmEffectScaleRate(mm_sfxhand handle, mm_word factor)
{
    int mix_channel = mme_get_mix_channel_index(handle);
    if (mix_channel < 0)
        return;

    mmMixerMulFreq(mix_channel, factor);
}

// Stop sound effect
mm_word mmEffectCancel(mm_sfxhand handle)
{
    int mix_channel = mme_get_mix_channel_index(handle);
    if (mix_channel < 0)
        return 0;

    // Free achannel
    mm_active_channel *act_ch = &mm_achannels[mix_channel];

    act_ch->type = ACHN_BACKGROUND;
    act_ch->fvol = 0; // Clear volume for channel allocator

    mm_word sfx_channel = (handle & 0xFF) - 1;
    mme_clear_sfx_channel(sfx_channel);

    mmMixerSetVolume(mix_channel, 0); // Zero voice volume

    return 1;
}

// Release sound effect (allow interruption)
void mmEffectRelease(mm_sfxhand handle)
{
    int mix_channel = mme_get_mix_channel_index(handle);
    if (mix_channel < 0)
        return;

    // Release achannel
    mm_active_channel *act_ch = &mm_achannels[mix_channel];

    act_ch->type = ACHN_BACKGROUND;

    mm_word sfx_channel = (handle & 0xFF) - 1;
    mme_clear_sfx_channel(sfx_channel);
}

// Stop all sound effects
void mmEffectCancelAll(void)
{
    for (int i = 0; i < EFFECT_CHANNELS; i++)
    {
        if ((mm_sfx_bitmask & (1 << i)) == 0)
            continue;

        int mix_channel = mm_sfx_channels[i].mix_channel - 1;
        if (mix_channel < 0)
            continue;

        mmMixerSetVolume(mix_channel, 0);

        // Free achannel
        mm_active_channel *act_ch = &mm_achannels[mix_channel];

        act_ch->type = ACHN_BACKGROUND;
        act_ch->fvol = 0;
    }

    mmResetEffects();
#ifdef SYS_NDS7
    mm_sfx_clearmask = ~0;
#endif
}

// Update sound effects
void mmUpdateEffects(void)
{
    // Keep track of the channels that are still active after the update
    mm_word new_bitmask = 0;

    for (int i = 0; i < EFFECT_CHANNELS; i++)
    {
        if ((mm_sfx_bitmask & (1 << i)) == 0)
            continue;

        // Get channel index

        int mix_channel = mm_sfx_channels[i].mix_channel - 1;
        if (mix_channel < 0)
            continue;

        // Test if channel is still active

#if defined(SYS_GBA)
        mm_mixer_channel *mix_ch = &mm_mixchannels[mix_channel];

        if ((mix_ch->src & (1U << 31)) == 0)
        {
            new_bitmask |= (1 << i);
            continue;
        }
#elif defined(SYS_NDS7)
        mm_mixer_channel *mix_ch = &mm_mix_channels[mix_channel];

        if (mix_ch->samp)
        {
            new_bitmask |= (1 << i);
            continue;
        }
#endif

        // Free achanel if it isn't active

        mm_active_channel *act_ch = &mm_achannels[mix_channel];

        act_ch->type = 0;
        act_ch->flags = 0;

        mm_sfx_channels[i].counter = 0;
        mm_sfx_channels[i].mix_channel = 0;
    }

#ifdef SYS_NDS7
    // Calculate bits that have just gone from 1 to 0 to send it to the ARM9
    mm_word one_to_zero = (mm_sfx_bitmask ^ new_bitmask) & mm_sfx_bitmask;
    mm_sfx_clearmask |= one_to_zero;
#endif

    // Update the mask of active channels
    mm_sfx_bitmask = new_bitmask;
}
