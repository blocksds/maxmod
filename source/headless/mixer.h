// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021-2026, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_HEADLESS_MIXER_H
#define MM_HEADLESS_MIXER_H

#include "core/channel_types.h"
#include "headless/main_headless.h"

extern mm_mixer_channel *mm_mix_channels;
extern mm_word mm_mixlen;

extern mm_word mm_bpmdv;

void mmMixerInit(mm_headless_system* setup);
void mmMixerMix(mm_addr wave_buffer, mm_word samples_count);
void mmMixerSetRead(int channel, mm_word value);
void mmMixerEnd(void);

#endif // MM_HEADLESS_MIXER_H
