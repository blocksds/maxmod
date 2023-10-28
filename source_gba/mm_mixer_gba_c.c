// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdint.h>
#include <string.h>

#include "maxmod.h"

#include "mp_format_mas.h"

#include "mm_main_gba.h"
#include "mm_mixer_gba.h"
#include "useful_qualifiers.h"

// Set channel volume
void mmMixerSetVolume(int channel, mm_word volume)
{
    mm_mixchannels[channel].vol = volume;
}

// Set channel panning
void mmMixerSetPan(int channel, mm_byte panning)
{
    mm_mixchannels[channel].pan = panning;
}

// Scale mixing frequency
void mmMixerMulFreq(int channel, mm_word factor)
{
    mm_word freq = mm_mixchannels[channel].freq;

    freq = (freq * factor) >> 10;

    mm_mixchannels[channel].freq = freq;
}

// Stop mixing channel
void mmMixerStopChannel(int channel)
{
    // Set MSB (disable) of source
    mm_mixchannels[channel].src = 1U << 31;
}

// Set channel read position
void mmMixerSetRead(int channel, mm_word value)
{
    // Store new offset
    mm_mixchannels[channel].read = value;
}

// Test if mixing channel is active
mm_word mmMixerChannelActive(int channel)
{
    // Nonzero (-1) = enabled, zero = disabled
    if (mm_mixchannels[channel].src & (1U << 31))
        return 0;

    return 0xFFFFFFFF;
}

// Set channel source
void mmMixerSetSource(int channel, mm_word p_sample)
{
    // Set sample data address
    mm_mixchannels[channel].src = p_sample + C_SAMPLE_DATA;

    // Reset read position
    mm_mixchannels[channel].read = 0;
}

// Set channel mixing rate
void mmMixerSetFreq(int channel, mm_word rate)
{
    mm_mixchannels[channel].freq = rate << 2;

    // TODO: This code was commented out in the original code (ARM, not Thumb):
#if 0
    // Fix frequency to match mixing rate
    // a = specified frequency
    // hz = a * 2y13 / pr

    uint64_t value = (rate * (uint64_t)mm_freqscalar + 32768) >> 16;

    // Set chan frequency
    mm_mixchannels[channel].freq = (mm_word)value;
#endif
}

static int vblank_handler_enabled = 0;

// VBL wrapper, used to reset DMA. It needs the highest priority.
// TODO: Build this as ARM code in IWRAM
IWRAM_CODE ARM_TARGET void mmVBlank(void)
{
    // Disable until ready
    if (vblank_handler_enabled != 0)
    {
        // Swap mixing segment
        mm_word r1 = (int32_t)(int8_t)mp_mix_seg;
        r1 = ~r1;
        mp_mix_seg = r1;

        if (r1 != 0)
        {
            // DMA control: Restart DMA

            // Disable DMA
            REG_DMA1CNT_H = 0x0440;
            REG_DMA2CNT_H = 0x0440;

            // Restart DMA
            REG_DMA1CNT_H = 0xB600;
            REG_DMA2CNT_H = 0xB600;
        }
        else
        {
            // Restart write position
            mp_writepos = mm_wavebuffer;
        }
    }

    // Acknowledge interrupt
    // TODO: This shouldn't be done here but in the master ISR
    mm_hword *irq_reg = (mm_hword *)0x3007FF8;
    *irq_reg |= 1;

    // Call user handler
    if (mm_vblank_function != NULL)
        mm_vblank_function();
}

// Initialize mixer
void mmMixerInit(mm_gba_system *setup)
{
    mm_mixch_count = setup->mix_channel_count;

    mm_mixchannels = setup->mixing_channels;

    mm_mixch_end = (uintptr_t)mm_mixchannels + mm_mixch_count * sizeof(mm_mixer_channel);

    mm_mixbuffer = (uintptr_t)setup->mixing_memory;

    mm_wavebuffer = (uintptr_t)setup->wave_memory;

    mp_writepos = mm_wavebuffer;

    mm_word mode = setup->mixing_mode;

    // round(rate / 59.737)
    static mm_hword mp_mixing_lengths[] = {
        136,  176,   224,   264,   304,   352,   448,   528
    //  8khz, 10khz, 13khz, 16khz, 18khz, 21khz, 27khz, 32khz
    };

    mm_mixlen = mp_mixing_lengths[mode];

    // 15768*16384 / rate
    static mm_hword mp_rate_scales[] = {
        31812, 24576, 19310, 16384, 14228, 12288,  9655,  8192
    //  8khz,  10khz, 13khz, 16khz, 18khz, 21khz, 27khz, 32khz
    //  8121,  10512, 13379, 15768, 18157, 21024, 26758, 31536
    };

    mm_ratescale = mp_rate_scales[mode];

    // gbaclock / rate
    static mm_hword mp_timing_sheet[] = {
        -2066, -1596, -1254, -1064, -924,  -798,  -627,  -532
    //  8khz,  10khz, 13khz, 16khz, 18khz, 21khz, 27khz, 32khz
    };

    mm_timerfreq = mp_timing_sheet[mode];

    // rate * 2.5
    static mm_word mp_bpm_divisors[] = {
        20302, 26280, 33447, 39420, 45393, 52560, 66895, 78840
    };

    mm_bpmdv = mp_bpm_divisors[mode];

    // Clear wave buffer
    memset((void *)mm_wavebuffer, 0, mm_mixlen * sizeof(mm_word));

    // Reset mixing segment
    mp_mix_seg = 0;

    // Disable mixing channels

    mm_mixer_channel *mx_ch = &mm_mixchannels[0];

    for (mm_word i = 0; i < mm_mixch_count; i++)
        mx_ch[i].src = 1U << 31;

    // Enable VBL routine
    vblank_handler_enabled = 1;

    // Clear fifo data
    *REG_SGFIFOA = 0;
    *REG_SGFIFOB = 0;

    // Reset direct sound
    REG_SOUNDCNT_H = 0;

    // Setup sound: DIRECT SOUND A/B reset, timer0, A=left, B=right, volume=100%
    REG_SOUNDCNT_H = 0x9A0C;

    // Setup DMA source addresses (playback buffers)
    REG_DMA1SAD = mm_wavebuffer;
    REG_DMA2SAD = mm_wavebuffer + mm_mixlen * 2;

    // Setup DMA destination (sound fifo)
    REG_DMA1DAD = (uintptr_t)REG_SGFIFOA;
    REG_DMA2DAD = (uintptr_t)REG_SGFIFOB;

    // Enable DMA [enable, fifo request, 32-bit, repeat]
    REG_DMA1CNT = 0xB6000000;
    REG_DMA2CNT = 0xB6000000;

    // Master sound enable
    REG_SOUNDCNT_X = 0x80;

    // Wait for new frame
    while (REG_VCOUNT >= 160); // Skip current vblank period
    while (REG_VCOUNT > 160); // Wait for new one

    // Pass number 2
    while (REG_VCOUNT >= 160); // Skip current vblank period
    while (REG_VCOUNT > 160); // Wait for new one

    // Enable sampling timer
    REG_TM0CNT = mm_timerfreq | (0x80 << 16);
}
