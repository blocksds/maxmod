// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

#include "mp_format_mas.inc"
#include "mp_mas_structs.inc"
#include "mp_defs.inc"
#include "mp_macros.inc"

//-----------------------------------------------------------------------------
#ifdef SYS_GBA
//-----------------------------------------------------------------------------

#include "mp_mixer_gba.inc"
#include "swi_gba.inc"

//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#ifdef SYS_NDS
//-----------------------------------------------------------------------------
#include "mp_mixer_ds.inc"
#include "swi_nds.inc"
//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------


.equ	S3M_FREQ_DIVIDER	,57268224	// (s3m,xm,it)
.equ	MOD_FREQ_DIVIDER_PAL	,56750314	// (mod)
.equ	MOD_FREQ_DIVIDER_NTSC	,57272724	// (---)



/******************************************************************************
 *
 * Memory
 *
 ******************************************************************************/



	.bss
	.syntax unified
	.align 2

/******************************************************************************
 * mmLayerMain
 *
 * Layer data for module playback.
 ******************************************************************************/
							.global mmLayerMain
mmLayerMain:		.space MPL_SIZE

/******************************************************************************
 * mmLayerSub
 *
 * Layer data for jingle playback.
 ******************************************************************************/
							.global mmLayerSub
mmLayerSub:		.space MPL_SIZE

/******************************************************************************
 * mpp_vars
 *
 * Holds intermediate data during the module processing.
 ******************************************************************************/
							.global mpp_vars
mpp_vars:		.space MPV_SIZE

	.align 2

/******************************************************************************
 * mpp_layerp
 *
 * Pointer to layer data during processing.
 ******************************************************************************/
							.global mpp_layerp
mpp_layerp:		.space 4

/******************************************************************************
 * mpp_channels
 *
 * Pointer to channel array during processing
 ******************************************************************************/
							.global mpp_channels
mpp_channels:		.space 4

/******************************************************************************
 * mm_mastertempo
 *
 * Master tempo scaler.
 ******************************************************************************/
							.global mm_mastertempo
mm_mastertempo:		.space 4

/******************************************************************************
 * mm_masterpitch
 * 
 * Master pitch scaler.
 ******************************************************************************/
							.global mm_masterpitch
mm_masterpitch:		.space 4

/******************************************************************************
 * mpp_nchannels
 *
 * Number of channels in layer
 ******************************************************************************/
							.global mpp_nchannels
mpp_nchannels:		.space 1

/******************************************************************************
 * mpp_clayer
 *
 * Layer selection, 0 = main, 1 = sub
 ******************************************************************************/
							.global mpp_clayer
mpp_clayer:		.space 1

/******************************************************************************
 * mm_achannels, mm_pchannels, mm_num_mch, mm_num_ach, mm_schannels
 *
 * Channel data/sizes, don't move these around--see mmInit first
 ******************************************************************************/
							.align 2
							
.global mm_achannels, mm_pchannels, mm_num_mch, mm_num_ach, mm_schannels
						
mm_achannels:		.space 4
mm_pchannels:		.space 4
mm_num_mch:		.space 4
mm_num_ach:		.space 4
mm_schannels:		.space MP_SCHANNELS*MCH_SIZE



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
1:	mpp_InstrumentPointer

