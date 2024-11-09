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

.global mmFlushBank

.text
.syntax unified
.thumb
.align 2

.thumb_func
@-------------------------------------------------------
mmFlushBank:
@-------------------------------------------------------

	ldr	r0,=mmMemoryBank	@{get memory bank address
	ldr	r0, [r0]		@}
	ldr	r1,=mmModuleCount	@{
	ldr	r1, [r1]		@
	ldr	r2,=mmSampleCount	@ get memory bank size (mods+samps)
	ldr	r2, [r2]		@
	adds	r1, r2			@}
	
	lsrs	r0, #5			@{align memory bank address
	lsls	r0, #5			@}(is this neccesary?)
	adds	r1, r1, #1		@-catch last bit (bug? should be #8?)
	 
@ r0 = bank address
@ r1 = module+sample count
@ cache line size == 32
	
	ldr	r3,=.flushbank		@{switch to arm (for cp15 instructions)
	bx	r3			@}

.arm
.align 2
	 
.flushbank:
	mcr	p15, 0, r0, c7, c14, 1	@-clean and invalidate cache line
	add	r0, r0, #32		@-next 32 bytes...
	subs	r1, #8			@-sub 8 (words)
	bpl	.flushbank		@-loop
	
	bx	lr

.pool
