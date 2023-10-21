// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "maxmod7.h"
#include "mm_main.h"
#include "mm_main7.h"
#include "mm_mixer_super.h"
#include "mm_mixer_ds.h"
#include "mm_mas.h"
#include "mm_effect.h"
#include "mp_defs.h"
#include "multiplatform_defs.h"

#define SWM_CHANNEL_1 6
#define SWM_CHANNEL_2 7

#define ROUND_CLOSEST(dividend, divisor) (((dividend) + (((divisor) + 1) >> 1)) / (divisor))

#define MIX_TIMER_NUMBER 0
#define MIX_TIMER_256_HZ_FREQ 1024
#define MIX_TIMER_256_HZ_DIV TIMER_DIV_1024
#define MIX_TIMER_194_8125_HZ_FREQ 256
#define MIX_TIMER_194_8125_HZ_DIV TIMER_DIV_256

// Get the value closest to -((BUS_CLOCK / frequency) / hz), where hz can be a non-integer value
#define MIX_TIMER_SET_VALUE(x, divisor, frequency) (-ROUND_CLOSEST(BUS_CLOCK, ROUND_CLOSEST(x * frequency, divisor)))
#define MIX_TIMER_SET_256_HZ_VALUE(x, divisor) MIX_TIMER_SET_VALUE(x, divisor, MIX_TIMER_256_HZ_FREQ)
#define MIX_TIMER_SET_194_8125_HZ_VALUE(x, divisor) MIX_TIMER_SET_VALUE(x, divisor, MIX_TIMER_194_8125_HZ_FREQ)

// Get the sound timer value closest to the effective frequency of the previously calculated timer -((BUS_CLOCK / 2) / (real_freq * num_samples))
// num_samples is the amount of samples per "regular timer" call.
#define MIX_SOUND_TIMER_SET_VALUE(timer_value, frequency, num_samples) (-ROUND_CLOSEST(BUS_CLOCK, ROUND_CLOSEST(((int64_t)BUS_CLOCK) * 2 * num_samples, timer_value * frequency)))
#define MIX_SOUND_TIMER_SET_256_HZ_VALUE(x, divisor, num_samples) MIX_SOUND_TIMER_SET_VALUE(-MIX_TIMER_SET_256_HZ_VALUE(x, divisor), MIX_TIMER_256_HZ_FREQ, num_samples)
#define MIX_SOUND_TIMER_SET_194_8125_HZ_VALUE(x, divisor, num_samples) MIX_SOUND_TIMER_SET_VALUE(-MIX_TIMER_SET_194_8125_HZ_VALUE(x, divisor), MIX_TIMER_194_8125_HZ_FREQ, num_samples)

// Get the resolution closest to the effective frequency of the previously calculated timer (BUS_CLOCK * 2.5 * 64) / real_freq
#define RESOLUTION_VALUE(timer_value, frequency) (ROUND_CLOSEST(((int64_t)BUS_CLOCK) * 5 * (64 / 2), timer_value * frequency))
#define RESOLUTION_256_HZ_VALUE(x, divisor) RESOLUTION_VALUE(-MIX_TIMER_SET_256_HZ_VALUE(x, divisor), MIX_TIMER_256_HZ_FREQ)
#define RESOLUTION_194_8125_HZ_VALUE(x, divisor) RESOLUTION_VALUE(-MIX_TIMER_SET_194_8125_HZ_VALUE(x, divisor), MIX_TIMER_194_8125_HZ_FREQ)

static void mm_reset_channels(void);
static void mm_startup_wait(void);
static void DisableSWM(void);
static void ClearAllChannels(void);
static void EnableSound(void);
static void mmSetupModeB(void);
static void SetupSWM(void);

// Reset all channels
static void mm_reset_channels(void)
{
    // Clear active channel data
    memset(mm_achannels, 0, sizeof(mm_active_channel) * NUM_CHANNELS);

    mm_module_channel *channels = mm_pchannels;

    // Reset channel allocation
    for(int i = 0; i < NUM_CHANNELS; i++)
        channels[i].alloc = 255;

    channels = &mm_schannels[0];

    // Reset channel allocation
    for(int i = 0; i < MP_SCHANNELS; i++)
        channels[i].alloc = 255;

    mmResetEffects();
}

static void mm_startup_wait(void)
{
	//delay 12 samples (pcm startup time + extra)
    volatile int val = 1024 * (16 / 4);
    while(val--);
}

static void EnableSound(void)
{
    // Full volume | Enable
    REG_SOUNDCNT |= (SOUND_ENABLE | SOUND_VOL(0x7F));
}

static void ClearAllChannels(void)
{
    // reset hardware channels
    mm_word bitmask = mm_ch_mask;
    for(size_t i = 0; i < NUM_PHYS_CHANNELS; i++)
    {
        if(bitmask & (1 << i))
        {
            SCHANNEL_CR(i) = 0;
            SCHANNEL_SOURCE(i) = 0;
            SCHANNEL_TIMER(i) = 0;
            SCHANNEL_REPEAT_POINT(i) = 0;
            SCHANNEL_LENGTH(i) = 0;
        }
    }
}

static void DisableSWM(void)
{
    // Lock Software channels
    mmLockChannelsQuick(ALL_SOFT_CHANNELS_MASK);
    // Restore Stream channels
    mmUnlockChannelsQuick((1 << SWM_CHANNEL_1) | (1 << SWM_CHANNEL_2));
}

