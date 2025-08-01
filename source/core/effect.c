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

#define channelCount    16
#define releaseLevel    200

static mm_word mm_sfx_mastervolume; // 0 to 1024

// Struct that holds information about a sfx being played
typedef struct {
    mm_byte channel; // Value stored: channel index + 1 (0 means disabled)
    mm_byte counter; // Taken from mm_sfx_counter
} mm_sfx_channel_state;

static mm_sfx_channel_state mm_sfx_channels[channelCount];

static mm_word mm_sfx_bitmask; // Channels in use
mm_word mm_sfx_clearmask;

// Counter that increments every time a new effect is played
static mm_byte mm_sfx_counter;

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

// Return index to free effect channel
static mm_word mmGetFreeEffectChannel(void)
{
    mm_word bitmask = mm_sfx_bitmask;

    // Look for the first clear bit
    for (mm_word i = 1; i < channelCount + 1; i++)
    {
        if ((bitmask & 1) == 0)
            return i;

        bitmask >>= 1;
    }

    // No handles available
    return 0;
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
mm_sfxhand mmEffectEx(mm_sound_effect* sound)
{
    if (sound->id >= mmGetSampleCount())
        return 0;

    // Test if handle was given

    mm_sfxhand handle = sound->handle;

    if (handle == 255)
    {
        handle = 0;
        goto got_handle;
    }

    // If there is a provided handle, reuse it
    if (handle != 0)
    {
        // Check if the channel is in use
        mm_word ch_idx = (handle - 1) & 0xFF;
        mm_byte channel = mm_sfx_channels[ch_idx].channel;

        // If no channel is assigned to this handle, we're done
        if (channel == 0)
            goto got_handle;

        // If there is a channel assigned to it, attempt to stop it
        if (mmEffectCancel(handle) == 0)
            goto got_handle;

        // If that fails, generate a new handle
    }

    // No provided handle, generate one
    handle = mmGetFreeEffectChannel();

    // If no available channels, try to continue. mmAllocChannel() will
    // deallocate a "released" channel if there is one available.
    if (handle == 0)
        goto got_handle;

    mm_sfx_counter++; // counter = ((counter + 1) & 255)
    handle |= ((mm_sfxhand)mm_sfx_counter) << 8;

got_handle:

    // allocate new channel
    mm_byte channel = mmAllocChannel();

    if (channel == 255)
        return 0; // Return error

    if (handle != 0)
    {
        // Register data

        mm_sfx_channel_state *channel_state;

        int ch_idx = (handle - 1) & 0xFF;

        channel_state = &mm_sfx_channels[ch_idx];

        channel_state->channel = channel + 1;
        channel_state->counter = handle >> 8;

        // set bit
        mm_sfx_bitmask |= 1U << ch_idx;
    }

    // setup channel
    mm_active_channel *act_ch = &mm_achannels[channel];

    act_ch->fvol = releaseLevel;

    if (handle == 0)
        act_ch->type = ACHN_BACKGROUND;
    else
        act_ch->type = ACHN_CUSTOM;

    act_ch->flags = MCAF_EFFECT;

    // Setup voice

#if defined(SYS_GBA)

    mm_mixer_channel *mix_ch = &mm_mixchannels[channel];

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

    mm_mixer_channel *mix_ch = &mm_mix_channels[channel];

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

        source += 8;
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
#elif defined(SYS_NDS7)
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
    mm_active_channel *act_ch = &mm_achannels[channel];

    act_ch->type = ACHN_BACKGROUND;
    act_ch->fvol = 0; // Clear volume for channel allocator

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
    mm_active_channel *act_ch = &mm_achannels[channel];

    act_ch->type = ACHN_BACKGROUND;

    mme_clear_channel((handle & 0xFF) - 1);
}

// Stop all sound effects
void mmEffectCancelAll(void)
{
    mm_word bitmask = mm_sfx_bitmask;

    for (int i = 0; bitmask != 0; bitmask >>= 1, i++)
    {
        if ((bitmask & 1) == 0)
            continue;

        int channel = ((int)mm_sfx_channels[i].channel) - 1;

        if (channel < 0)
            continue;

        mmMixerSetVolume(channel, 0);

        // Free achannel
        mm_active_channel *act_ch = &mm_achannels[channel];

        act_ch->type = ACHN_BACKGROUND;
        act_ch->fvol = 0;
    }

    mmResetEffects();
}

// Update sound effects
void mmUpdateEffects(void)
{
    mm_word bitmask = mm_sfx_bitmask;

    for (int i = 0; bitmask != 0; bitmask >>= 1, i++)
    {
        if ((bitmask & 1) == 0)
            continue;

        // Get channel index

        int channel = ((int)mm_sfx_channels[i].channel) - 1;

        if (channel < 0)
            continue;

        // Test if channel is still active

#if defined(SYS_GBA)
        mm_mixer_channel *mix_ch = &mm_mixchannels[channel];

        if ((mix_ch->src & (1U << 31)) == 0)
            continue;
#elif defined(SYS_NDS7)
        mm_mixer_channel *mix_ch = &mm_mix_channels[channel];

        if (mix_ch->samp)
            continue;
#endif

        // Free achanel if it isn't active

        mm_active_channel *act_ch = &mm_achannels[channel];

        act_ch->type = 0;
        act_ch->flags = 0;

        mm_sfx_channels[i].counter = 0;
        mm_sfx_channels[i].channel = 0;
    }

    mm_word new_bitmask = 0;

    mm_sfx_channel_state *channel_state = &mm_sfx_channels[0];

    for (mm_word flag = 1u << (32 - channelCount); flag != 0; flag <<= 1)
    {
        if (channel_state->channel != 0)
            new_bitmask |= flag;

        channel_state++;
    }

    new_bitmask >>= 32 - channelCount;

    // Bits that change from 1->0

    mm_word one_to_zero = (mm_sfx_bitmask ^ new_bitmask) & mm_sfx_bitmask;

    mm_sfx_bitmask = new_bitmask;

    // Write 1->0 mask

    mm_sfx_clearmask |= one_to_zero;
}
