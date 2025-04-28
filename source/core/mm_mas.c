// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021-2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdint.h>
#include <string.h>

#include "maxmod.h"

#include "mm_mas.h"
#include "mm_mas_arm.h"
#include "mp_defs.h"
#include "mp_format_mas.h"
#include "mm_main.h"
#include "mp_mas_structs.h"

#if defined(SYS_GBA)
#include "mm_main_gba.h"
#include "mm_mixer_gba.h"
#elif defined(SYS_NDS)
#include "mm_main_ds.h"
#include "mm_mixer_ds.h"
#endif

#ifdef SYS_NDS
#define IWRAM_CODE
#endif

#ifdef SYS_GBA
#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#endif

// TODO: Make this static
void mpp_setbpm(mpl_layer_information*, mm_word);
void mpp_setposition(mpl_layer_information*, mm_word);

mm_word mpp_resolution;

// Suspend main module and associated channels.
static void mpp_suspend(void)
{
    mm_active_channel *act_ch = &mm_achannels[0];

#ifdef SYS_NDS
    mm_mixer_channel *mix_ch = &mm_mix_channels[0];
#endif
#ifdef SYS_GBA
    mm_mixer_channel *mix_ch = &mm_mixchannels[0];
#endif

    for (mm_word count = mm_num_ach; count != 0; count--, act_ch++, mix_ch++)
    {
        if ((act_ch->flags >> 6) != 0)
            continue;

#ifdef SYS_GBA
        mix_ch->freq = 0;
#else
        mix_ch->freq = 0;
        mix_ch->vol = 0;
#endif
    }
}

// Pause module playback.
void mmPause(void)
{
    if (mmLayerMain.valid == 0)
        return;

    mmLayerMain.isplaying = 0;

    mpp_suspend();
}

// Resume module playback.
void mmResume(void)
{
    if (mmLayerMain.valid == 0)
        return;

    mmLayerMain.isplaying = 1;
}

// Returns true if module is playing.
mm_bool mmActive(void)
{
    return mmLayerMain.isplaying;
}

// Returns true if a jingle is playing.
mm_bool mmActiveSub(void)
{
    return mmLayerSub.isplaying;
}

// Set master module volume.
//
// volume : 0->1024
void mmSetModuleVolume(mm_word volume)
{
    // Clamp volume 0->1024
    if (volume > 1024)
        volume = 1024;

    mmLayerMain.volume = volume;
}

// Set master jingle volume.
//
// volume : 0->1024
void mmSetJingleVolume(mm_word volume)
{
    // Clamp volume 0->1024
    if (volume > 1024)
        volume = 1024;

    mmLayerSub.volume = volume; // mpp_layerB
}

static void mpps_backdoor(mm_word id, mm_pmode mode, mm_word layer)
{
#if defined(SYS_GBA)
    // Resolve song address
    uintptr_t solution = (uintptr_t)mp_solution;

    uintptr_t r3 = *(mm_hword *)solution;

    r3 = solution + 12 + (r3 * 4);

    uintptr_t r0 = r3 + (id * 4);

    r0 = solution + 8 + *(mm_word *)r0;

    mmPlayModule(r0, mode, layer);
#elif defined(SYS_NDS)
    mm_word *bank = (mm_word *)mmModuleBank;

    uintptr_t address = bank[id];

    if (address == 0)
        return;

    mmPlayModule(address + 8, mode, layer);
#endif
}

// Start module playback
//
// module_ID : id of module
// mode : mode of playback
void mmStart(mm_word id, mm_pmode mode)
{
    mpps_backdoor(id, mode, 0);
}

// Start jingle playback
//
// module_ID : index of module
void mmJingle(mm_word module_ID)
{
    mpps_backdoor(module_ID, MPP_PLAY_ONCE, 1);
}

// Reset channel data, and any active channels linked to the layer.
static void mpp_resetchannels(mpl_layer_information *layer_info,
                              mm_module_channel *channels,
                              mm_word num_ch)
{
    (void)layer_info;

    // Clear channel data to 0
    memset(channels, 0, sizeof(mm_module_channel) * num_ch);

    // Reset channel indexes
    for (mm_word i = 0; i < num_ch; i++)
        channels[i].alloc = 255;

    // Reset active channels linked to this layer.

#ifdef SYS_NDS
    mm_mixer_channel *mix_ch = &mm_mix_channels[0];
#endif
#ifdef SYS_GBA
    mm_mixer_channel *mix_ch = &mm_mixchannels[0];
#endif

    mm_byte layer = mpp_clayer;
    mm_active_channel *act_ch = &mm_achannels[0];

    for (mm_word i = 0; i < mm_num_ach; i++, act_ch++, mix_ch++)
    {
        // Test if layer matches
        if ((act_ch->flags >> 6) != layer)
            continue;

        // Clear achannel data to zero
        memset(act_ch, 0, sizeof(mm_active_channel));

        // Disabled mixer channel. Disabled status differs between systems.
#ifdef SYS_NDS
        mix_ch->key_on = 1;
        mix_ch->samp = 0;
        mix_ch->tpan = 0;
#endif
#ifdef SYS_GBA
        mix_ch->src = 0;
#endif
    }
}

