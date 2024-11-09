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

@ Layer Information
@ -----------------

.equ	MPL_TICK	,0
.equ	MPL_ROW		,1
.equ	MPL_POSITION	,2
.equ	MPL_NROWS	,3
.equ	MPL_GV		,4
.equ	MPL_SPEED	,5
.equ	MPL_ISPLAYING	,6
.equ	MPL_BPM		,7
.equ	MPL_INSTTABLE	,8
.equ	MPL_SAMPTABLE	,12
.equ	MPL_PATTTABLE	,16
.equ	MPL_SONGADR	,20
.equ	MPL_FLAGS	,24
.equ	MPL_OLDEFFECTS	,25
.equ	MPL_PATTJUMP	,26
.equ	MPL_PATTJUMP_ROW,27
.equ	MPL_FPATTDELAY	,28
.equ	MPL_PATTDELAY	,29

.equ	MPL_PLOOP_ROW	,30
.equ	MPL_PLOOP_TIMES	,31
.equ	MPL_PLOOP_ADR	,32
.equ	MPL_PATTREAD	,36
.equ	MPL_PLOOP_JUMP	,40
.equ	MPL_VALID		,41

.equ	MPL_TICKRATE	,42	@ 1.15 fixed point OR sample count
.equ	MPL_SAMPCOUNT	,44	@ sample timing
.equ	MPL_TICKFRAC	,44	@ vsync  timing 0.16 fixed point

.equ	MPL_MODE	,46
.equ	mpl_reserved2	,47
.equ	MPL_MCH_UPDATE	,48
.equ	MPL_VOLUME	,52
.equ	mpl_reserved3	,54
.equ	MPL_SIZE	,56

@ active information
@--------------------

.equ	MPV_PATTREAD_P	,4
.equ	MPV_AFVOL	,8
.equ	MPV_SAMPOFF	,9
.equ	MPV_VOLPLUS	,10
.equ	MPV_NOTEDELAY	,11
.equ	MPV_PANPLUS	,12
.equ	MPV_SIZE	,14

@ Module Channel
@ --------------

.equ	MCH_ALLOC,	0 @ ALLOCATED ACTIVE CHANNEL#
.equ	MCH_CFLAGS,	1 @ PATTERN COMRESSION FLAGS, called "maskvariable" in ITTECH.TXT
.equ	MCH_PANNING,	2
.equ	MCH_VOLCMD,	3 @ VOLUME COLUMN COMMAND
.equ	MCH_EFFECT,	4 @ EFFECT#				} COMBINED
.equ	MCH_PARAM,	5 @ EFFECT PARAMETER	}
.equ	MCH_FXMEM,	6 @ EFFECT MEMORY
.equ	MCH_NOTE,	7 @ TRANSLATED NOTE
.equ	MCH_FLAGS,	8 @ CHANNEL FLAGS
.equ	MCH_INST,	9 @ INSTRUMENT#
//.equ	MCH_PFLAGS,	10 @ PLAYBACK FLAGS (???)
.equ	MCH_VIBDEP,	11
.equ	MCH_VIBSPD,	12
.equ	MCH_VIBPOS,	13
.equ	MCH_VOLUME,	14 //}combined
.equ	MCH_CVOLUME,	15 //}
.equ	MCH_PERIOD,	16
.equ	MCH_BFLAGS,	20
.equ	MCH_PNOTE,	22 @ PATTERN NOTE
.equ	MCH_MEMORY,	23 @ 15 bytes
.equ	MCH_SIZE,	40 @ sizeof( module_channel ) @ 2 extra bytes

@ Active Channel
@ --------------

.equ	MCA_FADE,	4	//   }
.equ	MCA_ENVC_VOL,	6	//   }
.equ	MCA_ENVC_PAN,	8	//   } COMBINED
.equ	MCA_ENVC_PIC,	10	//   } during volume reset
.equ	MCA_AVIB_DEP,	12	//   } AND NEWNOTE, CHECK NEWNOTE ON CHANGES
.equ	MCA_AVIB_POS,	14	//   }
.equ	MCA_FVOL,	16	// } COMBINED for SPEED
.equ	MCA_TYPE,	17	// }
.equ	MCA_INST,	18
.equ	MCA_PANNING,	19
.equ	MCA_VOLUME,	20
.equ	MCA_PERIOD,	0	@ internal period
.equ	MCA_SAMPLE,	21
.equ	MCA_PARENT,	22 // } COMBINED
.equ	MCA_FLAGS,	23 // }
.equ	MCA_ENVN_VOL,	24
.equ	MCA_ENVN_PAN,	25
.equ	MCA_ENVN_PIC,	26
.equ	MCA_SFX,	27	@ can store this anywhere
.equ	MCA_SIZE,	28

@ Active Channel Flags
@ --------------------

.equ	MCAF_KEYON	,1	@ key is on... LOCKED
.equ	MCAF_FADE	,2	@ note-fade is activated
.equ	MCAF_START	,4	@ [re]start sample
.equ	MCAF_UPDATED	,8	@ already updated by pchannel routine
.equ	MCAF_ENVEND	,16	@ end of envelope
.equ	MCAF_VOLENV	,32	@ volume envelope enabled
.equ	MCAF_SUB	,64	@ sublayer.................locked..
.equ	MCAF_EFFECT	,128	@ subsublayer.............................LOCKED (mpEffect)

@ Active Channel Types
@ --------------------

.equ	ACHN_DISABLED	,0	@ LOCKED (multiple routines)
.equ	ACHN_RESERVED	,1	@ (can't be used [alloc channel])
.equ	ACHN_BACKGROUND	,2	@ LOCKED (alloc channel)
.equ	ACHN_FOREGROUND	,3
.equ	ACHN_CUSTOM	,4

@ Module Channel Flags
@ --------------------

.equ	MF_START	,1
.equ	MF_DVOL		,2
.equ	MF_HASVCMD	,4
.equ	MF_HASFX	,8
.equ	MF_NEWINSTR	,16

.equ	MF_NOTEOFF	,64	@ LOCKED
.equ	MF_NOTECUT	,128	@ LOCKED

@.equ	MF_NEWINSTR	,1	@ new instrument
@.equ	MF_DVOL		,2
@.equ	MF_START	,4
@.equ	MF_HASFX	,8
@.equ	MF_HASVCMD	,16
@.equ	MF_NOTEOFF	,64	@ LOCKED
@.equ	MF_NOTECUT	,128	@ LOCKED

@ Other Definitions
@ -----------------

.equ	IT_NNA_CUT	,0	@ New note actions
.equ	IT_NNA_CONT	,1
.equ	IT_NNA_OFF	,2
.equ	IT_NNA_FADE	,3

.equ	IT_DCA_CUT	,0	@ Duplicate check actions
.equ	IT_DCA_OFF	,1
.equ	IT_DCA_FADE	,2

@ Misc Reference
@ --------------

@ BFLAGS:
@ /////ctv nnppttvv
@ nn...............new note action
@ pp...............panbrello waveform
@ tt...............tremolo waveform
@ vv...............vibrato waveform
@ dd...............duplicate check type
@ v................volume envelope enabled
@ t................tremor variable...
@ c................cut channel volume
@ //////...........reserved
