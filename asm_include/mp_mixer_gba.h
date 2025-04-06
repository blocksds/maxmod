// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MP_MIXER_GBA_H
#define MP_MIXER_GBA_H

#include <assert.h>

#include "mm_types.h"

typedef struct {
    mm_word     src;
    mm_word     read;
    mm_byte     vol;
    mm_byte     pan;
    mm_byte     unused_0;
    mm_byte     unused_1;
    mm_word     freq;
} mm_mixer_channel;

static_assert(sizeof(mm_mixer_channel) == 16);

#endif // MP_MIXER_GBA_H
