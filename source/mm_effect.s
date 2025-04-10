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
 *                           Sound Effect System                            *
 *                                                                          *
 ****************************************************************************/

#include	"mp_defs.inc"
#include	"mp_mas.inc"
#include	"mp_mas_structs.inc"
#include	"mp_format_mas.inc"
#include	"mp_macros.inc"

#ifdef SYS_GBA
#include	"mp_mixer_gba.inc"
#endif

#ifdef SYS_NDS
#include	"mp_mixer_ds.inc"
#endif

/***********************************************************************
 *
 * Definitions
 *
 ***********************************************************************/

// offsets of structure "mm_sound_effect"
.equ MM_SFX_SOURCE,	0			// word: source
.equ MM_SFX_RATE,	MM_SFX_SOURCE	+ 4	// hword: rate
.equ MM_SFX_HANDLE,	MM_SFX_RATE	+ 2	// byte:  handle
.equ MM_SFX_VOLUME,	MM_SFX_HANDLE	+ 2	// byte:  volume
.equ MM_SFX_PANNING,	MM_SFX_VOLUME	+ 1	// byte:  panning
.equ MM_SFX_SIZE,	MM_SFX_PANNING	+ 1 	// 8 bytes total

.equ	channelCount, 16
.equ	releaseLevel, 200

/***********************************************************************
 *
 * Memory
 *
 ***********************************************************************/

	.bss
	.align 2
	.global mm_sfx_bitmask, mm_sfx_clearmask

mm_sfx_bitmask:		.space 4
mm_sfx_clearmask:	.space 4

/***********************************************************************
 *
 * Program
 *
 ***********************************************************************/
 
	.text
	.syntax unified
	.thumb
	.align 2
	
/***********************************************************************
 * mmGetFreeEffectChannel()
 *
 * Return index to free effect channel
 ***********************************************************************/
						.thumb_func
mmGetFreeEffectChannel:
	
	ldr	r0,=mm_sfx_bitmask		// r0 = bitmask
	ldr	r0, [r0]			//
	movs	r1, #1				// r1 = channel counter
	
.channel_search:				// shift out bits until we find a cleared one
	lsrs	r0, #1				//
	bcc	.found_channel			//
	adds	r1, #1				// r1 = index value
	b	.channel_search			//
	
.found_channel:
	
	cmp	r1, #channelCount+1		// if r1 == cc+1 then r1 = 0 (no handles avail.)
	bne	.found_valid_channel		//
	movs	r1, #0				//
.found_valid_channel:				//
	
	movs	r0, r1				// return value
	bx	lr				//

/***********************************************************************
 * mmEffectEx( sound )
 *
 * Play sound effect with specified parameters
 ***********************************************************************/
						.global mmEffectEx
						.thumb_func
