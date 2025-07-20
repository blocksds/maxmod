// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MP_FORMAT_MAS_H
#define MP_FORMAT_MAS_H

// MAS header structure

typedef struct {
    mm_byte     len;
    mm_byte     instn;
    mm_byte     sampn;
    mm_byte     pattn;
    mm_byte     flags;
    mm_byte     gv;
    mm_byte     speed;
    mm_byte     tempo;
    mm_byte     rep;
    mm_byte     padding[3]; // TODO: Check this
    mm_byte     chanvol[32];
    mm_byte     chanpan[32];
    mm_byte     order[200];
    mm_word     tables[];
} mas_header;

#define C_FLAGS_GS          1
#define C_FLAGS_OS          2
#define C_FLAGS_SS          3
#define C_FLAGS_XS          4
#define C_FLAGS_DS          5
// #define C_FLAGS_LS          5 // HUH???
#define C_FLAGS_LS          6

#define C_FLAGS_X           (1 << 3)

// Instrument struct : mm_mas_instrument

#define C_MASI_GVOL         0
#define C_MASI_FADE         1
#define C_MASI_RANDVOL      2
#define C_MASI_DCT          3
#define C_MASI_NNA          4
#define C_MASI_ENVFLAGS     5
#define C_MASI_PAN          6
#define C_MASI_DCA          7
#define C_MASI_MAP          8
#define C_MASI_ENVELOPES    12 // TODO: This is misleading!
// #define C_MASI_ENVELOPES    248

// Instrument envelope struct : mm_mas_envelope

#define C_MASIE_SIZE        0
#define C_MASIE_LSTART      1
#define C_MASIE_LEND        2
#define C_MASIE_SSTART      3
#define C_MASIE_SEND        4
#define C_MASIE_NODEC       5
#define C_MASIE_FILTER      6
#define C_MASIE_NODES       8

#define ENVFLAG_A           (1 << 3)

// Sample structure : mm_mas_sample_info

#define C_MASS_DV           0
#define C_MASS_PAN          1
#define C_MASS_FREQ         2
#define C_MASS_VIT          4
#define C_MASS_VID          5
#define C_MASS_VIS          6
#define C_MASS_GV           7
#define C_MASS_VIR          8
#define C_MASS_MSLID        10

// Pattern structure

#define C_MASP_NROWS        0
#define C_MASP_DATA         1

// Sample structure

#define C_SAMPLE_LEN        0
#define C_SAMPLE_LOOP       4
#define C_SAMPLE_POINT      12
#define C_SAMPLE_DATA       16

#define C_SAMPLEN_LSTART    0
#define C_SAMPLEN_LEN       4
#define C_SAMPLEN_FORMAT    8
#define C_SAMPLEN_REP       9
#define C_SAMPLEN_POINT     12
#define C_SAMPLEN_DATA      16

#define C_SAMPLEC_DFREQ     10

#endif // MP_FORMAT_MAS_H
