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
 *                       Communication System (ARM9)                        *
 *                                                                          *
 ****************************************************************************/

#include "mp_macros.inc"
#include "mp_defs.inc"

/*****************************************************************************************************************************

[value] represents a 1 byte value
[[value]] represents a 2 byte value
[[[value]]] represents a 3 byte value
[[[[value]]]] represents a 4 byte value
... represents data with a variable length

message table:
-----------------------------------------------------------------------------------------------------------------
message			size	parameters			desc
-----------------------------------------------------------------------------------------------------------------
0: BANK			6	[[#songs]] [[[mm_bank]]]	get sound bank
1: SELCHAN		4	[[bitmask]] [cmd]		select channels
2: START		4	[[id]] [mode] 			start module
3: PAUSE		1	---				pause module
4: RESUME		1	---				resume module
5: STOP			1	---				stop module
6: POSITION		2	[position]			set playback position
7: STARTSUB		3	[[id]]				start submodule
8: MASTERVOL		3	[[volume]]			set master volume
9: MASTERVOLSUB		3	[[volume]]			set master volume for sub module
A: MASTERTEMPO		3	[[tempo]]			set master tempo
B: MASTERPITCH		3	[[pitch]]			set master pitch
C: MASTEREFFECTVOL	3	[[volume]]			set master effect volume, bbaa= volume
D: OPENSTREAM		10	[[[[wave]]]] [[clks]] [[len]] [format]    open audio stream
E: CLOSESTREAM		1	---				close audio stream
F: SELECTMODE		2	[mode]				select audio mode

10: EFFECT		5	[[id]] [[handle]]		play effect, default params
11: EFFECTVOL		4	[[handle]] [volume]		set effect volume
12: EFFECTPAN		4	[[handle]] [panning]		set effect panning
13: EFFECTRATE		5	[[handle]] [[rate]]		set effect pitch
14: EFFECTMULRATE	5	[[handle]] [[factor]]		scale effect pitch
15: EFFECTOPT		4	[[handle]] [options]		set effect options
16: EFFECTEX		11	[[[[sample/id]]]] [[rate]] [[handle]] [vol] [pan] play effect, full params
17: ---			-	---				---

18: REVERBENABLE	1	---				enable reverb
19: REVERBDISABLE	1	---				disable reverb
1A: REVERBCFG		3..14	[[flags]] : [[[[memory]]]] [[delay]] [[rate]] [[feedback]] [panning]
1B: REVERBSTART		1	[channels]			start reverb
1C: REVERBSTOP		1	[channels]			stop reverb

1D: EFFECTCANCELALL	1	---				cancel all effects

1E->3F: Reserved
******************************************************************************************************************************/

/***********************************************************************
 * Value32 format
 *
 * [cc] [mm] [bb] [aa]
 *
 * [mm] : ppmmmmmm, p = parameters, m = message type
 * [aa] : argument1, use if p >= 1
 * [bb] : argument2, use if p >= 2
 * [cc] : argument3, use if p == 3
 ***********************************************************************/
 
/***********************************************************************
 * Datamsg format
 *
 * First byte: Length of data
 * Following bytes: data
 ***********************************************************************/
 
.equ	PARAMS_0,		0x00
.equ	PARAMS_1,		0x40
.equ	PARAMS_2,		0x80
.equ	PARAMS_3,		0xC0

.equ	MSG_BANK,		0x00		// string
.equ	MSG_SELCHAN,		0x01 + PARAMS_3
.equ	MSG_START,		0x02 + PARAMS_3
.equ	MSG_PAUSE,		0x03 + PARAMS_0
.equ	MSG_RESUME,		0x04 + PARAMS_0
.equ	MSG_STOP,		0x05 + PARAMS_0
.equ	MSG_POSITION,		0x06 + PARAMS_1
.equ	MSG_STARTSUB,		0x07 + PARAMS_2
.equ	MSG_MASTERVOL,		0x08 + PARAMS_2
.equ	MSG_MASTERVOLSUB,	0x09 + PARAMS_2
.equ	MSG_MASTERTEMPO,	0x0A + PARAMS_2
.equ	MSG_MASTERPITCH,	0x0B + PARAMS_2
.equ	MSG_MASTEREFFECTVOL,	0x0C + PARAMS_2
.equ	MSG_OPENSTREAM,		0x0D		// string
.equ	MSG_CLOSESTREAM,	0x0E + PARAMS_0
.equ	MSG_SELECTMODE,		0x0F + PARAMS_1

