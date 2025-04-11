// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "maxmod.h"

#include "mm_mas.h"
#include "mm_main.h"
#include "mp_mas_structs.h"

#define ARM_CODE   __attribute__((target("arm")))

#ifdef SYS_NDS
#define IWRAM_CODE
#endif

#ifdef SYS_GBA
#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#endif


#define NO_CHANNEL_AVAILABLE 255

// Finds a channel to use
// Returns invalid channel [NO_CHANNEL_AVAILABLE] if none available
IWRAM_CODE ARM_CODE mm_word mmAllocChannel(void)
{
    // Pointer to active channels
    mm_active_channel *ch = &mm_achannels[0];

    mm_word bitmask = mm_ch_mask;
    mm_word best_channel = NO_CHANNEL_AVAILABLE; // 255 = none
    mm_word best_volume = 0x80000000;

    for (mm_word i = 0; bitmask != 0; i++, bitmask >>= 1, ch++)
    {
        // If not available, skip
        if ((bitmask & 1) == 0)
            continue;

        mm_word fvol = ch->fvol;
        mm_word type = ch->type;

        // Compare background/disabled
        if (ACHN_BACKGROUND < type)
            continue; // It's important, don't use this channel
        else if (ACHN_BACKGROUND > type)
            return i; // It's disabled, use this channel

        // Compare volume with our record
        if (best_volume <= (fvol << 23))
            continue;

        // Save this channel and volume level
        best_channel = i;
        best_volume = fvol << 23;

        // Exit immediately if volume is zero
        //if (best_volume == 0) // TODO: This was commented out
        //    break;
    }

    return best_channel;
}
