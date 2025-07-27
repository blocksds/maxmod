// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <nds.h>

#include <maxmod9.h>
#include <mm_mas.h>
#include <mm_types.h>

#include "ds/arm9/main9.h"

#define BASE_SAMPLE_ADDRESS 0x2000000

static void mmFlushMemoryBank(void)
{
    DC_FlushRange(mmMemoryBank, (mmModuleCount + mmSampleCount) * 4);
}

// Load a module into memory. Must be used before starting
// a module.
// module_ID : ID of module. (defined in soundbank header)
void mmLoad(mm_word module_ID)
{
    // Check if Free
    if (mmModuleBank[module_ID] != NULL)
        return;

    // Load module into memory
    mm_addr ptr = (mm_addr)mmcbMemory(MMCB_SONGREQUEST, module_ID);
    mmModuleBank[module_ID] = ptr;

    mm_mas_head *header = (mm_mas_head *)(mmModuleBank[module_ID] + 8);

    mm_word *sample_table = (mm_word *)&header->tables[header->instr_count];

    // Load samples
    for (mm_word i = 0; i < header->sampl_count; i++)
        mmLoadEffect(((mm_mas_sample_info*)(sample_table[i] + ((mm_word)header)))->msl_id);

    mmFlushMemoryBank();
}

// Unload a module from memory.
// module_ID : ID of module. (defined in soundbank header)
void mmUnload(mm_word module_ID)
{
    // Check existence
    if (mmModuleBank[module_ID] == NULL)
        return;

    mm_mas_head *header = (mm_mas_head *)(((mm_word)mmModuleBank[module_ID]) + 8);

    mm_word *sample_table = (mm_word *)&header->tables[header->instr_count];

    // Free samples
    for (mm_word i = 0; i < header->sampl_count; i++)
        mmUnloadEffect(((mm_mas_sample_info*)(sample_table[i] + ((mm_word)header)))->msl_id);

    // Free module
    mmcbMemory(MMCB_DELETESONG, (mm_word)mmModuleBank[module_ID]);
    mmModuleBank[module_ID] = NULL;

    mmFlushMemoryBank();
}

// Load a sound effect into memory. Use before mmEffect.
// sample_ID : ID of sample. (defined in soundbank header)
void mmLoadEffect(mm_word sample_ID)
{
    mm_word sample_data = mmSampleBank[sample_ID];

    // Alloc if not existing
    if (sample_data == 0)
    {
        mm_word ptr = mmcbMemory(MMCB_SAMPREQUEST, sample_ID);
        mmSampleBank[sample_ID] = ptr & 0x00FFFFFF;
    }

    // Increment instance count
    mmSampleBank[sample_ID] += 1 << 24;

    mmFlushMemoryBank();
}

// Unload sound effect from memory.
// sample_ID : ID of sample. (defined in soundbank header)
void mmUnloadEffect(mm_word sample_ID)
{
    mm_word sample_data = mmSampleBank[sample_ID];

    // Check existence
    if (sample_data == 0)
        return;

    // Decrement instance count
    sample_data -= 1 << 24;

    // If there are no more instances left, delete it
    if ((sample_data & 0xFF000000) == 0)
    {
        mmcbMemory(MMCB_DELETESAMPLE, (sample_data & 0x00FFFFFF) + BASE_SAMPLE_ADDRESS);
        sample_data = 0;
    }

    mmSampleBank[sample_ID] = sample_data;

    mmFlushMemoryBank();
}
