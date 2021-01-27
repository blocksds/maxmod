// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_MIXER_GBA_H
#define MM_MIXER_GBA_H

#include "mp_mixer_gba.h"

extern mm_word mm_mixlen;
extern mm_mixer_channel *mm_mixchannels;
extern mm_word mm_bpmdv;


void mmMixerInit(mm_gba_system* setup);

#endif // MM_MIXER_GBA_H