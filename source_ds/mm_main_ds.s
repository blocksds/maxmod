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

/******************************************************************************
 *
 * Definitions
 *
 ******************************************************************************/

#include "mp_defs.inc"
#include "mp_mas.inc"
#include "mp_mas_structs.inc"
#include "mp_macros.inc"
#include "mp_mixer_ds.inc"

/******************************************************************************
 *
 * NDS
 *
 ******************************************************************************/

	.TEXT
	.THUMB
	.ALIGN 2

/******************************************************************************
 * mmSuspendIRQ_t
 *
 * Function to disable interrupts via the status register
 ******************************************************************************/
						.global mmSuspendIRQ_t
						.thumb_func
mmSuspendIRQ_t:
	ldr	r0,=1f
	bx	r0

.arm
.align 2
1:	mrs	r0, cpsr
	and	r1, r0, #0x80
	orr	r0, #0x80
	msr	cpsr, r0
	str	r1, previous_irq_state
	bx	lr
.thumb

/******************************************************************************
 * mmRestoreIRQ_t
 *
 * Function to enable interrupts via the status register
 ******************************************************************************/	
						.global	mmRestoreIRQ_t
						.thumb_func
mmRestoreIRQ_t:
	ldr	r0,=1f
	bx	r0

.arm
.align 2
1:	mrs	r0, cpsr
	ldr	r1, previous_irq_state
	bic	r0, #0x80
	orr	r0, r1
	msr	cpsr, r0
	bx	lr
	
.thumb

previous_irq_state:
	.space	4

.pool

//-----------------------------------------------------------------------------
.end
//-----------------------------------------------------------------------------
