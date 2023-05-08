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

// MAXMOD SOUNDBANK FORMAT DEFINITIONS

#ifndef MM_MSL_H
#define MM_MSL_H

#include "mm_types.h"

typedef struct tmslheaddata
{
	mm_hword	sampleCount;
	mm_hword	moduleCount;
	mm_word		reserved[2];
} msl_head_data;

typedef struct tmslhead
{
	msl_head_data head_data;
	mm_addr		sampleTable[]; // [MSL_NSAMPS];
	//mm_addr	moduleTable[MSL_NSONGS];
} msl_head;

#endif

