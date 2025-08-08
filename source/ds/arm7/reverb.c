// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <nds.h>

#include <maxmod7.h>

#include "ds/arm7/mixer.h"

/***********************************************************************
 * Reverb Parameters
 *-------------------------------------
 * MEMORY   output address  32-bit
 * DELAY    buffer length   16-bit
 * RATE     sampling rate   16-bit
 * FEEDBACK volume          11-bit
 * PANNING  panning          7-bit
 * DRY      output dry       1-bit
 * FORMAT   output format    1-bit
 *-------------------------------------
 *
 * 2-bit channel selection
 *
 *-------------------------------------
 *
 * Reverb output uses channels 1 and 3
 *
 ***********************************************************************/

#define REVERB_CHN_LEFT     1
#define REVERB_CHN_RIGHT    3
#define REVERB_CHN_MASK     ((1 << REVERB_CHN_LEFT) | (1 << REVERB_CHN_RIGHT))

static mm_word ReverbFlags;
static mm_byte ReverbNoDryLeft;
static mm_byte ReverbNoDryRight;
static mm_bool ReverbEnabled;
static mm_byte ReverbStarted;

// Set the reverb channel back to the default
static void SetReverbChannelToDefault(mm_byte channel)
{
    REG_SOUNDXCNT(channel) = SOUNDXCNT_VOL_MUL(127) | // Max
                             SOUNDXCNT_PAN(64) | // Center
                             SOUNDXCNT_REPEAT | SOUNDXCNT_FORMAT_16BIT;

    REG_SOUNDXSAD(channel) = 0;
    REG_SOUNDXTMR(channel) = 0xFE00;
    REG_SOUNDXPNT(channel) = 0;
    REG_SOUNDXLEN(channel) = 0;
}

// Disables/Resets sound channels, resets capture and clears flags.
static void ResetSoundAndCapture(void)
{
    SetReverbChannelToDefault(REVERB_CHN_LEFT);
    SetReverbChannelToDefault(REVERB_CHN_RIGHT);

    // Reset capture
    REG_SNDCAP0CNT = 0;
    REG_SNDCAP0DAD = 0;
    REG_SNDCAP0LEN = 0;

    REG_SNDCAP1CNT = 0;
    REG_SNDCAP1DAD = 0;
    REG_SNDCAP1LEN = 0;

    // Clear no dry and flags
    ReverbFlags = 0;
    ReverbNoDryLeft = 0;
    ReverbNoDryRight = 0;
}


// Enable Reverb System (lock reverb channels from sequencer)
void mmReverbEnable(void)
{
    // Catch if reverb is already enabled
    if (ReverbEnabled)
        return;

    // Set enabled flag
    ReverbEnabled = 1;

    // Lock reverb channels
    mmLockChannels(REVERB_CHN_MASK);

    ResetSoundAndCapture();
}

// Disable Reverb System
void mmReverbDisable(void)
{
    // Catch if reverb is already disabled
    if (!ReverbEnabled)
        return;

    // Set disabled flag
    ReverbEnabled = 0;

    ResetSoundAndCapture();

    // Unlock reverb channels
    mmLockChannels(REVERB_CHN_MASK);
}

void SetupChannel(mm_reverb_cfg* config, mm_byte channel)
{
    // Memory/SRC
    if (config->flags & MMRF_MEMORY)
        REG_SOUNDXSAD(channel) = (u32)(config->memory);

    // delay/LEN
    if (config->flags & MMRF_DELAY)
        REG_SOUNDXLEN(channel) = config->delay;

    // rate/TMR
    if (config->flags & MMRF_RATE)
        REG_SOUNDXTMR(channel) = -config->rate;

    // feedback/CNT:volume,shift
    if (config->flags & MMRF_FEEDBACK)
    {
        mm_word vol = mmVolumeDivTable[(config->feedback >> 7) & 0xF];
        mm_word shift = mmVolumeShiftTable[(config->feedback >> 7) & 0xF];

        REG_SOUNDXCNT(channel) &= 0xFFFF0000;
        REG_SOUNDXCNT(channel) |= (vol << 8) | (config->feedback >> shift);
    }

    // panning/CNT:panning
    if (config->flags & MMRF_PANNING)
        REG_SOUNDXPAN(channel) = config->panning;
}

// Setup SOUNDCNT with dry settings
void CopyDrySettings(void)
{
    // Catch if reverb has not started
    if (!ReverbStarted)
        return;

    mm_hword mixer_info = REG_SOUNDCNT;

    // Clear source bits
    mixer_info &= ~(SOUNDCNT_LEFT_OUTPUT_MASK | SOUNDCNT_RIGHT_OUTPUT_MASK);

    if (ReverbNoDryLeft)
        mixer_info |= SOUNDCNT_LEFT_OUTPUT_CH1; // Left output = channel 1
    if (ReverbNoDryRight)
        mixer_info |= SOUNDCNT_RIGHT_OUTPUT_CH3; // Right output = channel 3

    REG_SOUNDCNT = mixer_info;
}

