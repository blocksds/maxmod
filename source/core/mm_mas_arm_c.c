// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "maxmod.h"

#include "mm_mas.h"
#include "mm_mas_arm.h"
#include "mm_main.h"
#include "mp_mas_structs.h"
#include "mp_format_mas.h"

#define ARM_CODE   __attribute__((target("arm")))

#ifdef SYS_NDS
#define IWRAM_CODE
#endif

#ifdef SYS_GBA
#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#endif

#define COMPR_FLAG_NOTE     (1 << 0)
#define COMPR_FLAG_INSTR    (1 << 1)
#define COMPR_FLAG_VOLC     (1 << 2)
#define COMPR_FLAG_EFFC     (1 << 3)

#define NO_CHANNEL_AVAILABLE 255


#define NOTE_CUT 254
#define NOTE_OFF 255
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

IWRAM_CODE ARM_CODE void mmReadPattern(mpl_layer_information *mpp_layer)
{
    // Prepare vars
    mm_word instr_count = ((mas_header*)mpp_layer->songadr)->instn;
    mm_word flags = mpp_layer->flags;
    mm_module_channel* module_channels = (mm_module_channel*)mpp_channels;

    mpp_vars.pattread_p = mpp_layer->pattread;
    mm_byte* pattern = (mm_byte*)mpp_vars.pattread_p;

    mm_word update_bits = 0;

    while (1)
    {
        mm_byte read_byte = *pattern++;

        // 0 = end of row
        if ((read_byte & 0x7F) == 0)
            break;

        mm_word pattern_flags = 0;

        mm_byte chan_num = (read_byte & 0x7F) - 1;

        update_bits |= 1 << chan_num;

        mm_module_channel *module_channel = &(module_channels[chan_num]);

        // Read new maskvariable if bit was set and save it, otherwise use the
        // previous flags.
        if (read_byte & (1 << 7))
            module_channel->cflags = *pattern++;

        mm_word compr_flags = module_channel->cflags;

        if (compr_flags & COMPR_FLAG_NOTE)
        {
            mm_byte note = *pattern++;

            if (note == NOTE_CUT)
                pattern_flags |= MF_NOTECUT;
            else if (note == NOTE_OFF)
                pattern_flags |= MF_NOTEOFF;
            else
                module_channel->pnoter = note;
        }

        if (compr_flags & COMPR_FLAG_INSTR)
        {
            // Read instrument value
            mm_byte instr = *pattern++;

            // Act if it's playing
            if ((pattern_flags & (MF_NOTECUT | MF_NOTEOFF)) == 0)
            {
                // TODO: Shouldn't this be >= ?!
                // Cleans input
                if (instr > instr_count)
                    instr = 0;

                if (module_channel->inst != instr)
                {
                    // Check 'mod/s3m' flag
                    if (flags & (1 << (C_FLAGS_LS - 1)))
                        pattern_flags |= MF_START;

                    // Set new instrument flag
                    pattern_flags |= MF_NEWINSTR;
                }

                // Update instrument
                module_channel->inst = instr;
            }
        }

        // Copy VCMD
        if (compr_flags & COMPR_FLAG_VOLC)
            module_channel->volcmd = *pattern++;

        // Copy Effect and param
        if (compr_flags & COMPR_FLAG_EFFC)
        {
            module_channel->effect = *pattern++;
            module_channel->param = *pattern++;
        }

        module_channel->flags = pattern_flags | (compr_flags >> 4);
    }

    mpp_layer->pattread = (mm_word)pattern;
    mpp_layer->mch_update = update_bits;
}
