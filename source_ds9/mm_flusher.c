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
