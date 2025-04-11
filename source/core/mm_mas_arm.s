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
 *                    Module Processing (ARM segment)                       *
 *                                                                          *
 ****************************************************************************/

@=============================================================
@                         DEFINITIONS
@=============================================================

#include "mp_format_mas.inc"
#include "mp_mas_structs.inc"
#include "mp_defs.inc"
#include "mp_macros.inc"

#ifdef SYS_GBA
#include "mp_mixer_gba.inc"
#include "swi_gba.inc"
#endif

#ifdef SYS_NDS
#include "mp_mixer_ds.inc"
#include "swi_nds.inc"
#endif

__SECTION_IWRAM

.arm
.syntax unified
.align 2

.macro get_channel branch
	ldrb	r0, [r7, #MCH_ALLOC]	// get channel
	cmp	r0, #255		//
	bge	\branch		// no channel!
	ldr	r6,=mm_achannels	//
	ldr	r6, [r6]		//
	mov	r1, #MCA_SIZE		//
	mla	r6, r0, r1, r6		//
.endm

.global mmUpdateChannel_T0, mmUpdateChannel_TN
@***************************************************************************
mmUpdateChannel_T0:			// for tick 0
@***************************************************************************
	push	{lr}
	
	ldrb	r5, [r7, #MCH_FLAGS]	// read channel flags
	tst	r5, #MF_START		// test start flag
	beq	dont_start_channel	// skip section if cleared
	
	tst	r5, #MF_HASFX		// test effect flag
	beq	no_channel_effect
	ldrh	r0, [r7, #MCH_EFFECT]	// read effect
	and	r1, r0, #0xFF
//	cmp	r1, #19			// test for 'SDx' (note delay)
//	lsreq	r2, r0, #12
//	cmpeq	r2, #0xD
//	beq	dont_start_channel	// dont start channel if delayed
	
					// test for glissando
	tst	r5, #MF_NEWINSTR
	bne	start_channel		// always start channel if it's a new instrument
	cmp	r1, #0x7		// test if effect is Gxx
	beq	glissando_affected
	
no_channel_effect:
	tst	r5, #MF_NEWINSTR	// copeee
	bne	start_channel		// always start channel if it's a new instrument
	
	tst	r5, #MF_HASVCMD		// test for volume commmand
	beq	start_channel		// none = start channel
	ldrb	r0, [r8, #MPL_FLAGS]	// read mod flags
	movs	r0, r0, lsr#C_FLAGS_XS	// test for XM mode
	ldrb	r0, [r7, #MCH_VOLCMD]	// read volume command
	bcs	xm_vcmd			// branch
it_vcmd:				// using IT effects:
	cmp	r0, #193		// glissando is 193..202
	blt	start_channel
	cmp	r0, #202
	bgt	start_channel
	b	glissando_affected
xm_vcmd:				// using XM effects:
	cmp	r0, #0xF0		// glissando is Fx
	bge	glissando_affected
	b	start_channel
no_channel_vcmd:

glissando_affected:
	
	get_channel start_channel
/*	ldrb	r0, [r7, #MCH_ALLOC]	// if channel is invalid
	cmp	r0, #255		// then skip this
	bge	start_channel
	
	ldr	r6,=mm_achannels	// get channel
	ldr	r6, [r6]		//
	mov	r1, #MCA_SIZE		//
	mla	r6, r0, r1, r6		//
*/	
	bl	mmChannelStartACHN	// start achn
	bic	r5, #MF_START		// clear start flag
	strb	r5, [r7, #MCH_FLAGS]
	b	dont_start_channel
	
start_channel:				// ok start channel...
	
	ldr	r2,=mpp_Channel_NewNote
	mov	lr, pc
	bx	r2
	get_channel	mmUpdateChannel_TN_
	
//	cmp	r6, #0
//	beq	mmUpdateChannel_TN_
	
	bl	mmChannelStartACHN	// start achn
//----------------------------------------------
	// r2 = note, calculate period
	ldrb	r0, [r6, #MCA_SAMPLE]
	subs	r0, #1
	bcc	no_sample_to_make_period
	
	ldrb	r1, [r8, #MPL_FLAGS]
	lsrs	r1, #C_FLAGS_SS
	bcs	linear_periods
	
	ldr	r3, [r8, #MPL_SONGADR]	// read song addr
	ldr	r1, [r8, #MPL_SAMPTABLE]// get sample table
	ldr	r0, [r1, r0, lsl#2]	// get sample address
	add	r0, r3			// add module base
	
	ldrh	r1, [r0, #C_MASS_FREQ]	// read tuning freq
	lsl	r1, #2			// shift ...
//	mov	r0, r2			// move this.
	bl	get_amiga_period
	b	got_period
linear_periods:
	LDR	R1,=IT_PitchTable
	LDR	R0, [R1, R2, LSL#2]
got_period:
//--------------------------------
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	str	r0, [r7, #MCH_PERIOD]	// save period
	ldrb	r0, [r6, #MCA_FLAGS]	// set start flag
	orr	r0, #MCAF_START
	strb	r0, [r6, #MCA_FLAGS]
no_sample_to_make_period:
//----------------------------------------------------
	
	b	channel_started
	
dont_start_channel:
	
	get_channel	mmUpdateChannel_TN___
	
channel_started:
	tst	r5, #MF_DVOL
	beq	dvol_skip
	ldrb	r0, [r7, #MCH_INST]
	subs	r0, #1
	bcc	dvol_no_instrument
	
	ldr	r2, [r8, #MPL_SONGADR]	// get instrument pointer
	ldr	r1, [r8, #MPL_INSTTABLE]
	ldr	r0, [r1, r0, lsl#2]
	add	r0, r2
	
	ldr	r1, [r0, #C_MASI_NNA]	// read nna,envflags,pan
	ldrb	r2, [r7, #MCH_BFLAGS]	// read bflags
	bic	r2, #0b11000000		// clear old nna
	orr	r2, r2, r1, lsl#6		// set new nna
	strb	r2, [r7, #MCH_BFLAGS]
	
	ldrb	r2, [r6, #MCA_FLAGS]	// read achn flags
	bic	r2, #MCAF_VOLENV
	tst	r1, #ENVFLAG_A<<8
	orrne	r2, #MCAF_VOLENV
	strb	r2, [r6, #MCA_FLAGS]
	
	movs	r1, r1, lsl#8+1		// shift out panning MSB
	movcs	r1, r1, lsr#24		// if set, use new panning value
	strbcs	r1, [r7, #MCH_PANNING]
dvol_no_instrument:
	
	ldrb	r0, [r6, #MCA_SAMPLE]	// read sample#
	subs	r0, #1
	bcc	dvol_no_sample		// exit if invalid
	
	ldr	r2, [r8, #MPL_SONGADR]	// get sample address
	ldr	r1, [r8, #MPL_SAMPTABLE]
	ldr	r0, [r1, r0, lsl#2]
	//add	r0, r2
	
	ldrh	r1, [r0, r2]
	//ldrh	r1, [r0, #C_MASS_DV]	// read dvol & pan
	strb	r1, [r7, #MCH_VOLUME]	// copy volume
	movs	r1, r1, lsl#24-7
	mov	r1, r1, lsr#24
	strbcs	r1, [r7, #MCH_PANNING]
dvol_skip:
dvol_no_sample:

	tst	r5, #MF_START|MF_DVOL
	beq	dont_reset_volume
	
	ldrb	r0, [r8, #MPL_FLAGS]
	tst	r0, #C_FLAGS_X
	beq	1f
	tst	r5, #MF_DVOL
	beq	dont_reset_volume	// xm stuff
1:
reset_volume:
	mov	r0, #(1<<10)		// [1]
	mov	r1, #0			// [1]
	mov	r2, #0			// [1]
	add	r12, r6, #MCA_FADE	// [1]
	stmia	r12, {r0-r2}		// [19]
					// [23]
	strh	r1, [r6, #MCA_ENVN_VOL]
	strb	r1, [r6, #MCA_ENVN_PIC]
	
	// the last bit of code sets fade to (1<<10)
	// and clears ENVC_VOL,ENVC_PAN,ENVC_PIC,
	// AVIB_DEP, and AVIB_POS
	// second bit clears ENVN vars
	
	strb	r1, [r7, #MCH_FXMEM]	// clear fx memory
	
	ldrb	r1, [r6, #MCA_FLAGS]	// set keyon
	orr	r1, #MCAF_KEYON		// and clear envend+fade
	bic	r1, #MCAF_ENVEND | MCAF_FADE
	strb	r1, [r6, #MCA_FLAGS]
	
dont_reset_volume:

	tst	r5, #MF_NOTEOFF		// test noteoff bit
	beq	skip_noteoff
	
	ldrb	r1, [r6, #MCA_FLAGS]	// read flags
	bic	r1, #MCAF_KEYON		// clear key-on
	ldrb	r0, [r8, #MPL_FLAGS]	// read mod flags
	tst	r0, #C_FLAGS_X		// test xm mode
	orrne	r1, #MCAF_FADE		// XM starts fade immediately on note-off
	strb	r1, [r6, #MCA_FLAGS]
skip_noteoff:

	tst	r5, #MF_NOTECUT		// test notecut bit
	movne	r0, #0			// clear volume
	strbne	r0, [r7, #MCH_VOLUME]	// on note-cut
	
	bic	r5, #MF_START		// clear start flag
	strb	r5, [r7, #MCH_FLAGS]	// save flags
	b	mmUpdateChannel_TN_
@************************************************************
mmUpdateChannel_TN:			// for other ticks
@************************************************************
	push	{lr}
	
mmUpdateChannel_TN_:
	ldrb	r0, [r7, #MCH_ALLOC]	// get channel
	cmp	r0, #255		//
	bge	mmUpdateChannel_TN___	// no channel!
	ldr	r6,=mm_achannels	//
	ldr	r6, [r6]		//
	mov	r1, #MCA_SIZE		//
	mla	r6, r0, r1, r6		//
	
mmUpdateChannel_TN___:
	
	movge	r6, #0

	ldr	r5, [r7, #MCH_PERIOD]		// r5 will be used to hold the period
	ldr	r1,=mpp_vars
	mov	r0, #0
	strb	r0, [r1, #MPV_SAMPOFF]		// clear variables
	strb	r0, [r1, #MPV_VOLPLUS]
	strb	r0, [r1, #MPV_PANPLUS]
	strb	r0, [r1, #MPV_NOTEDELAY]
	
//---------------------------------------------
// Update Volume Commands
//---------------------------------------------

	ldrb	r0, [r7, #MCH_FLAGS]
	tst	r0, #MF_HASVCMD
	ldrne	r1,=mpp_Process_VolumeCommand
	movne	lr, pc
	bxne	r1
	//blne	mpp_Process_VolumeCommand
	
//---------------------------------------------
// Update Effects
//---------------------------------------------	
	
	ldrb	r0, [r7, #MCH_FLAGS]
	tst	r0, #MF_HASFX
	ldrne	r1,=mpp_Process_Effect
	movne	lr, pc
	bxne	r1
	//blne	mpp_Process_Effect
	
//---------------------------------------------

	cmp	r6, #0
	beq	no_achn
	ldrh	r0, [r7, #MCH_VOLUME]	// read volume & cvolume
	and	r1, r0, #255		// mask volume
	mov	r0, r0, lsr#8		// mask cvolume
	mul	r0, r1, r0		// multiply together
	mov	r0, r0, lsr#5		// shift
	strb	r0, [r6, #MCA_VOLUME]	// save in achn volume
	
	ldr	r1,=mpp_vars		// 
	ldrsb	r2, [r1, #MPV_VOLPLUS]	// read volume addition
	adds	r0, r0, r2, lsl#3		// add to volume
	movmi	r0, #0
	cmp	r0, #129
	movcs	r0, #128
	strb	r0, [r1, #MPV_AFVOL]	// store in immediate vol
	
	ldrb	r0, [r1, #MPV_NOTEDELAY]// read note delay
	cmp	r0, #0			// dont update if nonzero
	beq	channel_update_achn
	
	ldrb	r0, [r6, #MCA_FLAGS]
	orr	r0, #MCAF_UPDATED
	strb	r0, [r6, #MCA_FLAGS]
	b	no_achn
	
channel_update_achn:

	ldrb	r0, [r7, #MCH_PANNING]	// copy panning
	strb	r0, [r6, #MCA_PANNING]
	ldr	r0, [r7, #MCH_PERIOD]	// copy period
	str	r0, [r6, #MCA_PERIOD]
	mov	r0, #0
	strh	r0, [r1, #MPV_PANPLUS]	// WHAT IS THIS??? "@ <---- RESERVED FOR LATER USE, CLEAR TO ZERO TEMPORARILY"
	
	ldrb	r0, [r6, #MCA_FLAGS]
	orr	r0, #MCAF_UPDATED
	strb	r0, [r6, #MCA_FLAGS]
	
	push	{r4}
	ldrb	r4, [r7, #MCH_ALLOC]
	ldr	r1,=mpp_Update_ACHN_notest
	mov	lr, pc
	bx	r1
	
	pop	{r4}
no_achn:
	pop	{lr}
	bx	lr

@***********************************************************
mmChannelStartACHN:		// returns r2=note
@***********************************************************
	ldrb	r2, [r7, #MCH_BFLAGS+1]		// clear tremor/cutvol
	bic	r2, #0b110
	strb	r2, [r7, #MCH_BFLAGS+1]
	
	cmp	r6, #0				// achn==0?
	beq	1f				// then skip this part
	
	mov	r0, #ACHN_FOREGROUND		// set foreground type
	strb	r0, [r6, #MCA_TYPE]
	
	ldrb	r0, [r6, #MCA_FLAGS]		// read flags
	bic	r0, #0b11000000			// clear SUB, EFFECT
	ldr	r1,=mpp_clayer			// get layer
	ldrb	r1, [r1]
	orr	r0, r0, r1, lsl#6			// orr into flags
	orr	r0, r10, r0, lsl#8		// orr PARENT
	strh	r0, [r6, #MCA_PARENT]		// store parent/flags
	ldrb	r0, [r7, #MCH_INST]		// copy instrument
	strb	r0, [r6, #MCA_INST]
1:	ldrbeq	r0, [r7, #MCH_INST]
	subs	r0, #1
	bcc	invalid_instrument
	
	ldr	r2, [r8, #MPL_SONGADR]		// get instrument pointer
	ldr	r1, [r8, #MPL_INSTTABLE]
	ldr	r0, [r1, r0, lsl#2]
	add	r0, r2
	
	ldrb	r2, [r7, #MCH_PNOTE]		// get pattern note
	
	ldrh	r1, [r0, #C_MASI_MAP]		// read notemap offset
	tst	r1, #0x8000			// test MSB
	beq	full_notemap			// if set: notemap doesnt exist!
						// use single entry
	cmp	r6, #0				// if channel is valid
	strbne	r1, [r6, #MCA_SAMPLE]		//   write sample value
	strb	r2, [r7, #MCH_NOTE]		// write note value (without notemap, all entries == PNOTE)
	bx	lr				// return
	
full_notemap:
	
	add	r0, r0, r2, lsl#1			// add note offset
	ldrh	r2, [r0, r1]			// read notemap entry [instr+note*2+notemap_offset]
	strb	r2, [r7, #MCH_NOTE]		// write note value
	cmp	r6, #0				// if channel is valid
	mov	r0, r2, lsr#8			//   write sample value
	strbne	r0, [r6, #MCA_SAMPLE]		//   ..
	and	r2, #255
invalid_instrument:
	bx	lr				// return
	
/**********
	
	cmp	r6, #0				// read notemap [sample]
	ldrneb	r2, [r0, #C_MASI_MAP+1]
	strneb	r2, [r6, #MCA_SAMPLE]
	
//invalid_instrument:				// BUG???
	ldrb	r2, [r0, #C_MASI_MAP]		// read notemap [note]
	strb	r2, [r7, #MCH_NOTE]
invalid_instrument:
	
	bx	lr
**************/
	
@.global mmGetPeriod
@***********************************************************
@mmGetPeriod:
@***********************************************************

get_amiga_period:
	adr	r3, note_table_mod	// get octave/mod from note
	ldrb	r0, [r3, r2]
	sub	r3, #3*10
	ldrb	r2, [r3, r2, lsr#2]
	
	// r0 = (note mod 12) << 1
	// r1 = tuning
	// r2 = (note / 12)
	
	ldr	r3,=ST3_FREQTABLE	// read st3 table entry
	ldrh	r0, [r3, r0]
	
	mov	r3, #0x00AB0		// r3 = 133808
	orr	r3, #0x20000
	mul	r0, r3, r0		// multiply by magic number...
	mov	r0, r0, lsr r2		// shift by octave
	cmp	r1, #0
	beq	1f
	swi	SWI_DIVIDE<<16		// divide value / tuning
1:	bx	lr
	
.global note_table_oct	// remove this, make this static
.global	note_table_mod	// remove this, make this staic
note_table_oct:
.byte	0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5, 6,6,6 ,7,7,7,8,8,8,9,9,9
note_table_mod:
.byte	0,2,4,6,8,10,12,14,16,18,20,22,0,2,4,6,8,10,12,14,16,18,20,22,0,2,4,6,8,10,12,14,16,18,20,22,0,2,4,6,8,10,12,14,16,18,20,22,0,2,4,6,8,10,12,14,16,18,20,22,0,2,4,6,8,10,12,14,16,18,20,22,0,2,4,6,8,10,12,14,16,18,20,22,0,2,4,6,8,10,12,14,16,18,20,22,0,2,4,6,8,10,12,14,16,18,20,22,0
.align 2

.pool
