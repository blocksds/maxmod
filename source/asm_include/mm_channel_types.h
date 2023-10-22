// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MM_CHANNEL_TYPES_H
#define MM_CHANNEL_TYPES_H

#include <assert.h>
#include <mm_types.h>

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

#ifdef SYS_NDS

typedef struct {
    mm_word     samp   : 24;    // 0:23  mainram address
    mm_word     tpan   : 7;     // 24:30 = target panning
    mm_word     key_on : 1;     // 31 = key-on

    mm_hword    freq;           // unsigned 3.10, top 3 cleared
    mm_hword    vol;            // target volume   0..65535
    mm_word     read;           // unsigned 22.10
    mm_hword    cvol;           // current volume  0..65535
    mm_hword    cpan;           // current panning 0..65535
} mm_mixer_channel;

static_assert(sizeof(mm_mixer_channel) == 16);

// scale = 65536*1024*2 / mixrate
#define MIXER_SCALE         4096 //6151

#define MIXER_CF_START      128
#define MIXER_CF_SOFT       2
#define MIXER_CF_SURROUND   4
#define MIXER_CF_FILTER     8
#define MIXER_CF_REVERSE    16

#endif // SYS_NDS

#ifdef SYS_GBA

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

#endif // SYS_GBA

#endif // MM_CHANNEL_TYPES_H
