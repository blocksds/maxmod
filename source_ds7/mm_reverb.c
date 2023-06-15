// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "maxmod7.h"
#include "mm_mixer_super.h"
#include "multiplatform_defs.h"
#include "useful_qualifiers.h"

/***********************************************************************
 * Reverb Parameters
 *-------------------------------------
 * MEMORY	output address	32-bit
 * DELAY	buffer length	16-bit
 * RATE		sampling rate	16-bit
 * FEEDBACK	volume		11-bit
 * PANNING	panning		7-bit
 * DRY		output dry	1-bit
 * FORMAT	output format	1-bit
 *-------------------------------------
 *
 * 2-bit channel selection
 *
 *-------------------------------------
 *
 * Reverb output uses channels 1 and 3
 *
 ***********************************************************************/

#define DEFAULT_VOLUME 0x7F
#define DEFAULT_PANNING 0x40
#define DEFAULT_LOOP 1
#define DEFAULT_FORMAT 1 
#define DEFAULT_REVERB_CHANNEL_CNT ((DEFAULT_VOLUME << 0) | (DEFAULT_PANNING << 16) | (DEFAULT_LOOP << 27) | (DEFAULT_FORMAT << 29))
#define DEFAULT_REVERB_SOURCE 0
#define DEFAULT_REVERB_TIMER 0xFE00
#define DEFAULT_REVERB_LOOP_START 0
#define DEFAULT_REVERB_LENGTH 0

#define REVERB_FEEDBACK_MASK 0x7F
//#define REVERB_FEEDBACK_MASK 0xFFFF

#define REVERB_CHN_FIRST 1
#define REVERB_CHN_SECOND 3
#define REVERB_CHN_MASK ((1 << REVERB_CHN_FIRST) | (1 << REVERB_CHN_SECOND))

mm_word ReverbFlags;
mm_byte ReverbNoDryLeft;
mm_byte ReverbNoDryRight;
mm_bool ReverbEnabled;
mm_byte ReverbStarted;

static void SetReverbChannelToDefault(mm_byte);
static void ResetSoundAndCapture(void);
static void SetupChannel(mm_reverb_cfg*, mm_byte, mm_addr, mm_byte);
static void ConfigReverbChannel(mm_reverb_cfg*, mm_byte, mm_addr, mm_byte, vu32*, vu16*);
static void ConfigChannelReverbBits(mm_bool, mm_byte, vu8*);
static void CopyDrySettings(void);
static void StartReverbChannel(mm_byte, vu8*);
static void StopReverbChannel(mm_byte, vu8*);

// Enable Reverb System (lock reverb channels from sequencer)
void mmReverbEnable(void) {
    // Catch if reverb is already enabled
    if(ReverbEnabled)
        return;
    
    // Set enabled flag
    ReverbEnabled = 1;
    
    // Lock reverb channels
    mmLockChannels(REVERB_CHN_MASK);
    
    ResetSoundAndCapture();
}

// Disable Reverb System
void mmReverbDisable(void) {
    // Catch if reverb is already disabled
    if(!ReverbEnabled)
        return;
    
    // Set disabled flag
    ReverbEnabled = 0;
    
    ResetSoundAndCapture();
    
    // Unlock reverb channels
    mmLockChannels(REVERB_CHN_MASK);
}

// Set the reverb channel back to the default
static void SetReverbChannelToDefault(mm_byte channel) {
    SCHANNEL_CR(channel) = DEFAULT_REVERB_CHANNEL_CNT;
    SCHANNEL_SOURCE(channel) = DEFAULT_REVERB_SOURCE;
    SCHANNEL_TIMER(channel) = DEFAULT_REVERB_TIMER;
    SCHANNEL_REPEAT_POINT(channel) = DEFAULT_REVERB_LOOP_START;
    SCHANNEL_LENGTH(channel) = DEFAULT_REVERB_LENGTH;
}