static void mmSetupModeB(void)
{
    // Reset output slice
    mm_output_slice = 0;

    TIMER_CR(MIX_TIMER_NUMBER) = 0;
    mmSetResolution(RESOLUTION_256_HZ_VALUE(256, 1));

    mm_word bitmask = mm_ch_mask;
    for(int i = 0; i < NUM_PHYS_CHANNELS; i++)
    {
        if(bitmask & (1 << i))
        {
            SCHANNEL_CR(i) = 0xA8000000;
            SCHANNEL_SOURCE(i) = (uintptr_t)mm_mix_data.mix_data_b.output[i];
            SCHANNEL_TIMER(i) = MIX_SOUND_TIMER_SET_256_HZ_VALUE(256, 1, MM_MIX_B_NUM_SAMPLES);
            SCHANNEL_REPEAT_POINT(i) = 0;
            SCHANNEL_LENGTH(i) = MM_MIX_B_NUM_SAMPLES;
        }
    }

    EnableSound();
    
    mm_startup_wait();

    timerStart(MIX_TIMER_NUMBER, MIX_TIMER_256_HZ_DIV, MIX_TIMER_SET_256_HZ_VALUE(256, 1), NULL);
    TIMER_CR(MIX_TIMER_NUMBER) |= TIMER_IRQ_REQ;
}

static void SetupSWM(void)
{
    // Reset output slice
    mm_output_slice = 0;

    TIMER_CR(MIX_TIMER_NUMBER) = 0;
    mmSetResolution(RESOLUTION_194_8125_HZ_VALUE(3117, 16));

    // Disable sound channel and clear loop points
    SCHANNEL_CR(SWM_CHANNEL_1) = 0;
    SCHANNEL_CR(SWM_CHANNEL_2) = 0;
    SCHANNEL_REPEAT_POINT(SWM_CHANNEL_1) = 0;
    SCHANNEL_REPEAT_POINT(SWM_CHANNEL_2) = 0;

    // Setup sources
    SCHANNEL_SOURCE(SWM_CHANNEL_1) = (uintptr_t)mm_mix_data.mix_data_c.mix_output[0];
    SCHANNEL_SOURCE(SWM_CHANNEL_2) = (uintptr_t)mm_mix_data.mix_data_c.mix_output[1];

    // Set sampling frequency
    SCHANNEL_TIMER(SWM_CHANNEL_1) = MIX_SOUND_TIMER_SET_194_8125_HZ_VALUE(3117, 16, MM_SW_BUFFERLEN/2);
    SCHANNEL_TIMER(SWM_CHANNEL_2) = MIX_SOUND_TIMER_SET_194_8125_HZ_VALUE(3117, 16, MM_SW_BUFFERLEN/2);

    // Set source length
    SCHANNEL_LENGTH(SWM_CHANNEL_1) = MM_SW_BUFFERLEN/2;
    SCHANNEL_LENGTH(SWM_CHANNEL_2) = MM_SW_BUFFERLEN/2;

    // Setup control
    SCHANNEL_CR(SWM_CHANNEL_1) = SOUND_VOL(0x7F) | SOUND_PAN(0) | SOUND_REPEAT | SOUND_FORMAT_16BIT | SCHANNEL_ENABLE;
    SCHANNEL_CR(SWM_CHANNEL_2) = SOUND_VOL(0x7F) | SOUND_PAN(0x7F) | SOUND_REPEAT | SOUND_FORMAT_16BIT | SCHANNEL_ENABLE;

    mm_startup_wait();

    timerStart(MIX_TIMER_NUMBER, MIX_TIMER_194_8125_HZ_DIV, MIX_TIMER_SET_194_8125_HZ_VALUE(3117, 16), NULL);
    TIMER_CR(MIX_TIMER_NUMBER) |= TIMER_IRQ_REQ;

    // Lock Stream channels
    mmLockChannelsQuick((1 << SWM_CHANNEL_1) | (1 << SWM_CHANNEL_2));
    // Unlock Software channels
    mmUnlockChannelsQuick(ALL_SOFT_CHANNELS_MASK);
}

// Init mixer system & setup nds control
void mmMixerInit(void)
{
    // Default to mode A
    mmSelectMode(MM_MODE_A);
}

// Select audio mode
void mmSelectMode(mm_mode_enum mode)
{
    uint32_t saved_ime = REG_IME;
    REG_IME = IME_DISABLE;

    // reset mixer channels
    mm_mixer_channel *mix_ch = &mm_mix_channels[0];
    for(size_t i = 0; i < NUM_CHANNELS; i++)
        mix_ch[i].samp_cnt = 0;

    ClearAllChannels();

    mm_reset_channels();

    REG_SOUNDCNT &= ~(SOUND_ENABLE | 0x7F);

    memset(&mm_mix_data, 0, sizeof(mm_mix_data_ds));

    mm_mixing_mode = mode;

    switch(mode)
    {
        case MM_MODE_A:
            DisableSWM();
            ClearAllChannels();
            // 256hz resolution
            mmSetResolution(RESOLUTION_256_HZ_VALUE(256, 1));
            timerStart(MIX_TIMER_NUMBER, MIX_TIMER_256_HZ_DIV, MIX_TIMER_SET_256_HZ_VALUE(256, 1), NULL);
            TIMER_CR(MIX_TIMER_NUMBER) |= TIMER_IRQ_REQ;
            EnableSound();
            break;
        case MM_MODE_B:
            DisableSWM();
            ClearAllChannels();
            mmSetupModeB();
            break;
        case MM_MODE_C:
        default:
            ClearAllChannels();
            SetupSWM();
            EnableSound();
            break;
    }

    REG_IME = saved_ime;
}
