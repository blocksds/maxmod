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
 *                          DS Memory Operations                            *
 *                                                                          *
 ****************************************************************************/

	.global mmLoad
	.global mmUnload

	.global	mmLoadEffect
	.global	mmUnloadEffect

@----------------------------------------------
	
#include "mp_defs.inc"
#include "mp_format_mas.inc"

@----------------------------------------------

.text
.syntax unified
.thumb
.align 2

.thumb_func
@--------------------------------------------------------------------------------
mmLoad:			@ params={ module_ID }
@--------------------------------------------------------------------------------
	
	push	{r4-r7, lr}
	ldr	r7,=mmcbMemory
	ldr	r7, [r7]
	
@---------------------------------------
@ check for existing module
@---------------------------------------
	
	ldr	r1,=mmModuleBank
	ldr	r1, [r1]
	lsls	r2, r0, #2
	adds	r4, r1, r2
	ldr	r2, [r4]
	cmp	r2, #0
	beq	1f
	
	pop	{r4-r7, pc}
	
@---------------------------------------
@ load module into memory
@---------------------------------------
	
1:	movs	r1, r0
	movs	r0, #MMCB_SONGREQUEST
	
	blx	r7
	str	r0, [r4]
	
	movs	r4, r0
	adds	r4, #8
	
@---------------------------------------
@ load samples into memory
@---------------------------------------
	
@ calculate sample numbers offset
	ldrb	r0, [r4, #C_MAS_INSTN]
	ldrb	r6, [r4, #C_MAS_SAMPN]
	movs	r5, r4
	adds	r5, #255
	adds	r5, #C_MAS_TABLES-255
	lsls	r0, #2
	adds	r5, r0
	
@ r5 = sample table
	
@ load samples...
	
.mppl_samples:
	ldr	r0, [r5]
	adds	r5, #4
	adds	r0, r4
	ldrh	r0, [r0, #C_MASS_MSLID]
	
	bl	mmLoadEffect
	
	subs	r6, #1
	bne	.mppl_samples
	
@----------------------------------------
@ ready for playback! :D
@----------------------------------------
	bl	mmFlushBank @ arm function
	
	pop	{r4-r7, pc}
.pool
.align 2
	
.thumb_func
@--------------------------------------------------------------------------------
mmUnload:		@ params={ module_ID }
@--------------------------------------------------------------------------------
	
	push	{r4-r7, lr}
	push	{r0}
	ldr	r7,=mmcbMemory
	ldr	r7, [r7]
	movs	r6, #0
	ldr	r1,=mmModuleBank
	ldr	r1, [r1]
	lsls	r0, #2
	ldr	r4, [r1, r0]
	cmp	r4, #0
	beq	1f
	
	adds	r4, r4, #8
	movs	r6, r4
	ldrb	r5, [r4, #C_MAS_SAMPN]
	
	ldrb	r0, [r4, #C_MAS_INSTN]
	lsls	r0, #2
	adds	r4, r0
	adds	r4, #255
	adds	r4, #C_MAS_TABLES-255
	
@------------------------------------
@ free samples
@------------------------------------
	
3:	ldr	r0, [r4]
	adds	r0, r6
	adds	r4, #4
	ldrh	r0, [r0, #C_MASS_MSLID]
	
	bl	mmUnloadEffect
	
	subs	r5, #1		@ dec sample counter
	bne	3b
	
@------------------------------------
@ free module
@------------------------------------
	
	pop	{r0}
	lsls	r0, #2
	ldr	r2,=mmModuleBank
	ldr	r2, [r2]
	ldr	r1, [r2,r0]
	movs	r3, #0
	str	r3, [r2,r0]
	movs	r0, #MMCB_DELETESONG
	blx	r7

@------------------------------------
@ flush bank
@------------------------------------

	bl	mmFlushBank

1:	pop	{r4-r7, pc}
.pool
.align 2

.thumb_func
@--------------------------------------------------------
mmLoadEffect:		@ params={ msl_id }
@--------------------------------------------------------
	
@ load a sample into memory
@ OR increase instance count of existing sample
	
	push	{lr}
	
	ldr	r1,=mmSampleBank	@ get sample bank
	ldr	r1, [r1]
	
	lsls	r2, r0, #2		@ read sample entry
	ldr	r3, [r1, r2]
	cmp	r3, #0			@ check if instance exists
	bne	.mppls_exists
	
	push	{r1-r2}
	movs	r1, r0			@ no instance
	movs	r0, #MMCB_SAMPREQUEST	@ request sample from user
	ldr	r2,=mmcbMemory
	ldr	r2, [r2]
	blx	r2
	pop	{r1-r2}
	lsls	r0, #8			@ clear high byte of address
	lsrs	r3, r0, #8		@
	
.mppls_exists:
	
	ldr	r0,=0x1000000		@ increment instance count
	adds	r3, r0
	str	r3, [r1, r2]		@ write sample entry
	
	bl	mmFlushBank
	
	pop	{pc}
.pool
.align 2

.thumb_func
@--------------------------------------------------------------------------------
mmUnloadEffect:		@ params={ msl_id }
@--------------------------------------------------------------------------------

@ decrease instance counter
@ unload sample from memory if zero

	push	{lr}
	
	ldr	r1,=mmSampleBank	@ get sample bank
	ldr	r1, [r1]
	
	lsls	r0, #2			@ load sample entry
	ldr	r2, [r1, r0]
	lsrs	r3, r2, #24		@ mask counter value
	
	beq	1f			@ exit if already zero
	ldr	r3,=0x1000000		@ subtract 1 from instance counter
	subs	r2, r3
	lsrs	r3, r2, #24		@ mask counter vaue
	bne	2f			@ skip unload if sample
					@   is still referenced
	
	push	{r0-r2, r7}
	ldr	r3,=0x2000000		@ unload sample from memory
	adds	r1, r2, r3		@ param_b = sample address
	movs	r0, #MMCB_DELETESAMPLE	@ param_a = message
	ldr	r7,=mmcbMemory		@ jump to callback
	ldr	r7, [r7]
	blx	r7
	pop	{r0-r2, r7}
	
1:
	movs	r2, #0			@ clear sample entry
	
2:
	str	r2, [r1, r0]		@ save sample entry
	
	bl	mmFlushBank
	
	pop	{pc}
	
.pool

