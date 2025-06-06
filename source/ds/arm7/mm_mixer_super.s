// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

    .syntax unified

#include "mp_format_mas.inc"
#include "swi_nds.inc"
#include "mp_macros.inc"
#include "mp_defs.inc"

//======================================================================

    .global mm_mix_data
    .global mm_output_slice

    .global mmMixB
    .type   mmMixB STT_FUNC
    .global mmMixC
    .type   mmMixC STT_FUNC

    .global mmVolumeTable
    .global mmVolumeDivTable
    .global mmVolumeShiftTable

//----------------------------------------------------------------------

// sound clock : 16756991 hz
//
// rate     bsize       clock       hz
// 32k      360         512         32768.4980
// 26k      304         640         26182.7984
// 22k      256         768         21818.9987   <-------

    .equ    MM_nDSCHANNELS,  32     // [channels]
    .equ    MM_SW_CHUNKLEN,  112    // [samples]
    .equ    MM_SW_BUFFERLEN, 224    // [samples], note: nothing
    .equ    CLK_DIV,         524288 // VALUE = 16777216 * CLK / 512 / 32

    .equ    SFRAC, 10

    .equ    REG_DMA,     0x40000BC
    .equ    DMA_ENABLE,  (1 << 31)
    .equ    DMA_32BIT,   (1 << 26)
    .equ    DMA_CONTROL, (DMA_ENABLE | DMA_32BIT)

    .equ    CSOUND_CNT,  0
    .equ    CSOUND_SAD,  4
    .equ    CSOUND_TMR,  8
    .equ    CSOUND_PNT,  0xA
    .equ    CSOUND_LEN,  0xC

    .equ    SOUNDxCNT_ENABLE, 0x80000000

//----------------------------------------------------------------------
// channel structure
//----------------------------------------------------------------------

    // mm_mixer_channel in asm_include/mp_mixer_ds.h
    .equ    C_SAMP, 0   // mainram address
    .equ    C_CNT,  3   // LSBs = target panning 0..127, MSB = key-on
    .equ    C_FREQ, 4   // unsigned 3.10, top 3 cleared
    .equ    C_VOL,  6   // target volume   0..65535
    .equ    C_READ, 8   // unsigned 22.10
    .equ    C_CVOL, 12  // current volume  0..65535
    .equ    C_CPAN, 14  // current panning 0..65535
    .equ    C_SIZE, 16

    .equ    CF_START, 128

    .equ    C_READ_FRAC, 10

//----------------------------------------------------------------------
    .bss
//----------------------------------------------------------------------

    .align 2

#define mix_data_len    8544    // yikes

mm_mix_data:
    .space mix_data_len

//**********************************************************
// MODE A [fast, light]
//**********************************************************

// (no extra data needed)

//**********************************************************
// MODE B [interpolated]
//**********************************************************
.equ    MB_FETCH_SIZE,    (256)
.equ    MB_FETCH_PADDING, (32)

// structure
.equ MB_SH_VLEVEL,   0                  // next volume level
.equ MB_SH_VMUL,     MB_SH_VLEVEL + 0
.equ MB_SH_VSHIFT,   MB_SH_VMUL + 1
.equ MB_SH_PLEVEL,   MB_SH_VSHIFT + 1   // next panning level
.equ MB_SH_RESERVED, MB_SH_PLEVEL + 1
.equ MB_SH_LEN,      MB_SH_RESERVED + 1

// structure
.equ MB_OUTPUT, 0                       // 512 bytes/channel (128 samples, double-buffered)
.equ MB_FETCH,  MB_OUTPUT + (16 * 512)
.equ MB_SHADOW, MB_FETCH + (MB_FETCH_SIZE + MB_FETCH_PADDING)
.equ MB_LEN,    MB_SHADOW + (MB_SH_LEN * 16)

//**********************************************************
// MODE C [sw mixing]
//**********************************************************
//.equ    MC_FETCH, (MC_MIX_TIMER+8)
.equ    MC_FETCH_SIZE, (256)
.equ    MC_FETCH_PADDING, (16)

// structure        mode C - shadow data
.equ MC_SH_CNT,     0               // CNT updated every tick -locked
                                    // top 8 bits only updated on new source
                                    //
.equ MC_SH_SRC,     MC_SH_CNT + 4   // SRC
.equ MC_SH_TMR,     MC_SH_SRC + 4   // TMR updated every tick
.equ MC_SH_PNT,     MC_SH_TMR + 2   // PNT } updated if SRC != 0
.equ MC_SH_LEN,     MC_SH_PNT + 2   // LEN } (SRC cleared after)
.equ MC_SH_SIZE,    MC_SH_LEN + 4   // (length of struct)

// structure
.equ MC_MIX_OUTPUT, 0               // 16-bit stereo
.equ MC_MIX_WMEM,   MC_MIX_OUTPUT + MM_SW_BUFFERLEN * 4 // (working buffer)
.equ MC_FETCH,      MC_MIX_WMEM + MM_SW_CHUNKLEN * 4
.equ MC_SHADOW,     MC_FETCH + MC_FETCH_SIZE + MC_FETCH_PADDING
// length of MC_SHADOW: MC_SH_LEN*16
//----------------------------------------------------------

.if MC_SH_CNT != 0
.error "MC_SH_CNT MUST be zero (see mode_c_tick)"
.endif

mm_output_slice: // 0 = first half, 1 = second half
    .space 1

//----------------------------------------------------------------------
    .text
    .arm
    .align 2
//----------------------------------------------------------------------