.equ	MSG_EFFECT,		0x10		// string
.equ	MSG_EFFECTVOL,		0x11 + PARAMS_3
.equ	MSG_EFFECTPAN,		0x12 + PARAMS_3
.equ	MSG_EFFECTRATE,		0x13
.equ	MSG_EFFECTMULRATE,	0x14
.equ	MSG_EFFECTOPT,		0x15 + PARAMS_3
.equ	MSG_EFFECTEX,		0x16		// string
.equ	MSG_EFFECTEXT,		0x17		// string

.equ	MSG_REVERBENABLE,	0x18 + PARAMS_0
.equ	MSG_REVERBDISABLE,	0x19 + PARAMS_0
.equ	MSG_REVERBCFG,		0x1A		// STRING
.equ	MSG_REVERBSTART,	0x1B + PARAMS_1
.equ	MSG_REVERBSTOP,		0x1C + PARAMS_1

.equ	MSG_EFFECTCANCELALL,	0x1D + PARAMS_0

.equ	EFFECT_CHANNELS,	16

.struct 0					// mm_sound_effect
MM_SFX_SRC:	.space 4			// hword: source
MM_SFX_RATE:	.space 2			// hword: rate
MM_SFX_HANDLE:	.space 2			// byte:  handle
MM_SFX_VOLUME:	.space 2			// hword: volume
MM_SFX_PANNING:	.space 1			// byte:  panning
MM_SFX_SIZE:

//----------------------------------------------------------------------
	.bss
	.syntax unified
	.align 2
//----------------------------------------------------------------------

/***********************************************************************
 * mmFifoChannel
 *
 * Fifo channel to use for communications
 ***********************************************************************/
mmFifoChannel:			.space 4

sfx_bitmask:			.space 4

sfx_instances:			.space EFFECT_CHANNELS

//----------------------------------------------------------------------
	.text
	.thumb
	.align 2
//----------------------------------------------------------------------

/***********************************************************************
 * mmSetupComms( channel )
 *
 * ARM9 Communication Setup
 ***********************************************************************/
						.global mmSetupComms
						.thumb_func
mmSetupComms:
	push	{lr}
	ldr	r1,=mmFifoChannel
	str	r0, [r1]
	
	ldr	r1,=mmReceiveMessage
	movs	r2, #0
	bl	fifoSetValue32Handler
	
	pop	{pc}

/***********************************************************************
 * SendSimple{Ext}( data, ext, head )
 *
 * Send data via Value32
 ***********************************************************************/
						.thumb_func
SendSimpleExt:
	lsls	r1, #24				// assemble data
	orrs	r0, r1				//
.thumb_func
SendSimple:
	lsls	r2, #16				//
	orrs	r0, r2				//
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
					// use datamsg instead:
	lsls	r2, r0, #8		// r2 = param count
	lsrs	r2, #32-2
	lsrs	r1, r0, #24		// r1 = param3
	lsls	r3, r0, #8+2		// r3 = message number
	lsrs	r3, #24+2
	adds	r2, #1
	lsls	r0, #8
	orrs	r0, r3
	lsls	r0, #8
	orrs	r0, r2
	cmp	r2, #3
	bgt	SendString2
	b	SendString1
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@	movs	r1, r0				//
@	ldr	r0,=mmFifoChannel		// send message
@	ldr	r0, [r0]			//
@	ldr	r2,=fifoSendValue32		//
@	bx	r2				//


/***********************************************************************
 * SendString()
 *
 * Send data via Datamsg
 ***********************************************************************/
						.thumb_func
SendString1:
	push	{r0,r4,lr}
	movs	r4, #1*4
	b	SendString
						.thumb_func
SendString2:
	
	push	{r0,r1,r4,lr}
	movs	r4, #2*4
	b	SendString
						.thumb_func
SendString3:
	push	{r0,r1,r2,r4,lr}
	movs	r4, #3*4
	
SendString:
	
	ldr	r0,=mmFifoChannel		//
	ldr	r0, [r0]			//
	movs	r1, r4				//
	mov	r2, sp				//
	bl	fifoSendDatamsg			//
	
	add	sp, r4
	pop	{r4,pc}

