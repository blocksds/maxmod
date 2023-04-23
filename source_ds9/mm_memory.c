// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "maxmod9.h"
#include "mm_main9.h"
#include "mm_flusher.h"
#include "mp_defs.h"
#include "mp_format_mas.h"
#include "mm_mas.h"
#include "mm_types.h"

#define BASE_SAMPLE_ADDRESS 0x2000000

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

    mas_header *header = (mas_header*)(mmModuleBankArr[module_ID] + 8);

    mm_word instn_size = header->instn;

    mm_word *sample_table = &header->tables[instn_size];

    // Load samples
    for (mm_word i = 0; i < header->sampn; i++)
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

    mas_header *header = (mas_header*)(mmModuleBankArr[module_ID] + 8);

    mm_word instn_size = header->instn;

    mm_word *sample_table = &header->tables[instn_size];

    // Free samples
    for (mm_word i = 0; i < header->sampn; i++)
        mmUnloadEffect(((mm_mas_sample_info*)(sample_table[i] + ((mm_word)header)))->msl_id);

    // Free module
    mmModuleBankArr[module_ID] = 0;
    mmcbMemory(MMCB_DELETESONG, mmModuleBankArr[module_ID]);

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
    if ((sample_data & 0xFF000000) == 0)
    {
        sample_data = 0;
    }
    else
    {
        // Decrement instance count
        sample_data -= 1 << 24;

        if ((sample_data & 0xFF000000) == 0)
        {
            mmcbMemory(MMCB_DELETESAMPLE, (sample_data & 0x00FFFFFF) + BASE_SAMPLE_ADDRESS);
            sample_data = 0;
        }
    }

    mmSampleBankArr[sample_ID] = sample_data;

    // Flush the Bank
    mmFlushBank();
}