// Stop module playback.
static void mppStop(void)
{
    mpl_layer_information *layer_info;
    mm_module_channel *channels;
    mm_word num_ch;

    if (mpp_clayer != 0)
    {
        layer_info = &mmLayerSub;
        channels = &mm_schannels[0];
        num_ch = MP_SCHANNELS;
    }
    else
    {
        layer_info = &mmLayerMain;
        channels = mm_pchannels;
        num_ch = mm_num_mch;
    }

    layer_info->isplaying = 0;
    layer_info->valid = 0;

    mpp_resetchannels(layer_info, channels, num_ch);
}

// Stop module playback.
void mmStop(void)
{
    mpp_clayer = 0;
    mppStop();
}

// Set sequence position.
// Input r5 = layer, position = r0
void mpp_setposition(mpl_layer_information *layer_info, mm_word position)
{
    mas_header *header = (mas_header *)layer_info->songadr;

    mm_byte entry;

    while (1)
    {
        layer_info->position = position;

        // Get sequence entry
        entry = header->order[position];
        if (entry == 254)
        {
            position++;
            continue;
        }
        else if (entry != 255) // TODO: Possible bug. Should it be "=="?
        {
            break;
        }

        if (layer_info->mode != MPP_PLAY_LOOP)
        {
            // It's playing once

            mppStop();

            if (mmCallback != NULL)
                mmCallback(MPCB_SONGFINISHED, mpp_clayer);
        }

        // If the song has ended in the call to mppStop()
        if (layer_info->isplaying == 0)
            return;

        // Set position to 'restart'
        position = header->rep;
    }

    // Calculate pattern offset (in table)
    uintptr_t patt_offset = layer_info->patttable + entry * 4;

    // Calculate pattern address
    uintptr_t patt_addr = (uintptr_t)header + *(mm_word *)patt_offset;

    // Save pattern size
    layer_info->nrows = *(mm_byte *)patt_addr;

    // Reset tick/row
    layer_info->tick = 0;
    layer_info->row = 0;
    layer_info->fpattdelay = 0;
    layer_info->pattdelay = 0;

    // Store pattern data address
    layer_info->pattread = patt_addr + 1;

    // Reset pattern loop
    layer_info->ploop_adr = patt_addr + 1;
    layer_info->ploop_row = 0;
    layer_info->ploop_times = 0;
}

// Set playback position
void mmPosition(mm_word position)
{
    // TODO: This was commented out in the original code
    //mpp_resetchannels(&mmLayerMain, mm_pchannels, mm_num_mch);

    mpp_setposition(&mmLayerMain, position);
}

// Get current number of elapsed ticks in the row being played.
mm_word mmGetPositionTick(void)
{
    return mmLayerMain.tick;
}

// Get current row being played.
mm_word mmGetPositionRow(void)
{
    return mmLayerMain.row;
}

// Get playback position
mm_word mmGetPosition(void)
{
    return mmLayerMain.position;
}

// Set BPM. bpm = 32..255
// Input r5 = layer, r0 = bpm
void mpp_setbpm(mpl_layer_information *layer_info, mm_word bpm)
{
    layer_info->bpm = bpm;

#ifdef SYS_GBA
    if (mpp_clayer == 0)
    {
        // Multiply by master tempo
        mm_word r1 = (mm_mastertempo * bpm) >> 10;

        // Samples per tick ~= mixfreq / (bpm / 2.5) ~= mixfreq * 2.5 / bpm
        mm_word r0 = mm_bpmdv;

        // DO NOT TRUST THE COMMENTS IN THE ASM!
        r0 = r0 / r1;

        // Make it a multiple of two
        r0 &= ~1;

        layer_info->tickrate = r0;

        //layer_info->sampcount = 0; // TODO: Commented out in original code

        return;
    }
    else
    {
        // SUB LAYER, time using vsync (rate = (bpm / 2.5) / 59.7)

        layer_info->tickrate = (bpm << 15) / 149;
    }
#endif

#ifdef SYS_NDS
    // vsync = ~59.8261 HZ (says GBATEK)
    // divider = hz * 2.5 * 64

    if (mpp_clayer == 0)
    {
        // Multiply by master tempo
        bpm = bpm * mm_mastertempo;
        bpm <<= 16 + 6 - 10;
    }
    else
    {
        bpm <<= 16 + 6;
    }

    // using 60hz vsync for timing
    // Should this be better approximated?!
    layer_info->tickrate = (bpm / mpp_resolution) >> 1;
#endif
}