/***********************************************************************
 * mmSendBank( #songs, mm_bank )
 *
 * Send a soundbank to ARM7
 ***********************************************************************/
						.global mmSendBank
						.thumb_func
mmSendBank:
						// r1 = --bbbbbb
	lsls	r0, #8				// r0 = --ssss--
	adds	r0, #MSG_BANK			// r0 = --ssssmm
	lsls	r0, #8				// r0 = ssssmm--
	adds	r0, #6				// r0 = ssssmm06 (size)
	b	SendString2
	
/***********************************************************************
 * mmLockChannels( bitmask )
 * 
 * Lock channels to prevent use by maxmod
 ***********************************************************************/
						.global mmLockChannels
						.thumb_func
mmLockChannels:
	movs	r2, #MSG_SELCHAN
	movs	r1, #1
	b	SendSimpleExt
	
/***********************************************************************
 * mmUnlockChannels( bitmask )
 *
 * Unlock channels to allow use by maxmod
 ***********************************************************************/
						.global mmUnlockChannels
						.thumb_func
mmUnlockChannels:
	movs	r2, #MSG_SELCHAN
	movs	r1, #0				// nonzero = unlock channels
	b	SendSimpleExt

/***********************************************************************
 * mmStart( id, mode )
 *
 * Start module playback
 ***********************************************************************/
						.global mmStart
						.thumb_func
mmStart:
	ldr	r2,=mmActiveStatus
	movs	r3, #1
	strb	r3, [r2]
	
	movs	r2, #MSG_START
	b	SendSimpleExt

/***********************************************************************
 * mmPause()
 *
 * Pause module playback
 ***********************************************************************/
						.global mmPause
						.thumb_func
mmPause:
	movs	r2, #MSG_PAUSE
	b	SendSimple

/***********************************************************************
 * mmResume()
 *
 * Resume module playback
 ***********************************************************************/
						.global mmResume
						.thumb_func
mmResume:
	movs	r2, #MSG_RESUME
	b	SendSimple

/***********************************************************************
 * mmStop()
 *
 * Stop module playback
 ***********************************************************************/
						.global mmStop
						.thumb_func
mmStop:
	movs	r2, #MSG_STOP
	b	SendSimple

/***********************************************************************
 * mmPosition( position )
 *
 * Set playback position
 ***********************************************************************/
						.global mmPosition
						.thumb_func
mmPosition:
	
	movs	r2, #MSG_POSITION
	b	SendSimple
	
/***********************************************************************
 * mmJingle( id )
 *
 * Start jingle
 ***********************************************************************/
						.global mmJingle
						.thumb_func
mmJingle:
	movs	r2, #MSG_STARTSUB
	b	SendSimple

/***********************************************************************
 * mmSetModuleVolume( vol )
 *
 * Set module volume
 ***********************************************************************/
						.global mmSetModuleVolume
						.thumb_func
mmSetModuleVolume:
	movs	r2, #MSG_MASTERVOL
	b	SendSimple

/***********************************************************************
 * mmSetJingleVolume( vol )
 *
 * Set jingle volume
 ***********************************************************************/
						.global mmSetJingleVolume
						.thumb_func
mmSetJingleVolume:
	movs	r2, #MSG_MASTERVOLSUB
	b	SendSimple

/***********************************************************************
 * mmSetModuleTempo( tempo )
 *
 * Set master tempo
 ***********************************************************************/
						.global mmSetModuleTempo
						.thumb_func
mmSetModuleTempo:
	movs	r2, #MSG_MASTERTEMPO
	b	SendSimple

/***********************************************************************
 * mmSetModulePitch( pitch )
 *
 * Set master pitch
 ***********************************************************************/
						.global mmSetModulePitch
						.thumb_func
mmSetModulePitch:
	movs	r2, #MSG_MASTERPITCH
	b	SendSimple

/***********************************************************************
 * mmSetEffectsVolume( vol )
 *
 * Set master effect volume
 ***********************************************************************/
						.global mmSetEffectsVolume
						.thumb_func
mmSetEffectsVolume:
	movs	r2, #MSG_MASTEREFFECTVOL
	b	SendSimple
	
/***********************************************************************
 * mmSelectMode( mode )
 *
 * Select audio mode
 ***********************************************************************/
						.global mmSelectMode
						.thumb_func
mmSelectMode:
	movs	r2, #MSG_SELECTMODE
	b	SendSimple