// Configure reverb system
void mmReverbConfigure(mm_reverb_cfg* config)
{
    // Catch if reverb is not enabled
    if (!ReverbEnabled)
        return;

    // Setup left channel, if flagged
    if (config->flags & MMRF_LEFT)
    {
        SetupChannel(config, REVERB_CHN_LEFT);

        // Reverse panning if needed
        if (config->flags & MMRF_INVERSEPAN)
        {
            mm_word panning = 0x80 - config->panning;

            if (panning >= 0x80)
                panning = 0x7F;

            config->panning = panning;
        }

        // Set capture dad
        if (config->flags & MMRF_MEMORY)
            REG_SNDCAP0DAD = (u32)config->memory;

        // Set capture len
        if (config->flags & MMRF_DELAY)
            REG_SNDCAP0LEN = config->delay;

        // memory += length for other channel
        config->memory = (mm_addr)(((mm_word)config->memory) + (config->delay << 2));
    }

    // Setup right channel, if flagged
    if (config->flags & MMRF_RIGHT)
    {
        SetupChannel(config, REVERB_CHN_RIGHT);

        // Set capture dad
        if (config->flags & MMRF_MEMORY)
            REG_SNDCAP1DAD = (u32)config->memory;

        // Set capture len
        if (config->flags & MMRF_DELAY)
            REG_SNDCAP1LEN = config->delay;
    }

    // Check/Set 8-bit left
    if (config->flags & MMRF_8BITLEFT)
    {
        REG_SNDCAP0CNT |= SNDCAPCNT_FORMAT_8BIT;

        REG_SOUNDXCNT(REVERB_CHN_LEFT) &= 0x00FFFFFF;
        REG_SOUNDXCNT(REVERB_CHN_LEFT) |= SOUNDXCNT_FORMAT_8BIT | SOUNDXCNT_REPEAT;
    }

    // Check/Set 16-bit left
    if (config->flags & MMRF_16BITLEFT)
    {
        REG_SNDCAP0CNT &= ~SNDCAPCNT_FORMAT_8BIT;

        REG_SOUNDXCNT(REVERB_CHN_LEFT) &= 0x00FFFFFF;
        REG_SOUNDXCNT(REVERB_CHN_LEFT) |= SOUNDXCNT_FORMAT_16BIT | SOUNDXCNT_REPEAT;
    }

    // Check/Set 8-bit right
    if (config->flags & MMRF_8BITRIGHT)
    {
        REG_SNDCAP1CNT |= SNDCAPCNT_FORMAT_8BIT;

        REG_SOUNDXCNT(REVERB_CHN_RIGHT) &= 0x00FFFFFF;
        REG_SOUNDXCNT(REVERB_CHN_RIGHT) |= SOUNDXCNT_FORMAT_8BIT | SOUNDXCNT_REPEAT;
    }

    // Check/Set 16-bit right
    if (config->flags & MMRF_16BITRIGHT)
    {
        REG_SNDCAP1CNT &= ~SNDCAPCNT_FORMAT_8BIT;

        REG_SOUNDXCNT(REVERB_CHN_RIGHT) &= 0x00FFFFFF;
        REG_SOUNDXCNT(REVERB_CHN_RIGHT) |= SOUNDXCNT_FORMAT_16BIT | SOUNDXCNT_REPEAT;
    }

    mm_bool update_dry_settings = 0;

    // Check/Set dry bits
    if (config->flags & MMRF_NODRYLEFT)
    {
        ReverbNoDryLeft = 1;
        update_dry_settings = 1;
    }
    if (config->flags & MMRF_NODRYRIGHT)
    {
        ReverbNoDryRight = 1;
        update_dry_settings = 1;
    }
    if (config->flags & MMRF_DRYLEFT)
    {
        ReverbNoDryLeft = 0;
        update_dry_settings = 1;
    }
    if (config->flags & MMRF_DRYRIGHT)
    {
        ReverbNoDryRight = 0;
        update_dry_settings = 1;
    }

    if (update_dry_settings)
        CopyDrySettings();

    // Save flags
    ReverbFlags = config->flags;
}

// Start reverb output
void mmReverbStart(mm_reverbch channels)
{
    // Stop first (to reset)
    mmReverbStop(channels);

    if (channels & MMRC_LEFT)
    {
        // Enable capture
        REG_SNDCAP0CNT |= SNDCAPCNT_START_BUSY;

        // Starts the channel
        REG_SOUNDXCNT(REVERB_CHN_LEFT) |= SOUNDXCNT_ENABLE;
    }

    if (channels & MMRC_RIGHT)
    {
        // Enable capture
        REG_SNDCAP1CNT |= SNDCAPCNT_START_BUSY;

        // Starts the channel
        REG_SOUNDXCNT(REVERB_CHN_RIGHT) |= SOUNDXCNT_ENABLE;
    }

    ReverbStarted = 1;

    CopyDrySettings();
}

// Stop reverb output
// It automatically restores the output to the default...
void mmReverbStop(mm_reverbch channels)
{
    mm_hword mixer_info = REG_SOUNDCNT;

    if (channels & MMRC_LEFT)
    {
        // Disable capture
        REG_SNDCAP0CNT &= ~SNDCAPCNT_START_BUSY;

        // Stops the channel
        REG_SOUNDXCNT(REVERB_CHN_LEFT) &= ~SOUNDXCNT_ENABLE;

        mixer_info &= ~SOUNDCNT_LEFT_OUTPUT_MASK;
        mixer_info |= SOUNDCNT_LEFT_OUTPUT_MIXER; // Left output = all channels
    }

    if (channels == MMRC_RIGHT)
    {
        // Disable capture
        REG_SNDCAP1CNT &= ~SNDCAPCNT_START_BUSY;

        // Stops the channel
        REG_SOUNDXCNT(REVERB_CHN_RIGHT) &= ~SOUNDXCNT_ENABLE;

        mixer_info &= ~SOUNDCNT_RIGHT_OUTPUT_MASK;
        mixer_info |= SOUNDCNT_RIGHT_OUTPUT_MIXER; // Right output = all channels
    }

    REG_SOUNDCNT = mixer_info;

    ReverbStarted = 0;
}
