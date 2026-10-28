// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz

#ifndef MM_GBA_MIXER_H
#define MM_GBA_MIXER_H

#include "core/channel_types.h"

// Alias used by common code
typedef mm_mas_gba_sample mm_mas_platform_sample;

extern mm_mixer_channel *mm_mix_channels;
extern mm_word mm_mixlen;

extern mm_word mm_bpmdv;

void mmMixerInit(mm_gba_system* setup);
void mmMixerMix(mm_word samples_count);
void mmMixerEnd(void);

static inline bool mmMixerChannelIsActive(mm_mixer_channel *mix_ch)
{
    if (mix_ch->src & MIXCH_GBA_SRC_STOPPED)
        return false;

    return true;
}

static inline void mmMixerChannelStop(mm_mixer_channel *mix_ch)
{
    mix_ch->src = MIXCH_GBA_SRC_STOPPED;
}

static inline void mmMixerChannelSetPan(mm_mixer_channel *mix_ch, mm_word pan)
{
    mix_ch->pan = pan;
}

static inline void mmMixerChannelPlaySample(mm_mixer_channel *mix_ch,
                                            mm_mas_gba_sample *sample)
{
    mix_ch->src = (uintptr_t)&(sample->data[0]);
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
    mix_ch->vol = volume >> 8; // GBA mixer channels use 8-bit values
}

#endif // MM_GBA_MIXER_H
