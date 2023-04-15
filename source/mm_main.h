// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_MAIN_H
#define MM_MAIN_H

#include "mm_types.h"

extern mm_word mm_ch_mask;
extern mm_callback mmCallback;
extern mm_word mmModuleCount;
extern mm_addr mmModuleBank;
extern mm_addr mmSampleBank;
extern mm_voidfunc mm_vblank_function;
extern mm_byte mmInitialized;

#endif // MM_MAIN_H
