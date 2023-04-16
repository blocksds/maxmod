// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)

/****************************************************************************
 *                                                          __              *
 *                ____ ___  ____ __  ______ ___  ____  ____/ /              *
 *               / __ `__ \/ __ `/ |/ / __ `__ \/ __ \/ __  /               *
 *              / / / / / / /_/ />  </ / / / / / /_/ / /_/ /                *
 *             /_/ /_/ /_/\__,_/_/|_/_/ /_/ /_/\____/\__,_/                 *
 *                                                                          *
 *               Nintendo DS & Gameboy Advance Sound System                 *
 *                                                                          *
 ****************************************************************************/

/******************************************************************************
 *
 * Definitions
 *
 ******************************************************************************/

#include "mm_main.h"
#include "mm_main_ds.h"

void mmInit7(void);

// Load sound bank address
void mmGetSoundBank(mm_word n_songs, mm_addr bank) {
    mmModuleCount = n_songs;
    mmModuleBank = bank;
    // What is this * 4 from? Is it a sizeof?!
    mmSampleBank = bank + (n_songs * 4);
    mmInit7();
}
