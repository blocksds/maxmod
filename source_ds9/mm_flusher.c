// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "mm_flusher.h"
#include "multiplatform_defs.h"
#include "useful_qualifiers.h"
#include "mm_main9.h"
#include <stddef.h>

void mmFlushBank(void) {
    FlushDataSize(mmMemoryBank, mmModuleCount + mmSampleCount);
}

// Flush address for size bytes
ARM_TARGET void FlushDataSize(mm_addr address, size_t size) {
    uintptr_t addr = (uintptr_t)address;
    
    if(size == 0)
        return;

    // Make sure this is 32 bytes-aligned
    CP15_CleanAndFlushDCacheEntry((addr & (~31)));
    
    if(size <= (32-(addr & 31)))
        return;
    
    // Skip the bytes we already covered
    size -= (32-(addr & 31));
    
    // Clean the various 32 bytes-aligned slates
    // Until all bytes are flushed
    for(size_t i = 0; i < (size + 32 - 1) / 32; i++)
        CP15_CleanAndFlushDCacheEntry((addr & (~31)) + ((i + 1) * 32));
}

// Wait until a byte magically becomes 'wanted_value'
ARM_TARGET void WaitUntilValue(mm_addr address, mm_byte wanted_value) {
    uintptr_t addr = (uintptr_t)address;
    while(((mm_byte*)addr)[0] != wanted_value)
        CP15_CleanAndFlushDCacheEntry(addr & (~31));
}
