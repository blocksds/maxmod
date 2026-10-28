// SPDX-License-Identifier: ISC
//
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz

#ifndef MM_DS_ARM7_MIXER_H__
#define MM_DS_ARM7_MIXER_H__

#include <mm_types.h>

#include "core/channel_types.h"
#include "ds/arm7/mixer_types.h"

#define NUM_CHANNELS        32
#define NUM_PHYS_CHANNELS   16

// Alias used by common code
typedef mm_mas_ds_sample mm_mas_platform_sample;

void mmMixerInit(void);
void mmMixerMix(void);
void mmMixerPre(void);

extern mm_byte mm_output_slice;
extern mm_mode_enum mm_mixing_mode;
extern const mm_byte mmVolumeDivTable[];
extern const mm_byte mmVolumeShiftTable[];
extern mm_mix_data_ds mm_mix_data;

extern mm_mixer_channel mm_mix_channels[NUM_CHANNELS];

static inline bool mmMixerChannelIsActive(mm_mixer_channel *mix_ch)
{
    if (mix_ch->samp == 0)
        return false;

    return true;
}

static inline void mmMixerChannelStop(mm_mixer_channel *mix_ch)
{
    mix_ch->key_on = 0;
    mix_ch->samp = 0;

    // Setting the panning isn't really needed, but it may help the compiler
    // optimize all 3 accesses into one single 32-bit write.
    mix_ch->tpan = 0;
}

static inline void mmMixerChannelSetPan(mm_mixer_channel *mix_ch, mm_word pan)
{
    mix_ch->tpan = pan >> 1;
}

static inline void mmMixerChannelPlaySample(mm_mixer_channel *mix_ch,
                                            mm_mas_ds_sample *sample)
{
    mix_ch->samp = ((mm_word)sample) & 0xFFFFFF; // Ignore top 8 bits
    mix_ch->tpan = 0;
    mix_ch->key_on = 1;
}

static inline void mmMixerChannelSetSampleOffset(mm_mixer_channel *mix_ch,
                                                 mm_word offset)
{
    // We store the value without shifting it. mmMixA(), mmMixB() and mmMixC()
    // take this value when the note starts and it shifts it by the right value
    // to adjust it for 8 or 16-bit samples.
    mix_ch->read = offset;
}

// Volume is a 16-bit value (0 to 65535)
static inline void mmMixerChannelSetVolume(mm_mixer_channel *mix_ch,
                                           mm_word volume)
{
    mix_ch->vol = volume; // DS mixer channels use 16-bit values
}

#endif // MM_DS_ARM7_MIXER_H__
