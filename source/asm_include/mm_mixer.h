// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_MIXER_H
#define MM_MIXER_H

#include "mm_types.h"

extern mm_word mm_mixlen;

void mmMixerSetSource(int channel, mm_word p_sample);
void mmMixerSetVolume(int channel, mm_word volume);
void mmMixerSetPan(int channel, mm_byte panning);
void mmMixerSetFreq(int channel, mm_word rate);
void mmMixerMulFreq(int channel, mm_word factor);
void mmMixerStopChannel(int channel);
mm_word mmMixerChannelActive(int channel);

#endif // MM_MIXER_H


