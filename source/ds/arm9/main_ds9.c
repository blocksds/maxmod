// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025-2026, Antonio Niño Díaz

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <maxmod9.h>
#include <mm_types.h>
#include <mm_msl.h>

#include "ds/arm9/comms_ds9.h"
#include "ds/arm9/main_ds9.h"

#define FIFO_MAXMOD 3

// Function pointer to soundbank operation callback
mm_callback mmcbMemory;

// Number of modules in sound bank
mm_word mmModuleCount;

// Number of samples in sound bank
mm_word mmSampleCount;

// Address of bank in memory. It contains the module bank followed by the sample
// bank.
mm_addr mmMemoryBank;

// This is a pointer to an array of pointers. Each pointer points to a module in
// MAS format stored in main RAM, or to NULL if the module hasn't been loaded
// by mmLoad().
mm_addr *mmModuleBank;

// Same as mmModuleBank, but for samples instead of modules. The MAS files it
// points to only contain one sample.
//
// mmSampleBank should start right after the end mmModuleBank. However, only the
// bottom 24 bits hold the address (minus 0x2000000). The top 8 bit are the
// number of times that the sample has been requested to be loaded (in case a
// sample is used by multiple modules).
mm_word *mmSampleBank;

// Pointer to event handler
mm_callback mmCallback;

// Pointers to the sample and module dictionaries
mm_word *mmSampleNameList = NULL;
mm_word *mmModuleNameList = NULL;
mm_bool mmNameListsAllocated = false; // Set to true if allocated with malloc()

// Set function for handling playback events
void mmSetEventHandler(mm_callback handler)
{
    mmCallback = handler;
}

// Get function for handling playback events
mm_callback mmGetEventHandler(void)
{
    return mmCallback;
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
static mm_bool mmTryToInitializeDefault(mm_word first_word)
{
    mm_ds_system system = { 0 };

    // The first word of the soundbank contains the number of samples followed
    // by the number of songs.
    system.samp_count = first_word & 0xFFFF;
    system.mod_count = (first_word >> 16) & 0xFFFF;

    system.fifo_channel = FIFO_MAXMOD;

    size_t size = (system.mod_count * sizeof(mm_word)) + (system.samp_count * sizeof(mm_word));
    if (size > 0)
    {
        system.mem_bank = calloc(size, 1);
        if (system.mem_bank == NULL)
            return false;
    }

    if (!mmInit(&system))
    {
        free(system.mem_bank);
        return false;
    }

    return true;
}

static void mmLoadDictionaryFromFile(const char *soundbank_file)
{
    mmSampleNameList = NULL;
    mmModuleNameList = NULL;

    FILE *f = fopen(soundbank_file, "rb");
    if (f == NULL)
        return;

    msl_head_data header;

    if (fread(&header, sizeof(header), 1, f) != 1)
        goto error;

    mm_word parapointer_offset = sizeof(msl_head)
                + sizeof(mm_word) * (header.sampleCount + header.moduleCount);

    if (fseek(f, parapointer_offset, SEEK_SET) != 0)
        goto error;

    mm_word parapointer;

    if (fread(&parapointer, sizeof(parapointer), 1, f) != 1)
        goto error;

    // Exit if no parapointer
    if (parapointer == 0xFFFFFFFF)
        goto error;

    if (fseek(f, parapointer, SEEK_SET) != 0)
        goto error;

    msl_names_dictionary dict_header;

    if (fread(&dict_header, sizeof(dict_header), 1, f) != 1)
        goto error;

    mmSampleNameList = malloc(dict_header.samplesDictSize);
    if (mmSampleNameList == NULL)
        goto error;

    mmModuleNameList = malloc(dict_header.modulesDictSize);
    if (mmModuleNameList == NULL)
        goto error;

    if (fread(mmSampleNameList, dict_header.samplesDictSize, 1, f) != 1)
        goto error;

    if (fread(mmModuleNameList, dict_header.modulesDictSize, 1, f) != 1)
        goto error;

    if (fclose(f) != 0)
        return;

    mmNameListsAllocated = true;
    return;

error:
    free(mmSampleNameList);
    free(mmModuleNameList);
    mmSampleNameList = NULL;
    mmModuleNameList = NULL;

    fclose(f);
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
    mmLoadDictionaryFromFile(soundbank_file);

    return true;
}

static void mmLoadDictionaryFromMemory(mm_addr soundbank)
{
    mmSampleNameList = NULL;
    mmModuleNameList = NULL;

    msl_head *mp_solution = soundbank;
    mm_word *offset = (mm_word *)&(mp_solution->sampleTable[mmSampleCount + mmModuleCount]);

    mm_word dictOffset = *offset;

    // Exit if there is no dictionary
    if (dictOffset == 0xFFFFFFFF)
        return;

    msl_names_dictionary *dict = (void *)(dictOffset + (uintptr_t)mp_solution);

    uintptr_t samples_address = (uintptr_t)dict + sizeof(msl_names_dictionary);
    uintptr_t modules_address = samples_address + dict->samplesDictSize;

    mmSampleNameList = (mm_word *)samples_address;
    mmModuleNameList = (mm_word *)modules_address;
    mmNameListsAllocated = false;
}

// Initialize Maxmod with default setup
// (when the entire soundbank is loaded into memory)
bool mmInitDefaultMem(mm_addr soundbank)
{
    mm_word first_word = ((mm_word*)soundbank)[0];

    if (!mmTryToInitializeDefault(first_word))
        return false;

    mmSoundBankInMemory(soundbank);
    mmLoadDictionaryFromMemory(soundbank);
    return true;
}

bool mmInitNoSoundbank(void)
{
    if (!mmTryToInitializeDefault(0))
        return false;

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

mm_word *mppGetSampleNameList(void)
{
    return mmSampleNameList;
}

mm_word *mppGetModuleNameList(void)
{
    return mmModuleNameList;
}
