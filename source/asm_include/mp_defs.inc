// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

/****************************************************************************
 *                                                          __              *
 *                ____ ___  ____ __  ______ ___  ____  ____/ /              *
 *               / __ `__ \/ __ `/ |/ / __ `__ \/ __ \/ __  /               *
 *              / / / / / / /_/ />  </ / / / / / /_/ / /_/ /                *
 *             /_/ /_/ /_/\__,_/_/|_/_/ /_/ /_/\____/\__,_/                 *
 *                                                                          *
 *               Nintendo DS & Gameboy Advance Sound System                 *
 *                                                                          *
 ****************************************************************************/

#ifndef MP_DEFS_INC
#define MP_DEFS_INC

@ song 'mode' can be one of the following:
.equ		MPP_PLAY_LOOP	,0
.equ		MPP_PLAY_ONCE	,1
.equ		MPP_PLAY_JINGLE	,2

@ other definitions
#ifdef SYS_GBA

// This struct is in mm_types.h: mm_gba_system
.equ MM_GBA_SYSTEM_MODE,	0
.equ MM_GBA_SYSTEM_MCH_COUNT,	MM_GBA_SYSTEM_MODE	+ 4
.equ MM_GBA_SYSTEM_ACH_COUNT,	MM_GBA_SYSTEM_MCH_COUNT	+ 4
.equ MM_GBA_SYSTEM_MODCH,	MM_GBA_SYSTEM_ACH_COUNT	+ 4
.equ MM_GBA_SYSTEM_ACTCH,	MM_GBA_SYSTEM_MODCH	+ 4
.equ MM_GBA_SYSTEM_MIXCH,	MM_GBA_SYSTEM_ACTCH	+ 4
.equ MM_GBA_SYSTEM_MIXMEM,	MM_GBA_SYSTEM_MIXCH	+ 4
.equ MM_GBA_SYSTEM_WAVEMEM,	MM_GBA_SYSTEM_MIXMEM	+ 4
.equ MM_GBA_SYSTEM_SOUNDBANK,	MM_GBA_SYSTEM_WAVEMEM	+ 4
.equ MM_GBA_SYSTEM_SIZE,	MM_GBA_SYSTEM_SOUNDBANK + 4

.equ		SAMPFRAC,		12	@ # of bits used in fractional part of sample reading
#endif

.equ		MP_SCHANNELS	,4

@ callback parameters

.equ	MMCB_SONGREQUEST	,0x1A	@ nds9
.equ	MMCB_SAMPREQUEST	,0x1B	@ nds9
.equ	MMCB_DELETESONG		,0x1C	@ nds9
.equ	MMCB_DELETESAMPLE	,0x1D	@ nds9

@.equ	MPCB_SAMPMEMORY		,0x1E	@ ---
@.equ	MPCB_SONGMEMORY		,0x1F	@ ---
.equ	MMCB_BANKMEMORY		,0x1E	@ nds9

.equ	MPCB_SONGMESSAGE	,0x2A	@ gba/nds7 song playback
.equ	MPCB_SONGFINISHED	,0x2B	@ gba/nds7

.equ mms_rate,		0
.equ mms_len,		mms_rate	+ 4
.equ mms_function,	mms_len		+ 4
.equ mms_format,	mms_function	+ 4
.equ mms_timer,		mms_format	+ 4
.equ mms_manual,	mms_timer	+ 4
.equ mms_size,		mms_manual	+ 1

// reverb cfg structure
.equ mmrc_flags,	0
.equ mmrc_memory,	mmrc_flags	+ 4
.equ mmrc_delay,	mmrc_memory	+ 4
.equ mmrc_rate,		mmrc_delay	+ 2
.equ mmrc_feedback,	mmrc_rate	+ 2
.equ mmrc_panning,	mmrc_feedback	+ 2
// mmrc_panning is length 1

.equ	MMRFS_MEMORY,		0
.equ	MMRFS_DELAY,		1
.equ	MMRFS_RATE,		2
.equ	MMRFS_FEEDBACK,		3
.equ	MMRFS_PANNING,		4
.equ	MMRFS_LEFT,		5
.equ	MMRFS_RIGHT,		6
.equ	MMRFS_INVERSEPAN,	7
.equ	MMRFS_NODRYLEFT,	8
.equ	MMRFS_NODRYRIGHT,	9
.equ	MMRFS_8BITLEFT,		10
.equ	MMRFS_16BITLEFT,	11
.equ	MMRFS_8BITRIGHT, 	12
.equ	MMRFS_16BITRIGHT,	13
.equ	MMRFS_DRYLEFT,		14
.equ	MMRFS_DRYRIGHT,		15

// reverbcfg flags
.equ	MMRF_MEMORY,	1<<MMRFS_MEMORY
.equ	MMRF_DELAY,	1<<MMRFS_DELAY
.equ	MMRF_RATE,	1<<MMRFS_RATE
.equ	MMRF_FEEDBACK,	1<<MMRFS_FEEDBACK
.equ	MMRF_PANNING,	1<<MMRFS_PANNING
.equ	MMRF_LEFT,	1<<MMRFS_LEFT
.equ	MMRF_RIGHT,	1<<MMRFS_RIGHT
.equ	MMRF_INVERSEPAN,1<<MMRFS_INVERSEPAN
.equ	MMRF_NODRYLEFT,	1<<MMRFS_DRYLEFT
.equ	MMRF_NODRYRIGHT,1<<MMRFS_DRYRIGHT
.equ	MMRF_8BITLEFT,	1<<MMRFS_8BITLEFT
.equ	MMRF_16BITLEFT,	1<<MMRFS_16BITLEFT
.equ	MMRF_8BITRIGHT, 1<<MMRFS_8BITRIGHT
.equ	MMRF_16BITRIGHT,1<<MMRFS_16BITRIGHT
.equ	MMRF_DRYLEFT,	1<<MMRFS_DRYLEFT
.equ	MMRF_DRYRIGHT,	1<<MMRFS_DRYRIGHT

#endif
