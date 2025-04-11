// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include <stddef.h>

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

#define GLISSANDO_EFFECT 7
#define GLISSANDO_IT_VOLCMD_START 193
#define GLISSANDO_IT_VOLCMD_END 202
#define GLISSANDO_MX_VOLCMD_START 0xF0
#define GLISSANDO_MX_VOLCMD_END 0xFF

#define NO_CHANNEL_AVAILABLE 255

#define MAX_VOL_ACH 0x80

#define NOTE_CUT 254
#define NOTE_OFF 255

#define MULT_PERIOD 133808

static mm_byte note_table_oct[] =
{
    0, 0, 0,
    1, 1, 1,
    2, 2, 2,
    3, 3, 3,
    4, 4, 4,
    5, 5, 5,
    6, 6, 6,
    7, 7, 7,
    8, 8, 8,
    9, 9, 9
};

static mm_byte note_table_mod[] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0
};

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

static IWRAM_CODE ARM_CODE mm_mas_instrument *get_instrument(mpl_layer_information *mpp_layer, mm_byte instN)
{
    return (mm_mas_instrument*)(mpp_layer->songadr + ((mm_word*)mpp_layer->insttable)[instN - 1]);
}

IWRAM_CODE ARM_CODE mm_byte mmChannelStartACHN(mm_module_channel *module_channel, mm_active_channel *active_channel, mpl_layer_information *mpp_layer, mm_byte channel_counter)
{
    // Clear tremor/cutvol
    module_channel->bflags &= ~(6 << 8);

    if (active_channel)
    {
        // Set foreground type
        active_channel->type = ACHN_FOREGROUND;
        // Clear SUB/EFFECT and store layer
        active_channel->flags = (active_channel->flags & (~(3 << 6))) | (mpp_clayer << 6);
        // Store parent
        active_channel->parent = channel_counter;
        // Copy instrument
        active_channel->inst = module_channel->inst;
    }

    // Previously, it did not set the return parameter properly
    if (module_channel->inst == 0)
        return module_channel->bflags; // TODO: This is what the code does, is it a bug?

    // Get instrument pointer
    mm_mas_instrument *instrument = get_instrument(mpp_layer, module_channel->inst);

    // Check if note_map exists
    // If this is set, it doesn't!
    if (instrument->is_note_map_invalid)
    {
        if (active_channel)
            active_channel->sample = instrument->note_map_offset & 0xFF;

        // write note value (without notemap, all entries == PNOTE)
        module_channel->note = module_channel->pnoter;
    }
    else
    {
        // Read notemap entry
        mm_hword notemap_entry = *((mm_hword*)(((mm_word)instrument) + instrument->note_map_offset + (module_channel->pnoter << 1)));
        // Write note value
        module_channel->note = notemap_entry & 0xFF;
        // Write sample value
        if (active_channel)
            active_channel->sample = notemap_entry >> 8;
    }

    return module_channel->note;
}

IWRAM_CODE ARM_CODE mm_word mmGetPeriod(mpl_layer_information *mpp_layer, mm_word tuning, mm_byte note)
{
    // Tuning not used here with linear periods
    if (mpp_layer->flags & (1 << (C_FLAGS_SS - 1)))
        return IT_PitchTable[note];

    mm_word r0 = note_table_mod[note];      // (note mod 12) << 1
    mm_word r2 = note_table_oct[note >> 2]; // (note / 12)

    // Uses pre-calculated results of /12 and %12
    mm_word ret_val = (ST3_FREQTABLE[r0] * MULT_PERIOD) >> r2;

    if (tuning)
        ret_val /= tuning;

    return ret_val;
}

static IWRAM_CODE ARM_CODE mm_mas_sample_info *get_sample(mpl_layer_information *mpp_layer, mm_byte sampleN)
{
    return (mm_mas_sample_info*)(mpp_layer->songadr + ((mm_word*)mpp_layer->samptable)[sampleN - 1]);
}

static IWRAM_CODE ARM_CODE mm_active_channel *get_active_channel(mm_module_channel *module_channel)
{
    mm_active_channel* active_channel = NULL;

    // Get channel, if available
    if (module_channel->alloc != NO_CHANNEL_AVAILABLE)
        active_channel = &mm_achannels[module_channel->alloc];

    return active_channel;
}

