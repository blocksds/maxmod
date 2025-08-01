// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_CORE_MAS_STRUCTS_H__
#define MM_CORE_MAS_STRUCTS_H__

#include <assert.h>

#include <mm_types.h>

#include "core/channel_types.h"

typedef struct {
    mm_byte     tick;       // Current tick count
    mm_byte     row;        // Current row being played
    mm_byte     position;   // Module sequence position
    mm_byte     nrows;      // Number of rows in current pattern
    mm_byte     gv;         // Global volume multiplier
    mm_byte     speed;      // Speed of module (ticks / row)
    mm_byte     isplaying;  // Module is active
    mm_byte     bpm;        // Tempo of module
    mm_word    *insttable;  // Table of offsets (from mm_mas_head base) to instrument data
    mm_word    *samptable;  // Table of offsets (from mm_mas_head base) to sample data
    mm_word    *patttable;  // Table of offsets (from mm_mas_head base) to pattern data
    mm_mas_head *songadr;   // Pointer to the current MAS module being played
    mm_byte     flags;
    mm_byte     oldeffects;
    mm_byte     pattjump;
    mm_byte     pattjump_row;
    mm_byte     fpattdelay;
    mm_byte     pattdelay;

    mm_byte     ploop_row;   // }
    mm_byte     ploop_times; // } Variables used for pattern loops
    mm_byte    *ploop_adr;   // }
    mm_byte    *pattread;
    mm_byte     ploop_jump;
    mm_byte     valid;

    mm_hword    tickrate;  // 1.15 fixed point OR sample count
    union {
        mm_hword    sampcount; // sample timing
        mm_hword    tickfrac;  // vsync  timing 0.16 fixed point
    };

    mm_byte     mode;
    mm_byte     reserved2;
    mm_word     mch_update;
    mm_hword    volume;
    mm_hword    reserved3;
} mpl_layer_information;

static_assert(sizeof(mpl_layer_information) == 56);

// Active Information
// ------------------

typedef struct {
    mm_word     reserved;
    mm_byte    *pattread_p;
    mm_byte     afvol;
    mm_byte     sampoff;
    mm_sbyte    volplus;
    mm_byte     notedelay;
    mm_hword    panplus;
    mm_hword    reserved2;
} mpv_active_information;

static_assert(sizeof(mpv_active_information) == 16);

// Active Channel Flags
// --------------------

#define MCAF_KEYON      (1 << 0) // key is on... LOCKED
#define MCAF_FADE       (1 << 1) // note-fade is activated
#define MCAF_START      (1 << 2) // [re]start sample
#define MCAF_UPDATED    (1 << 3) // already updated by pchannel routine
#define MCAF_ENVEND     (1 << 4) // end of envelope
#define MCAF_VOLENV     (1 << 5) // volume envelope enabled
#define MCAF_SUB        (1 << 6) // sublayer.................locked..
#define MCAF_EFFECT     (1 << 7) // subsublayer.............................LOCKED (mpEffect)

// Active Channel Types
// --------------------

#define ACHN_DISABLED   0 // LOCKED (multiple routines)
#define ACHN_RESERVED   1 // (can't be used [alloc channel])
#define ACHN_BACKGROUND 2 // LOCKED (alloc channel)
#define ACHN_FOREGROUND 3
#define ACHN_CUSTOM     4

// Module Channel Flags
// --------------------

#define MF_START        1
#define MF_DVOL         2
#define MF_HASVCMD      4
#define MF_HASFX        8
#define MF_NEWINSTR     16  // New instrument
#define MF_UNKNOWN      32  // TODO: This is set by mmutil but not used
#define MF_NOTEOFF      64  // LOCKED
#define MF_NOTECUT      128 // LOCKED

// Other Definitions
// -----------------

#define IT_NNA_CUT      0 // New note actions
#define IT_NNA_CONT     1
#define IT_NNA_OFF      2
#define IT_NNA_FADE     3

#define IT_DCA_CUT      0 // Duplicate check actions
#define IT_DCA_OFF      1
#define IT_DCA_FADE     2

#endif // MM_CORE_MAS_STRUCTS_H__