/***********************************************************************
 * mmStreamBegin( wave, clks, len, format )
 *
 * Open audio stream
 ***********************************************************************/
						.global mmStreamBegin
						.thumb_func
mmStreamBegin:
	
	// [wwww,mm,0A]
	// [cccc,wwww]
	// [--,ff,llll]
	
	lsls	r3, #16				// r3 = --ff----
	orrs	r2, r3				// r2 = --ffllll
	lsls	r1, #16				// r1 = cccc----
	lsrs	r3, r0, #16			// r3 = ----wwww
	orrs	r1, r3				// r1 = ccccwwww
	lsls	r0, #8				// r0 = --wwww--
	adds	r0, #MSG_OPENSTREAM		// r0 = --wwwwmm
	lsls	r0, #8				// r0 = wwwwmm--
	adds	r0, #10				// r0 = wwwwmm0A
	
	b	SendString3
	
/***********************************************************************
 * mmStreamEnd()
 *
 * Close audio stream
 ***********************************************************************/
						.global mmStreamEnd
						.thumb_func
mmStreamEnd:
	movs	r2, #MSG_CLOSESTREAM
	b	SendSimple
	
	
	
/***********************************************************************
 *
 * Sound Effects
 *
 ***********************************************************************/
 


/***********************************************************************
 * mmValidateEffectHandle(handle)
 *
 * Returns same handle, or a newer valid handle
 ***********************************************************************/
						.thumb_func
mmValidateEffectHandle:
	
	ldr	r1,=sfx_instances		// check if instance # matches value in array
	lsls	r2, r0, #24			//
	lsrs	r2, #24				//
	subs	r2, #1				//
	ldrb	r1, [r1, r2]			//
	lsrs	r2, r0, #8			//
	cmp	r1, r2				//
	bne	1f				//
	bx	lr				//-exit on match
	
1:						// handle is invalid, generate new one
	b	mmCreateEffectHandle

/***********************************************************************
 * mmCreateEffectHandle()
 *
 * Return effect handle
 * 0 = no channels available
 ***********************************************************************/
						.thumb_func
mmCreateEffectHandle:
	
	push	{r4-r7,lr}
	
	ldr	r0,=sfx_bitmask
	ldr	r0, [r0]
	
	movs	r1, #0				// search for channel
2:	adds	r1, #1				//
	lsrs	r0, #1				//
	bcs	2b				//
	
	cmp	r1, #EFFECT_CHANNELS		// catch invalid index
	bgt	.mmgeh_invalid			//
	
	ldr	r2,=0x4000208			// disable IRQ
	ldrh	r4, [r2]			// (cannot be interrupted by sfx update!)
	movs	r3, #0				//
	strh	r3, [r2]			//
	
	ldr	r3,=sfx_bitmask			// set sfx bit
	ldr	r5, [r3]			//
	subs	r6, r1, #1			//
	movs	r7, #1				//
	lsls	r7, r6				//
	orrs	r5, r7				//
	str	r5, [r3]			//
	
	strh	r4, [r2]			// enable IRQ
	
	
	ldr	r0,=sfx_instances		// add instance #
	ldrb	r2, [r0, r6]			//
	adds	r2, #1				//
	strb	r2, [r0, r6]			//
//	ldr	r0,=sfx_instances		//-save instance # to array
//	strb	r2, [r0, r6]			//-
	lsls	r2, #24				//
	lsrs	r0, r2, #16			//
	orrs	r0, r1				//
	
	b	.mmgeh_exit
	
.mmgeh_invalid:	
	movs	r0, #0
	
.mmgeh_exit:
	pop	{r4-r7,pc}
	
/***********************************************************************
 * mmEffect( id )
 *
 * Play sound effect, default parameters
 ***********************************************************************/
						.global mmEffect
						.thumb_func
mmEffect:
	
	push	{lr}
	lsls	r0, #8				// r0 = xxxxmm05
	adds	r0, #MSG_EFFECT			//
	lsls	r0, #8				//
	adds	r0, #0x05			//
	push	{r0}				//
	
	bl	mmCreateEffectHandle		// r1 = ----hhhh
	movs	r1, r0				//
	pop	{r0}				//
	
	cmp	r1, #0
	beq	.no_handles_avail
	
	push	{r1}				// send data
	bl	SendString2			//
	
	pop	{r0,pc}				// return handle
	
