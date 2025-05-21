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
	b	mppe_SetSpeed_Wrapper
	b	mppe_PositionJump_Wrapper
	b	mppe_PatternBreak_Wrapper
	b	mppe_VolumeSlide_Wrapper
	b	mppe_Portamento
	b	mppe_Portamento
	b	mppe_Glissando
	b	mppe_Vibrato_Wrapper
	b	mppe_todo
	b	mppe_Arpeggio_Wrapper
	b	mppe_VibratoVolume_Wrapper
	b	mppe_PortaVolume
	b	mppe_ChannelVolume
	b	mppe_ChannelVolumeSlide
	b	mppe_SampleOffset
	b	mppe_todo
	b	mppe_Retrigger
	b	mppe_Tremolo		@ tremolo
	b	mppe_Extended_Wrapper
	b	mppe_SetTempo_Wrapper
	b	mppe_FineVibrato_Wrapper
	b	mppe_SetGlobalVolume_Wrapper
	b	mppe_GlobalVolumeSlide_Wrapper
	b	mppe_SetPanning_Wrapper
	b	mppe_Panbrello
	b	mppe_ZXX
	b	mppe_SetVolume_Wrapper
	b	mppe_KeyOff_Wrapper
	b	mppe_EnvelopePos
	b	mppe_OldTremor
.pool

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_SetSpeed_Wrapper: @ EFFECT Axy: SET SPEED
@---------------------------------------------------------------------------------
	movs	r0, r1
	mov	r1, r8

	push	{lr}
	bl mppe_SetSpeed
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_todo:
@---------------------------------------------------------------------------------
	bx	lr			@ exit

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_PositionJump_Wrapper: @ EFFECT Bxy: SET POSITION
@---------------------------------------------------------------------------------
	movs	r0, r1
	mov	r1, r8

	push	{lr}
	bl mppe_PositionJump
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_PatternBreak_Wrapper: @ EFFECT Cxy: PATTERN BREAK
@---------------------------------------------------------------------------------
	movs	r0, r1
	mov	r1, r8

	push	{lr}
	bl mppe_PatternBreak
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@------------------------------------------------------------------------------------------
mppe_VolumeSlide_Wrapper: @ EFFECT Dxy: VOLUME SLIDE
@------------------------------------------------------------------------------------------
	movs	r0, r1
	movs	r1, r7
	mov	r2, r8

	push	{lr}
	bl mppe_VolumeSlide
	pop	{r2}
	bx	r2

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
mppe_Vibrato_Wrapper: @ EFFECT Hxy: Vibrato
@---------------------------------------------------------------------------------
	movs	r0, r1
	movs	r1, r5
	movs	r2, r7
	mov	r3, r8

	push	{lr}
	bl	mppe_Vibrato
	movs	r5, r0
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Tremor:					@ EFFECT Ixy: Tremor
@---------------------------------------------------------------------------------
	bx	lr

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_Arpeggio_Wrapper: @ EFFECT Jxy: Arpeggio
@---------------------------------------------------------------------------------
	push	{lr}

	mov	r0, r8
	push	{r0} // Push mpl_layer_information to the stack as 5th argument

	movs	r0, r1
	movs	r1, r5
	movs	r2, r6
	movs	r3, r7

	bl	mppe_Arpeggio
	movs	r5, r0

	pop	{r1} // Pop the argument passed on the stack
	pop	{r1}
	bx	r1

.align 2
.thumb_func
@---------------------------------------------------------------------------------
mppe_VibratoVolume_Wrapper: @ EFFECT Kxy: Vibrato+Volume Slide
@---------------------------------------------------------------------------------
	movs	r0, r1
	movs	r1, r5
	movs	r2, r7
	mov	r3, r8

	push	{lr}
	bl	mppe_VibratoVolume
	movs	r5, r0
	pop	{r2}
	bx	r2

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

	bl	mppe_VolumeSlide_Wrapper

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
mppe_Extended_Wrapper: // EFFECT Sxy: Extended Effects
@---------------------------------------------------------------------------------
	// r1 = param
	// r6 = mm_active_channel
	// r7 = mm_module_channel
	// r8 = mpl_layer_information

	movs	r0, r1
	movs	r1, r6
	movs	r2, r7
	mov	r3, r8

	push	{lr}
	bl	mppe_Extended
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@----------------------------------------------------------------------------------------
mppe_SetTempo_Wrapper: @ EFFECT Txy: Set Tempo / Tempo Slide
@----------------------------------------------------------------------------------------
	movs	r0, r1
	mov	r1, r8

	push	{lr}
	bl	mppe_SetTempo
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@----------------------------------------------------------------------------------------
mppe_FineVibrato_Wrapper: @ EFFECT Uxy: Fine Vibrato
@----------------------------------------------------------------------------------------
	movs	r0, r1
	movs	r1, r5
	movs	r2, r7
	mov	r3, r8

	push	{lr}
	bl	mppe_FineVibrato
	movs	r5, r0
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@------------------------------------------------------------------------------------
mppe_SetGlobalVolume_Wrapper: @ EFFECT Vxy: Set Global Volume
@------------------------------------------------------------------------------------
	movs	r0, r1
	mov	r1, r8

	push	{lr}
	bl	mppe_SetGlobalVolume
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@----------------------------------------------------------------------------------
mppe_GlobalVolumeSlide_Wrapper: @ EFFECT Wxy: Global Volume Slide
@----------------------------------------------------------------------------------
	movs	r0, r1
	mov	r1, r8

	push	{lr}
	bl	mppe_GlobalVolumeSlide
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@---------------------------------------------------------------------------------------
mppe_SetPanning_Wrapper: @ EFFECT Xxy: Set Panning
@---------------------------------------------------------------------------------------
	movs	r0, r1
	movs	r1, r7
	mov	r2, r8

	push	{lr}
	bl mppe_SetPanning
	pop	{r2}
	bx	r2

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
mppe_SetVolume_Wrapper: @ EFFECT 0xx: Set Volume
@-----------------------------------------------------------------------------------
	movs	r0, r1
	movs	r1, r7
	mov	r2, r8

	push	{lr}
	bl mppe_SetVolume
	pop	{r2}
	bx	r2

.align 2
.thumb_func
@-----------------------------------------------------------------------------------
mppe_KeyOff_Wrapper: @ EFFECT 1xx: Key Off
@-----------------------------------------------------------------------------------
	movs	r0, r1
	movs	r1, r6
	mov	r2, r8

	push	{lr}
	bl mppe_KeyOff
	pop	{r2}
	bx	r2

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
