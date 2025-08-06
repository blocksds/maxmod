// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MM_CORE_CHANNEL_TYPES_H__
#define MM_CORE_CHANNEL_TYPES_H__

#include <assert.h>

#ifdef __GBA__
#include <maxmod.h>
#endif
#include <mm_types.h>

// Module Channel
// --------------

typedef struct {
    mm_byte     alloc;  // Allocated active channel number
    mm_byte     cflags; // Pattern comression flags, called "maskvariable" in ITTECH.TXT
    mm_byte     panning;
    mm_byte     volcmd; // Volume column command
    mm_byte     effect; // Effect number
    mm_byte     param;  // Effect parameter
    mm_byte     fxmem;  // Effect memory
    mm_byte     note;   // Translated note
    mm_byte     flags;  // Channel flags
    mm_byte     inst;   // Instrument#
    mm_byte     pflags; // Playback flags (unused)
    mm_byte     vibdep;
    mm_byte     vibspd;
    mm_byte     vibpos;
    mm_byte     volume;
    mm_byte     cvolume;
    mm_word     period;
    mm_hword    bflags;
    mm_byte     pnoter;  // Pattern note
    mm_byte     memory[15];
    mm_byte     padding[2];
} mm_module_channel;

static_assert(sizeof(mm_module_channel) == 40);
#ifdef __GBA__
static_assert(sizeof(mm_module_channel) == MM_SIZEOF_MODCH);
#endif

// New note action (NNA)
#define MCH_BFLAGS_NNA_SHIFT    6
#define MCH_BFLAGS_NNA_MASK     (3 << MCH_BFLAGS_NNA_SHIFT)
#define MCH_BFLAGS_NNA_GET(x)   (((x) & MCH_BFLAGS_NNA_MASK) >> MCH_BFLAGS_NNA_SHIFT)
#define MCH_BFLAGS_NNA_SET(x)   (((x) << MCH_BFLAGS_NNA_SHIFT) & MCH_BFLAGS_NNA_MASK)

#define MCH_BFLAGS_TREMOR       (1 << 9)    // Tremor variable
#define MCH_BFLAGS_CUT_VOLUME   (1 << 10)   // Cut channel volume

// BFLAGS
//
// Bits  | Meaning
// ------+------------------------------------
// 0-1   | Vibrato waveform         // Unused
// 2-3   | Tremolo waveform         // Unused
// 4-5   | Panbrello waveform       // Unused
// 6-7   | New note action (NNA)
//
// ??-?? | Duplicate check type     // Unused
//
// 8     | Volume envelope enabled  // Unused
// 9     | Tremor variable
// 10    | Cut channel volume
// 11-15 | Unused

// Active Channel
// --------------

typedef struct {
    mm_word     period;     // internal period
    mm_hword    fade;
    mm_hword    envc_vol;
    mm_hword    envc_pan;
    mm_hword    envc_pic;
    mm_hword    avib_dep;
    mm_hword    avib_pos;
    mm_byte     fvol;
    mm_byte     type;
    mm_byte     inst;
    mm_byte     panning;
    mm_byte     volume;
    mm_byte     sample;
    mm_byte     parent;
    mm_byte     flags;
    mm_byte     envn_vol;
    mm_byte     envn_pan;
    mm_byte     envn_pic;
    mm_byte     sfx;        // can store this anywhere
} mm_active_channel;

static_assert(sizeof(mm_active_channel) == 28);
#ifdef __GBA__
static_assert(sizeof(mm_active_channel) == MM_SIZEOF_ACTCH);
#endif

#ifdef __NDS__

// A DS mixer channel is active if "samp" isn't zero.
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

#define C_READ_FRAC 10

// scale = 65536*1024*2 / mixrate
#define MIXER_SCALE         4096 //6151

/*
TODO: The defines are unused, they refer to the tpan/key_on byte
#define MIXER_CF_START      128
#define MIXER_CF_SOFT       2
#define MIXER_CF_SURROUND   4
#define MIXER_CF_FILTER     8
#define MIXER_CF_REVERSE    16
*/

#endif // __NDS__

#ifdef __GBA__

#define MIXCH_GBA_SRC_STOPPED (1u << 31)

// A GBA mixer channel is active if "src & (1 << 31)" is zero.
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
static_assert(sizeof(mm_mixer_channel) == MM_SIZEOF_MIXCH);

#endif // __GBA__

#endif // MM_CORE_CHANNEL_TYPES_H__
