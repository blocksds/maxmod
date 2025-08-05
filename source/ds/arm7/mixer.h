// SPDX-License-Identifier: ISC
//
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_DS_ARM7_MIXER_H__
#define MM_DS_ARM7_MIXER_H__

#include <mm_types.h>

#include "core/channel_types.h"
#include "ds/arm7/mixer_types.h"

#define NUM_CHANNELS        32
#define NUM_PHYS_CHANNELS   16

void mmMixerInit(void);
void mmMixerMix(void);
void mmMixerPre(void);

extern mm_byte mm_output_slice;
extern mm_mode_enum mm_mixing_mode;
extern const mm_byte mmVolumeDivTable[];
extern const mm_byte mmVolumeShiftTable[];
extern mm_mix_data_ds mm_mix_data;

extern mm_mixer_channel mm_mix_channels[NUM_CHANNELS];

#endif // MM_DS_ARM7_MIXER_H__
