// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "maxmod9.h"
#include "mm_types.h"
#include "mm_main9.h"
#include "mm_comms.h"
#include "mm_comms9.h"

#define FIFO_MAXMOD 3

mm_bool mmTryToInitializeDefault(mm_word);

// Function pointer to soundbank operation callback
mm_callback mmcbMemory;

// Number of modules in sound bank
mm_word mmModuleCount;

// Number of samples in sound bank
mm_word mmSampleCount;

// Address of bank in memory
mm_addr mmMemoryBank;

// Address of module bank
mm_addr mmModuleBank;

// Address of sample bank
mm_addr mmSampleBank;

// Pointer to event handler
mm_callback mmCallback;

// Record of playing status.
mm_byte mmActiveStatus;

// Returns nonzero if module is playing
mm_bool mmActive(void)
{
    return mmActiveStatus;
}

// Set function for handling playback events
void mmSetEventHandler(mm_callback handler)
{
    mmCallback = handler;
}

// Initialize Maxmod (manual settings)
bool mmInit(mm_ds_system *system)
{
    mmModuleCount = system->mod_count;
    mmSampleCount = system->samp_count;
    mmMemoryBank = system->mem_bank;
    mmModuleBank = system->mem_bank;
    mmSampleBank = system->mem_bank + mmModuleCount;

    for (mm_word i = 0; i < mmModuleCount; i++)
        ((mm_word*)mmModuleBank)[i] = 0;

    for (mm_word i = 0; i < mmSampleCount; i++)
        ((mm_word*)mmSampleBank)[i] = 0;

    // Setup communications
    mmSetupComms(system->fifo_channel);

    // Send soundbank info to ARM7
    mmSendBank(mmModuleCount, mmModuleBank);

    return true;
}

// Shared initialization code for default setup
mm_bool mmTryToInitializeDefault(mm_word first_word)
{
    mm_ds_system system = { 0 };

    // The first word of the soundbank contains the number of samples followed
    // by the number of songs.
    system.samp_count = first_word & 0xFFFF;
    system.mod_count = (first_word >> 16) & 0xFFFF;

    system.fifo_channel = FIFO_MAXMOD;

    size_t size = (system.mod_count * sizeof(mm_word)) + (system.samp_count * sizeof(mm_word));
    system.mem_bank = calloc(size, 1);
    if (system.mem_bank == NULL)
        return false;

    if (!mmInit(&system))
    {
        free(system.mem_bank);
        return false;
    }

    return true;
}

// Initialize Maxmod with default setup
bool mmInitDefault(const char *soundbank_file)
{
    mm_word first_word;

    FILE *f = fopen(soundbank_file, "rb");
    if (f == NULL)
        return false;

    if (fread(&first_word, sizeof(first_word), 1, f) != 1)
    {
        fclose(f);
        return false;
    }

    if (fclose(f) != 0)
        return false;

    if (!mmTryToInitializeDefault(first_word))
        return false;

    mmSoundBankInFiles(soundbank_file);
    return true;
}

// Initialize Maxmod with default setup
// (when the entire soundbank is loaded into memory)
bool mmInitDefaultMem(mm_addr soundbank)
{
    mm_word first_word = ((mm_word*)soundbank)[0];

    if (!mmTryToInitializeDefault(first_word))
        return false;

    mmSoundBankInMemory(soundbank);
    return true;
}

mm_word mmGetModuleCount(void)
{
    return mmModuleCount;
}

mm_word mmGetSampleCount(void)
{
    return mmSampleCount;
}
