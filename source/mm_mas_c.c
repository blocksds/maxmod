// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdint.h>
#include <string.h>

#include "maxmod.h"

#include "mm_mas.h"
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
void mpp_resetchannels(mpl_layer_information *layer_info,
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
        mix_ch->samp_cnt = 1U << 31;
#endif
#ifdef SYS_GBA
        mix_ch->src = 0;
#endif
    }
}

// Stop module playback.
void mppStop(void)
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
