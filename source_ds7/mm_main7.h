// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MM_MAIN7_H__
#define MM_MAIN7_H__

#include "mm_types.h"

void mmLockChannelsQuick(mm_word);
void mmUnlockChannelsQuick(mm_word);
void mmGetSoundBank(mm_word, mm_addr);

#define NUM_CHANNELS 32
#define NUM_PHYS_CHANNELS 16
#define ALL_PHYS_CHANNELS_MASK ((1<<NUM_PHYS_CHANNELS)-1)
#define ALL_SOFT_CHANNELS_MASK (~ALL_PHYS_CHANNELS_MASK)

#endif
