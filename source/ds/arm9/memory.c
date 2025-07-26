// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <nds.h>

#include <maxmod9.h>
#include <mm_mas.h>
#include <mm_types.h>

#include "core/format_mas.h"
#include "ds/arm9/main9.h"

#define BASE_SAMPLE_ADDRESS 0x2000000

static void mmFlushBank(void)
{
    DC_FlushRange(mmMemoryBank, (mmModuleCount + mmSampleCount) * 4);
}

// Load a module into memory. Must be used before starting
// a module.
// module_ID : ID of module. (defined in soundbank header)
void mmLoad(mm_word module_ID)
{
    mm_word *mmModuleBankArr = (mm_word*)mmModuleBank;

    // Check if Free
    if (mmModuleBankArr[module_ID] != 0)
        return;

    // Load module into memory
    mm_addr ptr = (mm_addr)mmcbMemory(MMCB_SONGREQUEST, module_ID);
    mmModuleBankArr[module_ID] = (mm_word)ptr;

    mas_header *header = (mas_header *)(mmModuleBankArr[module_ID] + 8);

    mm_word *sample_table = &header->tables[header->inst_count];

    // Load samples
    for (mm_word i = 0; i < header->samp_count; i++)
        mmLoadEffect(((mm_mas_sample_info*)(sample_table[i] + ((mm_word)header)))->msl_id);

    // Flush the Bank
    mmFlushBank();
}

// Unload a module from memory.
// module_ID : ID of module. (defined in soundbank header)
void mmUnload(mm_word module_ID)
{
    mm_word *mmModuleBankArr = (mm_word*)mmModuleBank;

    // Check existence
    if (mmModuleBankArr[module_ID] == 0)
        return;

    mas_header *header = (mas_header *)(mmModuleBankArr[module_ID] + 8);

    mm_word *sample_table = &header->tables[header->inst_count];

    // Free samples
    for (mm_word i = 0; i < header->samp_count; i++)
        mmUnloadEffect(((mm_mas_sample_info*)(sample_table[i] + ((mm_word)header)))->msl_id);

    // Free module
    mmcbMemory(MMCB_DELETESONG, mmModuleBankArr[module_ID]);
    mmModuleBankArr[module_ID] = 0;

    // Flush the Bank
    mmFlushBank();
}

// Load a sound effect into memory. Use before mmEffect.
// sample_ID : ID of sample. (defined in soundbank header)
void mmLoadEffect(mm_word sample_ID)
{
    mm_word *mmSampleBankArr = (mm_word*)mmSampleBank;

    mm_word sample_data = mmSampleBankArr[sample_ID];

    // Alloc if not existing
    if (sample_data == 0)
    {
        mm_addr ptr = (mm_addr)mmcbMemory(MMCB_SAMPREQUEST, sample_ID);
        mmSampleBankArr[sample_ID] = ((mm_word)ptr) & 0x00FFFFFF;
    }

    // Increment instance count
    mmSampleBankArr[sample_ID] += 1 << 24;

    // Flush the Bank
    mmFlushBank();
}

// Unload sound effect from memory.
// sample_ID : ID of sample. (defined in soundbank header)
void mmUnloadEffect(mm_word sample_ID)
{
    mm_word *mmSampleBankArr = (mm_word*)mmSampleBank;

    mm_word sample_data = mmSampleBankArr[sample_ID];

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

    mmSampleBankArr[sample_ID] = sample_data;

    // Flush the Bank
    mmFlushBank();
}
