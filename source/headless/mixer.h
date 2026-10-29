// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021-2026, Antonio Niño Díaz

#ifndef MM_HEADLESS_MIXER_H
#define MM_HEADLESS_MIXER_H

#include <mm_types.h>

#include "headless/main_headless.h"

typedef struct {
    uintptr_t   src;
    mm_word     read; // Fixed point 20.12. See MP_SAMPFRAC
    mm_byte     vol;
    mm_byte     pan;
    mm_word     freq;
    mm_bool     active; // true if the channel is active, false if not
} mm_mixer_channel;

// Fractionary part of the sample read offset
#define MP_SAMPFRAC             12

#define MIXER_SCALE             ((4096 * 65536) / 15768)

// Alias used by common code
typedef mm_mas_gba_sample mm_mas_platform_sample;

extern mm_mixer_channel *mm_mix_channels;
extern mm_word mm_mixlen;

extern mm_word mm_bpmdv;

void mmMixerInit(mm_headless_system* setup);
mm_addr mmMixerMix(mm_addr wave_buffer, mm_word samples_count,
                   mm_headless_output_format output_format);
void mmMixerEnd(void);

static inline bool mmMixerChannelIsActive(mm_mixer_channel *mix_ch)
{
    return mix_ch->active;
}

static inline void mmMixerChannelStop(mm_mixer_channel *mix_ch)
{
    mix_ch->active = false;
}

static inline void mmMixerChannelSetPan(mm_mixer_channel *mix_ch, mm_word pan)
{
    mix_ch->pan = pan;
}

static inline void mmMixerChannelPlaySample(mm_mixer_channel *mix_ch,
                                            mm_mas_gba_sample *sample)
{
    mix_ch->src = (uintptr_t)&(sample->data[0]);
    mix_ch->active = true;
}

static inline void mmMixerChannelSetSampleOffset(mm_mixer_channel *mix_ch,
                                                 mm_word offset)
{
    // GBA soundbanks only support 8-bit samples, so we don't have to worry
    // about having different shifts for 8-bit or 16-bit samples.
    mix_ch->read = offset << MP_SAMPFRAC;
}

// Volume is a 16-bit value (0 to 65535)
static inline void mmMixerChannelSetVolume(mm_mixer_channel *mix_ch,
                                           mm_word volume)
{
    mix_ch->vol = volume >> 8; // Headless mixer channels use 8-bit values
}

static inline void mmMixerChannelSuspend(mm_mixer_channel *mix_ch)
{
    mix_ch->freq = 0;
}

#endif // MM_HEADLESS_MIXER_H
