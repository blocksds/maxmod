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

#ifndef MP_MIXER_GBA_INC
#define MP_MIXER_GBA_INC

@ definitions

.equ	MP_SAMPFRAC 		,12

.equ	MIXER_CHN_SRC		,0
.equ	MIXER_CHN_READ		,4
.equ	MIXER_CHN_VOL		,8	
.equ	MIXER_CHN_PAN		,9
// 10
// 11
.equ	MIXER_CHN_FREQ		,12
.equ	MIXER_CHN_SIZE		,16

//////////////////////////////////.equ	MIXER_CHN_LOOP		,8
/////////////////////////////////////.equ	MIXER_CHN_LEN		,16

#endif