.no_handles_avail:
	movs	r0, #0
	pop	{pc}
	
/***********************************************************************
 * mmEffectVolume( handle, volume )
 *
 * Set effect volume
 ***********************************************************************/
						.global mmEffectVolume
						.thumb_func
mmEffectVolume:
	movs	r2, #MSG_EFFECTVOL
	b	SendSimpleExt
	
/***********************************************************************
 * mmEffectPanning( handle, panning )
 *
 * Set effect panning
 ***********************************************************************/
						.global mmEffectPanning
						.thumb_func
mmEffectPanning:
	movs	r2, #MSG_EFFECTPAN
	b	SendSimpleExt

/***********************************************************************
 * mmEffectRate( handle, rate )
 * 
 * Set effect playback rate
 ***********************************************************************/
						.global mmEffectRate
						.thumb_func
mmEffectRate:
	lsls	r0, #8
	adds	r0, #MSG_EFFECTRATE
	lsls	r0, #8
	adds	r0, #5
	
	b	SendString2
	
/***********************************************************************
 * mmEffectScaleRate( handle, factor )
 *
 * Scale effect playback rate by some factor
 ***********************************************************************/
						.global mmEffectScaleRate
						.thumb_func
mmEffectScaleRate:
	lsls	r0, #8
	adds	r0, #MSG_EFFECTMULRATE
	lsls	r0, #8
	adds	r0, #5
	
	b	SendString2
	
/***********************************************************************
 * mmEffectRelease( handle )
 *
 * Release sound effect
 ***********************************************************************/
						.global mmEffectRelease
						.thumb_func
mmEffectRelease:
	movs	r1, #1
	movs	r2, #MSG_EFFECTOPT
	b	SendSimpleExt
	
/***********************************************************************
 * mmEffectCancel( handle )
 *
 * Stop sound effect
 ***********************************************************************/
						.global mmEffectCancel
						.thumb_func
mmEffectCancel:
	movs	r1, #0
	movs	r2, #MSG_EFFECTOPT
	b	SendSimpleExt

/***********************************************************************
 * mmEffectEx( sound )
 *
 * Play sound effect, parameters supplied
 ***********************************************************************/
						.global mmEffectEx
						.thumb_func