// Set master tempo
//
// tempo : x.10 fixed point tempo, 0.5->2.0
void mmSetModuleTempo(mm_word tempo)
{
    // Clamp value: 512->2048

    mm_word max = 1 << 11;
    if (tempo > max)
        tempo = max;

    mm_word min = 1 << 9;
    if (tempo < min)
        tempo = min;

    mm_mastertempo = tempo;
    mpp_clayer = 0;

    if (mmLayerMain.bpm != 0)
       mpp_setbpm(&mmLayerMain, mmLayerMain.bpm);
}

// Reset pattern variables
// Input r5 = layer
static void mpp_resetvars(mpl_layer_information *layer_info)
{
    layer_info->pattjump = 255;
    layer_info->pattjump_row = 0;
}

// Start playing module.
void mmPlayModule(mm_word address, mm_word mode, mm_word layer)
{
    mpp_clayer = layer;

    mpl_layer_information *layer_info;
    mm_module_channel *channels;
    mm_word num_ch;

    if (layer == 0)
    {
        layer_info = &mmLayerMain;
        channels = mm_pchannels;
        num_ch = mm_num_mch;
    }
    else
    {
        layer_info = &mmLayerSub;
        channels = &mm_schannels[0];
        num_ch = MP_SCHANNELS;
    }

    layer_info->mode = mode;

    layer_info->songadr = address;

    mpp_resetchannels(layer_info, channels, num_ch);

    mas_header* header = (mm_addr)address;

    mm_word instn_size = header->instn;
    mm_word sampn_size = header->sampn;

    // Setup instrument, sample and pattern tables
    layer_info->insttable = (mm_word)&header->tables[0];
    layer_info->samptable = (mm_word)&header->tables[instn_size];
    layer_info->patttable = (mm_word)&header->tables[instn_size + sampn_size];

    // Set pattern to 0
    mpp_setposition(layer_info, 0);

    // Load initial tempo
    mpp_setbpm(layer_info, header->tempo);

    // Load initial global volume
    layer_info->gv = header->gv;

    // Load song flags
    mm_word flags = header->flags;
    layer_info->flags = flags;
    layer_info->oldeffects = (flags >> 1) & 1;

    // Load speed
    layer_info->speed = header->speed;

    // mpp_playing = true, set valid flag
    layer_info->isplaying = 1;
    layer_info->valid = 1;

    mpp_resetvars(layer_info);

    // Setup channel volumes
    for (mm_word i = 0; i < num_ch; i++)
        channels[i].cvolume = header->chanvol[i];

    // Setup channel pannings
    for (mm_word i = 0; i < num_ch; i++)
        channels[i].panning = header->chanpan[i];
}

// Set master pitch
//
// pitch : x.10 fixed point value, range = 0.5->2.0
void mmSetModulePitch(mm_word pitch)
{
    // Clamp value: 512->2048

    mm_word max = 1 << 11;
    if (pitch > max)
        pitch = max;

    mm_word min = 1 << 9;
    if (pitch < min)
        pitch = min;

    mm_masterpitch = pitch;
}

#ifdef SYS_NDS7

// Set update resolution
void mmSetResolution(mm_word divider)
{
    mpp_resolution = divider;

    mpp_clayer = 0;
    if (mmLayerMain.bpm != 0)
       mpp_setbpm(&mmLayerMain, mmLayerMain.bpm);

    mpp_clayer = 1;
    if (mmLayerSub.bpm != 0)
       mpp_setbpm(&mmLayerSub, mmLayerSub.bpm);
}

#endif

#ifdef SYS_GBA

// Update sub-module/jingle, this is bad for some reason...
void mppUpdateSub(void)
{
    if (mmLayerSub.isplaying == 0)
        return;

    mpp_channels = &mm_schannels[0];
    mpp_nchannels = MP_SCHANNELS;
    mpp_clayer = 1;
    mpp_layerp = &mmLayerSub;

    mm_word tickrate = mmLayerSub.tickrate;
    mm_word tickfrac = mmLayerSub.tickfrac;

    tickfrac = tickfrac + (tickrate << 1);
    mmLayerSub.tickfrac = tickfrac;

    tickfrac >>= 16;

    while (tickfrac > 0)
    {
        mppProcessTick();
        tickfrac--;
    }
}

#endif

#ifdef SYS_NDS

