// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MM_MIXER_DS_TYPES_H__
#define MM_MIXER_DS_TYPES_H__

#include <mm_types.h>

#include "ds/arm7/mm_main7.h"

#define MM_MIX_B_NUM_SAMPLES 128
#define MM_MIX_B_OUTPUT_CH_SIZE (128 * 4)
#define MM_SW_BUFFERLEN 224 // [samples], note: nothing
#define MM_SW_CHUNKLEN 112 // [samples]

// This is actually the first 4 bytes of SOUNDxCNT, isn't it? :/
typedef struct t_mmshadowbds
{
    mm_byte vol_mul;
    mm_byte vol_shift;
    mm_byte panning;
    mm_byte reserved;
}
mmshadow_b_ds;

typedef struct t_mmixdatabds
{
    // 128 samples per channel, double-buffered
    mm_byte output[NUM_PHYS_CHANNELS][MM_MIX_B_OUTPUT_CH_SIZE];
    mm_byte fetch[256];
    mm_byte padding[32];
    mmshadow_b_ds shadow[NUM_PHYS_CHANNELS];
}
mm_mix_data_b_ds;

// This is actually SOUNDxCNT, isn't it? :/
// So mmsoundcnt_ds...
typedef struct t_mmshadowcds
{
    mm_word cnt;
    mm_word src;
    mm_hword tmr;
    mm_hword pnt;
    mm_word len;
}
mmshadow_c_ds;

typedef struct t_mmixdatacds
{
    // 16 bit stereo
    mm_hword mix_output[2][MM_SW_BUFFERLEN];
    mm_hword mix_work_mem[2][MM_SW_CHUNKLEN];
    mm_byte fetch[256];
    mm_byte padding[16];
    mmshadow_c_ds shadow[NUM_PHYS_CHANNELS];
}
mm_mix_data_c_ds;

typedef union t_mmixdatads
{
    mm_mix_data_b_ds mix_data_b;
    mm_mix_data_c_ds mix_data_c;
}
mm_mix_data_ds;

#endif // MM_MIXER_DS_TYPES_H__
