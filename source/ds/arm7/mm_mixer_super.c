// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nds.h>

#include "maxmod7.h"
#include "mm_main.h"
#include "mm_main7.h"
#include "mm_mixer_super.h"
#include "mm_mixer_ds.h"
#include "mm_mas.h"
#include "mm_effect.h"
#include "mp_defs.h"

#define SWM_CHANNEL_1 6
#define SWM_CHANNEL_2 7

#define MIX_TIMER_NUMBER 0
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
    for (int i = 0; i < NUM_CHANNELS; i++)
        channels[i].alloc = 255;

    channels = &mm_schannels[0];

    // Reset channel allocation
    for (int i = 0; i < MP_SCHANNELS; i++)
        channels[i].alloc = 255;

    mmResetEffects();
}

static void mm_startup_wait(void)
{
    // Delay 12 samples (pcm startup time + extra)
    volatile int val = 1024 * (16 / 4);
    while (val--);
    // TODO: Replace this by a swiDelay()
}

static void EnableSound(void)
{
    // Full volume | Enable
    REG_SOUNDCNT |= (SOUND_ENABLE | SOUND_VOL(100)); // 100 out of 127
    // TODO: Fix this, it doesn't clear the volume before setting it with OR
}

static void ClearAllChannels(void)
{
    // reset hardware channels
    mm_word bitmask = mm_ch_mask;
    for (size_t i = 0; i < NUM_PHYS_CHANNELS; i++)
    {
        if (bitmask & (1 << i))
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
    // Disable timer
    TIMER_CR(MIX_TIMER_NUMBER) = 0;

    mmSetResolution(40960);

    mm_word bitmask = mm_ch_mask;
    for (int i = 0; i < NUM_PHYS_CHANNELS; i++)
    {
        if (bitmask & (1 << i))
        {
            SCHANNEL_CR(i) = 0xA8000000;
            SCHANNEL_SOURCE(i) = (uintptr_t)mm_mix_data.mix_data_b.output[i];
            SCHANNEL_TIMER(i) = 0xFE00;
            SCHANNEL_REPEAT_POINT(i) = 0;
            SCHANNEL_LENGTH(i) = MM_MIX_B_NUM_SAMPLES;
        }
    }

    // Reset output slice
    mm_output_slice = 0;

    EnableSound();

    mm_startup_wait();

    TIMER_DATA(MIX_TIMER_NUMBER) = 0xFF80;
    TIMER_CR(MIX_TIMER_NUMBER) = 0x00C3;
}

static void SetupSWM(void)
{
    // Reset output slice
    mm_output_slice = 0;

    TIMER_CR(MIX_TIMER_NUMBER) = 0;

    // Disable sound channel and clear loop points
    SCHANNEL_CR(SWM_CHANNEL_1) = 0;
    SCHANNEL_CR(SWM_CHANNEL_2) = 0;
    SCHANNEL_REPEAT_POINT(SWM_CHANNEL_1) = 0;
    SCHANNEL_REPEAT_POINT(SWM_CHANNEL_2) = 0;

    // Setup sources
    SCHANNEL_SOURCE(SWM_CHANNEL_1) = (uintptr_t)mm_mix_data.mix_data_c.mix_output[0];
    SCHANNEL_SOURCE(SWM_CHANNEL_2) = (uintptr_t)mm_mix_data.mix_data_c.mix_output[1];

    // Set sampling frequency
    SCHANNEL_TIMER(SWM_CHANNEL_1) = -768;
    SCHANNEL_TIMER(SWM_CHANNEL_2) = -768;

    // Set source length
    SCHANNEL_LENGTH(SWM_CHANNEL_1) = MM_SW_BUFFERLEN / 2;
    SCHANNEL_LENGTH(SWM_CHANNEL_2) = MM_SW_BUFFERLEN / 2;

    // Setup control
    SCHANNEL_CR(SWM_CHANNEL_1) = SOUND_VOL(0x7F) | SOUND_PAN(0) | SOUND_REPEAT | SOUND_FORMAT_16BIT | SCHANNEL_ENABLE;
    SCHANNEL_CR(SWM_CHANNEL_2) = SOUND_VOL(0x7F) | SOUND_PAN(0x7F) | SOUND_REPEAT | SOUND_FORMAT_16BIT | SCHANNEL_ENABLE;

    mm_startup_wait();

    TIMER_DATA(MIX_TIMER_NUMBER) = 0xFD60;
    TIMER_CR(MIX_TIMER_NUMBER) = 0x00C2;

    mmSetResolution(31170);

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
    for (size_t i = 0; i < NUM_CHANNELS; i++)
        mix_ch[i].samp_cnt = 0;

    ClearAllChannels();

    mm_reset_channels();

    // Clear volume and enable bit
    REG_SOUNDCNT &= ~(SOUND_ENABLE | SOUND_VOL(0xFF));

    memset(&mm_mix_data, 0, sizeof(mm_mix_data_ds));

    mm_mixing_mode = mode;

    switch (mode)
    {
        case MM_MODE_A: // Mode A: Hardware mixing
            DisableSWM();
            ClearAllChannels();
            // 256hz resolution
            mmSetResolution(40960);

            TIMER_DATA(MIX_TIMER_NUMBER) = 0xFF80;
            TIMER_CR(MIX_TIMER_NUMBER) = 0x00C3;

            EnableSound();
            break;

        case MM_MODE_B: // Mode B: Interpolated mixing
            DisableSWM();
            ClearAllChannels();
            mmSetupModeB();
            break;

        default:
        case MM_MODE_C: // Mode C: Extended mixing
            ClearAllChannels();
            SetupSWM();
            EnableSound();
            break;
    }

    REG_IME = saved_ime;
}