// Disables/Resets sound channels, resets capture and clears flags.
static void ResetSoundAndCapture(void) {
    SetReverbChannelToDefault(REVERB_CHN_FIRST);
    SetReverbChannelToDefault(REVERB_CHN_SECOND);
    
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

static void SetupChannel(mm_reverb_cfg* config, mm_byte channel, mm_addr memory, mm_byte panning) {
    // Memory/SRC
    if(config->flags & MMRF_MEMORY)
        SCHANNEL_SOURCE(channel) = (u32)memory;

    // delay/LEN
    if(config->flags & MMRF_DELAY)
        SCHANNEL_LENGTH(channel) = config->delay;

    // rate/TMR
    if(config->flags & MMRF_RATE)
        SCHANNEL_TIMER(channel) = -config->rate;
    
    // feedback/CNT:volume,shift
    // The index into mmVolumeTable could influence the volume previously...?!
    // Fixed because it feels unintended...
    // To revert the fix back, change REVERB_FEEDBACK_MASK from 0x7F to 0xFFFF
    if(config->flags & MMRF_FEEDBACK)
        SCHANNEL_CR(channel) = (SCHANNEL_CR(channel) & 0xFFFF0000) | (mmVolumeDivTable[(config->feedback >> 7) & 0xF] << 8) | ((config->feedback & REVERB_FEEDBACK_MASK) >> mmVolumeShiftTable[(config->feedback >> 7) & 0xF]);
    
    // panning/CNT:panning
    if(config->flags & MMRF_PANNING)
        SCHANNEL_PAN(channel) = panning;
}

static void ConfigReverbChannel(mm_reverb_cfg* config, mm_byte channel, mm_addr memory, mm_byte panning, vu32* dad_reg, vu16* len_reg) {
    SetupChannel(config, channel, memory, panning);
    
    // Set capture dad
    if(config->flags & MMRF_MEMORY)
        *dad_reg = (u32)memory;
    
    // Set capture len
    if(config->flags & MMRF_DELAY)
        *len_reg = config->delay;
}

static void ConfigChannelReverbBits(mm_bool is_8_bits, mm_byte channel, vu8* cap_cnt_reg) {
    if(is_8_bits)
        is_8_bits = 1;

    *cap_cnt_reg = ((*cap_cnt_reg) & (~(1 << 3))) | (is_8_bits << 3);
    
    // It disables it...
    SCHANNEL_CR(channel) = (SCHANNEL_CR(channel) & 0x00FFFFFF) | ((0x08 | ((1-is_8_bits) << 5)) << 24);
}

// Configure reverb system
void mmReverbConfigure(mm_reverb_cfg* config) {
    // Catch if reverb is not enabled
    if(!ReverbEnabled)
        return;

    mm_addr memory = config->memory;
    mm_byte panning = config->panning & 0x7F;
    mm_bool update_dry_settings = 0;

    // Setup left channel, if flagged
    if(config->flags & MMRF_LEFT) {
        ConfigReverbChannel(config, REVERB_CHN_FIRST, memory, panning, &REG_SNDCAP0DAD, &REG_SNDCAP0LEN);
        
        // Reverse panning if needed
        if(config->flags & MMRF_INVERSEPAN)
            panning = 0x80 - panning;
        if(panning >= 0x80)
            panning = 0x7F;
        
        // Handle right channel
        memory = (mm_addr)(((mm_word)memory) + (config->delay << 2));
    }

    // Setup right channel, if flagged
    if(config->flags & MMRF_RIGHT)
        ConfigReverbChannel(config, REVERB_CHN_SECOND, memory, panning, &REG_SNDCAP1DAD, &REG_SNDCAP1LEN);
    
    // Check/Set 8-bit left
    if(config->flags & MMRF_8BITLEFT)
        ConfigChannelReverbBits(1, REVERB_CHN_FIRST, &REG_SNDCAP0CNT);
    
    // Check/Set 16-bit left
    if(config->flags & MMRF_16BITLEFT)
        ConfigChannelReverbBits(0, REVERB_CHN_FIRST, &REG_SNDCAP0CNT);
    
    // Check/Set 8-bit right
    if(config->flags & MMRF_8BITRIGHT)
        ConfigChannelReverbBits(1, REVERB_CHN_SECOND, &REG_SNDCAP1CNT);
    
    // Check/Set 16-bit right
    if(config->flags & MMRF_16BITRIGHT)
        ConfigChannelReverbBits(0, REVERB_CHN_SECOND, &REG_SNDCAP1CNT);
    
    // Check/Set dry bits
    if(config->flags & MMRF_NODRYLEFT) {
        ReverbNoDryLeft = 1;
        update_dry_settings = 1;
    }
    if(config->flags & MMRF_NODRYRIGHT) {
        ReverbNoDryRight = 1;
        update_dry_settings = 1;
    }
    if(config->flags & MMRF_DRYLEFT) {
        ReverbNoDryLeft = 0;
        update_dry_settings = 1;
    }
    if(config->flags & MMRF_DRYRIGHT) {
        ReverbNoDryRight = 0;
        update_dry_settings = 1;
    }
    
    if(update_dry_settings)
        CopyDrySettings();
    
    // Save flags
    ReverbFlags = config->flags;
}

// Setup SOUNDCNT with dry settings
static void CopyDrySettings(void) {
    // Catch if reverb has not started
    if(!ReverbStarted)
        return;

    REG_SOUNDCNT &= 0xFFFFF0FF;
    mm_hword mixer_info = 0;
    
    if(ReverbNoDryLeft)
        mixer_info |= 1 << 8;
    if(ReverbNoDryRight)
        mixer_info |= 2 << 10;
        
    REG_SOUNDCNT |= mixer_info;
}

static void StartReverbChannel(mm_byte channel, vu8* cap_cnt_reg) {
    // Enable capture
    *cap_cnt_reg |= 1 << 7;
    
    // Starts the channel
    SCHANNEL_CR(channel) |= 1 << 31;
}

static void StopReverbChannel(mm_byte channel, vu8* cap_cnt_reg) {
    // Disable capture
    *cap_cnt_reg &= ~(1 << 7);
    
    // Stops the channel
    SCHANNEL_CR(channel) &= ~(1 << 31);
}

// Start reverb output
void mmReverbStart(mm_reverbch channels) {
    // Stop first (to reset)
    mmReverbStop(channels);
    
    if((channels == MMRC_LEFT) || (channels == MMRC_BOTH))
        StartReverbChannel(REVERB_CHN_FIRST, &REG_SNDCAP0CNT);
    
    if((channels == MMRC_RIGHT) || (channels == MMRC_BOTH))
        StartReverbChannel(REVERB_CHN_SECOND, &REG_SNDCAP1CNT);
    
    ReverbStarted = 1;
    
    CopyDrySettings();
}

// Stop reverb output
// It automatically restores the output to the default...
void mmReverbStop(mm_reverbch channels) {
    mm_hword mixer_info = 0;
    
    if((channels == MMRC_LEFT) || (channels == MMRC_BOTH)) {
        StopReverbChannel(REVERB_CHN_FIRST, &REG_SNDCAP0CNT);
        mixer_info |= 3 << 8;
    }
    
    if((channels == MMRC_RIGHT) || (channels == MMRC_BOTH)) {
        StopReverbChannel(REVERB_CHN_SECOND, &REG_SNDCAP1CNT);
        mixer_info |= 3 << 10;
    }
    
    REG_SOUNDCNT &= ~mixer_info;
    
    ReverbStarted = 0;
}

