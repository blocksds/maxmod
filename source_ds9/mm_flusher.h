// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MM_FLUSHER_H__
#define MM_FLUSHER_H__

#include <stddef.h>
#include "mm_types.h"

void mmFlushBank(void);
void FlushDataSize(mm_addr, size_t);
void WaitUntilValue(mm_addr, mm_byte);

#endif
