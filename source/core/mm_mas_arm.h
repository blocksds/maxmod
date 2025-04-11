// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_MAS_ARM_H
#define MM_MAS_ARM_H

#include "mm_types.h"
#include "mp_mas_structs.h"

mm_word mmAllocChannel(void);
mm_word mmGetPeriod(mpl_layer_information*, mm_word, mm_byte);
void mmReadPattern(mpl_layer_information*);

extern mm_word IT_PitchTable[];
extern mm_hword ST3_FREQTABLE[];

#endif // MM_MAS_ARM_H
