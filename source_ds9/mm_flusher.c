// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <nds.h>

#include "mm_main.h"
#include "mm_main9.h"

void mmFlushBank(void)
{
    DC_FlushRange(mmMemoryBank, (mmModuleCount + mmSampleCount) * 4);
}

// Wait until a byte magically becomes 'wanted_value'.
ARM_CODE void WaitUntilValue(mm_addr address, mm_byte wanted_value)
{
    uintptr_t addr = (uintptr_t)address;
    while (((mm_byte*)addr)[0] != wanted_value)
        CP15_CleanAndFlushDCacheEntry(addr & (~31));
}