//*********************************************************
mmMixB:
//*********************************************************
    stmfd   sp!, {r4-r11, lr}

    mov     r7, r0
    mov     r10, r1
    mov     r12, r2

    ldr     r0, =REG_DMA
    ldr     r1, =mm_mix_data+MB_FETCH
    str     r1, [r0, #4]                    // set dma destination

    ldr     r11, =mm_mix_data + MB_SHADOW   // get mode B shadow data
    bic     r10, #0x00FF0000                // clear top 16 bits
    bic     r10, #0xFF000000                // (only 16 channels)

    movs    r10, r10, lsr #1                // start loop
    bcc     mmb_next
mmb_loop:
    ldr     r1, [r12, #C_SAMP]
    bics    r6, r1, #0xFF000000
    beq     mmb_disabled
mmb_active:


    ldrb    r0, [r12, #C_CNT]               // shift out start bit of control
    movs    r0, r0, lsl #25                 //
    bcc     mmb_continue
mmb_newnote:
    movs    r0, r0, lsr #25                 // clear start bit
    strb    r0, [r12, #C_CNT]
    ldrh    r0, [r12, #C_VOL]               // copy volume/panning levels
    ldrb    r1, [r12, #C_CNT]
    orr     r0, r0, r1, lsl #16 + 9
    str     r0, [r12, #C_CVOL]

    // **todo: CLIP sample offset
    ldr     r0, [r12, #C_READ]
    lsl     r0, #8 + SFRAC
    str     r0, [r12, #C_READ]
    mov     r1, #1                          // add zero padding
    b       1f
mmb_continue:
    mov     r1, #0
1:  push    {r1}

//---------------------------------
// do volume ramping
//---------------------------------

    ldrh    r0, [r12, #C_CVOL]          // get volume+shift value
    bl      translateVolume             //

    ldrh    r4, [r12, #C_CPAN]          // assemble volume|shift|panning
    lsr     r4, #9                      //
    orr     r4, r0, r4, lsl #16         //

    str     r4, [r11]                   // -write to shadow

    bl      mmb_getdest                 // fill wave buffer

    pop     {r1}
    bl      mmbResampleData             // mmbResampleData( dest, zeropad )
    b       mmb_next                    //

mmb_disabled:
    mov     r0, #64 << 16               // write pan=center, vol=silent
    str     r0, [r11]                   //

    bl      mmb_getdest                 //
    bl      zerofill_buffer             // zero wavebuffer

mmb_next:
    add     r11, #4                     // increment pointers
    add     r12, #C_SIZE                //
    movs    r10, r10, lsr #1            // shift out next bit
    bcs     mmb_loop                    // loop until finished
    bne     mmb_next

    ldr     r0,=mm_output_slice         // swap mixing slice
    ldrb    r1, [r0]                    //
    cmp     r1, #0                      //
    moveq   r1, #1                      //
    movne   r1, #0                      //
//    add     r1, #1
//    cmp     r1, #2
//    movgt   r1, #0
    strb    r1, [r0]                    //

    ldmfd   sp!, {r4-r11, lr}
    bx      lr

//-----------------------------------------------------------------------
mmb_getdest:
//-----------------------------------------------------------------------
    ldr     r0, =mm_mix_data + MB_OUTPUT    // calc destination address...
    ldr     r1, =mm_mix_data + MB_SHADOW
    sub     r1, r11, r1                     // r0 = channel * 4
    add     r0, r0, r1, lsl #9 - 2          // add channel * 512 to dest
    //sub     r0, r0, r1, lsl #8 - 2        //
    ldr     r1, =mm_output_slice            //
    ldrb    r1, [r1]                        //
    add     r0, r0, r1, lsl #8              // add output slice offset
    bx      lr

//***************************************************************************************
mmbResampleData:
//***************************************************************************************

#define m1      r0      // [output data]
#define m2      r1      // [output data]
#define m3      r2      // [output data]
#define m4      r3      // [output data]
#define pos     r4      // [read position]
#define src     r5      // [data source]
#define count   r6      // [counter]
#define rate    r7      // [sampling rate]
#define curr    r8      // [current sample]
#define next    r9      // [next sample]
#define dest    r10     // [output address]
#define ta      r11     // [temporary value]
#define tb      r12     // [temporary value]

//---------------------------------------------------------------------------
.macro rs_routine shift, routine, restart
//---------------------------------------------------------------------------
    mul     r2, count, rate
    mov     r0, #MB_FETCH_SIZE << (10 - \shift)
    rsb     r1, pos, r3, lsl #(12 - \shift)
    bl      calc_mixcount                       // calculate how many samples to fetch&output

    ldr     r0, =REG_DMA                        // set dma SRC (32bit aligned)

    ldr     r1, [src, #C_SAMPLEN_POINT]
    cmp     r1, #0
    addeq   r1, src, #C_SAMPLEN_DATA

    add     r1, r1, pos, lsr #10
.if \shift == 1
    add     r1, r1, pos, lsr #10
.endif
    bic     r1, #0b11
    str     r1, [r0, #0]

    mov     r1, #DMA_ENABLE | DMA_32BIT         // set dma CNT
    add     r2, r2, #16384                      // add threshold for safety
    add     r1, r1, r2, lsr #10 + (2 - \shift)
    str     r1, [r0, #8]

    bl      \routine                            // resample data

    ldmia   src, {r0, r1}                       // read PNT+LEN
    add     r3, r0, r1                          // add for LENGTH
    cmp     pos, r3, lsl #SFRAC + (2 - \shift)  // check position against length
    bcc     1f

    ldrb    r0, [src, #C_SAMPLEN_REP]           // branch according to loop type
    cmp     r0, #1                              //
    beq     2f

// one shot
//----------------------------------------------------------------
    mov     pos, #0                             // clear sample
    str     pos, [r12, #C_SAMP]
    pop     {count}                             // pop mixcount
    cmp     count, #0                           // 0=exit
    beq     4f
3:  strh    pos, [dest], #2                     // fill the rest
    subs    count, #1                           // of the slice
    bne     3b                                  // with zero
    b       4f

2: // forward loop
//-----------------------------------------------------------------
    sub     pos, pos, r1, lsl #SFRAC + (2 - \shift) // subtract loop length from position

1:  pop     {count}                                 // check if theres more samples to mix
    cmp     count, #0
    bne     \restart                                // (then loop)

    str     pos, [r12, #C_READ]                     // save position & return
4:  pop     {r10-r12, pc}

.endm
//-------------------------------------------------------------------------


#define zeropad_size 32

// note r11 = shadow entry, r12 = channel data
// r0 = dest, r1 = zeropadding

    push    {r10-r12, lr}
    ldr     src, [r12, #C_SAMP]
    bic     src, #0xFF000000
    add     src, #0x2000000
    ldrh    rate, [r12, #C_FREQ]
    ldr     pos,  [r12, #C_READ]

    mov     dest, r0

    mov     count, #128

    cmp     r1, #0                  // if r1 then zero the first X samples (newnote)
    beq     1f                      //
    sub     count, #zeropad_size    //
    mov     r1, #0                  //
    mov     r0, #zeropad_size / 2   //
2:  stmia   dest!, {r1}             //
    subs    r0, #1                  //
    bne     2b                      //
1:                                  //

    ldmia    src, {r0, r1}
    add     r3, r0, r1
    ldrb    r0, [src, #C_SAMPLEN_FORMAT]
    cmp     r0, #0
    beq     mmb_8bit

//*************************************************************************************
mmb_16bit:
//*************************************************************************************

    rs_routine  1, mmb_resamp_16bit, mmb_16bit

//*************************************************************************************
mmb_8bit:
//*************************************************************************************

    rs_routine  0, mmb_resamp_8bit, mmb_8bit

//*************************************************************************************
//*************************************************************************************
// resampling routines
//*************************************************************************************
//*************************************************************************************

//--------------------------------------------------------------------------------
.macro resample_data rout, exit, mode, dsize
//--------------------------------------------------------------------------------

.if \mode == 1                                  // mode1: increment before load
    subs    pos, pos, rate, lsl #32 - SFRAC
    subcc   src, #\dsize
    movcc   next,curr
.endif

    subs    count, #8           // output 8 sample chunks
    bmi     1f                  // ..
2:  \rout   m1,1                // build 1st word
    \rout   m2,1                // build 2nd word
    \rout   m3,1                // 3rd
    \rout   m4,1                // 4th
    stmia   dest!, {m1 - m4}    // write to dest
    subs    count, #8           // and loop
    bpl     2b                  // ..
1:

    adds    count, #8           // output remaining samples
    beq     \exit               // ..
                                // ..
1:  \rout   m1,0                // build 1 half-word
    strh    m1, [dest], #2      // output to dest
    subs    count, #1           // decrement count
    bne     1b                  // and loop
    b       \exit

.endm
//-------------------------------------------------------------------------

//***************************************************************************
mmb_resamp_16bit:
//***************************************************************************
// resample 16-bit data

    push    {src, r12, lr}
    ldr     src, =mm_mix_data + MB_FETCH
    mov     m1, pos, lsr #SFRAC             // save position integer
    bic     pos, pos, m1, lsl #SFRAC
    and     m2, m1, #0b1                    // mask bit 0
    add     src, src, m2, lsl #1            // add offset to fetch
    push    {m1, src}

    cmp     rate, #1 << SFRAC               // use nearest resampling for rates > 32khz
    blt     mmb_resamp_16bit_linear
//******************************************************************
mmb_resamp_16bit_nearest:
//******************************************************************

//----------------------------------------------------
.macro mb_buildw16n target, double
//----------------------------------------------------
    and     curr, ta, pos, lsr #SFRAC - 1   // shift position & mask out low bit
    ldrh    \target, [src, curr]            // read sample
    add     pos, rate                       // increment pos
.if \double != 0
    and     next, ta, pos, lsr #SFRAC - 1
    ldrh    next, [src, next]               // read another sample
    add     pos, rate                       // increment pos
    orr     \target, \target, next, lsl #16 // combine
.endif
.endm
//----------------------------------------------------

    mvn     ta, #1                          // <- used for clearing low bit in shifted position


    tst     dest, #0b11                     // align destination
    beq     1f

    bic     m1, ta, pos, lsr #SFRAC - 1
    ldrh    m1, [src, m1]
    add     pos, rate
    strh    m1, [dest], #2
    subs    count, #1
    beq     _mb16n_exit
1:

    resample_data   mb_buildw16n, _mb16n_exit, 0, 2

//******************************************************************
mmb_resamp_16bit_linear:
//******************************************************************

//-------------------------------------------------------------------------------
.macro mb_buildw16 target, double
//-------------------------------------------------------------------------------
    adds    pos, pos, rate, lsl #32 - SFRAC     // 1   add rate FRACTION to position FRACTION
    movcs   curr, next                          // 1   load new sample on overflow
    ldrshcs next, [src, #2]!                    // 1/3 ..
    sub     ta, next, curr                      // 1   calculate delta
    mov     tb, pos, lsr #24                    // 1   get top 8 bits of position fraction
    mul     tb, ta, tb                          // 2   multiply delta * position
    add     \target, curr, tb, asr #8           // 1   add base sample to product (shifted to 16 bits)
//    lsl     \target, #16
//    lsr     \target, #16
.if \double != 0
    adds    pos, pos, rate, lsl #32 - SFRAC     // 1   add rate FRACTION to position FRACTION
    movcs   curr, next                          // 1   load new sample on overflow
    ldrshcs next, [src, #2]!                    // 1/3 ..
    sub     ta, next, curr                      // 1   calculate delta
    mov     tb, pos, lsr #24                    // 1   get top 8 bits of position fraction
    mul     tb, ta, tb                          // 2   multiply delta * position
    add     ta, curr, tb, asr #8                // 1   add base sample to product (shifted to 16 bits)
    add     \target, \target, ta, lsl #16       // 1
.endif

.endm
//------------------------------------------------------------------------------------

    mov     pos, pos, lsl #32 - SFRAC           // shift out integer bits

    tst     dest, #0b11                         // align destination
    beq     1f                                  // ..
    ldrsh   curr, [src]                         // ..
    ldrsh   next, [src, #2]                     // ..
    sub     ta, next, curr                      // ..
    mov     tb, pos, lsr #24                    // ..
    mul     ta, tb, ta                          // ..
    add     m1, curr, ta, asr #8                // ..
    strh    m1, [dest], #2                      // ..
    adds    pos, pos, rate, lsl #32 - SFRAC     // ..
    addcs   src, #2                             // ..
    subs    count, #1                           // ..
    beq     _mb16_exit2
1:

    ldrsh   curr, [src]                         // prime samples
    ldrsh   next, [src, #2]!

    resample_data   mb_buildw16, _mb16_exit, 1, 2

//-----------------------------------------------------------------------------------
_mb16_exit:
//-----------------------------------------------------------------------------------
    sub     src, #2
_mb16_exit2:
    movs    pos, pos, lsr #32 - SFRAC
    add     pos, pos, rate
//-----------------------------------------------------------------------------------
_mb16n_exit:
//-----------------------------------------------------------------------------------
    pop     {m1, m2}                        // pop old position, starting src
    sub     m2, src, m2                     // get difference
    add     pos, pos, m1, lsl #SFRAC        // add old position
    add     pos, pos, m2, lsl #SFRAC - 1    // add src difference
    pop     {src, r12, pc}                  // return

//****************************************************************************
mmb_resamp_8bit:
//****************************************************************************
// resample and expand 8-bit data

    push    {src, r12, lr}
    ldr     src, =mm_mix_data + MB_FETCH
    mov     m1, pos, lsr #SFRAC             // save & clear position integer
    bic     pos, pos, m1, lsl #SFRAC        //
    and     m2, m1, #0b11                   // mask alignment bits
    add     src, m2                         // add offset to fetch
    push    {m1, src}

    cmp     rate, #1 << SFRAC               // use nearest resampling for rates >= 1.0
    blt     mmb_resamp_8bit_linear

//****************************************************************************
mmb_resamp_8bit_nearest:
//****************************************************************************
// resample with no interpolation
// use for rates >= 1.0

//---------------------------------------------------
.macro mb_buildw8n target, double, alternate
//---------------------------------------------------
    ldrb    \target, [src, pos, lsr #SFRAC] // read sample
    add     pos, rate                       // increment pos
.if \double != 0
    ldrb    next, [src, pos, lsr #SFRAC]    // read another sample
    add     pos, rate                       // increment pos
    orr     \target, \target, next, lsl #16 // combine
.endif
    mov     \target, \target, lsl #8        // expand to 16 bits
.endm
//---------------------------------------------------

    tst     dest, #0b11                     // 32-bit align destination
    beq     1f
    ldrb    m1, [src, pos, lsr #SFRAC]      // (output 1 sample if misaligned)
    add     pos, rate
    lsl     m1, #8
    strh    m1, [dest], #2
    subs    count, #1
    beq     mb8n_exit
1:

    resample_data   mb_buildw8n, mb8n_exit, 0, 1

//***********************************************************************************
mmb_resamp_8bit_linear:
//***********************************************************************************
// resample with linear interpolation
// only works with sample rates <= 1.0

    mov     pos, pos, lsl #32 - SFRAC       // shift out integer bits

    tst     dest, #0b11                     // align destination
    beq     1f                              // ..

    ldrsb   curr, [src]                     // ..
    ldrsb   next, [src, #1]                 // ..
    sub     ta, next, curr                  // ..
    mov     tb, pos, lsr #24                // ..
    mul     ta, tb, ta                      // ..
    add     m1, ta, curr, lsl #8            // ..
    strh    m1, [dest], #2                  // ..
    adds    pos, pos, rate, lsl #32 - SFRAC // ..
    addcs   src, #1                         // ..
    subs    count, #1                       // ..

    beq     mb8_exit2
1:

    ldrsb   curr, [src]                     // prime samples
    ldrsb   next, [src, #1]!

    mov     r14, #0xFF0000                  // for masking position var

//-------------------------------------------------------------------------------
.macro mb_buildw8 target, double
//-------------------------------------------------------------------------------
    adds    pos, pos, rate, lsl #32 - SFRAC // 1   add rate FRACTION to position FRACTION
    movcs   curr, next                      // 1   load new sample on overflow
    ldrsbcs next, [src, #1]!                // 1/3 ..
    sub     ta, next, curr                  // 1   calculate delta
    mov     tb, pos, lsr #24                // 1   get top 8 bits of position fraction
    mul     ta, tb, ta                      // 2   multiply position * delta
    add     \target, ta, curr, lsl #8       // 1   add base sample to product (shifted to 16 bits)

.if \double != 0
    adds    pos, pos, rate, lsl #32 - SFRAC // 1   add rate FRACTION to position FRACTION
    movcs   curr, next                      // 1   load new sample on overflow
    ldrsbcs next, [src, #1]!                // 1/3 ..
    sub     ta, next, curr                  // 1   calculate delta
    and     tb, r14, pos, lsr #8            // 1   get top 8 bits of position fraction, shifted into top hword
    mla     \target, tb, ta, \target        // 3   multiply position * delta, add to target
    add     \target, \target, curr, lsl #24 // 1   add base sample to product

.endif
.endm
//------------------------------------------------------------------------------------

    resample_data   mb_buildw8, mb8_exit, 1, 1

//------------------------------------------------------------------------------------
mb8_exit:
//------------------------------------------------------------------------------------

    sub     src, #1
mb8_exit2:
    movs    pos, pos, lsr #32 - SFRAC       // shift position back to normal
    add     pos, pos, rate
mb8n_exit:
    pop     {m1, m2}                        // pop old position, starting src

    sub     m2, src, m2                     // get difference

    add     pos, pos, m1, lsl #SFRAC        // add old position
    add     pos, pos, m2, lsl #SFRAC        // add src difference

    pop     {src, r12, pc}                  // return

//*********************************************************
zerofill_buffer:
//*********************************************************
// destination size must be a multiple of 32 samples

// **todo: complete zerofill not neccesary??

    push    {r10-r12, lr}
    mov     dest, r0
    mov     count, #128
    mov     m1, #0
    mov     m2, #0
    mov     m3, #0
    mov     m4, #0
1:  stmia   dest!, {m1-m4}
    stmia   dest!, {m1-m4}
    stmia   dest!, {m1-m4}
    stmia   dest!, {m1-m4}
    subs    count, #32
    bne     1b

    pop     {r10-r12, pc}

//--------------------------------------------------------
calc_mixcount: // { a, b, amount }
//--------------------------------------------------------
// returns r2 = sample count

    mov     r3, #0
    cmp     r2, r0
    movhi   r2, r0
    movhi   r3, #1

    cmp     r2, r1
    movhi   r2, r1
    bhi     _value_clipped

    cmp     r3, #0
    beq     _value_unclipped
_value_clipped:

    mov     r0, r2
    push    {lr}
    bl      div19
    pop     {lr}
    b       1f
_value_unclipped:
    mov     r0, count
1:  sub     count, r0
    push    {count}
    mov     count, r0
    bx      lr

//------------------------------------------------------------------------------------
translateVolume: // { volume 0..65535 }
//------------------------------------------------------------------------------------
    ldr     r3, =mmVolumeTable
    ldrb    r1, [r3, r0, lsr #7 + 5]    // r1 = shift data
    add     r2, r0, #16 << (7 + 5)
    ldrb    r2, [r3, r2, lsr #7 + 5]    // r2 = shift level
    add     r2, #5                      // ***add this to the table values instead
    movs    r1, r1, lsl #8              // assemble data
    orr     r0, r1, r0, lsr r2
    bx      lr                          // return

//*********************************************************
mmMixC:
//*********************************************************
    stmfd   sp!, {r4-r11, lr}

    mov     r7, r0
    mov     r10, r1
    mov     r12, r2

    ldr     r11, =mm_mix_data + MC_SHADOW
    mov     r8, #1

    tst     r10, r8                     // test channel bit and update if bit set
    beq     .mmc_next                   //

//--------------------------------------------------
.mmc_update_loop:
//--------------------------------------------------

    ldr     r1, [r12, #C_SAMP]          // read sample address
    bics    r6, r1, #0xFF000000         // channel is disabled if address == 0
    beq     .mmc_disabled               //
    add     r6, r6, #0x2000000          // add wram offset to address

    ldrb    r1, [r12, #C_CNT]           // test and clear start bit
    tst     r1, #CF_START               //
    bic     r1, #CF_START               //
    strb    r1, [r12, #C_CNT]           //
    beq     .mmc_continue               // continue channel / start new note
                        //--------------------------------------

    ldrb    r4, [r12, #C_READ]          // shift sample offset (for swm only)
    mov     r0, r4, lsl#C_READ_FRAC + 8 // r4 = offset/256
    str     r0, [r12, #C_READ]          //

    ldrh    r0, [r12, #C_VOL]           // set direct volume levels on key-on
    ldrb    r1, [r12, #C_CNT]           //
    orr     r0, r0, r1, lsl #16 + 9     //
    str     r0, [r12, #C_CVOL]          //

    cmp     r8, #1 << 16                // skip the rest for software channels
    bcs     .mmc_next                   //

    cmp     r4, #0                      // convert sample offset into wordcount
    beq     1f                          //
    ldrb    r3, [r6, #C_SAMPLEN_FORMAT] //
    cmp     r3, #1                      //
    movgt   r4, #0                      // adpcm,else = 0
    moveq   r4, r4, lsl #(9 - 2)        // 16-bit = lsl#1
    movne   r4, r4, lsl #(8 - 2)        // 8-bit = lsl#0
1:  //--------------------------------------
    //add    r2, r6, #C_SAMPLEN_DATA      // r2 = source address (+offset)
    ldr     r2, [r6, #C_SAMPLEN_POINT]
    cmp     r2, #0
    addeq   r2, r6, #C_SAMPLEN_DATA

    add     r2, r2, r4, lsl #2          // r3 = loop start (-offset)
                                        // r4 = length
    ldrb    r3, [r6, #C_SAMPLEN_REP]    //
    cmp     r3, #1                      //
    bne     .mmc_nlooping               // if no sample loop ->
                                        //
    ldrh    r3, [r6, #C_SAMPLEN_LSTART] //
    subs    r3, r3, r4                  //
    addmi   r2, r2, r3, lsl#2           //-truncate offsets that enter looped region
    movmi   r3, #0                      //-
    ldr     r4, [r6, #C_SAMPLEN_LEN]    //
    //--------------------------------------

    str     r4, [r11, #MC_SH_LEN]       // write LEN to shadow

    b       .mmc_looping

.mmc_nlooping:

    ldr     r5, [r6, #C_SAMPLEN_LEN]    // r14 = length
    subs    r14, r5, r4                 //
    submi   r2, r2, r4, lsl #2          // cancel sample offset if greater than length
    movmi   r14, r5                     //

    mov     r3, #0

    str     r14, [r11, #MC_SH_LEN]      // write LEN to shadow

.mmc_looping:

    str     r2, [r11, #MC_SH_SRC]       // write new source+loop point data to shadow buffer
    strh    r3, [r11, #MC_SH_PNT]       //

    //------------------------------
    // set top 8 bits of CNT
    ldrh    r0, [r6, #C_SAMPLEN_FORMAT] //
    lsr     r1, r0, #8                  //-r1 = rep
    and     r0, #0xFF                   //-r0 = fmt
    orr     r1, r1, r0, lsl #2          //-combine
    lsl     r1, #3                      //-shift into place
    add     r1, #0x80                   //-add start bit
    strb    r1, [r11, #MC_SH_CNT + 3]   //
    //------------------------------

    b    .mmc_started

.mmc_continue:
//-----------------
// adjust pitch, volume, panning

    cmp     r8, #1 << 16
    bcs     .mmc_next
//.mmc_hw_ch:
//-------------

    ldr     r1, =mm_mix_data + MC_SHADOW    // GREAT HACK OF JUSTICE
    sub     r1, r11, r1                     //
    add     r1, #0x4000000                  //
    add     r1, #0x0000400                  //
    ldrb    r1, [r1, #3]                    //
    tst     r1, #128                        //
    beq     .mmc_disabled                   //

//    ldr     r1, [r11, #CSOUND_CNT]
//    tst     r1, #SOUNDxCNT_ENABLE
//
//    moveq   r1, #0
//    streq   r1, [r12, #C_SAMP]
//    beq     .mmc_next

.mmc_started:
//----------------------------------------------------------------

    ldr     r0, =CLK_DIV                // calc & set timer
    ldrh    r1, [r12, #C_FREQ]
    cmp     r1, #0
    moveq   r0, #0
    beq     1f
    swi     SWI_DIVIDE << 16
    neg     r0, r0
1:  strh    r0, [r11, #MC_SH_TMR]

    ldrh    r0, [r12, #C_CVOL]          // set volume
    bl      translateVolume             //
    strh    r0, [r11, #MC_SH_CNT]       //

    ldrh    r2, [r12, #C_CPAN]          // set panning
    lsr     r2, #9                      //
    strb    r2, [r11, #MC_SH_CNT + 2]   //

.mmc_next:

    add     r11, #MC_SH_SIZE            // point to next shadow channel
    add     r12, #C_SIZE                // point to next mixing channel
    movs    r8, r8, lsl #1              // shift channel bit
    beq     mmSoftwareMixingRoutine     // exit if past 32 channels
    tst     r10, r8                     // test ch mask with channel bit
    bne     .mmc_update_loop            // process if set
    b       .mmc_next                   // loop

.mmc_disabled:
    cmp     r8, #1 << 16
    bcs     .mmc_next                   // [do nothing]
    mov     r0, #0
    str     r0, [r11, #CSOUND_CNT]      // clear channel cnt
    str     r0, [r12, #C_SAMP]          // ***hope this works
    b       .mmc_next

.pool

/**************************************************************************************
 * mmVolumeTable
 *
 * LUT containing values to help convert a certain value into
 * value and shift amount for the hardware channels
 **************************************************************************************/
mmVolumeTable:
mmVolumeDivTable:
    // divider values
    .byte   3, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0

mmVolumeShiftTable:
// shift values
    .byte   0, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4

/**************************************************************************************
 * mmDoSoftwareMixingRoutine()
 *
 * Software mix extended channels into the streams.
 **************************************************************************************/
mmSoftwareMixingRoutine:

    ldr     r9, =mm_ch_mask             // r9 = software channel selection
    ldrh    r9, [r9, #2]                //

    ldr     r0, =REG_DMA                // setup dma destination
    ldr     r1, =mm_mix_data + MC_FETCH //
    str     r1, [r0, #0x4]              // 0x4=DMAxDAD

    bl      mmMixChunk

    ldmfd   sp!, {r4-r11, lr}           // return
    bx      lr

//---------------------------------------------
// software mix chunk
//---------------------------------------------
mmMixChunk:
//---------------------------------------------

// clear work buffer
//--------------------

    stmfd   sp!, {lr}                       // save mix count, channels, return

    ldr     r0, =mm_mix_data + MC_MIX_WMEM
    mov     r1, #0                          // clear work buffer
    mov     r2, r1, lsr #9                  //
    mov     r3, r1, lsr #15                 //
    mov     r4, r1, lsr #30                 //
    mov     r5, r1, lsr #2                  //
    mov     r6, r1, lsr #12                 //
    mov     r7, r1, lsr #21                 //
    mov     r8, r1, lsr #19                 //
    mov     r9, #MM_SW_CHUNKLEN / 16        //
.clear_work_buffer:                         //
    stmia   r0!, {r1-r8}                    //
    stmia   r0!, {r1-r8}                    //
    subs    r9, #1                          //
    bne     .clear_work_buffer              //

// register list :)
#define rch   r12   // channel data
#define cbits r11   // active channel bits
#define rsamp r10   // sample pointer
#define cvol  r8    // channel volume
#define sfreq r7    // sampling frequency
#define mixc  r6    // mix count
#define mdest r5    // mixing destination (work buffer)
#define sread r4    // sampling position
// r0-r3,lr : free

    ldr     rch, =mm_mix_channels + 16 * C_SIZE // point to first software channel
    //ldr     cbits, [sp, #4]                   // read channel bits
    ldr     cbits, =mm_ch_mask
    ldrh    cbits, [cbits, #2]

    mov     r0, #0
    str     r0, volume_addition

    movs    cbits, cbits, lsr #1                // shift out channel bit
    bcc     .ch_next                            // NEXT if not set ------>

//-------------------------
.ch_loop:
//-------------------------

//--------------------------------------------
    ldr     rsamp, [rch, #C_SAMP]               // read sample entry
    bics    rsamp, #0xFF000000                  // mask sample address
    beq     .ch_next
    add     rsamp, #0x02000000                  // add ewram offset

// calculate volume
//-----------------------------------------------
    ldrh    r0, [rch, #C_CPAN]              // r0 = pan: 0..127
//    add     r0, #1                        //
    lsr     r0, #9                          //
    ldrh    r1, [rch, #C_CVOL]              // r1 = vol: 0..2047
    lsr     r1, #5                          //

    mul     r9, r0, r1                      // calc right vol
    mov     r9, r9, lsr #10                 // 18->8 bit
    rsb     r0, r0, #128
    mul     cvol, r0, r1                    // calc left vol
    movs    cvol, cvol, lsr #10             // 18->8 bit
    orrs    cvol, cvol, r9, lsl #16

    ldrne   r0, volume_addition             // add up values
    addne   r0, r0, cvol
    strne   r0, volume_addition

    //ldr     mixc, [sp]                          // load mix count
    mov     mixc, #MM_SW_CHUNKLEN               //
    ldr     mdest, =mm_mix_data + MC_MIX_WMEM   // destination [work buffer]

    ldrh    sfreq, [rch, #C_FREQ]           // read frequency
    ldr     r0, =49212                      // adjust scale (32khz -> 21khz)
    mul     sfreq, r0, sfreq                //
    lsr     sfreq, #15                      //

    ldr     sread, [rch, #C_READ]           // get sample position
    ldmia   rsamp, {r1, r2}                 // read loop start, loop length
    add     r3, r1, r2                      // sample length (words)

// time to mix the segment...
// mix count is <samples>

//------------------------------------------------------------------------
.mix_segment:
//------------------------------------------------------------------------

    mul     r2, mixc, sfreq                 // multiply mix count
    ldrb    r0, [rsamp, #C_SAMPLEN_FORMAT]  // read format
    mov     lr, #0                          // flag for later [shortmix]
    cmp     r0, #0                          // 8-bit?
    beq     .mix_8bit
    cmp     r0, #1                          // 16-bit?
    beq     .mix_16bit

    // ima-adpcm: will crash :D

//=============================================================================
.macro calc_lengths fshift, lshift
// fshift = fetch shift (amount to shift MM_FETCH by when comparing)
// lshift = shift amount to convert words to samples (4bit=3,8bit=2,16bit=1)
//-----------------------------------------------------------------------------
    cmp     r2, #MC_FETCH_SIZE << \fshift       // test if samples required
    movhi   r2, #MC_FETCH_SIZE << \fshift       // are larger than fetch & clamp
    movhi   lr, #1
//----------------------------
    rsb     r0, sread, r3, lsl #(10 + \lshift)  // test if fetch will exceed
    cmp     r2, r0                              // sample length
    movhi   r2, r0                              // & clamp
    bhi     1f
//----------------------------
    cmp     lr, #0                              // check if flag was set
    beq     2f
1:  mov     r0, r2                              // divide samples/ freq
    bl      div19                               // to get new mix count
    b       1f
2:  mov     r0, mixc                            // mix full amount
1:  sub     mixc, mixc, r0                      // subtract from total count
    stmfd   sp!, {mixc}                         // preserve remaining counter
    mov     mixc, r0                            // move

.endm
//----------------------------------------------------------
.macro copy_and_mix sh, mixer, eb, restart
//----------------------------------------------------------
// sh = shift [1=16bit]
// mixer = mixing function
// eb = ending branch [0 = skip branch to .ch_next]
// restart = label to jump to on remix
//----------------------------------------------------------
    ldr     r0, =REG_DMA

    ldr     r1, [rsamp, #C_SAMPLEN_POINT]
    cmp     r1, #0
    addeq   r1, rsamp, #C_SAMPLEN_DATA      // r1 = sample data
    add     r1, r1, sread, lsr #10 - \sh    // add read position (*2 if 16bit)
    bic     r1, #0b11                       // 32-bit alignment
    str     r1, [r0, #0]                    // write to DMA_SAD
//---------------------------------
    mov     r1, #DMA_ENABLE | DMA_32BIT
    add     r2, r2, #8192                   // add threshold for safety
    add     r1, r1, r2, lsr #10 + 2 - \sh   // # of words
    str     r1, [r0, #8]                    // write to CNT [start dma]

    // [bus locked while dma is active]

    bl      \mixer                          // jump to mixing routine

    ldmia   rsamp, {r0, r1}                 // get length of sample and compare
    add     r3, r0, r1                      // read position with it
    cmp     sread, r3, lsl #10 + 2 -\sh
    bcc     2f
    ldrb    r0, [rsamp, #C_SAMPLEN_REP]     // read loop type
    cmp     r0, #1
    beq     1f

// one shot (stop sample!)
//--------------------------------
    mov     sread, #0                       // reset read
    str     sread, [rch, #C_SAMP]           // clear sample

    ldmfd   sp!, {mixc}                     // read mixc
    mov     sfreq, #0                       // zero freq
    ldr     r1, =mm_mix_data + MC_FETCH     // write zero sample
    str     sread, [r1]
    bl      mm_mix_pcm8                     // mix empty data

    b       3f
1: // forward loop
//---------------------------------
    // r1 = loop length
    // subtract from read position
    sub     sread, sread, r1, lsl #10 + 2 - \sh
2:
    ldmfd   sp!, {mixc}
    cmp     mixc, #0                    // mix more samples?
    beq     3f                          // yes:
    mul     r2, mixc, sfreq             //   get samps * freq
    mov     r14, #0                     //   clear flag
                                        //   r3 is samp length
    b       \restart
3:  str     sread, [rch, #C_READ]       // save read position
.if \eb == 1
    b       .ch_next
.endif
.endm
//-------------------------------------------------------------

//-----------------------------------
.mix_16bit:
//-----------------------------------

    calc_lengths    9, 1
    copy_and_mix    1, mm_mix_pcm16, 1, .mix_16bit

//-----------------------------------
.mix_8bit:
//-----------------------------------

    calc_lengths    10, 2
    copy_and_mix    0, mm_mix_pcm8, 0, .mix_8bit

.ch_next:
    add     rch, rch, #C_SIZE
    movs    cbits, cbits, lsr#1
    bcs     .ch_loop
    bne     .ch_next

    ldr     r1, =mm_mix_data + MC_MIX_WMEM      // r1 = pointer to work buffer
    ldr     r3, =mm_mix_data + MC_MIX_OUTPUT    // r2 = pointer to output
    ldr     r2, =mm_output_slice                // r2 += slice * buffersize/2*2
    ldrb    r2, [r2]                            //
    cmp     r2, #0                              //
    addne   r3, #MM_SW_CHUNKLEN * 2             //
    mov     r7, #MM_SW_BUFFERLEN * 2            // r7 = offset to second wave buffer
    mov     r10, #0x10000                       // r10 = 0xFFFF (for masking samples)
    sub     r10, #1                             //
    eor     r6, r10, #0x8000                    // r6 = 0x7FFF (for clamping samples)

    ldr     r4, volume_addition                 // parse volume_addition data
    mov     r14, r4, lsr #16                    // r14 = right volume
    mov     r14, r14, lsl #3                    //
    mov     r4, r4, lsl #16                     // r4 = left volume
    mov     r4, r4, lsr #16 + 1 - 4             //

    mov     r0, #MM_SW_CHUNKLEN

.macro clamp_s s
//---------------------
    cmp     \s, r6
    movgt   \s, r6
    cmn     \s, r6
    rsblt   \s, r6, #0
.endm
.macro cad s                            // convert and assemble data
//------------------------
    sub     r11, \s, r4                 // convert to signed
    mov     r11, r11, lsl #16           // sign extend
    mov     r11, r11, asr #12           // shift to 16-bit
    clamp_s r11                         // clamp
    rsb     r12, lr, \s, lsr #16        // convert to signed
    mov     r12, r12, lsl #4            // conv to 16-bit
    clamp_s r12                         // clamp
.endm
//-------------------------

    // mix16 format is 12-bit

.output_mix:
    ldmia   r1!, {r8, r9}               // read 2 samples
    cad     r8
    strh    r12, [r3, r7]               // write to output [right]
    strh    r11, [r3], #2
    cad     r9
    strh    r12, [r3, r7]               // write to output [left]
    strh    r11, [r3], #2
    subs    r0, r0, #2                  // count
    bne     .output_mix                 // loop

    ldr     r1,=mm_output_slice         // swap mixing slice
    ldrb    r2, [r1]                    //
    eor     r2, #1                      //
    strb    r2, [r1]                    //

    ldmfd   sp!, {pc}                   // return

volume_addition:
    .space 4

//==========================================================================================

#define m1 r0
#define m2 r1
#define m3 r2
#define m4 r3
#define m5 r9
#define m6 r11
#define m7 r12
#define m8 r14

#define rsrc rsamp // (r10 current)
#define smp1 m8

/****************************************************************************
 * mm_mix_pcm8(*)
 *
 * very internal function to mix 8bit data into the work buffer
 ****************************************************************************/
mm_mix_pcm8:

    stmfd   sp!, {rsamp, cbits, rch, lr}    // preserve regs

    cmp     cvol, #0                        // if volume == 0:
    muleq   r0, sfreq, mixc                 //   then skip mixing
    addeq   sread, sread, r0                //   add freq*samples to read position
    popeq   {rsamp, cbits, rch, pc}         //   return

    ldr     rsrc,=mm_mix_data+MC_FETCH      // load rsrc with fetch pointer
    mov     r0, sread, lsr #10              // get read position integer
    sub     sread, sread, r0, lsl #10       // clear integer in read
    and     r1, r0, #0b11                   // mask low 2 bits
    add     rsrc, rsrc, r1                  // add to fetch offset
    stmfd   sp!, {r0}                       // save the old integer value for later

//-----------------------------------------------------------------------
.macro mix8w sa
//-----------------------------------------------------------------------
    ldrb    smp1, [rsrc, sread, lsr #10]    // 3
    add     sread, sfreq                    // 1
    eor     smp1, #0x80                     // 1 unsign sample
    mul     smp1, cvol, smp1                // 2 multiply by volume (both left and right)
    bic     smp1, #0x0F0000                 // 1 prepare for shift
    add     \sa, \sa, smp1, lsr #4          // 1 add shifted value to mix
.endm                                       // 9
//-----------------------------------------------------------------------

// mix large chunks
//--------------------------------------
    subs    mixc, mixc, #7
    bmi     .mp8a_mixn_exit                         // skip if too small

.mp8a_mixn:

    ldmia   mdest, {m1, m2, m3, m4, m5, m6, m7}     // 9 read words [7 samples]
    mix8w   m1                                      // 9 mix data
    mix8w   m2                                      // 9
    mix8w   m3                                      // 9
    mix8w   m4                                      // 9
    mix8w   m5                                      // 9
    mix8w   m6                                      // 9
    mix8w   m7                                      // 9
    stmia   mdest!, {m1, m2, m3, m4, m5, m6, m7}    // 8 write words
    subs    mixc, mixc, #7                          // 1 count samples
    bpl     .mp8a_mixn                              // 3 loop if still remaining
                                                    // 84 cycles    [12/s]
.mp8a_mixn_exit:
//--------------------------------------------------------------------------

// mix the leftover samples
//------------------------------------

    adds    mixc, mixc, #7                  // fix counter
    beq     .mp8a_mix1_exit                 // exit if zero
.mp8a_mix1:
//-------------------------------------------------
    ldr     m1, [mdest]                     // read 1 sample
    mix8w   m1                              // mix data
    str     m1, [mdest], #4                 // write 1 sample
    subs    mixc, mixc, #2                  // subtract 2 from count
    bmi     .mp8a_mix1_exit                 // exit if negative
    ldr     m1, [mdest]                     // read 1 more sample
    mix8w   m1                              // mix data
    str     m1, [mdest], #4                 // write 1 more sample
    bne     .mp8a_mix1                      // loop if above result wasn't zero
//-------------------------------------------------
.mp8a_mix1_exit:

    ldmfd   sp!, {r0}                       // add old integer
    add     sread, sread, r0, lsl #10       // to read position
    ldmfd   sp!, {rsamp, cbits, rch, pc}    // pop stuff and return
//-----------------------------------------------------------------------------------

/************************************************************************************
 * mm_mix_pcm16(*)
 *
 * very internal function to mix 16-bit data into the work buffer
 ************************************************************************************/
mm_mix_pcm16:

    stmfd   sp!, {rsamp, cbits, rch, lr}

    cmp     cvol, #0                        // skip mixing if volume is zero
    muleq   r0, sfreq, mixc                 //
    addeq   r0, sfreq, r0                   // (ie just add mix * freq to position)
    popeq   {rsamp, cbits, rch, pc}         //

    ldr     rsrc, =mm_mix_data + MC_FETCH   // point to fetch
    mov     r0, sread, lsr #10              // get read integer
    sub     sread, sread, r0, lsl #10       // clear integer in read
    and     r1, r0, #1                      // mask low bit
    add     rsrc, rsrc, r1, lsl #1          // add offset to fetch pointer
    stmfd   sp!, {r0}
    add     rsrc, rsrc, #1                  // we will only use the high byte of each sample :\

//-----------------------------------------------------------------------
.macro    mix16w    sa
//-----------------------------------------------------------------------
    mov     smp1, sread, lsr #10            // 1 get read integer
    ldrb    smp1, [rsrc, smp1, lsl #1]      // 3 load byte from src + read * 2
    add     sread, sfreq                    // 1 add rate to position
    eor     smp1, #0x80                     // 1 unsign sample :(
    mul     smp1, cvol, smp1                // 2 multiply by volume (both left and right)
    bic     smp1, #0x0F0000                 // 1 prepare for shift
    add     \sa, \sa, smp1, lsr #4          // 1 add shifted value to mix
.endm                                       // 10 [1 more than 8-bit]
//-----------------------------------------------------------------------

    subs    mixc, mixc, #7
    bmi     .mp16_mixn_exit
.mp16_mixn:
//-----------------------------------------------------------------
    ldmia    mdest, {m1, m2, m3, m4, m5, m6, m7}    // 9  read 7 mix-samples
    mix16w   m1                                     // 10 mix data
    mix16w   m2                                     // 10
    mix16w   m3                                     // 10
    mix16w   m4                                     // 10
    mix16w   m5                                     // 10
    mix16w   m6                                     // 10
    mix16w   m7                                     // 10
    stmia    mdest!, {m1, m2, m3, m4, m5, m6, m7}   // 8  write 7 mix-samples
    subs     mixc, mixc, #7                         // count N samples
    bpl     .mp16_mixn                              // loop if still remaining
//-------------------------------------------------------------------
.mp16_mixn_exit:

    adds    mixc, mixc, #7                  // fix count
    beq     .mp16_mix1_exit                 // exit if zero

.mp16_mix1:                                 // mix single samples until finished
//-------------------------------------------------------------------
    ldmia   mdest, {m1}                     // mix single sample
    mix16w  m1
    stmia   mdest!, {m1}
    subs    mixc, mixc, #2                  // subtract 2
    bmi     .mp16_mix1_exit                 // exit if negative
    ldmia   mdest, {m1}                     // mix one more sample
    mix16w  m1
    stmia   mdest!, {m1}
    bne     .mp16_mix1                      // loop if not zero
//--------------------------------------------------------
.mp16_mix1_exit:

    ldmfd   sp!, {r0}                       // pop start, offset
    add     sread, sread, r0, lsl #10       // add old read integer
    ldmfd   sp!, { rsamp, cbits, rch, pc}   // pop stuff and return
//-----------------------------------------------------------------------------------

/************************************************************************************
 * div19(num, denom)
 *
 * divide function, 40..68 cycles, 66 words of code
 * result is rounded upward
 *
 * denom is hacked to be "rate"
 ************************************************************************************/
div19:

    mov     r3, #0
    cmp     r0, rate, lsl #10       // speed hack
    bcc     1f

.macro div_iter s

    cmp     r0, rate, lsl #\s       // 3 cycles
    subcs   r0, r0, rate, lsl #\s   //
    addcs   r3, r3, #1 << \s        //

.endm
    div_iter 19
    div_iter 18
    div_iter 17
    div_iter 16
    div_iter 15
    div_iter 14
    div_iter 13
    div_iter 12
    div_iter 11
    div_iter 10
1:  div_iter 9
    div_iter 8
    div_iter 7
    div_iter 6
    div_iter 5
    div_iter 4
    div_iter 3
    div_iter 2
    div_iter 1
    div_iter 0

    cmp     r0, #1                  // round up result
    adc     r0, r3, #0

    bx      lr