mmEffectEx:
	
	push	{r4-r6, lr}
	
	movs	r4, r0
	ldrh	r5, [r4, #MM_SFX_HANDLE]	// test if handle was given
	
	cmp	r5, #255
	bne	1f
	movs	r5, #0
	b	.got_handle
	
1:	cmp	r5, #0				//
	beq	.generate_new_handle		//
	
	lsls	r1, r5, #24			// check if channel is in use
	lsrs	r1, #23				//
	subs	r1, #2				//
	ldr	r0,=mm_sfx_channels		//
	ldrb	r0, [r0, r1]			//
	cmp	r0, #0				//
	beq	.got_handle			// [valid handle otherwise]
	
	movs	r0, r5
	bl	mmEffectCancel			// attempt to stop active channel
	cmp	r0, #0				//
	bne	.got_handle			//
						// failure: generate new handle
.generate_new_handle:
	
	bl	mmGetFreeEffectChannel		// generate handle
	movs	r5, r0				//
	beq	.got_handle			//-(no available channels)
						//
	ldr	r0,=mm_sfx_counter		// (counter = ((counter+1) & 255))
	ldrb	r1, [r0]			//
	adds	r1, #1				//
	strb	r1, [r0]			//
	lsls	r1, #8
	orrs	r5, r1
	lsls	r5, #16
	lsrs	r5, #16
	
.got_handle:
	
	ldr	r1,=mmAllocChannel		// allocate new channel
	bl	mpp_call_r1			//
	cmp	r0, #255			//
	bge	.no_available_channels		//
	
	movs	r6, r0
	
	cmp	r5, #0
	beq	1f 
	
	ldr	r1,=mm_sfx_channels		// register data
	subs	r2, r5, #1			// r3 = bit
	lsls	r2, #24				//
	lsrs	r2, #24				//
	movs	r3, #1				//
	lsls	r3, r2				//
	lsls	r2, #1				//
	adds	r1, r2				//
	adds	r2, r0, #1			//
	strb	r2, [r1, #0]			//
	lsrs	r2, r5, #8			//
	strb	r2, [r1, #1]			//
	
	ldr	r1,=mm_sfx_bitmask		// set bit
	ldr	r2, [r1]			//
	orrs	r2, r3				//
	str	r2, [r1]			//
	
1:	

	ldr	r1,=mm_achannels		// setup channel
	ldr	r1, [r1]			//
	movs	r2, #MCA_SIZE			//
	muls	r2, r0				//
	adds	r1, r2				//
						//
	movs	r2, #releaseLevel		//
	strb	r2, [r1, #MCA_FVOL]		//
	
	cmp	r5, #0				//
	bne	1f				//
	movs	r2, #ACHN_BACKGROUND		//
	b	2f				//
1:	movs	r2, #ACHN_CUSTOM		//
2:						//
	strb	r2, [r1, #MCA_TYPE]		//
	movs	r2, #MCAF_EFFECT		//
	strb	r2, [r1, #MCA_FLAGS]		//
	
	GET_MIXCH r1				// setup voice
	movs	r2, #MIXER_CHN_SIZE		//
	muls	r2, r0				//
	adds	r3, r1, r2			//
	
#ifdef SYS_GBA

	ldr	r0,=mp_solution			// set sample data address
	ldr	r0, [r0]			//
	ldrh	r1, [r4, #MM_SFX_SOURCE]	//
	lsls	r1, #2				//
	adds	r1, #12				//
	ldr	r1, [r0, r1]			//
	adds	r1, r0				//
	ldrh	r2, [r1, #8+C_SAMPLEC_DFREQ]	//
	adds	r1, #8+C_SAMPLE_DATA		//
	str	r1, [r3, #MIXER_CHN_SRC]	//
	
	ldrh	r0, [r4, #MM_SFX_RATE]		// set pitch to original * pitch
	muls	r2, r0				//
	lsrs	r2, #10-2 			//
	str	r2, [r3, #MIXER_CHN_FREQ]	//
	
	movs	r1, #0				// reset read position
	str	r1, [r3, #MIXER_CHN_READ]	//
	
	ldrb	r0, [r4, #MM_SFX_VOLUME]	// set volume
	
	ldr	r1,=mm_sfx_mastervolume
	ldr	r1, [r1]
	muls	r0, r1
	lsrs	r0, #10
	
	strb	r0, [r3, #MIXER_CHN_VOL]	//
	
	ldrb	r0, [r4, #MM_SFX_PANNING]	// set panning
	strb	r0, [r3, #MIXER_CHN_PAN]	//
	
#else
	ldr	r1, [r4, #MM_SFX_SOURCE]	// set sample address
	lsrs	r0, r1, #16			// > 0x10000 = external sample
	bne	1f				//
	ldr	r0,=mmSampleBank		// ID# otherwise
	ldr	r0, [r0]			//
	lsls	r1, #2				//
	ldr	r1, [r0, r1]			//
	lsls	r1, #8				//
	lsrs	r1, #8				//
	beq	.invalid_sample
	adds	r1, #8				//
	ldr	r0,=0x2000000			//
	adds	r1, r0				//
1:						//
	str	r1, [r3, #MIXER_CHN_SAMP]	//

	
	ldrh	r0, [r4, #MM_SFX_RATE]		// set pitch to original * pitch
	ldrh	r1, [r1, #C_SAMPLEC_DFREQ]	//
	muls	r1, r0				//
	lsrs	r1, #10 			//
	strh	r1, [r3, #MIXER_CHN_FREQ]	//
	
	movs	r1, #0				// clear sample offset
	str	r1, [r3, #MIXER_CHN_READ]	//
	
	ldrb	r1, [r4, #MM_SFX_PANNING]	// set panning + startbit
	lsrs	r1, #1				//
	adds	r1, #0x80			//
	strb	r1, [r3, #MIXER_CHN_CNT]	//
	
	ldrb	r1, [r4, #MM_SFX_VOLUME]	// set volume
	ldr	r0,=mm_sfx_mastervolume
	ldr	r0, [r0]
	muls	r1, r0
//	lsrs	r0, #10
	lsrs	r2, r1, #2
//	lsls	r2, r1, #8

	strh	r2, [r3, #MIXER_CHN_VOL]	//
	
	b	.valid_sample
.invalid_sample:
	movs	r0, #0
	str	r0, [r3, #MIXER_CHN_SAMP]
	
.valid_sample:
	
#endif

	movs	r0, r5				// return handle
	pop	{r4-r6}				//
	ret1					//
	
.no_available_channels:	
	movs	r0, #0				// return bad
	pop	{r4-r6}				//
	ret1					//

/***********************************************************************
 * mmEffectCancelAll()
 *
 * Stop all sound effects
 ***********************************************************************/
						.global mmEffectCancelAll
						.thumb_func
mmEffectCancelAll:

	push	{r4-r7,lr}
	
	ldr	r4,=mm_sfx_bitmask
	ldr	r4, [r4]
	ldr	r6,=mm_sfx_channels
	movs	r5, #0
	
	
	lsrs	r4, #1
	bcc	.mmeca_next
.mmeca_process:

	ldrb	r7, [r6, r5]
	subs	r7, #1
	bmi	.mmeca_next
	
	movs	r0, r7
	movs	r1, #0
	bl	mmMixerSetVolume
	
	ldr	r0,=mm_achannels		// free achannel
	ldr	r0, [r0]			//
	movs	r1, #MCA_SIZE			//
	muls	r1, r7				//
	adds	r0, r1				//
	movs	r1, #ACHN_BACKGROUND		//
	strb	r1, [r0, #MCA_TYPE]		//
	movs	r1, #0
	strb	r1, [r0, #MCA_FVOL]		//
	
.mmeca_next:
	adds	r5, #2
	lsrs	r4, #1
	bcs	.mmeca_process
	bne	.mmeca_next
	
	bl	mmResetEffects
	
	POP	{r4-r7}
	pop	{r3}
	bx	r3

/***********************************************************************
 * mmUpdateEffects()
 *
 * Update sound effects
 ***********************************************************************/
						.global mmUpdateEffects
						.thumb_func
mmUpdateEffects:
	
	push	{r4-r6,lr}
	
	ldr	r4,=mm_sfx_bitmask
	ldr	r4, [r4]
	ldr	r6,=mm_sfx_channels
	movs	r5, #0
	
	lsrs	r4, #1
	bcc	.next_channel
.process_channel:

	ldrb	r0, [r6, r5]			// get channel index
	subs	r0, #1				//
	bmi	.next_channel			//
	
	GET_MIXCH r1
	
	movs	r2, #MIXER_CHN_SIZE		// get mixing channel pointer
	muls	r2, r0				//
	adds	r1, r2				//
	
	#ifdef SYS_NDS				// test if channel is still active
						//
	ldr	r2, [r1, #MIXER_CHN_SAMP]	//
	lsls	r2, #8				//
	bne	.next_channel			//
						//
	#else					//
						//
	ldr	r2, [r1, #MIXER_CHN_SRC]	//
	asrs	r2, #31				//
	beq	.next_channel			//
						//
	#endif					//
	
	ldr	r1,=mm_achannels		// free achannel
	ldr	r1, [r1]			//
	movs	r2, #MCA_SIZE			//
	muls	r2, r0				//
	adds	r1, r2				//
	movs	r0, #0				//
	strb	r0, [r1, #MCA_TYPE]		//
	strb	r0, [r1, #MCA_FLAGS]		//
	strb	r0, [r6, r5]
	
.next_channel:
	adds	r5, #2				// look for another set bit
	lsrs	r4, #1				//
	bcs	.process_channel		//
	adds	r5, #2				//
	lsrs	r4, #1				//
	bcs	.process_channel		//
	bne	.next_channel			//
	
	movs	r4, #0
	movs	r5, #1
	lsls	r5, #32-channelCount
	ldr	r6,=mm_sfx_channels
	 
.build_new_bitmask:
	ldrb	r0, [r6]
	adds	r6, #2
	cmp	r0, #0
	beq	1f
	orrs	r4, r5
1:	lsls	r5, #1
	bne	.build_new_bitmask
	
	lsrs	r4, #32-channelCount
	ldr	r0,=mm_sfx_bitmask
	ldr	r1, [r0]			// r1 = bits that change from 1->0
	movs	r2, r1
	eors	r1, r4
	ands	r1, r2

	str	r4, [r0]
	ldr	r4, [r0, #4]
	orrs	r4, r1
	str	r4, [r0, #4]			// write 1->0 mask
	
	pop	{r4-r6,pc}

.pool

.end