// Update module layer
static void mppUpdateLayer(mpl_layer_information *layer)
{
    mpp_layerp = layer;
    mm_word new_tick = (layer->tickrate * 2) + layer->tickfrac;
    layer->tickfrac = new_tick;

    for (mm_word i = 0; i < (new_tick >> 16); i++)
        mppProcessTick();
}

// NDS Work Routine
void mmPulse(void)
{
    // Update main layer
    mpp_channels = mm_pchannels;
    mpp_nchannels = mm_num_mch;
    mpp_clayer = 0;
    mppUpdateLayer(&mmLayerMain);

    // Update sub layer
    mpp_channels = mm_schannels;
    mpp_nchannels = MP_SCHANNELS;
    mpp_clayer = 1;
    mppUpdateLayer(&mmLayerSub);
}

#endif

// It gets the active channel from a module channel. It returns NULL if there
// isn't an active channel.
IWRAM_CODE mm_active_channel *mpp_Channel_GetACHN(mm_module_channel *channel)
{
    mm_word alloc = channel->alloc;
    if (alloc == 255)
        return NULL;

    return &mm_achannels[alloc];
}
static void mpph_FastForward(mpl_layer_information *layer, int rows_to_skip)
{
    if (rows_to_skip == 0)
        return;

    if (rows_to_skip >= (layer->nrows + 1))
        return;

    layer->row = rows_to_skip;

    while (1)
    {
        mmReadPattern(layer);

        rows_to_skip--;
        if (rows_to_skip == 0)
            break;
    }
}

static void mpp_Update_ACHN(mpl_layer_information *layer, mm_active_channel *act_ch,
                            mm_module_channel *channel, mm_word period, mm_word ch)
{
    if (act_ch->flags & MCAF_UPDATED)
        return;

    mpp_Update_ACHN_notest_Wrapper(layer, act_ch, channel, period, ch);
}

// Process module tick
IWRAM_CODE void mppProcessTick(void)
{
    mpl_layer_information *layer = mpp_layerp;

    // Test if module is playing
    if (layer->isplaying == 0)
        return;

    // Read pattern data

    if ((layer->pattdelay == 0) && (layer->tick == 0))
    {
        // PROF_START
        mmReadPattern(layer);
        // PROF_END 4
    }

    // Loop through module channels

    mm_word update_bits = layer->mch_update;
    mm_module_channel *module_channels = (mm_module_channel*)mpp_channels;

    for (mm_word channel_counter = 0; ; channel_counter++)
    {
        if (update_bits & (1 << 0))
        {
            // Handle first tick and other ticks differently
            if (layer->tick == 0)
                mmUpdateChannel_T0(module_channels, layer, channel_counter);
            else
                mmUpdateChannel_TN(module_channels, layer);
        }

        module_channels++;

        update_bits >>= 1;
        if (update_bits == 0)
            break;
    }

    // PROF_START

    mm_active_channel *act_ch = &mm_achannels[0];

    for (mm_word ch = 0; ch < mm_num_ach; ch++)
    {
        if (act_ch->type != ACHN_DISABLED)
        {
            if (mpp_clayer == (act_ch->flags >> 6))
            {
                mpv_active_information *info = &mpp_vars;

                info->afvol = act_ch->volume;
                info->panplus = 0;

                mpp_Update_ACHN(layer, act_ch, module_channels, act_ch->period, ch);
            }
        }

        act_ch->flags &= ~MCAF_UPDATED;

        act_ch++;
    }

    // PROF_END 6

    // This is the inlined code of mppProcessTick_incframe()

    mm_word new_tick = layer->tick + 1;

    // If the new tick is lower than the speed continue with this row
    if (new_tick < layer->speed)
    {
        // Continue current row
        layer->tick = new_tick;
        return;
    }

    if (layer->fpattdelay != 0)
        layer->fpattdelay--;

    // Otherwise clear the tick count and advance to next row

    if (layer->pattdelay != 0)
    {
        layer->pattdelay--;
        if (layer->pattdelay != 0)
        {
            // Continue current row
            layer->tick = 0;
            return;
        }
    }

    layer->tick = 0;

    if (layer->pattjump != 255)
    {
        mpp_setposition(layer, layer->pattjump);
        layer->pattjump = 255;

        if (layer->pattjump_row == 0)
            return;

        mpph_FastForward(layer, layer->pattjump_row);
        layer->pattjump_row = 0;
        return;
    }

    if (layer->ploop_jump != 0)
    {
        layer->ploop_jump = 0;
        layer->row = layer->ploop_row;
        layer->pattread = layer->ploop_adr;
        return;
    }

    int new_row = layer->row + 1;
    if (new_row != (layer->nrows + 1))
    {
        // If they are different, continue playing this pattern
        layer->row = new_row;
        return;
    }

    // Advance position
    mpp_setposition(layer, layer->position + 1);
}