@ get envelope flags
	
	movs	r1, r0
	ldrb	r2, [r1, #C_MASI_ENVFLAGS]
	adds	r1, #C_MASI_ENVELOPES
	
	lsrs	r2, #1				@ shift out volume envelope bit
	bcc	.mppt_no_volenv

	ldrb	r3, [r6, #MCA_FLAGS]
	lsrs	r3, #6
	
	bcs	.mppt_achn_ve_enabled
	ldrb	r0, [r1]
	adds	r1, r0
	b	.mppt_no_volenv
.mppt_achn_ve_enabled:
	
	push	{r1, r2}
	
	ldrh	r0, [r6, #MCA_ENVC_VOL]
	movs	r2, r1
	ldrb	r1, [r6, #MCA_ENVN_VOL]
	bl	mpph_ProcessEnvelope
	strb	r1, [r6, #MCA_ENVN_VOL]
	strh	r0, [r6, #MCA_ENVC_VOL]
	movs	r1, r3
	
	cmp	r2, #1
	bne	.mpph_volenv_notend
	ldrb	r0, [r6, #MCA_FLAGS]
	
	mov	r3, r8				@ stupid xm doesn't fade out at envelope end
	ldrb	r3, [r3, #MPL_FLAGS]
	lsrs	r3, #C_FLAGS_XS
	
	movs	r3, #MCAF_ENVEND
	bcs	1f
	movs	r3, #MCAF_ENVEND+MCAF_FADE
1:
	orrs	r0, r3
	strb	r0, [r6, #MCA_FLAGS]
.mpph_volenv_notend:

	cmp	r2, #1
	blt	.mpph_volenv_normal
	
	@ check keyon and turn on fade...
	ldrb	r0, [r6, #MCA_FLAGS]
	movs	r2, #MCAF_KEYON
	tst	r0, r2
	bne	.mpph_volenv_normal
.mpph_volenv_notefade:
	movs	r2, #MCAF_FADE
	orrs	r0, r2
	strb	r0, [r6, #MCA_FLAGS]
	
.mpph_volenv_normal:
	
	ldr	r0,=mpp_vars
	ldrb	r2, [r0, #MPV_AFVOL]
	muls	r2, r1
	lsrs	r2, #6+6
	strb	r2, [r0, #MPV_AFVOL]
	pop	{r1, r2}
	ldrb	r0, [r1]
	adds	r1, r0
	b	.mppt_has_volenv
.mppt_no_volenv:
	
	ldrb	r0, [r6, #MCA_FLAGS]
	movs	r3, #MCAF_ENVEND
	orrs	r0, r3
	movs	r3, #MCAF_KEYON
	tst	r0, r3
	bne	.mppt_has_volenv
	movs	r3, #MCAF_FADE
	orrs	r0, r3
	strb	r0, [r6, #MCA_FLAGS]
	
	mov	r0, r8		@ check XM MODE and cut note
	ldrb	r0, [r0, #MPL_FLAGS]
	lsrs	r0, #C_FLAGS_XS
	bcc	.mppt_has_volenv
	movs	r0, #0
	strh	r0, [r6, #MCA_FADE]
.mppt_has_volenv:
	
	lsrs	r2, #1
	bcc	.mppt_no_panenv
	push	{r1, r2}
	ldrh	r0, [r6, #MCA_ENVC_PAN]
	movs	r2, r1
	ldrb	r1, [r6, #MCA_ENVN_PAN]
	bl	mpph_ProcessEnvelope
	strb	r1, [r6, #MCA_ENVN_PAN]
	strh	r0, [r6, #MCA_ENVC_PAN]
	movs	r1, r3

	ldr	r0,=mpp_vars
	movs	r3, #MPV_PANPLUS
	ldrsh	r2, [r0,r3]
	lsrs	r1, #4
	subs	r1, #128
	adds	r2, r1
	strh	r2, [r0,r3]
	pop	{r1, r2}
.mppt_no_panenv:
	
	lsrs	r2, #1
	bcc	.mppt_no_pitchenv
	ldrb	r0, [r1, #C_MASIE_FILTER]
	cmp	r0, #0
	bne	.mppt_no_pitchenv
	push	{r1, r2}
	ldrh	r0, [r6, #MCA_ENVC_PIC]
	movs	r2, r1
	ldrb	r1, [r6, #MCA_ENVN_PIC]
	bl	mpph_ProcessEnvelope
	strb	r1, [r6, #MCA_ENVN_PIC]
	strh	r0, [r6, #MCA_ENVC_PIC]
	movs	r1, r3
	lsrs	r1, #3
	subs	r1, #255
	movs	r0, r5
	subs	r1, #1
	bmi	.mppt_pitchenv_minus
	mov	r2, r8
#ifdef USE_IWRAM
	ldr	r3,=mpph_LinearPitchSlide_Up
	jump3
#else
	bl	mpph_LinearPitchSlide_Up
#endif
	b	.mppt_pitchenv_plus
.mppt_pitchenv_minus:
	negs	r1, r1
	mov	r2, r8
#ifdef USE_IWRAM
	ldr	r3,=mpph_LinearPitchSlide_Down
	jump3
#else
	bl	mpph_LinearPitchSlide_Down
#endif
.mppt_pitchenv_plus:
	movs	r5, r0
	pop	{r1, r2}
.mppt_no_pitchenv:
	
	ldrb	r0, [r6, #MCA_FLAGS]
	movs	r1, #MCAF_FADE
	tst	r0, r1
	beq	.mppt_achn_nofade
	ldrb	r0, [r6, #MCA_INST]
	subs	r0, #1
	
	mpp_InstrumentPointer
	ldrb	r0, [r0, #C_MASI_FADE]

	ldrh	r1, [r6, #MCA_FADE]

	subs	r1, r0
	bcs	.mppt_achn_fadeout_clip
	movs	r1, #0
.mppt_achn_fadeout_clip:
	strh	r1, [r6, #MCA_FADE]

.mppt_achn_nofade:
	
.mppt_achn_keyon:
	

@----------------------------------------------------------------------------------
@ *** PROCESS AUTO VIBRATO
@----------------------------------------------------------------------------------
	
	ldrb	r0, [r6, #MCA_SAMPLE]
	subs	r0, #1
	bcc	.mppt_achn_nostart	@ no sample!!
	
	@bl	mpp_SamplePointer
	mpp_SamplePointer
	ldrh	r1, [r0, #C_MASS_VIR]	@ get av-rate
	cmp	r1, #0			@ 0?
	beq	.mppt_av_disabled	@  if 0 then its disabled
	ldrh	r2, [r6, #MCA_AVIB_DEP]	@ get depth counter
	adds	r2, r1			@ add rate
	lsrs	r1, r2, #15		@ check for 15-bit overflow
	beq	.mppt_av_depclip	@ ..
	
	ldr	r2,=32768		@ and clamp to 32768
.mppt_av_depclip:			
	strh	r2, [r6, #MCA_AVIB_DEP]	@ save depth counter
	ldrb	r1, [r0, #C_MASS_VID]	@ get av-depth
	muls	r1, r2			@ multiply

	ldrb	r3, [r6, #MCA_AVIB_POS]	@ get table position
	ldrb	r2, [r0, #C_MASS_VIS]	@ get av-speed
	adds	r3, r2			@ add to position
	lsls	r3, #32-8		@ wrap position to 0->255
	lsrs	r3, #32-8		@ ..
	strb	r3, [r6, #MCA_AVIB_POS]		@ save position
	ldr	r2,=mpp_TABLE_FineSineData	@ get table pointer
	ldrsb	r2, [r2, r3]			@ load table value at position
	muls	r2, r1				@ multiply with depth
	asrs	r2, #23				@ shift value
	bmi	.mppt_av_minus			@ and perform slide...
.mppt_av_plus:					@ --slide up
	movs	r1, r2				@ r1 = slide value
	movs	r0, r5				@ r0 = frequency
	mov	r2, r8
#ifdef USE_IWRAM
	fjump3	mpph_PitchSlide_Up
#else
	bl	mpph_PitchSlide_Up		@ pitch slide
#endif
	b	.mppt_av_finished		@
.mppt_av_minus:					@ --slide down
	negs	r1, r2				@ r1 = slide value
	movs	r0, r5				@ r0 = frequency
	mov	r2, r8
#ifdef USE_IWRAM
	ldr	r3,=mpph_PitchSlide_Down
	jump3
#else
	bl	mpph_PitchSlide_Down		@ pitch slide
#endif

.mppt_av_finished:
	movs	r5, r0				@ affect frequency
.mppt_av_disabled:
	
@---------------------------------------------------------------------------------
	
.mppt_achn_noinst:
	
	push	{r4}
	movs	r0, #MIXER_CHN_SIZE
	muls	r4, r0
	@ldr	r0,=mp_channels
@	ldr	r0,=mm_mixchannels
@	ldr	r0,[r0]
	GET_MIXCH r0
	adds	r4, r0
	
	@ *** UPDATE MIXING INFORMATION
	
	ldrb	r0, [r6, #MCA_FLAGS]		@ read flags
	movs	r1, #MCAF_START			@ test start bit
	tst	r0, r1				@ ..
	beq	.mppt_achn_nostart		@
.mppt_achn_start:				@ START NOTE
	bics	r0, r1				@ clear bit
	strb	r0, [r6, #MCA_FLAGS]		@ save flags
	ldrb	r0, [r6, #MCA_SAMPLE]		@ get sample #

	subs	r0, #1				@ ..
	bcc	.mppt_achn_nostart		@ quit if invalid
	@bl	mpp_SamplePointer		@ get sample address
	mpp_SamplePointer
	
	ldrh	r3, [r0, #C_MASS_MSLID]
	
	adds	r1,r3,#1			@ msl id == 0xFFFF?
	lsrs	r1, #17
	
	bcc	.mppt_achn_msl_sample 

.mppt_achn_direct_sample:			@ then sample follows
	
	adds	r0, #12
	
//------------------------------------------------
#ifdef SYS_GBA
//	ldr	r1, [r0,#C_SAMPLE_LEN]	@ setup mixer (GBA)
//	lsls	r1, #MP_SAMPFRAC
//	str	r1, [r4,#MIXER_CHN_LEN]
//	ldr	r1, [r0,#C_SAMPLE_LOOP]
//	str	r1, [r4,#MIXER_CHN_LOOP]
	adds	r0, #C_SAMPLE_DATA
	str	r0, [r4,#MIXER_CHN_SRC]
	
#else
//-------------------------------------------

	ldr	r1,=0x2000000
	subs	r0, r1
	str	r0, [r4, #MIXER_CHN_SAMP]
	ldrb	r1, [r4, #MIXER_CHN_CNT]
	movs	r0, #MIXER_CF_START
	orrs	r1, r0
	strb	r1, [r4, #MIXER_CHN_CNT]
	
#endif
//-------------------
	
	b	.mppt_achn_gotsample
.mppt_achn_msl_sample:				@ otherwise get from solution

	#ifdef SYS_GBA

	ldr	r2,=mp_solution
	ldr	r2, [r2]
	movs	r1, r2
	adds	r1, #12
	lsls	r3, #2
	ldr	r1, [r1, r3]
	adds	r1, #8
	adds	r0, r1, r2
	
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ NOTICE, USE LDM HERE
	
//	ldr	r1, [r0,#C_SAMPLE_LEN]	@ setup mixer (GBA)
//	lsls	r1, #MP_SAMPFRAC
//	str	r1, [r4,#MIXER_CHN_LEN]
//	ldr	r1, [r0,#C_SAMPLE_LOOP]
//	str	r1, [r4,#MIXER_CHN_LOOP]
	adds	r0, #C_SAMPLE_DATA
	str	r0, [r4,#MIXER_CHN_SRC]

	#endif

	#ifdef SYS_NDS
			
	ldr	r2,=mmSampleBank	@ get samplebank pointer
	ldr	r2, [r2]
	lsls	r3, #2		@ add msl_id *4
	ldr	r1, [r2, r3]
	
	lsls	r1, #8		@ mask out counter value
	lsrs	r1, #8
	adds	r1, #8

	str	r1, [r4,#MIXER_CHN_SAMP]
	ldrb	r1, [r4,#MIXER_CHN_CNT]		// read control		**CNT was cleared, no need to read it
	movs	r0, #MIXER_CF_START		// set start bit
	orrs	r1, r0
	strb	r1, [r4,#MIXER_CHN_CNT]		// save control

	#endif
	
.mppt_achn_gotsample:
	ldr	r1,=mpp_vars
	ldrb	r1, [r1, #MPV_SAMPOFF]
#ifdef SYS_GBA
	lsls	r1, #MP_SAMPFRAC+8
	str	r1, [r4, #MIXER_CHN_READ]
#else
	str	r1, [r4, #MIXER_CHN_READ]
#endif
	
.mppt_achn_nostart:
	@ set pitch
	
	ldrb	r0, [r6, #MCA_SAMPLE]		@ get sample#
	subs	r0, #1				@ ..
	bcc	.mppt_achn_setvolumeA
	@bl	mpp_SamplePointer		@ quit if invalid
	mpp_SamplePointer
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
	
	
//	ldr	r0,=mm_freqscalar
//	ldr	r0, [r0]
	ldr	r0,=(4096*65536)/15768
	muls	r0, r1
	lsrs	r0, #16
	str	r0, [r4, #MIXER_CHN_FREQ]
	
	#else

	ldr	r0,=MIXER_SCALE
	muls	r0, r1
	lsrs	r0, #16+1
	strh	r0, [r4, #MIXER_CHN_FREQ]
	
	//strh	r1, [r4, #MIXER_CHN_FREQ]
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

//	ldr	r1,=mm_freqscalar
//	ldr	r1,[r1]
	ldr	r1,=(4096*65536)/15768
	muls	r0, r1
	lsrs	r0, #16

	str	r0, [r4, #MIXER_CHN_FREQ]
	#else
//	movs	r1, r0
//	ldr	r0,=16756991			@ calculate ds mixer timing
//	swi	SWI_DIVIDE
//	negs	r0,r0
	//lsrs	r0, #5
	ldr	r1,=MIXER_SCALE
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
	
.thumb_func
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
	
//	#ifdef SYS_NDS
//	ldrh	r3, [r4, #MIXER_CHN_CVOL]	// nds has volume ramping!!
//	cmp	r3, #0
//	bne	.mppt_achn_audible
//	#endif

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
	//pop	{pc}				@ exit
.pool

.align 2
.thumb_func
@-------------------------------------------------------------------------
mpph_ProcessEnvelope:			@ params={count,node,address}
@-------------------------------------------------------------------------

@ processes the envelope at <address>
@ returns:
@ r0=count
@ r1=node
@ r2=exit_code
@ r3=value*64
	
	push	{r4,r5}

@ get node and base
	lsls	r4, r1, #2
	adds	r4, #C_MASIE_NODES
	adds	r4, r2
	
	ldrh	r3, [r4, #2]
	lsls	r3, #32-7
	lsrs	r3, #32-7-6

@ check for zero count
	
	cmp	r0, #0
	bne	.mpph_pe_between
.mpph_pe_new:

@ process envelope loop
	
	ldrb	r5, [r2, #C_MASIE_LEND]
	cmp	r1, r5
	bne	1f
	ldrb	r1, [r2, #C_MASIE_LSTART]
	movs	r2, #2
	b	.mpph_pe_exit

@ process envelope sustain loop

1:	ldrb	r5, [r6, #MCA_FLAGS]
	lsrs	r5, #1	@ locked
	bcc	1f
	ldrb	r5, [r2, #C_MASIE_SEND]
	cmp	r1, r5
	bne	1f
	ldrb	r1, [r2, #C_MASIE_SSTART]
	movs	r2, #0
	b	.mpph_pe_exit

@ check for end

1:	ldrb	r5, [r2, #C_MASIE_NODEC]
	subs	r5, #1
	cmp	r1, r5
	bne	.mpph_count
	movs	r2, #2
	b	.mpph_pe_exit

.mpph_pe_between:

@                            delta * count
@ formula : y = base*2^6 + -----------------
@                                 2^3
	movs	r5, #0
	ldrsh	r5, [r4,r5]
	muls	r5, r0
	asrs	r5, #3
	adds	r3, r5
	
.mpph_count:

@ increment count and check if == read count

	adds	r0, #1
	ldrh	r5, [r4, #2]
	lsrs	r5, #7
	cmp	r0, r5
	bne	.mpph_pe_exit

@ increment node and reset counter

	movs	r0, #0
	adds	r1, #1

.mpph_pe_exit:
	
	pop	{r4,r5}
	bx	lr
.pool

@=============================================================================
@                                EFFECT MEMORY
@=============================================================================

.text

.equ	MPP_GLIS_MEM,		0
.equ	MPP_IT_PORTAMEM,	2

/******************************************************************************
 *
 * XM Volume Commands
 *
 ******************************************************************************/

.align 2
					.global mppe_glis_backdoor_Wrapper
					.thumb_func
mppe_glis_backdoor_Wrapper:

	push	{r5-r7, lr}
	mov	r7, r8
	push	{r7}

	// r1 = volcmd
	// r5 = period
	// r6 = act_ch
	// r7 = channel
	// r8 = layer

	movs	r1, r0
	movs	r7, r1
	movs	r6, r2
	mov	r8, r3
	// This argument is passed on the stack by the caller, but 6 registers have
	// been pushed to the stack as well.
	ldr	r5, [sp, #6 * 4]

	push {r1, lr}

	bl	.mppe_glis_backdoor

	movs	r0, r5

	pop	{r7}
	mov	r8, r7
	pop	{r4-r7}
	pop	{r1}
	bx	r1

/******************************************************************************
 *
 * Module Effects
 *
 ******************************************************************************/
.align 2
					.global mpp_Process_Effect_Wrapper
					.thumb_func
mpp_Process_Effect_Wrapper:
	push	{r5-r7,lr}
	mov	r7, r8
	push	{r7}

	mov	r8, r0
	movs	r6, r1
	movs	r7, r2
	movs	r5, r3
	bl	mpp_Process_Effect
	movs	r0, r5

	pop	{r7}
	mov	r8, r7
	pop	{r5-r7}
	pop	{r1}
	bx	r1

/******************************************************************************
 * mpp_ProcessEffect()
 *
 * Process pattern effect.
 ******************************************************************************/
						.global mpp_Process_Effect
						.thumb_func
mpp_Process_Effect:

	push	{lr}

	ldrb	r0, [r7, #MCH_EFFECT]	@ get effect#
	ldrb	r1, [r7, #MCH_PARAM]	@ r1 = param
	movs	r2, r7
	mov	r3, r8
	bl	mpp_Channel_ExchangeMemory
	movs	r1, r0

	pop	{r2}
	mov	lr, r2

	ldrb	r0, [r7, #MCH_EFFECT]	@ get effect#
	lsls	r0, #1
	add	r0, pc
	mov	pc, r0

	// r1 = param
	// r5 = period
	// r6 = mm_active_channel
	// r7 = mm_module_channel
	// r8 = mpl_layer_information

	b	mppe_todo
	b	mppe_SetSpeed
	b	mppe_PositionJump
	b	mppe_PatternBreak
	b	mppe_VolumeSlide
	b	mppe_Portamento
	b	mppe_Portamento
	b	mppe_Glissando
	b	mppe_Vibrato
	b	mppe_todo
	b	mppe_Arpeggio
	b	mppe_VibratoVolume
	b	mppe_PortaVolume
	b	mppe_ChannelVolume
	b	mppe_ChannelVolumeSlide
	b	mppe_SampleOffset
	b	mppe_todo
	b	mppe_Retrigger
	b	mppe_Tremolo		@ tremolo
	b	mppe_Extended
	b	mppe_SetTempo
	b	mppe_FineVibrato
	b	mppe_SetGlobalVolume
	b	mppe_GlobalVolumeSlide
	b	mppe_SetPanning
	b	mppe_Panbrello
	b	mppe_ZXX
	b	mppe_SetVolume
	b	mppe_KeyOff
	b	mppe_EnvelopePos
	b	mppe_OldTremor
.pool

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_SetSpeed:				@ EFFECT Axy: SET SPEED
@---------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_ss_exit		@ dont set on nonzero ticks
	cmp	r1, #0
	beq	.mppe_ss_exit
	mov	r0, r8
	
	strb	r1, [r0, #MPL_SPEED]
.mppe_ss_exit:
.thumb_func
mppe_todo:
	bx	lr			@ exit

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_PositionJump:			@ EFFECT Bxy: SET POSITION
@---------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_pj_exit		@ skip nonzero ticks
	mov	r0, r8
	strb	r1, [r0, #MPL_PATTJUMP]
.mppe_pj_exit:
	bx	lr			@ exit
.pool

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_PatternBreak:				@ EFFECT Cxy: PATTERN BREAK
@---------------------------------------------------------------------------------
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_pb_exit			@ skip nonzero ticks
	mov	r0, r8				@ get variables
	strb	r1, [r0, #MPL_PATTJUMP_ROW]	@ save param to row value
	ldrb	r1, [r0, #MPL_PATTJUMP]		@ check if pattjump=empty
	cmp	r1, #255			@ 255=empty
	bne	.mppe_pb_exit			@ ...
	ldrb	r1, [r0, #MPL_POSITION]		@ if empty, set pattjump=position+1
	adds	r1, #1
	strb	r1, [r0, #MPL_PATTJUMP]
.mppe_pb_exit:
	bx	lr				@ finished
.pool

.align 2
.thumb_func
@------------------------------------------------------------------------------------------
mppe_VolumeSlide:				@ EFFECT Dxy: VOLUME SLIDE
@------------------------------------------------------------------------------------------

	push	{lr}
	ldrb	r0, [r7, #MCH_VOLUME]		@ load volume

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	mov	r3, r8
	bl	mpph_VolumeSlide64
	
	strb	r0, [r7, #MCH_VOLUME]		@ save volume
.mppe_vs_zt:
	pop	{r0}
	bx	r0
//	pop	{pc}				@ exit

.align 2
.thumb_func
@----------------------------------------------------------------------------------
mppe_Portamento:				@ EFFECT Exy/Fxy: Portamento
@----------------------------------------------------------------------------------

	push	{lr}

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#

.mppe_pd_ot:
	movs	r3, #0
	movs	r0, r1
	lsrs	r0, #4				@ test for Ex param (Extra fine slide)
	cmp	r0, #0xE			@ ..
.mppe_pd_checkE:				@ ..
	bne	.mppe_pd_checkF			@ ..
	cmp	r2, #0				@ Extra fine slide: only slide on tick0
	bne	.mppe_pd_exit			@ ..
	lsls	r1, #32-4			@ mask out slide value
	lsrs	r1, #32-4			@ ..
	movs	r3, #1
	b	.mppe_pd_otherslide		@ skip the *4 multiplication
.mppe_pd_checkF:				@ ------------------------------------
	cmp	r0, #0xF			@ test for Fx param (Fine slide)
	bne	.mppe_pd_regslide		@ ..
	cmp	r2, #0				@ Fine slide: only slide on tick0
	bne	.mppe_pd_exit			@ ..
	lsls	r1, #32-4			@ mask out slide value
	lsrs	r1, #32-4			@ ..
	b	.mppe_pd_otherslide
.mppe_pd_regslide:
	cmp	r2, #0
	beq	.mppe_pd_exit
.mppe_pd_otherslide:
	
	ldrb	r0, [r7, #MCH_EFFECT]		@ check slide direction
	movs	r2, #MCH_PERIOD
	cmp	r0, #5				@ .. (5 = portamento down)
	ldr	r0, [r7, r2]			@ get period
	
	bne	.mppe_pd_slideup		@ branch to function
.mppe_pd_slidedown:				@ -------SLIDE DOWN-------
	
	cmp	r3, #0
	bne	.mppe_pd_fineslidedown
	mov	r2, r8
	bl	mpph_PitchSlide_Down
	
	b	.mppe_pd_store			@ store & exit

.mppe_pd_fineslidedown:
	mov	r2, r8
	bl	mpph_FinePitchSlide_Down
	b	.mppe_pd_store
	
.mppe_pd_slideup:				@ ---------SLIDE UP---------
	
	cmp	r3, #0
	bne	.mppe_pd_fineslideup
	mov	r2, r8
	bl	mpph_PitchSlide_Up
	b	.mppe_pd_store
.mppe_pd_fineslideup:
	mov	r2, r8
	bl	mpph_FinePitchSlide_Up

.mppe_pd_store:
	movs	r2, #MCH_PERIOD
	ldr	r1, [r7, #MCH_PERIOD]
	str	r0, [r7, #MCH_PERIOD]
	subs	r0, r1
	adds	r5, r0
.mppe_pd_exit:
	pop	{r0}
	bx	r0				@ exit

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Glissando:					@ EFFECT Gxy: Glissando
@---------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_glis_ot
	
	mov	r0, r8
	ldrb	r0, [r0, #MPL_FLAGS]
	lsrs	r0, #C_FLAGS_GS
	bcc	2f
	@ gxx is shared, IT MODE ONLY!!
	cmp	r1, #0
	bne	3f
	ldrb	r1, [r7, #MCH_MEMORY+MPP_IT_PORTAMEM]
	strb	r1, [r7, #MCH_PARAM]
3:	strb	r1, [r7, #MCH_MEMORY+MPP_IT_PORTAMEM]
	strb	r1, [r7, #MCH_MEMORY+MPP_GLIS_MEM]	@ for simplification later
	b	.mppe_glis_ot
	
2:	@ gxx is separate
	cmp	r1, #0
	bne	3f
	
	ldrb	r1, [r7, #MCH_MEMORY+MPP_GLIS_MEM]
	strb	r1, [r7, #MCH_PARAM]
3:	strb	r1, [r7, #MCH_MEMORY+MPP_GLIS_MEM]

	bx	lr
//	b	.mppe_glis_exit
.mppe_glis_ot:
	
	push	{lr}					// save return address

	ldrb	r1, [r7, #MCH_MEMORY+MPP_GLIS_MEM]

	push	{r1}

.mppe_glis_backdoor:
	
	cmp	r6, #0					// exit if no active channel
	bne	1f					//
	pop	{r1,r3}					//
	bx	r3					//
1:							//
	
	ldrb	r0, [r6, #MCA_SAMPLE]			// get target period
	subs	r0, #1					//
	mpp_SamplePointer				//
	ldrh	r1, [r0, #C_MASS_FREQ]			//
	lsls	R1, #2					//
	ldrb	r2, [r7, #MCH_NOTE]			//
	mov	r0,r8
	ldr	r3,=mmGetPeriod				//
	bl	mpp_call_r3				//
	
	pop	{r1}					// r1 = parameter
	push	{r0}					// 
	
	movs	r3, r0					// r3 = target period
	movs	r2, #MCH_PERIOD				// r0 = current period
	ldr	r0, [r7, r2]				//
	mov	r2, r8					// test S flag
	ldrb	r2, [r2, #MPL_FLAGS]			//
	lsrs	r2, #C_FLAGS_SS				//
	bCC	.mppe_glis_amiga
	cmp	r0, r3
	blt	.mppe_glis_slideup
	bgt	.mppe_glis_slidedown
.mppe_glis_noslide:
	pop	{r0}
	pop	{r3}
	bx	r3

.mppe_glis_slideup:
	mov	r2, r8
	bl	mpph_PitchSlide_Up
	pop	{r1}
	
	cmp	r0, r1
	blt	.mppe_glis_store
	movs	r0, r1
	b	.mppe_glis_store
.mppe_glis_slidedown:
	mov	r2, r8
	bl	mpph_PitchSlide_Down
	pop	{r1}
	cmp	r0, r1
	bgt	.mppe_glis_store
	movs	r0, r1
.mppe_glis_store:
	
	movs	r2, #MCH_PERIOD
	ldr	r1, [r7, r2] @#MCA_PERIOD]
	str	r0, [r7, r2] @#MCA_PERIOD]
	subs	r0, r1
	adds	r5, r0
	
.mppe_glis_exit:
	pop	{r3}
	bx	r3


	//bx	lr

.mppe_glis_amiga:

	cmp	r0, r3
	blt	.mppe_glis_amiga_up
	bgt	.mppe_glis_amiga_down
	pop	{r0}
	pop	{r3}
	bx	r3

.mppe_glis_amiga_down:
	mov	r2, r8
	bl	mpph_PitchSlide_Up
	pop	{r1}
	cmp	r0, r1
	bgt	.mppe_glis_store
	movs	r0, r1
	b	.mppe_glis_store
.mppe_glis_amiga_up:
	mov	r2, r8
	bl	mpph_PitchSlide_Down
	pop	{r1}
	cmp	r0, r1
	blt	.mppe_glis_store
	movs	r0, r1
	b	.mppe_glis_store

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Vibrato:					@ EFFECT Hxy: Vibrato
@---------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	beq 1f
	movs	r0, r5
	movs	r1, r7
	mov	r2, r8
	mov	r4, lr
	bl	mppe_DoVibrato
	movs	r5, r0
	bx	r4
1:

	lsrs	r0, r1, #4			@ if (x != 0) {
	beq	.mppe_v_nospd			@   speed = 4*x;
	lsls	r0, #2				@   ..
	strb	r0, [r7, #MCH_VIBSPD]		@   ..
.mppe_v_nospd:
	
	lsls	r0, r1, #32-4		 	@ if (y != 0) {
	beq	.mppe_v_nodep			@ ..
	lsrs	r0, #32-6			@   depth = y * 4;
	mov	r1, r8
	ldrb	r1, [r1, #MPL_OLDEFFECTS]	@   if(OldEffects)
	lsls	r0, r1				@      depth <<= 1;
	strb	r0, [r7, #MCH_VIBDEP]		@

	movs	r0, r5
	movs	r1, r7
	mov	r2, r8
	mov	r4, lr
	bl	mppe_DoVibrato
	movs	r5, r0
	bx	r4

.mppe_v_nodep:
	BX	LR

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Tremor:					@ EFFECT Ixy: Tremor
@---------------------------------------------------------------------------------
	bx	lr

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Arpeggio:					@ EFFECT Jxy: Arpeggio
@---------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_arp_ot

	movs	r0, #0
	strb	r0, [r7, #MCH_FXMEM]
.mppe_arp_ot:
	cmp	r6, #0
	beq	1f
	ldrb	r0, [r7, #MCH_FXMEM]
//	ldrb	r3, [r6, #MCA_SAMPLE]	?		???	
	cmp	r0, #1
	bgt	.mppe_arp_2
	beq	.mppe_arp_1
.mppe_arp_0:
	movs	r0, #1
	strb	r0, [r7, #MCH_FXMEM]
	@ do nothing! :)
1:	bx	lr
	
.mppe_arp_1:
	
	movs	r0, #2			@ set next tick to '2'
	strb	r0, [r7, #MCH_FXMEM]	@ save...
	movs	r0, r5
	lsrs	r1, #4			@ mask out high nibble of param
.mppe_arp_others:
	movs	r2, r5
	cmp	r1, #12			@ see if its >= 12
	blt	.mppea1_12		@ ..
	adds	r2, r5			@  add period if so... (octave higher)
.mppea1_12:				@  ..
	lsls	r1, #4			@ *16*hword
	
	movs	r0, r5
	push	{lr}
	mov	r2, r8
	bl	mpph_LinearPitchSlide_Up

	movs	r5, r0
	pop	{r0}
	bx	r0
//	pop	{pc}
	
.mppe_arp_2:
	movs	r0, #0
	strb	r0, [r7, #MCH_FXMEM]
	movs	r0, r5
	lsls	r1, #32-4
	lsrs	r1, #32-4
	b	.mppe_arp_others

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_VibratoVolume:			@ EFFECT Kxy: Vibrato+Volume Slide
@---------------------------------------------------------------------------------

	push	{lr}

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#

	push	{r1,r2}
	movs	r0, r5
	movs	r1, r7
	mov	r2, r8
	bl	mppe_DoVibrato
	movs	r5, r0
	pop	{r1,r2}

	bl	mppe_VolumeSlide

	pop	{r0}
	bx	r0
//	pop	{pc}

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_PortaVolume:			@ EFFECT Lxy: Portamento+Volume Slide
@---------------------------------------------------------------------------------

	push	{lr}

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#

	push	{r1,r2}
	ldrb	r1, [r7, #MCH_MEMORY+MPP_GLIS_MEM]
	bl	mppe_Glissando
	pop	{r1, r2}

	bl	mppe_VolumeSlide

	pop	{r0}
	bx	r0
//	pop	{pc}

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_ChannelVolume:				@ EFFECT Mxy: Set Channel Volume
@---------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_cv_exit			@ quite simple...
	cmp	r1, #0x40
	bgt	.mppe_cv_exit			@ ignore command if parameter is > 0x40
	strb	r1, [r7, #MCH_CVOLUME]
.mppe_cv_exit:
	bx	lr
	
.align 2
.thumb_func
@------------------------------------------------------------------------------------
mppe_ChannelVolumeSlide:			@ EFFECT Nxy: Channel Volume Slide
@------------------------------------------------------------------------------------

	push	{lr}

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#

	ldrb	r0, [r7, #MCH_CVOLUME]		@ load volume

	mov	r3, r8
	bl	mpph_VolumeSlide64

	strb	r0, [r7, #MCH_CVOLUME]		@ save volume
	pop	{r0}
	bx	r0
//	pop	{pc}				@ exit

.align 2
.thumb_func
@----------------------------------------------------------------------------------
mppe_SampleOffset:				@ EFFECT Oxy Sample Offset
@----------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_so_exit			@ skip on other ticks

	ldr	r0,=mpp_vars
	strb	r1, [r0, #MPV_SAMPOFF]		@ set offset
.mppe_so_exit:
	bx	lr
.pool

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_PanningSlide:				@ EFFECT Pxy Panning Slide
@---------------------------------------------------------------------------------

	// TODO: This is unused! Is that a mistake, or was this buggy?
	push	{lr}

	ldrb	r0, [r7, #MCH_PANNING]		@ load panning
	movs	r3, #255

	mov	r4, r8
	push	{r4}
	bl	mpph_VolumeSlide
	pop	{r4}
	mov	r8, r4

	strb	r0, [r7, #MCH_PANNING]		@ save panning
	pop	{r0}
	bx	r0
//	pop	{pc}				@ exit

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Retrigger:					@ EFFECT Qxy Retrigger Note
@---------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#

	ldrb	r0, [r7, #MCH_FXMEM]
	cmp	r0, #0
	bne	.mppe_retrig_refillN
.mppe_retrig_refill:
	lsls	r0, r1, #32-4
	lsrs	r0, #32-4
	adds	r0, #1
.mppe_retrig_exitcount:
	strb	r0, [r7, #MCH_FXMEM]
	bx	lr
.mppe_retrig_refillN:
	subs	r0, #1
	cmp	r0, #1
	bne	.mppe_retrig_exitcount
.mppe_retrig_fire:
	
	ldrb	r2, [r7, #MCH_VOLUME]
	lsrs	r0, r1, #4
	beq	.mppe_retrig_v_change0
	cmp	r0, #5
	ble	.mppe_retrig_v_change_sub
	cmp	r0, #6
	beq	.mppe_retrig_v_change_23
	cmp	r0, #7
	beq	.mppe_retrig_v_change_12
	cmp	r0, #8
	beq	.mppe_retrig_v_change0
	cmp	r0, #0xD
	ble	.mppe_retrig_v_change_add
	cmp	r0, #0xE
	beq	.mppe_retrig_v_change_32
	cmp	r0, #0xF
	beq	.mppe_retrig_v_change_21
.mppe_retrig_v_change_21:
	lsls	r2, #1
	cmp	r2, #64
	blt	.mppe_retrig_finish
	movs	r2, #64
.mppe_retrig_v_change0:
	b	.mppe_retrig_finish
.mppe_retrig_v_change_sub:
	subs	r0, #1
	movs	r3, #1
	lsls	r3, r0
	subs	r2, r3
	bcs	.mppe_retrig_finish
	movs	r2, #0
	b	.mppe_retrig_finish
.mppe_retrig_v_change_23:
	movs	r0, #171
	muls	r2, r0
	lsrs	r2, #8
	b	.mppe_retrig_finish
.mppe_retrig_v_change_12:
	lsrs	r2, #1
	b	.mppe_retrig_finish
.mppe_retrig_v_change_add:
	subs	r0, #9
	movs	r3, #1
	lsls	r3, r0
	adds	r2, r3
	cmp	r2, #64
	blt	.mppe_retrig_finish
	movs	r2, #64
	b	.mppe_retrig_finish
.mppe_retrig_v_change_32:
	movs	r0, #192
	muls	r2, r0
	lsrs	r2, #7
.mppe_retrig_finish:
	strb	r2, [r7, #MCH_VOLUME]
	cmp	r6, #0
	beq	.mppe_retrig_refill
	ldrb	r0, [r6, #MCA_FLAGS]
	movs	r2, #MCAF_START
	orrs	r0, r2
	strb	r0, [r6, #MCA_FLAGS]
	b	.mppe_retrig_refill

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Tremolo:					@ EFFECT Rxy: Tremolo
@---------------------------------------------------------------------------------

@ r1 = param

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	beq	.mppe_trem_zt			@ skip this part on tick0
.mppe_trem_ot:
	@ X = speed, Y = depth
	ldrb	r0, [r7, #MCH_FXMEM]		@ get sine position
	lsrs	r3, r1, #4			@ mask out SPEED
	lsls	r3, #2				@ speed*4 to compensate for larger sine table
	adds	r0, r3				@ add to position
	strb	r0, [r7, #MCH_FXMEM]		@ save (value & 255)
.mppe_trem_zt:
	ldrb	r0, [r7, #MCH_FXMEM]		@ get sine position
	ldr	r3,=mpp_TABLE_FineSineData	@ load sine table value
	ldrsb	r0, [r3, r0]
	lsls	r1, #32-4			@ mask out DEPTH
	lsrs	r1, #32-4
	muls	r0, r1				@ SINE*DEPTH / 64
	asrs	r0, #6
	mov	r1, r8
	ldrb	r1, [r1, #MPL_FLAGS]
	lsrs	r1, #C_FLAGS_XS
	bcs	1f
	asrs	r0, #1
1:	ldr	r1,=mpp_vars			@ set volume addition variable
	strb	r0, [r1, #MPV_VOLPLUS]
	bx	lr
.pool

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Extended:				@ EFFECT Sxy: Extended Effects
@---------------------------------------------------------------------------------
	
	lsrs	r0, r1, #4
	lsls	r0, #1
	add	r0, pc
	mov	pc, r0

	@ branch table...
	b	mppex_XM_FVolSlideUp	@ S0x
	b	mppex_XM_FVolSlideDown	@ S1x
	b	mppex_OldRetrig		@ S2x
	b	mppex_VibForm		@ S3x
	b	mppex_TremForm		@ S4x
	b	mppex_PanbForm		@ S5x
	b	mppex_FPattDelay	@ S6x
	b	mppex_InstControl	@ S7x
	b	mppex_SetPanning	@ S8x
	b	mppex_SoundControl	@ S9x
	b	mppex_HighOffset	@ SAx
	b	mppex_PatternLoop	@ SBx
	b	mppex_NoteCut		@ SCx
	b	mppex_NoteDelay		@ SDx
	b	mppex_PatternDelay	@ SEy
	b	mppex_SongMessage	@ SFx

.mppe_ex_quit:

@-------------------------------------------
.thumb_func
mppex_Unused:
	bx	lr

@-------------------------------------------

.thumb_func
mppex_XM_FVolSlideUp:
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	2f
	ldrb	r0, [r7, #MCH_VOLUME]
	lsls	r1, #32-4
	lsrs	r1, #32-4
	adds	r0, r1
	cmp	r0, #64
	blt	1f
	movs	r0, #64
1:	strb	r0, [r7, #MCH_VOLUME]
2:	bx	lr

.thumb_func
mppex_XM_FVolSlideDown:
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	2f
	ldrb	r0, [r7, #MCH_VOLUME]
	lsls	r1, #32-4
	lsrs	r1, #32-4
	subs	r0, r1
	bcs	1f
	movs	r0, #0
1:	strb	r0, [r7, #MCH_VOLUME]
2:	bx	lr

@-------------------------------------------
.thumb_func
mppex_OldRetrig:
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	1f
	lsls	r1, #32-4
	lsrs	r1, #32-4
	strb	r1, [r7, #MCH_FXMEM]
	bx	lr
	
1:	ldrb	r0, [r7, #MCH_FXMEM]
	subs	r0, #1

	bne	1f
	
	lsls	r0, r1, #32-4
	lsrs	r0, #32-4
	strb	r0, [r7, #MCH_FXMEM]

	cmp	r6, #0
	beq	1f
	ldrb	r1, [r6, #MCA_FLAGS]
	movs	r2, #MCAF_START
	
	orrs	r1, r2
	strb	r1, [r6, #MCA_FLAGS]
	
1:	strb	r0, [r7, #MCH_FXMEM]
	bx	lr

@-------------------------------------------
.thumb_func
mppex_VibForm:
	bx	lr

@-------------------------------------------
.thumb_func
mppex_TremForm:
	bx	lr

@-------------------------------------------
.thumb_func
mppex_PanbForm:
	bx	lr

@-------------------------------------------
.thumb_func
mppex_FPattDelay:
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppex_fpd_exit
	lsls	r1, #32-4
	lsrs	r1, #32-4
	mov	r0, r8
	strb	r1, [r0, #MPL_FPATTDELAY]
.mppex_fpd_exit:
	bx	lr

@-------------------------------------------
.thumb_func
mppex_InstControl:
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppex_ic_exit
	lsls	r1, #32-4
	lsrs	r1, #32-4
	cmp	r1, #2
	ble	.mppex_ic_pastnotes
	cmp	r1, #6
	ble	.mppex_ic_nna
	cmp	r1, #8
	ble	.mppex_ic_envelope
	bx	lr
.mppex_ic_pastnotes:
	@ todo...
	bx	lr
.mppex_ic_nna:
	@ overwrite NNA
	subs	r1, #3
	ldrb	r2, [r7, #MCH_BFLAGS]
	lsls	r2, #32-6
	lsrs	r2, #32-6
	lsls	r1, #6
	orrs	r2, r1
	strb	r2, [r7, #MCH_BFLAGS]
	bx	lr
.mppex_ic_envelope:
	cmp	r6, #0
	beq	.mppex_ic_exit
	ldrb	r2, [r6, #MCA_FLAGS]
	movs	r0, #32
	bics	r2, r0
	subs	r1, #7
	lsls	r1, #5
	orrs	r2, r1
	strb	r2, [r6, #MCA_FLAGS]

.mppex_ic_exit:
	bx	lr

@-------------------------------------------
.thumb_func
mppex_SetPanning:
	lsls	r1, #4
	strb	r1, [r7, #MCH_PANNING]
	bx	lr

@-------------------------------------------
.thumb_func
mppex_SoundControl:
	cmp	r1, #0x91
	beq	.mppex_sc_surround
	bx	lr
.mppex_sc_surround:
	@ set surround
	bx	lr

@-------------------------------------------
.thumb_func
mppex_HighOffset:
	@ todo...
	bx	lr

@-------------------------------------------
.thumb_func
mppex_PatternLoop:
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppex_pl_exit				@ dont update on nonzero ticks
	mov	r2, r8

	lsls	r1, #32-4				@ mask low nibble of parameter
	lsrs	r1, #32-4				@ ...
	bne	.mppex_pl_not0				@ is zero?
	
	ldrb	r1, [r2, #MPL_ROW]			@   ...
	strb	r1, [r2, #MPL_PLOOP_ROW]		@    ...
	ldr	r1,=mpp_vars				@    ....
	ldr	r1, [r1, #MPV_PATTREAD_P]		@   .. ...
	movs	r3, #MPL_PLOOP_ADR
	str	r1, [r2, r3]				@  ...  ...
	bx	lr					@ ...    ...
.mppex_pl_not0:						@ otherwise...
	ldrb	r0, [r2, #MPL_PLOOP_TIMES]		@   get pattern loop counter
	cmp	r0, #0					@   zero?
	bne	.mppex_pl_active			@   if not then its already active
	strb	r1, [r2, #MPL_PLOOP_TIMES]		@     zero: save parameter to counter
	b	.mppex_pl_exit_enable			@     exit & enable jump
.mppex_pl_active:					@  nonzero:
	subs	r0, #1					@    decrement counter
	strb	r0, [r2, #MPL_PLOOP_TIMES]		@    save
	beq	.mppex_pl_exit				@    enable jump if not 0
.mppex_pl_exit_enable:
	movs	r0, #1					@    enable jump
	movs	r3, #MPL_PLOOP_JUMP
	strb	r0, [r2, r3]				@    ..
.mppex_pl_exit:						@    exit
	bx	lr					@    ....
.pool

@-------------------------------------------
.thumb_func
mppex_NoteCut:
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	lsls	r1, #32-4			@ mask parameter
	lsrs	r1, #32-4			@ ..
	cmp	r1, r2				@ compare with tick#
	bne	.mppex_nc_exit			@ if equal:
	movs	r0, #0				@   cut volume
	strb	r0, [r7, #MCH_VOLUME]		@   ..
.mppex_nc_exit:					@ exit
	bx	lr				@ ..

@-------------------------------------------
.thumb_func
mppex_NoteDelay:
	
	mov	r0, r8
	ldrb	r2, [r0, #MPL_TICK]
	lsls	r1, #32-4
	lsrs	r1, #32-4
	cmp	r2, r1
	bge	1f
	ldr	r0,=mpp_vars
	strb	r1, [r0, #MPV_NOTEDELAY]
1:	bx	lr

@-------------------------------------------
.thumb_func
mppex_PatternDelay:
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppex_pd_quit			@ update on tick0
	lsls	r1, #32-4			@ mask parameter
	lsrs	r1, #32-4			@ ..
	mov	r0, r8
	ldrb	r2, [r0, #MPL_PATTDELAY]	@ get patterndelay
	cmp	r2, #0				@ only update if it's 0
	bne	.mppex_pd_quit			@ ..
	adds	r1, #1				@ set to param+1
	strb	r1, [r0, #MPL_PATTDELAY]	@ ..
.mppex_pd_quit:					@ exit
	bx	lr				@ ..

@-------------------------------------------
.thumb_func
mppex_SongMessage:

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppex_pd_quit			@ update on tick0
	push	{lr}				@ save return address
	lsls	r1, #32-4			@ mask parameter
	lsrs	r1, #32-4			@ ..
	ldr	r2,=mmCallback
	ldr	r2, [r2]
	cmp	r2, #0
	beq	1f
	movs	r0, #MPCB_SONGMESSAGE
	
	bl	mpp_call_r2
	@jump2
1:	//pop	{pc}
	pop	{r0}
	bx	r0
.pool

.align 2
.thumb_func
@----------------------------------------------------------------------------------------
mppe_SetTempo:					@ EFFECT Txy: Set Tempo / Tempo Slide
@----------------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#

	@ 0x  = slide down
	@ 1x  = slide up
	@ 2x+ = set
	
	// BUGGED???
	// not using setbpm for slides???

	cmp	r1, #0x20
	bge	.mppe_st_set
	cmp	r2, #0
	beq	.mppe_st_exit

	mov	r0, r8
	ldrb	r2, [r0, #MPL_BPM]

	cmp	r1, #0x10
	bge	.mppe_st_slideup
.mppe_st_slidedown:
	subs	r2, r1
	cmp	r2, #32
	bge	.mppe_st_save
	movs	r2, #32
.mppe_st_save:
	movs	r0, r2
	b	.mppe_st_set2
.mppe_st_slideup:
	
	lsls	r1, #32-4
	lsrs	r1, #32-4
	
	adds	r2, r1
	cmp	r2, #255
	blt	.mppe_st_save
	movs	r2, #255
	b	.mppe_st_save
.mppe_st_set:
	cmp	r2, #0
	bne	.mppe_st_exit
	movs	r0, r1
.mppe_st_set2:
	push	{lr}
	movs	r1, r0
	mov	r0, r8
	ldr	r2,=mpp_setbpm
	bl	mpp_call_r2
	@jump1
	pop	{r3}
	bx	r3
.mppe_st_exit:
	bx	lr
.pool

.align 2
.thumb_func
@----------------------------------------------------------------------------------------
mppe_FineVibrato:				@ EFFECT Uxy: Fine Vibrato
@----------------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_fv_ot
	lsrs	r0, r1, #4
	beq	.mppe_fv_nospd
	lsls	r0, #2
	strb	r0, [r7, #MCH_VIBSPD]
.mppe_fv_nospd:
	
	lsls	r0, r1, #32-4
	beq	.mppe_fv_nodep
//	lsrs	r0, #32			heh...
	lsrs	r0, #32-4
	mov	r1, r8
	ldrb	r1, [r1, #MPL_OLDEFFECTS]
	lsls	r0, r1
	strb	r0, [r7, #MCH_VIBDEP]
.mppe_fv_nodep:

.mppe_fv_ot:
	movs	r0, r5
	movs	r1, r7
	mov	r2, r8
	mov	r4, lr
	bl	mppe_DoVibrato
	movs	r5, r0
	bx	r4
.pool

.align 2
.thumb_func
@------------------------------------------------------------------------------------
mppe_SetGlobalVolume:			@ EFFECT Vxy: Set Global Volume
@------------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_sgv_exit		@ on tick0:
	mov	r0, r8
	ldrb	r2, [r0, #MPL_FLAGS]
	movs	r3, #(1<<(C_FLAGS_XS-1))+(1<<(C_FLAGS_LS-1))
	tst	r2, r3
	beq	1f
	movs	r2, #0x40
	b	2f
1:	movs	r2, #0x80
2:
	cmp	r1, r2
	blt	1f
	movs	r1, r2
1:	strb	r1, [r0, #MPL_GV]	@ save param to global volume
.mppe_sgv_exit:
	bx	lr
.pool


.align 2
.thumb_func
@----------------------------------------------------------------------------------
mppe_GlobalVolumeSlide:				@ EFFECT Wxy: Global Volume Slide
@----------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#

	push	{lr}

.mppe_gvs_ot:
	mov	r0, r8
	ldrb	r0, [r0, #MPL_FLAGS]
	lsrs	r0, #C_FLAGS_XS
	bcs	1f
	
	movs	r0, #128
	b	2f

1:	movs	r0, #64

2:
	movs	r3, r0

	mov	r0, r8
	ldrb	r0, [r0, #MPL_GV]		@ load global volume

	mov	r4, r8
	push	{r4}
	bl	mpph_VolumeSlide
	pop	{r4}
	mov	r8, r4

	mov	r1, r8
	strb	r0, [r1, #MPL_GV]		@ save global volume
	pop	{r0}
	bx	r0
//	pop	{pc}				@ exit
.pool


.align 2
.thumb_func
@---------------------------------------------------------------------------------------
mppe_SetPanning:				@ EFFECT Xxy: Set Panning
@---------------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_sp_exit			@ on tick0:
	strb	r1, [r7, #MCH_PANNING]		@ set panning=param
.mppe_sp_exit:
	bx	lr
.pool

.align 2
.thumb_func
@---------------------------------------------------------------------------------------
mppe_Panbrello:					@ EFFECT Yxy: Panbrello
@---------------------------------------------------------------------------------------

	@ todo
	bx	lr

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_ZXX:				@ EFFECT Zxy: Set Filter
@---------------------------------------------------------------------------------

@	ZXX IS NOT SUPPORTED
	bx	lr			@     exit
.pool

@=======================================================================================
@                                     OLD EFFECTS
@=======================================================================================

.align 2
.thumb_func
@-----------------------------------------------------------------------------------
mppe_SetVolume:				@ EFFECT 0xx: Set Volume
@-----------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_sv_exit		@ on tick0:
	strb	r1, [r7, #MCH_VOLUME]	@ set volume=param
.mppe_sv_exit:
	bx	lr

.align 2
.thumb_func
@-----------------------------------------------------------------------------------
mppe_KeyOff:				@ EFFECT 1xx: Key Off
@-----------------------------------------------------------------------------------
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r1, r2			@ if tick=param:
	bne	.mppe_ko_exit		@
	cmp	r6, #0
	beq	.mppe_ko_exit
	ldrb	r0, [r6, #MCA_FLAGS]	@   clear keyon from flags
	movs	r1, #MCAF_KEYON		@
	bics	r0, r1			@
	strb	r0, [r6, #MCA_FLAGS]	@
.mppe_ko_exit:
	bx	lr			@ finished
.pool

.align 2
.thumb_func
@-----------------------------------------------------------------------------------
mppe_EnvelopePos:			@ EFFECT 1xx: Envelope Position
@-----------------------------------------------------------------------------------
	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_ep_ot		@ on tick0:
	cmp	r6, #0
	beq	.mppe_ep_ot

	@ - NOT SUPPORTED ANYMORE -

@	strh	r1, [r6, #MCA_ENVP_VOL]	@ set volume envelope position
@	strh	r1, [r6, #MCA_ENVP_PAN]	@ set panning envelope positin
					@ pitch envelope wasn't invented yet
.mppe_ep_ot:
	bx	lr			@ finished

.align 2
.thumb_func
@-----------------------------------------------------------------------------------
mppe_OldTremor:				@ EFFECT 3xy: Old Tremor
@-----------------------------------------------------------------------------------

	mov	r2, r8
	ldrb	r2, [r2, #MPL_TICK]	@ r2 = tick#
	cmp	r2, #0			@ Z flag = tick0
	bne	.mppe_ot_ot
	bx	lr
.mppe_ot_ot:
	ldrb	r0, [r7, #MCH_FXMEM]
	cmp	r0, #0
	bne	.mppe_ot_old
.mppe_ot_new:
	ldrb	r0, [r7, #MCH_BFLAGS+1]
	movs	r3, #0b110
	eors	r0, r3
	strb	r0, [r7, #MCH_BFLAGS+1]
	lsrs	r0, #3
	bcc	.mppe_ot_low
	lsrs	r1, #4
	adds	r1, #1
	strb	r1, [r7, #MCH_FXMEM]
	b	.mppe_ot_apply
.mppe_ot_low:
	lsls	r1, #32-4
	lsrs	r1, #32-4
	adds	r1, #1
	strb	r1, [r7, #MCH_FXMEM]
	b	.mppe_ot_apply
	
.mppe_ot_old:
	subs	r0, #1
	strb	r0, [r7, #MCH_FXMEM]

.mppe_ot_apply:
	ldrb	r2, [r7, #MCH_BFLAGS+1]
	
	lsrs	r2, #3
	bcs	.mppe_ot_cut
	movs	r1, #-64&255
	ldr	r2,=mpp_vars
	strb	r1, [r2, #MPV_VOLPLUS]
.mppe_ot_cut:
	bx	lr

@==========================================================================================
@                                          TABLES
@==========================================================================================

.text

@-------------------------------------------------------------------------------------
    .global mpp_TABLE_FineSineData
mpp_TABLE_FineSineData:
@-------------------------------------------------------------------------------------

.byte	  0,  2,  3,  5,  6,  8,  9, 11, 12, 14, 16, 17, 19, 20, 22, 23
.byte	 24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44
.byte	 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59
.byte	 59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64
.byte	 64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60
.byte	 59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46
.byte	 45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26
.byte	 24, 23, 22, 20, 19, 17, 16, 14, 12, 11,  9,  8,  6,  5,  3,  2
.byte	  0, -2, -3, -5, -6, -8, -9,-11,-12,-14,-16,-17,-19,-20,-22,-23
.byte	-24,-26,-27,-29,-30,-32,-33,-34,-36,-37,-38,-39,-41,-42,-43,-44
.byte	-45,-46,-47,-48,-49,-50,-51,-52,-53,-54,-55,-56,-56,-57,-58,-59
.byte	-59,-60,-60,-61,-61,-62,-62,-62,-63,-63,-63,-64,-64,-64,-64,-64
.byte	-64,-64,-64,-64,-64,-64,-63,-63,-63,-62,-62,-62,-61,-61,-60,-60
.byte	-59,-59,-58,-57,-56,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46
.byte	-45,-44,-43,-42,-41,-39,-38,-37,-36,-34,-33,-32,-30,-29,-27,-26
.byte	-24,-23,-22,-20,-19,-17,-16,-14,-12,-11, -9, -8, -6, -5, -3, -2

.end
