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

    .text
    .thumb
    .align 2

.global mpp_call_r7, mpp_call_r1, mpp_call_r2, mpp_call_r3

    .thumb_func
mpp_call_r7:
    bx r7

    .thumb_func
mpp_call_r1:
    bx r1

    .thumb_func
mpp_call_r2:
    bx r2

    .thumb_func
mpp_call_r3:
    bx r3

.end
