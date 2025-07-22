// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

#include "mp_format_mas.inc"
#include "mp_mas_structs.inc"
#include "mp_defs.inc"

#ifdef SYS_GBA
#include "mp_mixer_gba.inc"
#include "swi_gba.inc"
#endif

#ifdef SYS_NDS
#include "mp_mixer_ds.inc"
#include "swi_nds.inc"
#endif

//-----------------------------------------------------------------------------

    .syntax unified

.equ	S3M_FREQ_DIVIDER	,57268224	// (s3m,xm,it)
.equ	MOD_FREQ_DIVIDER_PAL	,56750314	// (mod)
.equ	MOD_FREQ_DIVIDER_NTSC	,57272724	// (---)


/******************************************************************************
 *
 * Macros
 *
 ******************************************************************************/

/******************************************************************************
 * mpp_InstrumentPointer
 *
 * Calculate instrument address.
 * Requires r8 = layer
 * Returns in r0
 * Trashes r1, r2
 ******************************************************************************/
.macro mpp_InstrumentPointer
	mov	r1, r8
	ldr	r2,[r1,#MPL_SONGADR]
	ldr	r1,[r1,#MPL_INSTTABLE]
	lsls	r0, #2
	ldr	r0,[r1,r0]
	adds	r0, r2
.endm

/******************************************************************************
 * mpp_SamplePointer
 *
 * Calculate sample address.
 * Requires r8 = layer
 * Returns in r0
 * Trashes r1, r2
 ******************************************************************************/
.macro mpp_SamplePointer
	mov	r1, r8
	ldr	r2,[r1,#MPL_SONGADR]
	ldr	r1,[r1,#MPL_SAMPTABLE]
	lsls	r0, #2
	ldr	r0,[r1,r0]
	adds	r0, r2
.endm

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
	movs	r4, r0 // mxixer channel

.mppt_achn_nostart:

	@ set pitch

	ldrb	r0, [r6, #MCA_SAMPLE]		@ get sample#
	subs	r0, #1				@ ..
	bcc	.mppt_achn_setvolumeA
	mpp_SamplePointer		@ quit if invalid
	push	{r0}

	mov	r1, r8
	ldrb	r1, [r1, #MPL_FLAGS]
	lsrs	r1, #C_FLAGS_SS
	bcc	.mppt_achn_amigafreqs
.mppt_achn_linearfreqs:

	ldrh	r0, [r0, #C_MASS_FREQ]		@ get C5SPEED
	lsls	R0, #2
	lsrs	r1, r5, #8			@ do some stuff...
	muls	r1, r0				@ ... period * freq?

	lsrs	r1, #8

	ldr	r0,=mpp_clayer
	ldrb	r0, [r0]
	cmp	r0, #0
	bne	1f
	ldr	r0,=mm_masterpitch
	ldr	r0, [r0]
	muls	r1, r0
	lsrs	r1, #10
1:

#ifdef SYS_GBA
	ldr	r0,=(4096*65536)/15768
	muls	r0, r1
	lsrs	r0, #16
	str	r0, [r4, #MIXER_CHN_FREQ]
#else
	ldr	r0,=MIXER_SCALE
	muls	r0, r1
	lsrs	r0, #16+1
	strh	r0, [r4, #MIXER_CHN_FREQ]
#endif

	b	.mppt_achn_setvolume

.mppt_achn_amigafreqs:
	ldr	r0,=MOD_FREQ_DIVIDER_PAL
	movs	r1, r5

	beq	.mppt_achn_setvolume		@ 0 is a bad period
	swi	SWI_DIVIDE

	ldr	r1,=mpp_clayer
	ldrb	r1, [r1]
	cmp	r1, #0
	bne	1f
	ldr	r1,=mm_masterpitch
	ldr	r1, [r1]
	muls	r0, r1
	lsrs	r0, #10
1:

#ifdef SYS_GBA
	ldr	r1,=(4096*65536)/15768
	muls	r0, r1
	lsrs	r0, #16
	str	r0, [r4, #MIXER_CHN_FREQ]
#else
	ldr	r1,=MIXER_SCALE		@ calculate ds mixer timing
	muls	r0, r1
	lsrs	r0, #16+1
	strh	r0, [r4, #MIXER_CHN_FREQ]
#endif

@----------------------------------------------------------------------------------------------------
.mppt_achn_setvolume:
@----------------------------------------------------------------------------------------------------

	@ set volume

	pop	{r0}
						@ <-- stepped oct 28, 3:27pm
	ldrb	r3, [r0, #C_MASS_GV]		@ SV, 6-bit
	ldrb	r0, [r6, #MCA_INST]
	subs	r0, #1
	bcs	1f

.mppt_achn_setvolumeA:
	movs	r1, #0
	b	.mppt_achn_badinstrument
1:
	mpp_InstrumentPointer
	ldrb	r0, [r0, #C_MASI_GVOL]	@ IV, 7-bit
	muls	r3, r0

	ldr	r1,=mpp_vars
	ldrb	r0, [r1, #MPV_AFVOL]	@ ((CV*VOL)/32*VEV/64) 7-bit

	muls	r3, r0

	mov	r1, r8			@ get global vollume
	ldrb	r0, [r1, #MPL_FLAGS]
	lsrs	r0, #4
	ldrb	r0, [r1, #MPL_GV]	@ ..		     7-bit

	bcc	1f
	lsls	r0, #1			@ xm mode global volume is only 0->64, shift to 0->128

1:	muls	r3, r0			@ multiply

	lsrs	r3, #10

	ldrh	r0, [r6, #MCA_FADE]


	muls	r3, r0

	lsrs	r3, r3, #10

	mov	r0, r8
	movs	r1, #MPL_VOLUME
	ldrh	r0, [r0, r1]
	muls	r3, r0

//------------------------------------------------
#ifdef SYS_NDS
	lsrs	r1, r3, #19-3-5 ///#19-3	(new 16-bit levels!)
	ldr	r3,=65535  //2047
	cmp	r1, r3				@ clip values over 255
	blt	1f
	movs	r1, r3
1:
.mppt_achn_badinstrument:
//	lsrs	r3, r1, #3		(new 16-bit levels!)
	lsrs	r3, r1, #8
	strb	r3, [r6, #MCA_FVOL]
#else
	lsrs	r1, r3, #19
	cmp	r1, #255			@ clip values over 255
	blt	1f
	movs	r1, #255
1:
.mppt_achn_badinstrument:
	strb	r1, [r6, #MCA_FVOL]
#endif

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
