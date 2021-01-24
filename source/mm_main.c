// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#include "mm_types.h"

// Function pointer to user event handler
mm_callback mmCallback;

// Bitmask to select which hardware/software channels are free to use
mm_word mm_ch_mask;

// Set function for handling playback events
void mmSetEventHandler(mm_callback handler)
{
    mmCallback = handler;
}
