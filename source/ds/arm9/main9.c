// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <maxmod9.h>
#include <mm_types.h>

#include "ds/arm9/comms9.h"
#include "ds/arm9/main9.h"

#define FIFO_MAXMOD 3

mm_bool mmTryToInitializeDefault(mm_word);

// Function pointer to soundbank operation callback
mm_callback mmcbMemory;

// Number of modules in sound bank
mm_word mmModuleCount;

// Number of samples in sound bank
mm_word mmSampleCount;

// Address of bank in memory. It contains the module bank followed by the sample
// bank.
mm_addr mmMemoryBank;

// This is a pointer to an array of pointers. Each pointer points to the memory
// that holds a module in main RAM, or to NULL if the module hasn't been loaded
// by mmLoad().
mm_addr *mmModuleBank;

// Same as mmModuleBank, but for samples instead of modules. It should start
// right after the end mmModuleBank. However, only the bottom 24 bits hold the
// address (minus 0x2000000). The top 8 bit are the number of times that the
// sample has been requested to be loaded (in case a sample is used by multiple
// modules).
mm_word *mmSampleBank;

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
    mmModuleBank = (mm_addr *)system->mem_bank;
    mmSampleBank = (mm_word *)(system->mem_bank + mmModuleCount);

    for (mm_word i = 0; i < mmModuleCount; i++)
        mmModuleBank[i] = NULL;

    for (mm_word i = 0; i < mmSampleCount; i++)
        mmSampleBank[i] = 0;

    // Setup communications
    mmSetupComms(system->fifo_channel);

    // Send memory bank info to ARM7. We also need to send the number of songs
    // and samples because the soundbank isn't loaded to RAM if it is stored in
    // NitroFS. This means that the first word of the header (that contains the
    // number of songs and samples) isn't available for the ARM7 to read it.
    // This word is needed to know the sizes of the module bank and sample bank.
    mmSendBank(mmModuleCount, mmSampleCount, system->mem_bank);

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
