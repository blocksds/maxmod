// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include <nds.h>

#include "maxmod.h"
#include "mm_main.h"
#include "mp_mas_structs.h"
#include "mm_main_ds.h"
#include "mm_mas.h"
#include "mm_effect.h"
#include "mp_defs.h"

#include "mm_main_ds7.h"

// Load sound bank address
void mmGetSoundBank(mm_word n_songs, mm_addr bank)
{
    mmModuleCount = n_songs;
    mmModuleBank = bank;

    // What is this * 4 from? Is it a sizeof?!
    mmSampleBank = bank + (n_songs * 4);

    // Initialize system
    mmInit7();
}
