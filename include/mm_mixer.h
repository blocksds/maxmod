// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_MIXER_H
#define MM_MIXER_H

#include "mm_types.h"

extern mm_word mm_mixlen;

void mmMixerMix(mm_word samples_count);
void mmMixerSetVolume(int channel, mm_word volume);
void mmMixerSetPan(int channel, mm_byte panning);
void mmMixerSetFreq(int channel, mm_word rate);
void mmMixerMulFreq(int channel, mm_word factor);

#endif // MM_MIXER_H


