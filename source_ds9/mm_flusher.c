// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "mm_flusher.h"
#include "multiplatform_defs.h"
#include "useful_qualifiers.h"
#include "mm_main9.h"
#include <stddef.h>

ARM_TARGET void mmFlushBank(void) {
    uintptr_t bank_addr = (uintptr_t)mmMemoryBank;
    for(size_t i = 0; i < ((mmModuleCount + mmSampleCount + 32 - 1 + (bank_addr & 31)) / 32); i++)
        CP15_CleanAndFlushDCacheEntry((bank_addr & (~31)) + (i * 32));
}
