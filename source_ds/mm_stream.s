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
 *                            Audio Streaming                               *
 *                                                                          *
 ****************************************************************************/

#include "mp_defs.inc"
#include "swi_nds.inc"
#include "mp_macros.inc"

/***********************************************************************
 *
 * Definitions
 *
 ***********************************************************************/

	.equ	SOUND4CNT	,0x4000440
	.equ	CLOCK		,33554432
	.equ	TM0CNT		,0x4000100
	.equ	DELAY_SAMPLES	,16
	
	.equ	TIMER_AUTO,	0b11000011	// start, irq, /1024
	.equ	TIMER_MANUAL,	0b10000011	// start, /1024
	

// structure
//			preceding field + length of preceding field
//			ie "v_active + 1" means v_active has a length of 1
//
.equ v_active,		0
.equ v_format,		v_active	+ 1
.equ v_auto,		v_format	+ 1
.equ v_reserved,	v_auto		+ 1
.equ v_clks,		v_reserved	+ 1
.equ v_tmr,		v_clks		+ 2
.equ v_len,		v_tmr		+ 2
.equ v_lenw,		v_len		+ 2
.equ v_pos,		v_lenw		+ 2
.equ v_reserved2,	v_pos		+ 2
.equ v_hwtimer,		v_reserved2	+ 2
.equ v_wave,		v_hwtimer	+ 4
.equ v_workmem,		v_wave		+ 4
.equ v_function,	v_workmem	+ 4
.equ v_remainder,	v_function	+ 4
.equ v_size,		v_remainder	+ 4

/***********************************************************************
 *
 * Global Symbols
 *
 ***********************************************************************/

	.global mmStreamOpen
	.global mmStreamUpdate
	.global mmStreamClose
	
	.global mmStreamGetPosition

	.global mmStreamBegin
	.global mmStreamEnd

/***********************************************************************
 *
 * Memory
 *
 ***********************************************************************/

	.bss
	.syntax unified
	.align 2

//--------------------------------------------------
mmsPreviousTimer:	.space 2
//--------------------------------------------------
			.align 2
StreamCounter:		.space 4
mmsData:		.space v_size
//--------------------------------------------------

/***********************************************************************
 *
 * Program
 *
 ***********************************************************************/
 
	.text
	.thumb
	.align 2

/***********************************************************************
 * mmStreamOpen( stream )
 * mmStreamOpen( stream, wave, mem ) <- ARM7 cannot use malloc
 *
 * Open audio stream.
 ***********************************************************************/
						.thumb_func