// For tick 0
IWRAM_CODE ARM_CODE void mmUpdateChannel_T0(mm_module_channel *module_channel, mpl_layer_information* mpp_layer, mm_byte channel_counter)
{
    mm_active_channel *active_channel;

    // Test start flag
    if ((module_channel->flags & MF_START) == 0)
        goto dont_start_channel;

    // Test effect flag
    if (module_channel->flags & MF_HASFX)
    {
        // Test for 'SDx' (note delay). Don't start channel if delayed
        //if (module_channel->effect == 19)
        //{
        //    if ((module_channel->param >> 4) == 0xD)
        //        goto dont_start_Channel;
        //}

        // Always start channel if it's a new instrument
        if (module_channel->flags & MF_NEWINSTR)
            goto start_channel;

        // Check if the effect is glissando
        if (module_channel->effect == GLISSANDO_EFFECT)
            goto glissando_affected;
    }

    if (module_channel->flags & MF_NEWINSTR)
        goto start_channel;

    if ((module_channel->flags & MF_HASVCMD) == 0)
        goto start_channel;

    mm_bool is_xm_mode = mpp_layer->flags & C_FLAGS_X;

    if (is_xm_mode) // XM effects
    {
        // Glissando is 193..202
        if ((module_channel->volcmd < GLISSANDO_IT_VOLCMD_START) ||
            (module_channel->volcmd > GLISSANDO_IT_VOLCMD_END))
            goto start_channel;
    }
    else // IT effects
    {
        // Glissando is Fx
        if ((module_channel->volcmd < GLISSANDO_MX_VOLCMD_START) /* ||
            (module_channel->volcmd > GLISSANDO_MX_VOLCMD_END)*/ )
            goto start_channel;
    }

glissando_affected:

    active_channel = get_active_channel(module_channel);
    if (active_channel)
    {
        mmChannelStartACHN(module_channel, active_channel, mpp_layer, channel_counter);

        module_channel->flags &= ~MF_START;
        goto dont_start_channel;
    }

start_channel:

    mpp_Channel_NewNote(module_channel, mpp_layer);

    active_channel = get_active_channel(module_channel);
    if (active_channel == NULL)
    {
        mmUpdateChannel_TN(module_channel, mpp_layer);
        return;
    }

    mm_word note = mmChannelStartACHN(module_channel, active_channel, mpp_layer, channel_counter);

    if (active_channel->sample)
    {
        mm_mas_sample_info *sample = get_sample(mpp_layer, active_channel->sample);
        module_channel->period = mmGetPeriod(mpp_layer, sample->frequency << 2, note);
        active_channel->flags |= MCAF_START;
    }

    goto channel_started;

dont_start_channel:

    active_channel = get_active_channel(module_channel);
    if (active_channel == NULL)
    {
        mmUpdateChannel_TN(module_channel, mpp_layer);
        return;
    }

channel_started:

    if (module_channel->flags & MF_DVOL)
    {
        if (module_channel->inst)
        {
            // Get instrument pointer
            mm_mas_instrument *instrument = get_instrument(mpp_layer, module_channel->inst);

            // Clear old nna and set the new one
            module_channel->bflags &= ~(3 << 6);
            module_channel->bflags |= ((instrument->nna & 3) << 6);

            active_channel->flags &= ~MCAF_VOLENV;
            if (instrument->env_flags & ENVFLAG_A)
                active_channel->flags |= MCAF_VOLENV;

            // TODO: Is the << 1 a bug?
            if (instrument->panning & 0x80)
                module_channel->panning = (instrument->panning & 0x7F) << 1;
        }

        if (active_channel->sample)
        {
            // Get sample pointer
            mm_mas_sample_info *sample = get_sample(mpp_layer, active_channel->sample);
            module_channel->volume = sample->default_volume;

            // TODO: Is the << 1 a bug?
            if (sample->panning & 0x80)
                module_channel->panning = (sample->panning & 0x7F) << 1;
        }
    }

    if (module_channel->flags & (MF_START | MF_DVOL))
    {
        if (((mpp_layer->flags & C_FLAGS_X) == 0) || (module_channel->flags & MF_DVOL))
        {
            // Reset volume
            active_channel->fade = 1 << 10;
            active_channel->envc_vol = 0;
            active_channel->envc_pan = 0;
            active_channel->envc_pic = 0;
            active_channel->avib_dep = 0;
            active_channel->avib_pos = 0;
            active_channel->envn_vol = 0;
            active_channel->envn_pan = 0;
            active_channel->envn_pic = 0;

            // Clear fx memory
            module_channel->fxmem = 0;

            // Set keyon and clear envend + fade
            active_channel->flags |= MCAF_KEYON;
            active_channel->flags &= ~(MCAF_ENVEND | MCAF_FADE);
        }
    }

    if (module_channel->flags & MF_NOTEOFF)
    {
        active_channel->flags &= ~MCAF_KEYON;

        mm_bool is_xm_mode = mpp_layer->flags & C_FLAGS_X;

        // XM starts fade immediately on note-off
        if (is_xm_mode)
            active_channel->flags |= MCAF_FADE;
    }

    if (module_channel->flags & MF_NOTECUT)
        module_channel->volume = 0;

    module_channel->flags &= ~MF_START;

    // Execute the rest as normal
    mmUpdateChannel_TN(module_channel, mpp_layer);
}

// For ticks that are not the first one
IWRAM_CODE ARM_CODE void mmUpdateChannel_TN(mm_module_channel *module_channel, mpl_layer_information *mpp_layer)
{
    // Get channel, if available
    mm_active_channel *active_channel = get_active_channel(module_channel);

    // Get period, edited by other functions...
    mm_word period = module_channel->period;

    // Clear variables
    mpp_vars.sampoff = 0;
    mpp_vars.volplus = 0;
    mpp_vars.notedelay = 0;
    mpp_vars.panplus = 0;

    // Update volume commands
    if (module_channel->flags & MF_HASVCMD)
        period = mpp_Process_VolumeCommand_Wrapper(mpp_layer, active_channel, module_channel, period);

    // Update effects
    if (module_channel->flags & MF_HASFX)
        period = mpp_Process_Effect_Wrapper(mpp_layer, active_channel, module_channel, period);

    if (!active_channel)
        return;

    int volume = (module_channel->volume * module_channel->cvolume) >> 5;
    active_channel->volume = volume;
    mm_sword vol_addition = mpp_vars.volplus;

    volume += vol_addition << 3;

    // Clamp volume
    if (volume < 0)
        volume = 0;
    if (volume > MAX_VOL_ACH) // TODO: Is this right?! Isn't the limit 0x7F?!
        volume = MAX_VOL_ACH;

    mpp_vars.afvol = volume;

    if (mpp_vars.notedelay)
    {
        active_channel->flags |= MCAF_UPDATED;
        return;
    }

    // Copy panning and period
    active_channel->panning = module_channel->panning;
    active_channel->period = module_channel->period;
    // Set to 0 temporarily. Reserved for later use.
    mpp_vars.panplus = 0;

    active_channel->flags |= MCAF_UPDATED;

    period = mpp_Update_ACHN_notest_Wrapper(mpp_layer, active_channel, module_channel, period);
}
