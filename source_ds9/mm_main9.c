// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "maxmod9.h"
#include "mm_types.h"
#include "mm_main9.h"
#include "mm_comms.h"
#include "mm_comms9.h"
#include "multiplatform_defs.h"
#include <stdio.h>

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
mm_bool mmActive() {
    return mmActiveStatus;
}

// Set function for handling playback events
void mmSetEventHandler(mm_callback handler) {
    mmCallback = handler;
}

// Initialize Maxmod (manual settings)
void mmInit(mm_ds_system* system) {
    mmModuleCount = system->mod_count;
    mmSampleCount = system->samp_count;
    mmMemoryBank = system->mem_bank;
    mmModuleBank = system->mem_bank;
    mmSampleBank = system->mem_bank + mmModuleCount;
    
    for(mm_word i = 0; i < mmModuleCount; i++)
        ((mm_word*)mmModuleBank)[i] = 0;

    for(mm_word i = 0; i < mmSampleCount; i++)
        ((mm_word*)mmSampleBank)[i] = 0;
    
    // Setup communications
    mmSetupComms(system->fifo_channel);

    // Send soundbank info to ARM7
    mmSendBank(mmModuleCount, mmModuleBank);
}

// Shared initialization code for default setup
mm_bool mmTryToInitializeDefault(mm_word first_word) {
    mm_ds_system system;
    
    system.samp_count = first_word & 0xFFFF;
    system.mod_count = (first_word >> 16) & 0xFFFF;
    system.fifo_channel = FIFO_MAXMOD;
    system.mem_bank = malloc((system.mod_count * sizeof(mm_word)) + (system.samp_count * sizeof(mm_word)));
    
    // There was no handling of Malloc failure... :/
    if(system.mem_bank) {
        mmInit(&system);
        return true;
    }
    return false;
}

// Initialize Maxmod with default setup
void mmInitDefault(char* soundbank_file) {
    mm_word first_word = 0;
    FILE *f = fopen(soundbank_file, "rb");
    
    fread(&first_word, sizeof(first_word), 1, f);
    
    fclose(f);
    
    if(mmTryToInitializeDefault(first_word))
        mmSoundBankInFiles(soundbank_file);
}

// Initialize Maxmod with default setup
// (when the entire soundbank is loaded into memory)
void mmInitDefaultMem(mm_addr soundbank) {
    mm_word first_word = ((mm_word*)soundbank)[0];
    
    if(mmTryToInitializeDefault(first_word))
        mmSoundBankInMemory(soundbank);
}
