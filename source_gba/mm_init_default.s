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
 
.global mmInitDefault
.syntax unified

@----------------------------------------------------------------------------

#include "mp_defs.inc"

.equ mixlen, 1056	// 16khz

	.bss
	.align 2

__mixbuffer:
	.space mixlen

	.text
	.thumb
	.align 2

#define MM_SIZEOF_MODCH		40
#define MM_SIZEOF_ACTCH		28
#define MM_SIZEOF_MIXCH		24
	
/****************************************************************************
 * mmInitDefault( soundbank, #channels )
 *
 * Init maxmod with default settings.
 ****************************************************************************/
						.thumb_func
mmInitDefault:
	
	push	{r0,r4,r5,r6,r7,lr}		// preserve regs, push soundbank
	
	//0 mode (3)
	//1 mchcount (#channels)
	//2 achcount (#channels)
	//3 modch
	//4 actch
	//5 mixch
	//6 mixmem (__mixbuffer)
	//7 wavemem
	
	movs	r6, r1				// r6=#channels
	ldr	r0,=MM_SIZEOF_MODCH+MM_SIZEOF_ACTCH+MM_SIZEOF_MIXCH
	muls	r0, r6
	ldr	r4,=mixlen
	adds	r0, r4
	movs	r1, #1
	bl	calloc
	
	movs	r7, r0				// wavemem = beginning of buffer
	adds	r3, r0, r4			// split up buffer into addresses [r3,r4,r5]
	movs	r0, #MM_SIZEOF_MODCH		//
	muls	r0, r6				//
	adds	r4, r3, r0			//
	movs	r0, #MM_SIZEOF_ACTCH		//
	muls	r0, r6				//
	adds	r5, r4, r0			//
	movs	r0, #3				//
	movs	r1, r6				//
	movs	r2, r6				//
	ldr	r6,=__mixbuffer			// r6 = mixbuffer (iwram)
	
	push	{r0-r7}
	
	mov	r0, sp				// init maxmod, pass init struct
	bl	mmInit				//
	
	add	sp, #MM_GBA_SYSTEM_SIZE		// restore stack
	
	pop	{r4-r7}				// return
	pop	{r0}				//
	bx	r0				//
	
.pool