mmStreamOpen:
	
	push	{r4,r5,r6,lr}
	
	ldr	r6,=mmsData
	movs	r4, r0
	
	ldrb	r0, [r6, #v_active]		// catch if stream already opened
	cmp	r0, #0				//
	beq	1f				//
	pop	{r4,r5,r6}			//
	ret3
	
1:	
	#ifdef SYS_NDS7				// ARM7: save wave,workmem arguments
	str	r1, [r6, #v_wave]		//
	str	r2, [r6, #v_workmem]		//
	#endif					//
	
	movs	r0, #1				// set active flag
	strb	r0, [r6, #v_active]		//
	
	ldrb	r0, [r4, #mms_timer]		// calc hwtimer address
	ldr	r5,=TM0CNT			//
	lsls	r0, #2				//
	adds	r5, r0				//
	str	r5, [r6, #v_hwtimer]		//
	
	movs	r0, #0				// reset timer
	str	r0, [r5]			//
	
	
	ldrb	r1, [r4, #mms_timer]		// setup irq vector
	movs	r0, #0x8			//
	lsls	r0, r1				//
	ldr	r1,=mmStreamUpdate		//
	bl	irqSet				//
						//
	ldrb	r1, [r4, #mms_timer]		//
	movs	r0, #0x8			//
	lsls	r0, r1				//
	bl	irqEnable			//
	
	ldr	r0,=CLOCK			// calc CLKS
	ldr	r1, [r4, #mms_rate]		//
	swi	SWI_DIVIDE			//
	lsrs	r0, #1				// CLKS must be divisible by 2 for SOUND
	lsls	r0, #1				//
	strh	r0, [r6, #v_clks]		//
	
//------------------------------------------------
// copy information from struct
//------------------------------------------------

	ldr	r0, [r4, #mms_len]		// copy len
	lsrs	r0, #4				// cut to multiple of 16
	lsls	r0, #4
	strh	r0, [r6, #v_len]		//
	
	ldrb	r1, [r4, #mms_format]		// copy format
	strb	r1, [r6, #v_format]		//
	
	lsrs	r1, #1				// calculate buffer size
	bcc	1f				//
	lsls	r0, #1				// <- shift if stereo
1:						//
	cmp	r1, #0				//
	beq	2f				//
	cmp	r1, #1				//
	bne	1f				//
	lsls	r0, #1				// <- shift if 16-bit
	b	2f				//
1:	lsrs	r0, #1				// <- backshift if 4-bit
2:						//
	lsrs	r1, r0, #2			//-- save real length (words)
	strh	r1, [r6, #v_lenw]		//--
	
	#ifdef SYS_NDS9				// ARM9: use malloc
	push	{r0}				// alloc mem for wavebuffer
	bl	malloc				//
	str	r0, [r6, #v_wave]		//
	pop	{r0}				// alloc mem for workbuffer
	bl	malloc				//
	str	r0, [r6, #v_workmem]		//
	#endif					//
	
	ldrh	r0, [r6, #v_lenw]		// 
	ldr	r1, [r6, #v_wave]		//
	movs	r2, #0				//
						//
1:	stmia	r1!, {r2}			//
	subs	r0, #1				//
	bne	1b				//
	
	ldr	r0, [r4, #mms_function]		// copy function
	str	r0, [r6, #v_function]		//
	
	movs	r0, #0				// clear remainder
	str	r0, [r6, #v_remainder]		//
	
//	ldrh	r0, [r6, #v_len]
//	subs	r0, #DELAY_SAMPLES
	strh	r0, [r6, #v_pos]		// reset position
	
	ldrb	r0, [r4, #mms_manual]		// copy manual flag
	cmp	r0, #0
	beq	1f
	movs	r0, #0
	b	2f
1:	movs	r0, #1
2:
	strb	r0, [r6, #v_auto]		//
	
	ldrh	r0, [r6, #v_clks]		// tmr = (clks * buffer_length / 2) / 1024
	ldrh	r1, [r6, #v_len]		//
	muls	r0, r1				//
	lsrs	r0, #10 + 1			//
	strh	r0, [r6, #v_tmr]		//

	movs	r0, #0
	ldr	r1,=mmsPreviousTimer
	strh	r0, [r1]
	
	ldrh	r0, [r6, #v_len]		// force-fill stream with initial data
	subs	r0, #DELAY_SAMPLES		//
	bl	ForceStreamRequest		//
//	strh	r0, [r6, #v_pos]

	ldr	r0,=StreamCounter		// reset stream counter
	movs	r1, #0
	str	r1, [r0]			//
		
//	bl	mmSuspendIRQ_t			// ***********************************************
	
	ldr	r0, [r6, #v_wave]		// start stream
	
	#ifdef SYS_NDS9
	ldrh	r1, [r6, #v_lenw]		// set wait flag
	lsls	r1, #2				//
	subs	r1, #1				//
	movs	r2, #1				//
	strb	r2, [r0, r1]			//
	push	{r1}				//	
	ldr	r1,=DrainWriteBuffer		// <-okay?
	blx	r1
	#endif
	
	ldrh	r1, [r6, #v_clks]		//
	lsrs	r1, #1				// /2 for sound timer
	ldrh	r2, [r6, #v_len]		//
	ldrb	r3, [r6, #v_format]		//
	bl	mmStreamBegin			//
	
	
	#ifdef SYS_NDS9
	
	ldr	r0, [r6, #v_wave]		// wait until stream begins
	pop	{r1}				//
	adds	r0, r1
	ldr	r1,=WaitUntilZero		//
	blx	r1
	
	#endif
	
	ldrb	r0, [r6, #v_auto]		// start timer
	cmp	r0, #0				//
	bne	1f				//
	movs	r0, #TIMER_MANUAL		//  manual updates
	lsls	r0, #16				//
	b	.manual_updates			//
1:						//
	movs	r0, #TIMER_AUTO			//  auto updates
	lsls	r0, #16				//
	ldrh	r1, [r6, #v_tmr]		//
	negs	r1, r1
	lsls	r1, #16
	lsrs	r1, #16
	orrs	r0, r1				//
						//
.manual_updates:				//
						//
	str	r0, [r5]			//<- write to CNT
	
//	bl	mmRestoreIRQ_t			// ***********************************************
	
	pop	{r4-r6}
	ret3
	
/***********************************************************************
 * mmStreamGetPosition()
 *
 * Get number of samples that have played since start.
 * 32-bit variable overflows every ~36 hours @ 32khz...
 ***********************************************************************/
						.thumb_func
mmStreamGetPosition:
	push	{r4,lr}
	ldr	r4,=mmsData
	ldrb	r0, [r4, #v_active]
	
	cmp	r0, #0			// catch inactive stream
	bne	1f			//
2:	movs	r0, #0			//
_mmsgp_exit:
	pop	{r4}			//
	pop	{r3}			//
	bx	r3			//
1:	
	ldrb	r0, [r4, #v_auto]	// catch auto mode
	cmp	r0, #0			// (only manual mode supported)
	bne	2b			//
	
	//todo: combine this and mmStreamUpdate section into one function
	
	ldr	r0, [r4, #v_hwtimer]	//
	ldrh	r0, [r0]		//
	ldr	r2,=mmsPreviousTimer	//
	ldrh	r1, [r2]		//
	subs	r0, r1			//
	bpl	1f			//
	ldr	r1,=65536		//
	adds	r0, r1			//
1:	
	lsls	r0, #10			// samples = (t * 1024 + r) / clks
	ldr	r1, [r4, #v_remainder]	//
	adds	r0, r1			//
	ldrh	r1, [r4, #v_clks]	//
					//
	swi	SWI_DIVIDE		//
					//
					
	ldr	r1,=StreamCounter	// add sample counter
	ldr	r1, [ r1 ]		//
	adds	r0, r1			//
	b	_mmsgp_exit
	
	
/***********************************************************************
 * mmStreamUpdate()
 *
 * Update stream with new data.
 ***********************************************************************/
						.thumb_func
mmStreamUpdate:

	push	{r4-r6,lr}
	ldr	r4,=mmsData
	
	ldrb	r0, [r4, #v_active]		// catch if stream isn't active
	cmp	r0, #0				//
	bne	1f				//
	pop	{r4-r6}				//
	ret3					//
1:
	
//------------------------------------------------
// determine how many samples to mix
//------------------------------------------------

	ldrb	r0, [r4, #v_auto]		// branch: manual/auto mode
	cmp	r0, #0				//
	bne	.mmsu_auto			//
						//   manual mode: calculate difference
	ldr	r0, [r4, #v_hwtimer]		//
	ldrh	r0, [r0]			//
	ldr	r2,=mmsPreviousTimer		//	
	ldrh	r1, [r2]			//
	strh	r0, [r2]			//
	subs	r0, r1				//
	bpl	.mmsu_manual			//
	ldr	r1,=65536			//
	adds	r0, r1				//
	b	.mmsu_manual			//
.mmsu_auto:					//   auto mode: fixed amount
						//
	ldrh	r0, [r4, #v_tmr]		//
						//
.mmsu_manual:					//
	
	lsls	r0, #10				// samples = (t * 1024 + r) / clks
	ldr	r1, [r4, #v_remainder]		//
	adds	r0, r1				//
	ldrh	r1, [r4, #v_clks]		//
						//
	swi	SWI_DIVIDE			//
						//
						
	movs	r2, #0b11			// clip to multiple of 4
	ands	r2, r0				//
	ldrh	r3, [r4, #v_clks]		//
	muls	r2, r3				//
	adds	r1, r2				//
	
	str	r1, [r4, #v_remainder]		// save remainder

STREAM_FORCE_REQUEST:
	
	lsrs	r0, #2
	lsls	r0, #2
	
	ldr	r1,=StreamCounter
	ldr	r2, [r1]
	adds	r2, r0
	str	r2, [r1]
	
//------------------------------------------------
// request data
//------------------------------------------------
	
	movs	r5, r0
	
.fill_stream:
	movs	r0, r5				// r0 = #samples
	beq	.fill_complete
	ldrh	r1, [r4, #v_len]		// cut r0 to work buffer size
	cmp	r0, r1				//
	ble	1f				//
	movs	r0, r1				//
1:						//
	subs	r5, r0				// subtract from total (SHOULD be 0 unless an underrun occurred)
	
	ldr	r1, [r4, #v_workmem]		// do callback( nsamples, dest, format )
	ldrb	r2, [r4, #v_format]		// 
	ldr	r3, [r4, #v_function]		// 
	
	push	{r0}				// preserve nsamples
	#ifdef SYS_NDS7
	bl	_call_via_r3
	#else
	blx	r3
	#endif
	push	{r0}
	
	ldr	r1,=CopyDataToStream		// copy samples to stream ...
	bl	_call_via_r1			//
	pop	{r0, r1}			// r0 = samples filled, r1 = desired amount
	
	subs	r1, r0				// r1 = unsatisfied samples
	adds	r5, r1
	cmp	r0, #0
	beq	_no_samples_output		// break if 0 samples output
	
	cmp	r5, r0				// total += leftover
	bge	.fill_stream			// loop if remaining >= amount filled
	
_no_samples_output:
	ldrh	r2, [r4, #v_clks]		// add leftover to remaining cycles
	muls	r2, r5				// (samples * clks)
	ldr	r3, [r4, #v_remainder]		//
	adds	r3, r2				//
	str	r3, [r4, #v_remainder]		//
	
.fill_complete:
	
	#ifdef SYS_NDS9
	
	ldr	r1,=mmFlushStream
	bl	_call_via_r1

//	ldr	r1,=DrainWriteBuffer
//	blx	r1
//	bl	DrainWriteBuffer
	
	#endif
	
	#ifdef SYS_NDS7				// return
	pop	{r4-r6}				//
	ret3					//
	#else					//
	pop	{r4-r6, pc}			//
	#endif					//
	
/***********************************************************************
 * ForceStreamRequest( samples )
 *
 * Force a data request
 ***********************************************************************/
						.thumb_func
ForceStreamRequest:
	push	{r4-r6, lr}
	ldr	r4,=mmsData
	
	b	STREAM_FORCE_REQUEST
	
	.arm
	.align 2
	
	
#ifdef SYS_NDS9

/***********************************************************************
 * DrainWriteBuffer()
 *
 * Drain write buffer (important)
 ***********************************************************************/
DrainWriteBuffer:
	mcr	p15, 0, r0, c7, c10, 4	// drain write buffer
	bx	lr
	
/***********************************************************************
 * WaitUntilZero( address )
 *
 * Wait until a byte magically becomes zero.
 ***********************************************************************/
WaitUntilZero:

	bic	r1, r0, #0b11111
	
.wait:
	mcr	p15, 0, r1, c7, c14, 1		// clean and invalidate cache line
	ldrb	r2, [r0]
	cmp	r2, #0
	bne	.wait
	
	bx	lr
	
/***********************************************************************
 * WaitForMemorySignal( address, test )
 *
 * Wait until a byte magically becomes 'test'.
 ***********************************************************************/
WaitForMemorySignal:

	bic	r2, r0, #0b11111
	
.wait2:
	mcr	p15, 0, r2, c7, c14, 1		// clean and invalidate cache line
	ldrb	r3, [r0]
	cmp	r3, r1
	bne	.wait2
	
	bx	lr
	
#endif
	
/***********************************************************************
 * CopyDataToStream( nsamples )
 *
 * Copy/de-interleave data from work buffer into the wave buffer.
 ***********************************************************************/
CopyDataToStream:
	cmp	r0, #0
	bxeq	lr
	
	push	{r4-r9,r10, r11, lr}
	
	ldr	r4,=mmsData			// r4 = vars
	mov	r5, r0				// r5 = total samples
	ldr	r6, [r4, #v_workmem]
	ldrh	r8, [r4, #v_len]
	ldrh	r9, [r4, #v_pos]
	
.copydata:
	
	mov	r1, r9				// r1 = dest position
	
	sub	r2, r8, r9			// r2 = remaining samples until end
	
	mov	r0, r5
	
	cmp	r5, r2				// clip samples to remaining length
	movge	r0, r2				//
	movge	r9, #0				//
	
	addlt	r9, r0				// position += samples
	sub	r5, r0				//
	
	// r0 = #samples
	// r1 = position
	// r6 = wmem position
	
//-------------------------------------------------------------------------------------------------------------------------
// nsamples is a multiple of 4
// adpcm mono        2 bytes/chunk [2samples] [2samples]
// adpcm stereo      4 bytes/chunk ([2left] [2right]) ([2left] [2right])
// 8-bit mono        4 bytes/chunk [sample] [sample] [sample] [sample] ...
// 8-bit stereo      8 bytes/chunk ([left] [right]) ([left] [right]) ([left] [right]) ([left] [right]) ...
// 16-bit mono       8 bytes/chunk [[sample]] [[sample]] [[sample]] [[sample]] ...
// 16-bit stereo    16 bytes/chunk ([[left]] [[right]]) ([[left]] [[right]]) ([[left]] [[right]]) ([[left]] [[right]]) ...
//-------------------------------------------------------------------------------------------------------------------------
	
	ldr	r3, [r4, #v_wave]
	
	ldrb	r7, [r4, #v_format]
	
	cmp	r0, #0
	beq	.cd_next

	cmp	r7, #1			// determine format
	blt	.cd_mono8		//
	beq	.cd_stereo8		//
	cmp	r7, #3			//
	blt	.cd_mono16		//
	beq	.cd_stereo16		//
//	cmp	r7, #5			//
//	beq	.cd_stereo4		//
	// error :)

/********************************************************************
 * 4-bit mono [not used]
 *
 * Simple copy
 ********************************************************************/
 /*
.cd_mono4:
	adds	r3, r1, lsr#1
	
1:	ldrh	r1, [r6], #2
	subs	r0, #4
	strh	r1, [r3], #2
	bne	1b
	
	b	.cd_next
 */
	
/********************************************************************
 * 4-bit stereo [not used]
 *
 * Simple copy
 ********************************************************************/
 /*
.cd_stereo4:
	adds	r3, r1, lsr#1
	ldr	r7,=0xFF00FF
	
	lsrs	r8, #1

1:	ldr	r1, [r6], #4		// r1 = R2L2R1L1
					//      43432121
	subs	r0, #4				
	and	r10, r7, r1, lsr#8	// r10 = --R2--R1
	orrs	r10, r10, lsr#8		// r10 = --xxR2R1
	
	strh	r10, [r3, r8]
	
	and	r10, r7, r1		// r10 = --L2--L1
	orrs	r10, r10, lsr#8		// r10 = --xxL2L1
	
	strh	r10, [r3], #2
	bne	1b
	
	
	lsls	r8, #1
	
	b	.cd_next
 */

/********************************************************************
 * 8-bit mono
 *
 * Simple copy
 ********************************************************************/
.cd_mono8:
	add	r3, r1
	
1:	ldr	r1, [r6], #4
	subs	r0, #4
	str	r1, [r3], #4
	bne	1b
	
	b	.cd_next
	
/********************************************************************
 * 8-bit stereo
 *
 * De-interleaved copy
 ********************************************************************/
.cd_stereo8:
	add	r3, r1
	ldr	r7,=0xFF00FF00
	
1:	ldmia	r6!, {r1, r2}
					// r1/r2 = R4L4R3L3, R2L2R1L1
					
	and	r10, r7, r1, lsl#8	// r10 = L2--L1--
	orr	r10, r10, r10, lsl#8		// r10 = L2L1xx-- 2 samples deinterleaved
	and	r11, r7, r2, lsl#8	// r11 = L4--L3--
	orr	r11, r11, r11, lsl#8		// r11 = L4L3xx-- 2 more samples
	bic	r11, #0xFF00		// r11 = L4L3----
	orr	r12, r11, r10, lsr#16	// r11 = L4L3L2L1 4 samples
	
	and	r10, r7, r1		// r10 = R2--R1--
	orr	r10, r10, r10, lsl#8		// r10 = R2R1xx--
	and	r11, r7, r2		// r11 = R4--R3--
	orr	r11, r11, r11, lsl#8		// r11 = R4R3xx--
	bic	r11, #0xFF00		// r11 = R4R3----
	orr	r11, r11, r10, lsr#16	// r11 = R4R3R2R1
	
	str	r11, [r3, r8]		// write to right output
	str	r12, [r3], #4		// write to left output + increment
	
	subs	r0, #4
	bne	1b
	
	b	.cd_next
	
/********************************************************************
 * 16-bit mono
 *
 * Copy doublewords
 ********************************************************************/
.cd_mono16:
	add	r3, r3, r1, lsl#1
	
1:	ldmia	r6!, {r1, r2}
	subs	r0, #4
	stmia	r3!, {r1, r2}
	bne	1b
	
	b	.cd_next
	
/********************************************************************
 * 16-bit stereo
 *
 * De-interleaved copy
 ********************************************************************/
.cd_stereo16:
	add	r3, r3, r1, lsl#1
	
1:	ldmia	r6!, {r1, r2, r7, r10}	// read 4 samples

					// r10 r7 r2 r1 = R4L4 R3L3 R2L2 R1L1
	mov	r11, r1, lsr#16		// r11 = --R1
	mov	r12, r2, lsr#16		// r12 = --R2
	orr	r11, r11, r12, lsl#16	// r12 = R2R1
	
	str	r11, [r3, r8, lsl#1]	// write to right output
	
	mov	r11, r1, lsl#16		// r11 = L1--
	mov	r12, r2, lsl#16		// r12 = L2--
	orr	r12, r12, r11, lsr#16	// r12 = L2L1
	
	str	r12, [r3], #4		// write to left output & increment
	
	mov	r11, r7, lsr#16		// r11 = --R3
	mov	r12, r10, lsr#16	// r12 = --R4
	orr	r11, r11, r12, lsl#16	// r12 = R4R3
	
	str	r11, [r3, r8, lsl#1]	// write to right output
	
	mov	r11, r7, lsl#16		// r11 = L3--
	mov	r12, r10, lsl#16	// r12 = L4--
	orr	r12, r12, r11, lsr#16	// r12 = L4L3
	
	str	r12, [r3], #4		// write to left output & increment
	
	subs	r0, #4
	bne	1b
	
//	b	.cd_next
	
//----------------------------------------------------------------------
.cd_next:
//----------------------------------------------------------------------

	cmp	r5, #0
	bne	.copydata
	
	strh	r9, [r4, #v_pos]
	
	pop	{r4-r9,r10, r11, lr}
	bx	lr	
	
#ifdef SYS_NDS9
	
/***********************************************************************
 * mmFlushStream()
 *
 * Flush audio stream
 ***********************************************************************/
mmFlushStream:
	ldr	r0,=mmsData
	ldr	r1, [r0, #v_wave]
	ldrh	r2, [r0, #v_lenw]
	bic	r1, #0b11111
	adds	r2, #8
	
.flushstream:
	mcr	p15, 0, r1, c7, c14, 1		// clean and invalidate cache line
	adds	r1, #32
	subs	r2, #8
	bpl	.flushstream
	
	bx	lr
	
#endif
	
	.thumb
	.align 2
	
/***********************************************************************
 * mmStreamClose
 *
 * Close audio stream
 ***********************************************************************/
						.thumb_func
mmStreamClose:

	push	{r4, r5, lr}
	
	ldr	r4,=mmsData			// catch already disabled
	ldrb	r0, [r4, #v_active]		//
	cmp	r0, #0				//
	beq	.mmsc_exit			//
	
	movs	r0, #0				// disable hardware timer
	ldr	r1, [r4, #v_hwtimer]		//
	strh	r0, [r1, #2]			//
	
	ldr	r0,=TM0CNT			// disable irq
	subs	r1, r0				//
	lsrs	r1, #2				//
	movs	r0, #8				//
	lsls	r0, r1				//
	bl	irqDisable			//
	
	nop	// ...?
	nop
	
	ldr	r5, [r4, #v_wave]		// read byte of wavebuffer (for testing)
	ldrb	r5, [r5]			//
	
	movs	r0, #0				// disable system
	strb	r0, [r4, #v_active]		//
	strb	r0, [r4, #v_auto]		//
	bl	mmStreamEnd			//

	#ifdef SYS_NDS9				// ARM9:
	
	ldr	r0, [r4, #v_wave]		// block until arm7 sets 'stop' flag
	movs	r1, r5				//
	adds	r1, #1				//
	lsls	r1, #32-8			//
	lsrs	r1, #32-8			//
	ldr	r2,=WaitForMemorySignal		//
	blx	r2
	
	ldr	r0, [r4, #v_workmem]		// free malloc'd memory
	bl	free				//
	ldr	r0, [r4, #v_wave]		//
	bl	free				//
	#endif					//
	
.mmsc_exit:
	pop	{r4, r5}
	pop	{r3}
	bx	r3
	
/***********************************************************************
 *
 * ARM7 Only
 *
 ***********************************************************************/

//----------------------------------------------------------------------
#ifdef SYS_NDS7
//----------------------------------------------------------------------

/***********************************************************************
 * mmStreamBegin( wavebuffer, clks, len, format )
 *
 * Begin audio stream.
 ***********************************************************************/
						.thumb_func
mmStreamBegin:
	
	push	{r4-r7, lr}
	ldr	r4,=mmsData
	
	str	r0, [r4, #v_wave]
	strh	r1, [r4, #v_clks]
	strh	r2, [r4, #v_len]
	strb	r3, [r4, #v_format]

//----------------------------------------------------------------------
// lock channels
//----------------------------------------------------------------------

	movs	r0, #0b00010000			// channel 4 if stereo isn't set
	lsrs	r3, #1
	bcc	1f				
	movs	r0, #0b00110000			// channels 4&5 if stereo is set
1:
	bl	mmLockChannels
	
//----------------------------------------------------------------------
// prepare left/center channel
//----------------------------------------------------------------------
	
	ldr	r0,=SOUND4CNT
	movs	r1, #0
	str	r1, [r0]			// clear cnt
	str	r1, [r0, #8]			// clear tmr/pnt
	
	ldr	r5, [r4, #v_wave]		// copy src
	str	r5, [r0, #4]			//
	ldrh	r6, [r4, #v_clks]		// copy tmr
	negs	r6, r6
	strh	r6, [r0, #8]			//
	
	ldrb	r1, [r4, #v_format]		// set len
	lsrs	r2, r1, #1			//
	cmp	r2, #1				//
	ldrh	r7, [r4, #v_len]		//
	beq	.mmbs_16bit			//
	blt	.mmbs_8bit			//
.mmbs_4bit:					//
	lsrs	r7, #3				//
	b	1f				//
.mmbs_8bit:					//
	lsrs	r7, #2				//
	b	1f				//
.mmbs_16bit:					//
	lsrs	r7, #1				//
1:	str	r7, [r0, #12]			//

	strh	r7, [r4, #v_lenw]		// save word-length

	ldr	r2,=127|(64<<16)		// center panning
	str	r2, [r0, #0]			//
	
//----------------------------------------------------------------------
// prepare right channel
//----------------------------------------------------------------------

	lsrs	r1, #1				//
	bcc	.mmbs_mono			// stereo mode:
	
	lsls	r1, r7, #1
	strh	r1, [r4, #v_lenw]
	
	movs	r2, #127			// left panning instead
	str	r2, [r0, #0]			//
	
	movs	r1, #0				// setup right channel
	str	r1, [r0, #16]			//
	str	r1, [r0, #16+8]			//
	lsls	r1, r7, #2			// wave += len (for right buffer)
	adds	r5, r1				// 
	
	str	r5, [r0, #16+4]			//
	strh	r6, [r0, #16+8]			//
	str	r7, [r0, #16+12]		//
	
	ldr	r2,=127|(127<<16)
	str	r2, [r0, #16]
	
.mmbs_mono:

//----------------------------------------------------------------------
// start channels
//----------------------------------------------------------------------

	bl	mmSuspendIRQ_t
	
	ldr	r0,=SOUND4CNT
	
	movs	r1, #0b10001000			// r1 = cnt = enable + format + loop
	ldrb	r2, [r4, #v_format]		//
	lsrs	r3, r2, #1			//
	lsls	r3, #5				//
	orrs	r1, r3				//
	
	strb	r1, [r0, #3]			// start left/mono channel
	
	lsrs	r2, #1
	bcc	.mmbs_mono2
	
	strb	r1, [r0, #16+3]			// start right channel
	
.mmbs_mono2:
	
	ldr	r0, [r4, #v_wave]		// send 'start' signal
	ldrh	r1, [r4, #v_lenw]		//
	lsls	r1, #2
	subs	r1, #1
	movs	r2, #0				//
	strb	r2, [r0, r1]			//

	bl	mmRestoreIRQ_t			// restore irq..

	pop	{r4-r7}
	pop	{r3}
	bx	r3

/***********************************************************************
 * mmStreamEnd()
 *
 * End audio stream
 ***********************************************************************/
						.thumb_func
mmStreamEnd:

	push	{r4-r6, lr}
	
	bl	mmSuspendIRQ_t
	
	ldr	r1,=SOUND4CNT
	movs	r2, #0
	ldr	r6,=mmsData
	ldrb	r0, [r6, #v_format]
	ldr	r6, [r6, #v_wave]		// <-- for stop signal!!
	lsrs	r0, #1
	bcc	1f

//------------------------------------------------
// stereo mode
//------------------------------------------------
	
	str	r2, [r1, #16]			// stop right channel

//------------------------------------------------
// mono mode
//------------------------------------------------
1:
	str	r2, [r1, #0]			// stop left channel

//------------------------------------------------
// restore channels
//------------------------------------------------
	movs	r0, #0b00110000			// 4&5
	bl	mmUnlockChannels
	
	ldrb	r0, [r6]			// set stop signal
	adds	r0, #1				//
	strb	r0, [r6]			//
	
	bl	mmRestoreIRQ_t
	
	pop	{r4-r6}
	pop	{r3}
	bx	r3

//----------------------------------------------------------------------
#endif
//----------------------------------------------------------------------

.pool
