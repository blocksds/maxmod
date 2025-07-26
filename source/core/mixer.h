// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_CORE_MIXER_H__
#define MM_CORE_MIXER_H__

#include <mm_types.h>

extern mm_word mm_mixlen;

void mmMixerSetVolume(int channel, mm_word volume);
void mmMixerSetPan(int channel, mm_byte panning);
void mmMixerSetFreq(int channel, mm_word rate);
void mmMixerMulFreq(int channel, mm_word factor);
void mmMixerStopChannel(int channel);

// TODO: The functions below aren't used anywhere, but they are implemented for GBA and DS.
void mmMixerSetSource(int channel, mm_word p_sample);
mm_word mmMixerChannelActive(int channel);

#endif // MM_CORE_MIXER_H__
