// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

#include "mp_format_mas.inc"
#include "mp_mas_structs.inc"
#include "mp_defs.inc"

#ifdef SYS_GBA
#include "mp_mixer_gba.inc"
#endif

#ifdef SYS_NDS
#include "mp_mixer_ds.inc"
#endif

    .syntax unified

/******************************************************************************
 *
 * Program
 *
 ******************************************************************************/

//-----------------------------------------------------------------------------
// TEXT Code
//-----------------------------------------------------------------------------
	.text
	.thumb
	.align 2
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// IWRAM CODE
//-----------------------------------------------------------------------------
#ifdef USE_IWRAM
	.section ".iwram", "ax", %progbits
	.align 2
#endif
//-----------------------------------------------------------------------------

.align 2
					.global mpp_Update_ACHN_notest_Wrapper
					.thumb_func
mpp_Update_ACHN_notest_Wrapper:
	push	{r4-r7,lr}
	mov	r7, r8
	push	{r7}

	mov	r8, r0
	movs	r6, r1
	movs	r7, r2
	movs	r5, r3
	// This argument is passed on the stack by the caller, but 6 registers have
	// been pushed to the stack as well.
	ldr	r4, [sp, #6 * 4]

	bl	mpp_Update_ACHN_notest
	movs	r0, r5

	pop	{r7}
	mov	r8, r7
	pop	{r4-r7}
	pop	{r1}
	bx	r1

@--------------------------------------------

	.global mpp_Update_ACHN_notest
	.thumb_func
@----------------------------------------------------------------------------------------------------
mpp_Update_ACHN_notest:
@----------------------------------------------------------------------------------------------------
@ r5 = affected period
@ r6 = achannel address
	push	{lr}

@------------------------------------------------------------------------
@ Envelope Processing
@------------------------------------------------------------------------

	ldrb	r0, [r6, #MCA_INST]
	subs	r0, #1
	bcs	1f
	b	.mppt_achn_noinst
1:

	mov		r0, r8 // layer
	movs	r1, r6 // act_ch
	movs	r2, r5 // period
	bl		mpp_Update_ACHN_notest_envelopes
	movs	r5, r0 // period

@----------------------------------------------------------------------------------
@ *** PROCESS AUTO VIBRATO
@----------------------------------------------------------------------------------

	ldrb	r0, [r6, #MCA_SAMPLE]
	subs	r0, #1
	bcc	.mppt_achn_nostart	@ no sample!!

	mov		r0, r8 // layer
	movs	r1, r6 // act_ch
	movs	r2, r5 // period
	bl		mpp_Update_ACHN_notest_auto_vibrato
	movs	r5, r0 // period

@---------------------------------------------------------------------------------

.mppt_achn_noinst:

	push	{r4}

	mov		r0, r8 // layer
	movs	r1, r6 // act_ch
	movs	r2, r4 // channel
	bl		mpp_Update_ACHN_notest_update_mix
	movs	r4, r0 // mixer channel

.mppt_achn_nostart:

	mov		r0, r8 // layer
	movs	r1, r6 // act_ch
	movs	r2, r5 // period
	movs	r3, r4 // mx_ch
	bl	mpp_Update_ACHN_notest_set_pitch_volume
	movs	r1, r0 // volume

	movs	r0, r1 // volume
	movs	r1, r6 // act_ch
	movs	r2, r4 // mx_ch
	bl	mpp_Update_ACHN_notest_disable_and_panning

	pop	{r4}
	pop	{r0}
	bx	r0

.end
