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

#include "mp_defs.inc"
#include "mp_macros.inc"

.equ	FIFO_MAXMOD,	3

// offsets of structure "mm_ds9_system"
.equ MMDS9S_MOD_COUNT,		0
.equ MMDS9S_SAMP_COUNT,		MMDS9S_MOD_COUNT	+ 4
.equ MMDS9S_MEM_BANK,		MMDS9S_SAMP_COUNT	+ 4
.equ MMDS9S_FIFO_CHANNEL,	MMDS9S_MEM_BANK		+ 4

//-----------------------------------------------------------------------------
	.bss
	.syntax unified
	.align 2
//-----------------------------------------------------------------------------

/******************************************************************************
 * mmcbMemory : Word
 *
 * Function pointer to soundbank operation callback
 ******************************************************************************/
							.global mmcbMemory
mmcbMemory:	.space 4

/******************************************************************************
 * mmModuleCount : Word
 *
 * Number of modules in sound bank
 ******************************************************************************/
							.global mmModuleCount
mmModuleCount:	.space 4

/******************************************************************************
 * mmSampleCount : Word
 *
 * Number of samples in sound bank
 ******************************************************************************/
							.global mmSampleCount
mmSampleCount:	.space 4

/******************************************************************************
 * mmModuleBank : Word
 *
 * Address of module bank
 ******************************************************************************/
							.global mmMemoryBank
							.global mmModuleBank
mmMemoryBank:
mmModuleBank:	.space 4

/******************************************************************************
 * mmSampleBank : Word
 *
 * Address of sample bank
 ******************************************************************************/
							.global mmSampleBank
mmSampleBank:	.space 4

/******************************************************************************
 * mmCallback : Word
 *
 * Pointer to event handler
 ******************************************************************************/
							.global mmCallback
mmCallback: 	.space 4

/******************************************************************************
 * mmActiveStatus : Byte
 *
 * Record of playing status.
 ******************************************************************************/
							.global mmActiveStatus
mmActiveStatus:	.space 1

//-----------------------------------------------------------------------------
	.text
	.thumb
	.align 2
//-----------------------------------------------------------------------------

/******************************************************************************
 * mmActive
 *
 * Returns nonzero if module is playing
 ******************************************************************************/
						.global mmActive
						.thumb_func
mmActive:
	
	ldr	r0,=mmActiveStatus
	ldrb	r0, [r0]
	bx	lr

/******************************************************************************
 * mmSetEventHandler
 *
 * Set function for handling playback events
 ******************************************************************************/
						.global mmSetEventHandler
						.thumb_func
mmSetEventHandler:

	ldr	r1,=mmCallback
	str	r0, [r1]
	bx	lr

/******************************************************************************
 * mmInit( system )
 *
 * Initialize Maxmod (manual settings)
 ******************************************************************************/
						.global mmInit
						.thumb_func
mmInit:
	
	push	{r4-r7, lr}			// preserve registers
	movs	r7, r0				// r7 = system

	ldmia	r0!, {r1-r3}			// r1,r2,r3,r4 = mod_count, samp_count, mod_bank, samp_bank
	lsls	r4, r1, #2			//
	adds	r4, r3				//
	ldr	r5,=mmModuleCount		// write to local memory
	stmia	r5!, {r1-r4}			//
	
	adds	r1, r2				// clear the memory bank to zero
	beq	2f
	movs	r0, #0				//
1:	stmia	r3!, {r0}			//
	subs	r1, #1				//
	bne	1b				//
2:
	
	ldr	r0, [r7, #MMDS9S_FIFO_CHANNEL]	// setup communications
	bl	mmSetupComms			//

	ldr	r0, [r7, #MMDS9S_MOD_COUNT]	// send soundbank info to ARM7
	ldr	r1, [r7, #MMDS9S_MEM_BANK]
	bl	mmSendBank

	pop	{r4, r5,r6,r7, pc}		// pop regs and return

/******************************************************************************
 * mmInitDefault( soundbank filename )
 *
 * Initialize Maxmod with default setup
 ******************************************************************************/
						.global mmInitDefault
						.thumb_func
mmInitDefault:

	push	{r0, r4, r5, lr}

	ldr	r2,=fopen			// open soundbank
	ldr	r1,=mmstr_rbHAP			// "rb"
	blx	r2				//
	movs	r4, r0				// preserve FILE handle

	ldr	r5,=fread			// read first word (push on stack)
	sub	sp, #4				//
	mov	r0, sp				//
	movs	r1, #4				//
	movs	r2, #1				//
	movs	r3, r4				//
	blx	r5				//

	ldr	r1,=fclose			// close soundbank
	movs	r0, r4				//
	blx	r1				//

	pop	{r0}				// r0 = #samples | (#modules << 16)
	
	lsls	r1, r0, #16			// r1 = #samples (mask low hword)
	lsrs	r1, #16				//
	lsrs	r0, #16				// r0 = #modules (mask high hword)
	movs	r3, #FIFO_MAXMOD		// r3 = standard MAXMOD fifo channel
	push	{r0, r1, r2, r3}		// push onto stack, r2 = trash/spacer
	
	adds	r0, r1				// allocate memory ((mod+samp)*4) for the memory bank
	lsls	r0, #2				// 
	ldr	r1,=malloc			//
	blx	r1				//
	str	r0, [sp, #MMDS9S_MEM_BANK]	//
	
	mov	r0, sp				// pass struct to mmInit
	bl	mmInit				//
	
	add	sp, #16				// clear data
	pop	{r0}				// r0 = soundbank filename
	bl	mmSoundBankInFiles		// setup soundbank handler
	
	pop	{r4, r5, pc}			// restore regs and return
	
.align 2
mmstr_rbHAP:
	.byte	'r', 'b', 0
	
.align 2

/******************************************************************************
 * mmInitDefaultMem( address of soundbank )
 *
 * Initialize maxmod with default setup 
 * (when the entire soundbank is loaded into memory)
 ******************************************************************************/
						.global mmInitDefaultMem
						.thumb_func
mmInitDefaultMem:
	push	{r4,lr}
	
	ldrh	r2, [r0, #0]			// r2 = #samples
	ldrh	r1, [r0, #2]			// r1 = #modules
	movs	r4, #FIFO_MAXMOD		// r3 = standard maxmod channel
	
	push	{r0,r1,r2,r3,r4}		// push data onto stack
	
	adds	r0, r1, r2			// allocate memory for memory bank
	lsls	r0, #2				// size = (nsamples+nmodules) * 4
	ldr	r3,=malloc			//
	blx	r3				//
	str	r0, [sp, #MMDS9S_MEM_BANK+4]	//

	add	r0, sp, #4			// pass system struct to mmInit
	bl	mmInit				//
	
	pop	{r0}				// setup soundbank handler
	bl	mmSoundBankInMemory		// 
	
	pop	{r0-r3, r4, pc}			// trash registers and return

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
	ands	r1, r0, #0x80
	orrs	r0, #0x80
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
