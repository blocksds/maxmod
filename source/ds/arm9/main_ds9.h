// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MM_DS_ARM9_MAIN9_H__
#define MM_DS_ARM9_MAIN9_H__

#include <mm_types.h>

extern mm_word mmModuleCount;
extern mm_word mmSampleCount;

extern mm_addr mmMemoryBank;
extern mm_addr *mmModuleBank;
extern mm_word *mmSampleBank;

extern mm_callback mmcbMemory;
extern mm_callback mmCallback;
extern mm_byte mmActiveStatus;

#endif // MM_DS_ARM9_MAIN9_H__
