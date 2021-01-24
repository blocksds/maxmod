// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MP_MIXER_NDS_H
#define MP_MIXER_NDS_H

#include <assert.h>

#include "mm_types.h"

typedef struct {
    mm_word     samp_cnt;   // 0:23  mainram address
                            // 24:31 LSBs = target panning 0..127, MSB = key-on
    mm_hword    freq;       // unsigned 3.10, top 3 cleared
    mm_hword    vol;        // target volume   0..65535
    mm_word     read;       // unsigned 22.10
    mm_hword    cvol;       // current volume  0..65535
    mm_hword    cpan;       // current panning 0..65535
} mm_mixer_channel;

static_assert(sizeof(mm_mixer_channel) == 16);

// scale = 65536*1024*2 / mixrate
#define MIXER_SCALE         4096 //6151

#define MIXER_CF_START      128
#define MIXER_CF_SOFT       2
#define MIXER_CF_SURROUND   4
#define MIXER_CF_FILTER     8
#define MIXER_CF_REVERSE    16

#endif // MP_MIXER_NDS_H
