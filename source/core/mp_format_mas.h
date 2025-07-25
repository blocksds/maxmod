// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MP_FORMAT_MAS_H
#define MP_FORMAT_MAS_H

// MAS header structure

typedef struct {
    mm_byte     order_count;
    mm_byte     inst_count;
    mm_byte     samp_count;
    mm_byte     patt_count;
    mm_byte     flags;
    mm_byte     global_volume;
    mm_byte     initial_speed;
    mm_byte     initial_tempo;
    mm_byte     restart_pos;
    mm_byte     padding[3]; // Unused
    mm_byte     chanvol[32];
    mm_byte     chanpan[32];
    mm_byte     order[200];
    mm_word     tables[];
} mas_header;

#define MAS_HEADER_FLAG_LINK_GXX    (1 << 0) // Shared Gxx
#define MAS_HEADER_FLAG_OLD_EFFECTS (1 << 1) // TODO: Unused flag
#define MAS_HEADER_FLAG_FREQ_MODE   (1 << 2) // 1 = Linear freqs, 0 = Amiga freqs
#define MAS_HEADER_FLAG_XM_MODE     (1 << 3) // 1 = XM mode, 0 = Other/IT mode?
#define MAS_HEADER_FLAG_MSL_DEP     (1 << 4) // TODO: Unused flag
#define MAS_HEADER_FLAG_OLD_MODE    (1 << 5) // 1 = MOD/S3M, 0 = Other

// Instrument envelope struct : mm_mas_envelope

#define ENVFLAG_A           (1 << 3)

// Sample structure : mm_mas_gba_sample

#define C_SAMPLE_LEN        0
#define C_SAMPLE_LOOP       4
#define C_SAMPLE_DATA       12

// Sample structure : mm_mas_ds_sample

#define C_SAMPLEN_LSTART    0
#define C_SAMPLEN_LEN       4
#define C_SAMPLEN_FORMAT    8
#define C_SAMPLEN_REP       9
#define C_SAMPLEN_DFREQ     10
#define C_SAMPLEN_POINT     12
#define C_SAMPLEN_DATA      16

#endif // MP_FORMAT_MAS_H
