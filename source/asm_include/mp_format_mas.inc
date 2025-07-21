// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

.ifndef MP_FORMAT_MAS_INC
.equ	MP_FORMAT_MAS_INC, 1

@ MAS header structure flags : mas_header

.equ	C_FLAGS_GS	,1
.equ	C_FLAGS_OS	,2
.equ	C_FLAGS_SS	,3
.equ	C_FLAGS_XS	,4
.equ	C_FLAGS_DS	,5
///////.equ	C_FLAGS_LS	,5		HUH???
.equ	C_FLAGS_LS	,6

.equ	C_FLAGS_X,	(1<<3)

// Instrument struct : mm_mas_instrument

.equ	C_MASI_GVOL	,0
.equ	C_MASI_FADE	,1
.equ	C_MASI_RANDVOL	,2
.equ	C_MASI_DCT	,3
.equ	C_MASI_NNA	,4
.equ	C_MASI_ENVFLAGS	,5
.equ	C_MASI_PAN	,6
.equ	C_MASI_DCA	,7
.equ	C_MASI_MAP	,8
.equ	C_MASI_ENVELOPES,12 // TODO: This is misleading!
//.equ	C_MASI_ENVELOPES,248

// Instrument envelope struct : mm_mas_envelope

.equ	C_MASIE_SIZE	,0
.equ	C_MASIE_LSTART	,1
.equ	C_MASIE_LEND	,2
.equ	C_MASIE_SSTART	,3
.equ	C_MASIE_SEND	,4
.equ	C_MASIE_NODEC	,5
.equ	C_MASIE_FILTER	,6
.equ	C_MASIE_NODES	,8

.equ	ENVFLAG_A,	0b1000

// Sample structure : mm_mas_sample_info

.equ	C_MASS_DV	,0
.equ	C_MASS_PAN	,1
.equ	C_MASS_FREQ	,2
.equ	C_MASS_VIT	,4
.equ	C_MASS_VID	,5
.equ	C_MASS_VIS	,6
.equ	C_MASS_GV	,7
.equ	C_MASS_VIR	,8
.equ	C_MASS_MSLID	,10

// Sample structure : mm_mas_gba_sample

.equ	C_SAMPLE_LEN	,0
.equ	C_SAMPLE_LOOP	,4
.equ	C_SAMPLE_DATA	,12

// Sample structure : mm_mas_ds_sample

.equ	C_SAMPLEN_LSTART,0
.equ	C_SAMPLEN_LEN	,4
.equ	C_SAMPLEN_FORMAT,8
.equ	C_SAMPLEN_REP	,9
.equ	C_SAMPLEC_DFREQ	,10 // TODO: Why is the name different?
.equ	C_SAMPLEN_POINT	,12
.equ	C_SAMPLEN_DATA	,16

.endif