mmEffectEx:
	push	{r4,r5,lr}
	
	movs	r4, r0				// save struct address
	
	ldrh	r0, [r4, #MM_SFX_HANDLE]	// test handle and validate/create new one
	cmp	r0, #0				//	
	bne	1f				//
	bl	mmCreateEffectHandle		//
	b	2f				//
1:	bl	mmValidateEffectHandle		//
2:
	movs	r2, r0				//
	beq	.no_sfx_handles_availble
	movs	r5, r0				//-save for return value
	
	ldrh	r0, [r4, #MM_SFX_SRC]		// r0 = ssssmm0B
	lsls	r0, #8				//
	adds	r0, #MSG_EFFECTEX		//
	lsls	r0, #8				//
	adds	r0, #11				//
	
	ldrh	r1, [r4, #MM_SFX_SRC+2]		// r1 = rrrrssss
	ldrh	r3, [r4, #MM_SFX_RATE]		//
	lsls	r3, #16				//
	orrs	r1, r3				//
	
	ldrh	r3, [r4, #MM_SFX_VOLUME]	// r2 = ppvvhhhh
	lsls	r3, #16
	orrs	r2, r3
	
	bl	SendString3
	

	movs	r0, r5				// return handle
	pop	{r4,r5,pc}
	
.no_sfx_handles_availble:
	movs	r0, #0
	pop	{r4,r5,pc}

/***********************************************************************
 * mmEffectCancelAll()
 *
 * Cancel all sound effects.
 ***********************************************************************/
						.global mmEffectCancelAll
						.thumb_func
mmEffectCancelAll:
	movs	r2, #MSG_EFFECTCANCELALL
	b	SendSimple
	
	

/***********************************************************************
 *
 * REVERB
 *
 ***********************************************************************/



/***********************************************************************
 * mmReverbEnable()
 *
 * Enable reverb system.
 ***********************************************************************/
						.global mmReverbEnable
						.thumb_func
mmReverbEnable:

	movs	r2, #MSG_REVERBENABLE
	b	SendSimple
	
/***********************************************************************
 * mmReverbDisable()
 *
 * Disable reverb system.
 ***********************************************************************/
						.global mmReverbDisable
						.thumb_func
mmReverbDisable:

	movs	r2, #MSG_REVERBDISABLE
	b	SendSimple
	
/***********************************************************************
 * mmReverbConfigure( configuration )
 *
 * Configure reverb system.
 ***********************************************************************/
						.global mmReverbConfigure
						.thumb_func
mmReverbConfigure:
	push	{lr}
	sub	sp, #24
	mov	r3, sp
						// byte0 = size... (calculate)
	movs	r2, #MSG_REVERBCFG		// byte1 = reverbcfg
	strb	r2, [r3, #1]			//
	
	ldrh	r2, [r0, #mmrc_flags]		// write flags
	strh	r2, [r3, #2]			//
	adds	r3, #4				//
	
	lsrs	r2, #1				// write memory
	bcc	.mmrc_memory			//
						//
	ldr	r1, [r0, #mmrc_memory]		//
	str	r1, [r3]			//
	adds	r3, #4				//
.mmrc_memory:					//
	
	lsrs	r2, #1				// delay...
	bcc	.mmrc_delay			//
						//
	ldrh	r1, [r0, #mmrc_delay]		//
	strh	r1, [r3]			//
	adds	r3, #2				//
.mmrc_delay:					//
	
	lsrs	r2, #1				// rate...
	bcc	.mmrc_rate			//
						//
	ldrh	r1, [r0, #mmrc_rate]		//
	strh	r1, [r3]			//
	adds	r3, #2				//
.mmrc_rate:					//
	
	lsrs	r2, #1				// feedback...
	bcc	.mmrc_feedback			//
						//
	ldrh	r1, [r0, #mmrc_feedback]	//
	strh	r1, [r3]			//
	adds	r3, #2				//
.mmrc_feedback:					//
	
	lsrs	r2, #1				// panning...
	bcc	.mmrc_panning			//
						//
	ldrb	r1, [r0, #mmrc_panning]		//
	strb	r1, [r3]			//
	adds	r3, #1				//
.mmrc_panning:					//
	
	mov	r2, sp				// r2 = data pointer
	subs	r3, r2				// get byte count
	subs	r3, #1				//
	strb	r3, [r2]			// 
	
	adds	r3, #3	+1			// r1 = wordcount
	lsrs	r1, r3, #2			//
	lsls	r1, #2
	
	ldr	r0,=mmFifoChannel		// r0 = channel
	ldr	r0, [r0]			//
	bl	fifoSendDatamsg			// send data
	
	add	sp, #24				// free stack
	
	pop	{pc}
	
/***********************************************************************
 * mmReverbStart( channels )
 *
 * Enable reverb output.
 ***********************************************************************/
						.global mmReverbStart
						.thumb_func
mmReverbStart:
	movs	r2, #MSG_REVERBSTART
	b	SendSimple
	
/***********************************************************************
 * mmReverbStop( channels )
 *
 * Disable reverb output.
 ***********************************************************************/
						.global mmReverbStop
						.thumb_func
mmReverbStop:
	movs	r2, #MSG_REVERBSTOP
	b	SendSimple
	
	
 
/***********************************************************************
 * 
 * RECEIVING
 *
 ***********************************************************************/


 
/***********************************************************************
 * mmReceiveMessage( value32 )
 *
 * Value32 handler
 ***********************************************************************/
						.thumb_func
mmReceiveMessage:

	// 1 = event
	// 0 = sfx

	lsrs	r1, r0, #20
	cmp	r1, #1
	beq	.got_event
//	blt	.got_sfx
//.got_sfx:
	
	ldr	r1,=sfx_bitmask
	ldrh	r2, [r1]
	bics	r2, r0
	strh	r2, [r1]
	//strh	r0, [r1]
	
	ldr	r1,=mmActiveStatus
	lsls	r0, #15
	lsrs	r0, #31
	strb	r0, [r1]
	
	bx	lr
	
.got_event:
	
	ldr	r2,=mmCallback
	ldr	r2, [r2]
	cmp	r2, #0
	beq	1f
	
	lsls	r1, r0, #16			// r1 = param
	lsrs	r1, #24				//
	lsls	r0, #24				// r0 = msg
	lsrs	r0, #24				//
	bx	r2				// jump to callback
1:
	bx	lr
