// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MM_COMMS9_H__
#define MM_COMMS9_H__

#include <mm_types.h>

void mmSendBank(mm_word num_songs, mm_word num_samples, mm_addr bank_addr);
void mmSetupComms(mm_word);

#endif
