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

.ifndef MP_MIXER_DS_INC
.equ MP_MIXER_DS_INC, 1

@ definitions

.equ	MIXER_CHN_SAMP,		0	// mainram address
.equ	MIXER_CHN_CNT,		3	// keyon:1bit,panning:7bits
.equ	MIXER_CHN_FREQ,		4	// unsigned 3.10, top 3 cleared
.equ	MIXER_CHN_VOL,		6	// 0-65535
.equ	MIXER_CHN_READ,		8	// unsigned 22.10
.equ	MIXER_CHN_CVOL,		12	// current volume
.equ	MIXER_CHN_CPAN,		14	// current panning
.equ	MIXER_CHN_SIZE,		16

// scale = 65536*1024*2 / mixrate
.equ	MIXER_SCALE,		4096 //6151

.equ	MIXER_CF_START,		128
.equ	MIXER_CF_SOFT,		2
.equ	MIXER_CF_SURROUND,	4
.equ	MIXER_CF_FILTER,	8
.equ	MIXER_CF_REVERSE,	16

.endif
