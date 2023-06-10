// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MP_MAS_STRUCTS_H
#define MP_MAS_STRUCTS_H

#include <assert.h>

#include "mm_types.h"

// There was an incomplete version of this struct in mm_types: mm_modlayer
typedef struct {
    mm_byte     tick;       // Current tick count
    mm_byte     row;        // Current row being played
    mm_byte     position;   // Module sequence position
    mm_byte     nrows;      // Number of rows in current pattern
    mm_byte     gv;         // Global volume multiplier
    mm_byte     speed;      // Speed of module (ticks / row)
    mm_byte     isplaying;  // Module is active
    mm_byte     bpm;        // Tempo of module
    mm_word     insttable;
    mm_word     samptable;
    mm_word     patttable;
    mm_word     songadr;
    mm_byte     flags;
    mm_byte     oldeffects;
    mm_byte     pattjump;
    mm_byte     pattjump_row;
    mm_byte     fpattdelay;
    mm_byte     pattdelay;

    mm_byte     ploop_row;
    mm_byte     ploop_times;
    mm_word     ploop_adr;
    mm_word     pattread;
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

typedef struct __attribute__ ((packed)) { // TODO: Unpack
    mm_word     reserved;
    mm_word     pattread_p;
    mm_byte     afvol;
    mm_byte     sampoff;
    mm_byte     volplus;
    mm_byte     notedelay;
    mm_hword    panplus;
} mpv_active_information;

static_assert(sizeof(mpv_active_information) == 14);

// Module Channel
// --------------

typedef struct {
    mm_byte     alloc;  // Allocated active channel number
    mm_byte     cflags; // Pattern comression flags, called "maskvariable" in ITTECH.TXT
    mm_byte     panning;
    mm_byte     volcmd; // Volume column command
    mm_byte     effect; // Effect number    } Combined
    mm_byte     param;  // Effect parameter }
    mm_byte     fxmem;  // Effect memory
    mm_byte     note;   // Translated note
    mm_byte     flags;  // Channel flags
    mm_byte     inst;   // Instrument#
    mm_byte     pflags; // Playback flags (unused)
    mm_byte     vibdep;
    mm_byte     vibspd;
    mm_byte     vibpos;
    mm_byte     volume;  // } Combined
    mm_byte     cvolume; // }
    mm_word     period;
    mm_hword    bflags;
    mm_byte     pnoter;  // Pattern note
    mm_byte     memory[15];
    mm_byte     padding[2];
} mm_module_channel;

static_assert(sizeof(mm_module_channel) == 40);

// Active Channel
// --------------

typedef struct {
    mm_word     period;     // internal period
    mm_hword    fade;       //    }
    mm_hword    envc_vol;   //    }
    mm_hword    envc_pan;   //    } COMBINED
    mm_hword    envc_pic;   //    } during volume reset
    mm_hword    avib_dep;   //    } AND NEWNOTE, CHECK NEWNOTE ON CHANGES
    mm_hword    avib_pos;   //    }
    mm_byte     fvol;       // } COMBINED for SPEED
    mm_byte     type;       // }
    mm_byte     inst;
    mm_byte     panning;
    mm_byte     volume;
    mm_byte     sample;
    mm_byte     parent;     // } COMBINED
    mm_byte     flags;      // }
    mm_byte     envn_vol;
    mm_byte     envn_pan;
    mm_byte     envn_pic;
    mm_byte     sfx;        // can store this anywhere
} mm_active_channel;

static_assert(sizeof(mm_active_channel) == 28);

// Active Channel Flags
// --------------------

#define MCAF_KEYON      1   // key is on... LOCKED
#define MCAF_FADE       2   // note-fade is activated
#define MCAF_START      4   // [re]start sample
#define MCAF_UPDATED    8   // already updated by pchannel routine
#define MCAF_ENVEND     16  // end of envelope
#define MCAF_VOLENV     32  // volume envelope enabled
#define MCAF_SUB        64  // sublayer.................locked..
#define MCAF_EFFECT     128 // subsublayer.............................LOCKED (mpEffect)

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
#define MF_NEWINSTR     16

#define MF_NOTEOFF      64  // LOCKED
#define MF_NOTECUT      128 // LOCKED

// #define MF_NEWINSTR         1 // new instrument
// #define MF_DVOL             2
// #define MF_START            4
// #define MF_HASFX            8
// #define MF_HASVCMD          16
// #define MF_NOTEOFF          64  // LOCKED
// #define MF_NOTECUT          128 // LOCKED

// Other Definitions
// -----------------

#define IT_NNA_CUT      0 // New note actions
#define IT_NNA_CONT     1
#define IT_NNA_OFF      2
#define IT_NNA_FADE     3

#define IT_DCA_CUT      0 // Duplicate check actions
#define IT_DCA_OFF      1
#define IT_DCA_FADE     2

// Misc Reference
// --------------

// BFLAGS:
// /////ctv nnppttvv
// nn...............new note action
// pp...............panbrello waveform
// tt...............tremolo waveform
// vv...............vibrato waveform
// dd...............duplicate check type
// v................volume envelope enabled
// t................tremor variable...
// c................cut channel volume
// //////...........reserved

#endif // MP_MAS_STRUCTS_H
