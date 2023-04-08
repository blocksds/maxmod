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

#ifndef	MP_MAS_INC
#define	MP_MAS_INC

@#ifdef SYS_NDS
@.equ	MP_MCHANNELS	,16
@#else
@.equ	MP_MCHANNELS	,32
@#endif

.extern mpp_vars
.extern mpp_pattread
.extern mpp_sm_handle

.extern mpp_resetchannels
.extern mpp_PlaySong
.extern mpp_processtick

#endif
