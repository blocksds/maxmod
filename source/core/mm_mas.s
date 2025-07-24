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
	bl mpp_Update_ACHN_notest_set_pitch_volume
	movs	r1, r0 // volume

	cmp	r1, #0
	bne	.mppt_achn_audible

#ifdef SYS_NDS			// nds has volume ramping!
	ldrb	r3, [r6, #MCA_TYPE]
	cmp	r3, #ACHN_BACKGROUND
	bne	1f
	ldrb	r3, [r6, #MCA_VOLUME]
	cmp	r3, #0
	bne	1f
	ldrh	r3, [r4, #MIXER_CHN_CVOL]
	cmp	r3, #0
	beq	.mppt_achn_not_audible
#endif

1:	ldrb	r3, [r6, #MCA_FLAGS]
	movs	r2, #MCAF_ENVEND
	tst	r3, r2
	beq	.mppt_achn_audible
	movs	r2, #MCAF_KEYON
	tst	r3, r2
	bne	.mppt_achn_audible

.mppt_achn_not_audible:
	@ STOP CHANNEL

#ifdef SYS_GBA
	ldr	r0,=1<<31
	str	r0,[r4,#MIXER_CHN_SRC]	@ stop mixer channel
#else
	movs	r0,#0
	str	r0,[r4,#MIXER_CHN_SAMP]	@ stop mixer channel
#endif

	ldrb	r3, [r6, #MCA_TYPE]
	cmp	r3, #ACHN_FOREGROUND
	bne	.mpp_achn_noparent
	ldrb	r1, [r6, #MCA_PARENT]
	movs	r3, #MCH_SIZE
	muls	r1, r3
	ldr	r0,=mpp_channels
	ldr	r0, [r0]
	adds	r0, r1
	movs	r1, #255
	strb	r1, [r0, #MCH_ALLOC]
.mpp_achn_noparent:

	movs	r1, #ACHN_DISABLED
	strb	r1, [r6, #MCA_TYPE]
	b	.mpp_achn_updated
.mppt_achn_audible:

#ifdef SYS_NDS
	strh	r1, [r4, #MIXER_CHN_VOL]
#else
	strb	r1, [r4, #MIXER_CHN_VOL]
#endif

#ifdef SYS_GBA						// check if mixer channel has ended
	ldr	r0, [r4, #MIXER_CHN_SRC]
	asrs	r0, #31
	beq	1f
#else
	ldr	r0, [r4, #MIXER_CHN_SAMP]
	lsls	r0, #8
	bne	1f
#endif

	ldrb	r3, [r6, #MCA_TYPE]
	cmp	r3, #ACHN_FOREGROUND
	bne	2f

	ldrb	r1, [r6, #MCA_PARENT]			// stop channel if channel ended
	movs	r3, #MCH_SIZE
	muls	r1, r3
	ldr	r0,=mpp_channels
	ldr	r0, [r0]
	adds	r0, r1
	movs	r1, #255
	strb	r1, [r0, #MCH_ALLOC]
2:

#ifdef SYS_GBA
	ldr	r0,=1<<31
	str	r0,[r4,#MIXER_CHN_SRC]	@ stop mixer channel
#else
	movs	r0,#0
	str	r0,[r4,#MIXER_CHN_SAMP]	@ stop mixer channel
#endif

	movs	r1, #ACHN_DISABLED
	strb	r1, [r6, #MCA_TYPE]
	b	.mpp_achn_updated

	@ set panning
1:	ldr	r1,=mpp_vars
	movs	r3, #MPV_PANPLUS
	ldrsh	r0, [r1,r3]
	ldrb	r1, [r6, #MCA_PANNING]

	adds	r1, r0

	cmp	r1, #0
	bge	.mpp_achn_clippan1
	movs	r1, #0
.mpp_achn_clippan1:
	cmp	r1, #255
	ble	.mpp_achn_clippan2
	movs	r1, #255
.mpp_achn_clippan2:

#ifdef SYS_NDS
	lsrs	r1, #1
	ldrb	r0, [r4, #MIXER_CHN_CNT]
	lsrs	r0, #7
	lsls	r0, #7
	orrs	r0, r1
	strb	r0, [r4, #MIXER_CHN_CNT]
#endif

#ifdef SYS_GBA
	strb	r1, [r4, #MIXER_CHN_PAN]
#endif

.mpp_achn_updated:
	pop	{r4}
	pop	{r0}
	bx	r0
.pool

.end
