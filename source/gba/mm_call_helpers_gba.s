// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

/******************************************************************************
 *
 * Program
 *
 ******************************************************************************/

    .syntax unified

/******************************************************************************
 * mpp_call_*
 *
 * Functions to branch to a register
 ******************************************************************************/

    .section ".iwram", "ax", %progbits
    .thumb
    .align 2

    .global mpp_call_r7i, mpp_call_r2i, mpp_call_r1i, mpp_call_r3i

    .thumb_func
mpp_call_r7i:
    bx r7

    .thumb_func
mpp_call_r2i:
    bx r2

    .thumb_func
mpp_call_r1i:
    bx r1

    .thumb_func
mpp_call_r3i:
    bx r3

.end
