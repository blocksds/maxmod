// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021-2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdbool.h>
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

static mm_word mppe_DoVibrato(mm_word period, mm_module_channel *channel,
                              mpl_layer_information *layer);

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

static uintptr_t mpp_PatternPointer(mm_word entry, mpl_layer_information *layer)
{
    mas_header *header = (mas_header *)layer->songadr;

    // Calculate pattern offset (in table)
    uintptr_t patt_offset = layer->patttable + entry * 4;

    // Calculate pattern address
    uintptr_t patt_addr = (uintptr_t)header + *(mm_word *)patt_offset;

    return patt_addr;
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

    // Calculate pattern address
    uintptr_t patt_addr = mpp_PatternPointer(entry, layer_info);

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

static IWRAM_CODE mm_mas_instrument *get_instrument(mpl_layer_information *mpp_layer, mm_byte instN)
{
    return (mm_mas_instrument*)(mpp_layer->songadr + ((mm_word*)mpp_layer->insttable)[instN - 1]);
}

// Process new note
IWRAM_CODE void mpp_Channel_NewNote(mm_module_channel *module_channel, mpl_layer_information *layer)
{
    if (module_channel->inst == 0)
        return;

    mm_active_channel *act_ch = mpp_Channel_GetACHN(module_channel);
    if (act_ch == 0)
        goto mppt_alloc_channel;

    mm_mas_instrument *instrument = get_instrument(layer, module_channel->inst);

    if ((module_channel->bflags >> 6) == 0) // Fetch NNA
        goto mppt_NNA_CUT; // Skip if zero

    bool do_dca = false;

    mm_byte dct = instrument->dct & 3; // Otherwise check duplicate check type

    if (dct == 0)
    {
        // Don't do DCA
    }
    else if (dct == 1) // DTC Note
    {
        // Get pattern note and translate to real note with note/sample map
        mm_byte note = instrument->note_map[module_channel->note - 1] & 0xFF;

        // Compare it with the last note
        if (note == module_channel->note)
            do_dca = true; // Equal? perform DCA. Otherwise, skip.
    }
    else if (dct == 2) // DCT Sample
    {
        // **WARNING: code not functional with new instrument table

        // Get pattern note and translate to real note with note/sample map
        mm_byte sample = (instrument->note_map[module_channel->note - 1] >> 8) & 0xFF;

        // Compare it with achn's sample
        if (sample == act_ch->sample)
            do_dca = true; // Equal? perform DCA. Otherwise, skip.
    }
    else if (dct == 3) // DTC Instrument
    {
        // Load instrument and compare it with the active one
        if (module_channel->inst == act_ch->inst)
            do_dca = true; // Equal? perform DCA. Otherwise, skip.
    }

    if (do_dca) // Duplicate Check Action
    {
        mm_byte dca = instrument->dca; // Read type

        if (dca == IT_DCA_CUT) // Cut?
            goto mppt_NNA_CUT;

        if (dca == IT_DCA_OFF) // Note-off?
            goto mppt_NNA_OFF;

        // Otherwise, note-fade
        goto mppt_NNA_FADE;
    }
    else
    {
        mm_byte bflags = module_channel->bflags >> 6;

        if (bflags == 0)
            goto mppt_NNA_CUT;
        else if (bflags == 1)
            goto mppt_NNA_CONTINUE;
        else if (bflags == 2)
            goto mppt_NNA_OFF;
        else if (bflags == 3)
            goto mppt_NNA_FADE;
    }

mppt_NNA_CUT:

#ifdef SYS_NDS // NDS supports volume ramping
    if (act_ch->type == 0)
        return; // Use the same channel

    act_ch->type = ACHN_BACKGROUND;
    act_ch->volume = 0;

    goto mppt_NNA_FINISHED;
#else
    return; // Use the same channel
#endif

mppt_NNA_CONTINUE:
    // Use a different channel and set the active channel to "background"
    act_ch->type = ACHN_BACKGROUND;
    goto mppt_NNA_FINISHED;

mppt_NNA_OFF:
    act_ch->flags &= ~MCAF_KEYON;
    act_ch->type = ACHN_BACKGROUND; // Set the active channel to "background"
    goto mppt_NNA_FINISHED;

mppt_NNA_FADE:
    act_ch->flags |= MCAF_FADE;
    act_ch->type = ACHN_BACKGROUND; // Set the active channel to "background"
    goto mppt_NNA_FINISHED;

mppt_NNA_FINISHED:

mppt_alloc_channel:

    mm_word alloc = mmAllocChannel(); // Find new active channel
    module_channel->alloc = alloc; // Save it

#ifdef SYS_NDS
    if (act_ch == 0) // Same channel
        return;

    // Copy data from previous channel (for volume ramping wierdness)
    memcpy(&(mm_achannels[alloc]), act_ch, sizeof(mm_active_channel));
#endif
}

// value = 2^(val/192), 16.16 fixed
static mm_hword mpp_TABLE_LinearSlideUpTable[] = {
    0,     237,   475,   714,   953,   // 0->4 // ADD 1.0
    1194,  1435,  1677,  1920,  2164,  // 5->9
    2409,  2655,  2902,  3149,  3397,  // 10->14
    3647,  3897,  4148,  4400,  4653,  // 15->19
    4907,  5157,  5417,  5674,  5932,  // 20->24
    6190,  6449,  6710,  6971,  7233,  // 25->29
    7496,  7761,  8026,  8292,  8559,  // 30->34
    8027,  9096,  9366,  9636,  9908,  // 35->39
    10181, 10455, 10730, 11006, 11283, // 40->44
    11560, 11839, 12119, 12400, 12682, // 45->49
    12965, 13249, 13533, 13819, 14106, // 50->54
    14394, 14684, 14974, 15265, 15557, // 55->59
    15850, 16145, 16440, 16737, 17034, // 60->64
    17333, 17633, 17933, 18235, 18538, // 65->69
    18842, 19147, 19454, 19761, 20070, // 70->74
    20379, 20690, 21002, 21315, 21629, // 75->79
    21944, 22260, 22578, 22897, 23216, // 80->84
    23537, 23860, 24183, 24507, 24833, // 85->89
    25160, 25488, 25817, 26148, 26479, // 90->94
    26812, 27146, 27481, 27818, 28155, // 95->99
    28494, 28834, 29175, 29518, 29862, // 100->104
    30207, 30553, 30900, 31248, 31599, // 105->109
    31951, 32303, 32657, 33012, 33369, // 110->114
    33726, 34085, 34446, 34807, 35170, // 115->119
    35534, 35900, 36267, 36635, 37004, // 120->124
    37375, 37747, 38121, 38496, 38872, // 125->129
    39250, 39629, 40009, 40391, 40774, // 130->134
    41158, 41544, 41932, 42320, 42710, // 135->139
    43102, 43495, 43889, 44285, 44682, // 140->144
    45081, 45481, 45882, 46285, 46690, // 145->149
    47095, 47503, 47917, 48322, 48734, // 150->154
    49147, 49562, 49978, 50396, 50815, // 155->159
    51236, 51658, 52082, 52507, 52934, // 160->164
    53363, 53793, 54224, 54658, 55092, // 165->169
    55529, 55966, 56406, 56847, 57289, // 170->174
    57734, 58179, 58627, 59076, 59527, // 175->179
    59979, 60433, 60889, 61346, 61805, // 180->184
    62265, 62727, 63191, 63657, 64124, // 185->189
    64593, 65064, 0,     474,   950,   // 190->194 // ADD 2.0 w/ 192+
    1427,  1906,  2387,  2870,  3355,  // 195->199
    3841,  4327,  4818,  5310,  5803,  // 200->204
    6298,  6795,  7294,  7794,  8296,  // 205->209
    8800,  9306,  9814,  10323, 10835, // 210->214
    11348, 11863, 12380, 12899, 13419, // 215->219
    13942, 14467, 14993, 15521, 16051, // 220->224
    16583, 17117, 17653, 18191, 18731, // 225->229
    19273, 19817, 20362, 20910, 21460, // 230->234
    22011, 22565, 23121, 23678, 24238, // 235->239
    24800, 25363, 25929, 25497, 27067, // 240->244
    27639, 28213, 28789, 29367, 29947, // 245->249
    30530, 31114, 31701, 32289, 32880, // 250->254
    33473, 34068                       // 255->256
};

// value = 2^(-val/192), 16.16 fixed
static mm_hword mpp_TABLE_LinearSlideDownTable[] = {
    65535, 65300, 65065, 64830, 64596, 64364, 64132, 63901, // 0->7
    63670, 63441, 63212, 62984, 62757, 62531, 62306, 62081, // 8->15
    61858, 61635, 61413, 61191, 60971, 60751, 60532, 60314, // 16->23
    60097, 59880, 59664, 59449, 59235, 59022, 58809, 58597, // 24->31
    58386, 58176, 57966, 57757, 57549, 57341, 57135, 56929, // 32->39
    56724, 56519, 56316, 56113, 55911, 55709, 55508, 55308, // 40->47
    55109, 54910, 54713, 54515, 54319, 54123, 53928, 53734, // 48->55
    53540, 53347, 53155, 52963, 52773, 52582, 52393, 52204, // 56->63
    52016, 51829, 51642, 51456, 51270, 51085, 50901, 50718, // 64->71
    50535, 50353, 50172, 49991, 49811, 49631, 49452, 49274, // 72->79
    49097, 48920, 48743, 48568, 48393, 48128, 48044, 47871, // 80->87
    47699, 47527, 47356, 47185, 47015, 46846, 46677, 46509, // 88->95
    46341, 46174, 46008, 45842, 45677, 45512, 45348, 45185, // 96->103
    45022, 44859, 44698, 44537, 44376, 44216, 44057, 43898, // 104->111
    43740, 43582, 43425, 43269, 43113, 42958, 42803, 42649, // 112->119
    42495, 42342, 42189, 42037, 41886, 41735, 41584, 41434, // 120->127
    41285, 41136, 40988, 40840, 40639, 40566, 40400, 40253, // 128->135
    40110, 39965, 39821, 39678, 39535, 39392, 39250, 39109, // 136->143
    38968, 38828, 38688, 38548, 38409, 38271, 38133, 37996, // 144->151
    37859, 37722, 37586, 37451, 37316, 37181, 37047, 36914, // 152->159
    36781, 36648, 36516, 36385, 36254, 36123, 35993, 35863, // 160->167
    35734, 35605, 35477, 35349, 35221, 35095, 34968, 34842, // 168->175
    34716, 34591, 34467, 34343, 34219, 34095, 33973, 33850, // 176->183
    33728, 33607, 33486, 33365, 33245, 33125, 33005, 32887, // 184->191
    32768, 32650, 32532, 32415, 32298, 32182, 32066, 31950, // 192->199
    31835, 31720, 31606, 31492, 31379, 31266, 31153, 31041, // 200->207
    30929, 30817, 30706, 30596, 30485, 30376, 30226, 30157, // 208->215
    30048, 29940, 29832, 29725, 29618, 29511, 29405, 29299, // 216->223
    29193, 29088, 28983, 28879, 28774, 28671, 28567, 28464, // 224->231
    28362, 28260, 28158, 28056, 27955, 27855, 27754, 27654, // 232->239
    27554, 27455, 27356, 27258, 27159, 27062, 26964, 26867, // 240->247
    26770, 26674, 26577, 26482, 26386, 26291, 26196, 26102, // 248->255
    26008                                                   // 256
};

static mm_hword mpp_TABLE_FineLinearSlideUpTable[] = {
    0,   59,  118, 178, 237, // 0->4   ADD 1x
    296, 356, 415, 475, 535, // 5->9
    594, 654, 714, 773, 833, // 10->14
    893                      // 15
};

static mm_hword mpp_TABLE_FineLinearSlideDownTable[] = {
    65535, 65477, 65418, 65359, 65300, 65241, 65182, 65359, // 0->7
    65065, 65006, 64947, 64888, 64830, 64772, 64713, 64645  // 8->15
};

static mm_word mpph_psu(mm_word period, mm_word slide_value)
{
    if (slide_value >= 192)
        period *= 2;

    // mpph_psu_fine

    mm_word val = (period >> 5) * mpp_TABLE_LinearSlideUpTable[slide_value];

    mm_word ret = (val >> (16 - 5)) + period;

    // mpph_psu_clip

    if ((ret >> (16 + 5)) > 0) // Clip to 0.0 ~ 1.0
        return 1 << (16 + 5);

    return ret;
}

static mm_word mpph_psd(mm_word period, mm_word slide_value)
{
    mm_word val = (period >> 5) * mpp_TABLE_LinearSlideDownTable[slide_value];

    mm_word ret = val >> (16 - 5);

    // mpph_psd_clip

    //if (ret < 0)
    //    ret = 0;

    return ret;
}

// Linear/Amiga slide up
// The slide value is provided divided by 4
mm_word mpph_PitchSlide_Up(mm_word period, mm_word slide_value, mpl_layer_information *layer)
{
    if (layer->flags & (1 << (C_FLAGS_SS - 1)))
    {
        return mpph_psu(period, slide_value);
    }
    else // mpph_psu_amiga
    {
        mm_word delta = slide_value << 4;

        // mpph_psu_amiga_fine

        if (delta > period)
            return 0;

        return period - delta;
    }
}

// Linear slide up
mm_word mpph_LinearPitchSlide_Up(mm_word period, mm_word slide_value, mpl_layer_information *layer)
{
    if (layer->flags & (1 << (C_FLAGS_SS - 1)))
        return mpph_psu(period, slide_value);
    else
        return mpph_psd(period, slide_value);
}

// Slide value in range of (0 - 15)
mm_word mpph_FinePitchSlide_Up(mm_word period, mm_word slide_value, mpl_layer_information *layer)
{
    if (layer->flags & (1 << (C_FLAGS_SS - 1))) // mpph_psu_fine
    {
        // mpph_psu_fine

        mm_word val = (period >> 5) * mpp_TABLE_FineLinearSlideUpTable[slide_value];

        mm_word ret = (val >> (16 - 5)) + period;

        // mpph_psu_clip

        if ((ret >> (16 + 5)) > 0) // Clip to 0.0 ~ 1.0
            return 1 << (16 + 5);

        return ret;
    }
    else
    {
        mm_word delta = slide_value << 2;

        // mpph_psu_amiga_fine

        if (delta > period)
            return 0;

        return period - delta;
    }
}

mm_word mpph_PitchSlide_Down(mm_word period, mm_word slide_value, mpl_layer_information *layer)
{
    if (layer->flags & (1 << (C_FLAGS_SS - 1)))
    {
        return mpph_psd(period, slide_value);
    }
    else // mpph_psd_amiga
    {
        mm_word delta = slide_value << 4;

        // mpph_psd_amiga_fine

        period = (period + delta);

        if ((period >> (16 + 5)) > 0) // Clip to 0.0 ~ 1.0
            return 1 << (16 + 5);

        return period;
    }
}

mm_word mpph_LinearPitchSlide_Down(mm_word period, mm_word slide_value, mpl_layer_information *layer)
{
    if (layer->flags & (1 << (C_FLAGS_SS - 1)))
        return mpph_psd(period, slide_value);
    else
        return mpph_psu(period, slide_value);
}

// Slide value in range of (0 - 15)
mm_word mpph_FinePitchSlide_Down(mm_word period, mm_word slide_value, mpl_layer_information *layer)
{
    if (layer->flags & (1 << (C_FLAGS_SS - 1))) // mpph_psd_fine
    {
        // mpph_psd_fine

        mm_word val = (period >> 5) * mpp_TABLE_FineLinearSlideDownTable[slide_value];

        mm_word ret = val >> (16 - 5);

        // mpph_psd_clip

        //if (ret < 0)
        //    ret = 0;

        return ret;
    }
    else
    {
        mm_word delta = slide_value << 2;

        // mpph_psd_amiga_fine

        period = (period + delta);

        if ((period >> (16 + 5)) > 0) // Clip to 0.0 ~ 1.0
            return 1 << (16 + 5);

        return period;
    }
}

// =============================================================================
//                                EFFECT MEMORY
// =============================================================================

#define MPP_XM_VFX_MEM_VS       12  // Value = 0xUD : Up, Down
#define MPP_XM_VFX_MEM_FVS      13  // Value = 0xUD : Up, Down
#define MPP_XM_VFX_MEM_GLIS     14  // Value = 0x0X : Zero, Value
#define MPP_XM_VFX_MEM_PANSL    7   // Value = 0xLR : Left, Right

#define MPP_IT_VFX_MEM          14
#define MPP_GLIS_MEM            0
#define MPP_IT_PORTAMEM         2

mm_word mpp_Process_VolumeCommand(mpl_layer_information *layer,
                                  mm_active_channel *act_ch,
                                  mm_module_channel *channel, mm_word period)
{
    mm_byte tick = layer->tick;

    mas_header *mas = (mas_header *)layer->songadr;

    mm_byte volcmd = channel->volcmd;

    if (mas->flags & (1 << 3)) // XM commands
    {
        if (volcmd == 0) // 0 = none
        {
            // Do nothing
        }
        else if (volcmd <= 0x50) // Set volume : mppuv_xm_setvol
        {
            if (tick == 0)
                channel->volume = volcmd - 0x10;
        }
        else if (volcmd < 0x80) // Volume slide : mppuv_xm_volslide
        {
            if (tick == 0)
                return period;

            int volume = channel->volume;
            mm_byte mem = channel->memory[MPP_XM_VFX_MEM_VS];

            if (volcmd < 0x70) // mppuv_xm_volslide_down
            {
                volcmd -= 0x60;

                int delta;

                if (volcmd == 0)
                {
                    delta = mem & 0xF;
                }
                else
                {
                    delta = volcmd;
                    channel->memory[MPP_XM_VFX_MEM_VS] = (mem & ~0xF) | volcmd;
                }

                // mppuv_voldownA

                volume -= delta;
                if (volume < 0) // Clamp
                    volume = 0;

                channel->volume = volume;
            }
            else // mppuv_xm_volslide_up
            {
                volcmd -= 0x70;

                int delta;

                if (volcmd == 0)
                {
                    delta = mem >> 4;
                }
                else
                {
                    delta = volcmd;
                    channel->memory[MPP_XM_VFX_MEM_VS] = (volcmd << 4) | (mem & 0xF);
                }

                volume += delta;
                if (volume > 64)
                    volume = 64;

                channel->volume = volume;
            }
        }
        else if (volcmd < 0xA0) // Fine volume slide : mppuv_xm_fvolslide
        {
            if (tick != 0)
                return period;

            int volume = channel->volume;
            mm_byte mem = channel->memory[MPP_XM_VFX_MEM_FVS];

            if (volcmd < 0x90) // mppuv_xm_volslide_down
            {
                volcmd -= 0x80;

                int delta;

                if (volcmd == 0)
                {
                    delta = mem & 0xF;
                }
                else
                {
                    delta = volcmd;
                    channel->memory[MPP_XM_VFX_MEM_FVS] = (mem & ~0xF) | volcmd;
                }

                // mppuv_voldownA

                volume -= delta;
                if (volume < 0) // Clamp
                    volume = 0;

                channel->volume = volume;
            }
            else // mppuv_xm_volslide_up
            {
                volcmd -= 0x90;

                int delta;

                if (volcmd == 0)
                {
                    delta = mem >> 4;
                }
                else
                {
                    delta = volcmd;
                    channel->memory[MPP_XM_VFX_MEM_FVS] = (volcmd << 4) | (mem & 0xF);
                }

                volume += delta;
                if (volume > 64)
                    volume = 64;

                channel->volume = volume;
            }
        }
        else if (volcmd < 0xC0) // Vibrato : mppuv_xm_vibrato
        {
            // Sets speed or depth

            if (tick == 0)
                return period;

            if (volcmd < 0xB0) // mppuv_xm_vibspd
            {
                volcmd = (volcmd - 0xA0) << 2;
                if (volcmd != 0)
                    channel->vibspd = volcmd;
            }
            else // mppuv_xm_vibdepth
            {
                volcmd = (volcmd - 0xB0) << 3;
                if (volcmd != 0)
                    channel->vibdep = volcmd;
            }

            return mppe_DoVibrato(period, channel, layer);
        }
        else if (volcmd < 0xD0) // Panning : mppuv_xm_panning
        {
            if (tick != 0)
                return period;

            mm_word panning = (volcmd - 0xC0) << 4;

            // This is a hack to make the panning reach the maximum value
            if (panning == 240)
                channel->panning = 255;
            else
                channel->panning = panning;
        }
        else if (volcmd < 0xF0) // Panning slide : mppuv_xm_panslide
        {
            if (tick == 0)
                return period;

            int panning = channel->panning;
            mm_byte mem = channel->memory[MPP_XM_VFX_MEM_PANSL];

            if (volcmd < 0xE0) // mppuv_xm_panslide_left
            {
                volcmd -= 0xD0;

                int delta;

                if (volcmd == 0)
                {
                    delta = mem >> 4;
                }
                else
                {
                    channel->memory[MPP_XM_VFX_MEM_PANSL] = (mem & 0xF) | (volcmd << 4);
                    delta = volcmd & 0xF;
                }

                delta <<= 2;

                panning -= delta;
                if (panning < 0)
                    panning = 0;

                channel->panning = panning;
            }
            else // mppuv_xm_panslide_right
            {
                volcmd -= 0xE0;

                int delta;

                if (volcmd == 0)
                {
                    delta = mem & 0xF;
                }
                else
                {
                    delta = volcmd;
                    channel->memory[MPP_XM_VFX_MEM_VS] = volcmd | (mem & 0xF);
                }

                delta <<= 2;

                panning += delta;
                if (panning > 255)
                    panning = 255;

                channel->panning = panning;
            }
        }
        else // Glissando : mppuv_xm_toneporta
        {
            // On nonzero ticks, do a regular glissando slide at speed * 16
            if (tick == 0)
                return period;

            volcmd = (volcmd - 0xF0) << 4;

            if (volcmd != 0)
                channel->memory[MPP_XM_VFX_MEM_GLIS] = volcmd;

            volcmd = channel->memory[MPP_XM_VFX_MEM_GLIS];

            return mppe_glis_backdoor_Wrapper(volcmd, channel, act_ch, layer, period);
        }
    }
    else // IT commands
    {
        if (volcmd <= 64) // Set volume : mppuv_setvol
        {
            if (tick == 0)
                channel->volume = volcmd;
        }
        else if (volcmd <= 84) // Fine volume slide : mppuv_fvol
        {
            if (tick != 0)
                return period;

            int volume = channel->volume;

            if (volcmd < 75) // Slide up : mppuv_fvolup
            {
                volcmd -= 65; // 65-74 ==> 0-9

                if (volcmd == 0)
                    volcmd = channel->memory[MPP_IT_VFX_MEM];
                else
                    channel->memory[MPP_IT_VFX_MEM] = volcmd;

                volume += volcmd;
                if (volume > 64)
                    volume = 64;
            }
            else // Slide down : mppuv_fvoldown
            {
                volcmd -= 75; // 75-84 ==> 0-9

                if (volcmd == 0)
                    volcmd = channel->memory[MPP_IT_VFX_MEM];
                else
                    channel->memory[MPP_IT_VFX_MEM] = volcmd;

                volume -= volcmd;
                if (volume < 0)
                    volume = 0;
            }

            channel->volume = volume;
        }
        else if (volcmd <= 104) // Volume slide : mppuv_volslide
        {
            if (tick == 0)
                return period;

            int volume = channel->volume;

            if (volcmd < 95) // Slide up : mppuv_vs_up
            {
                volcmd -= 85; // 85-94 ==> 0-9

                if (volcmd == 0)
                    volcmd = channel->memory[MPP_IT_VFX_MEM];
                else
                    channel->memory[MPP_IT_VFX_MEM] = volcmd;

                volume += volcmd;
                if (volume > 64)
                    volume = 64;
            }
            else // Slide down : mppuv_vs_down
            {
                volcmd -= 95; // 95-104 ==> 0-9

                if (volcmd == 0)
                    volcmd = channel->memory[MPP_IT_VFX_MEM];
                else
                    channel->memory[MPP_IT_VFX_MEM] = volcmd;

                volume -= volcmd;
                if (volume < 0)
                    volume = 0;
            }

            channel->volume = volume;
        }
        else if (volcmd <= 124) // Portamento up/down : mppuv_porta
        {
            if (tick == 0)
                return period;

            mm_word r0;

            if (volcmd >= 115) // mppuv_porta_up
            {
                volcmd = (volcmd - 115) << 2;

                if (volcmd == 0)
                    volcmd = channel->memory[MPP_IT_PORTAMEM];

                channel->memory[MPP_IT_PORTAMEM] = volcmd;

                r0 = mpph_PitchSlide_Up(channel->period, volcmd, layer);
            }
            else // mppuv_porta_down
            {
                volcmd = (volcmd - 105) << 2;

                if (volcmd == 0)
                    volcmd = channel->memory[MPP_IT_PORTAMEM];

                channel->memory[MPP_IT_PORTAMEM] = volcmd;

                r0 = mpph_PitchSlide_Down(channel->period, volcmd, layer);
            }

            mm_word r1 = channel->period;

            channel->period = period;

            return period + r0 - r1;
        }
        else if (volcmd <= 192) // Panning : mppuv_panning
        {
            if (tick == 0)
            {
                int panning = volcmd - 128; // Map to 0->64

                panning <<= 2;
                if (panning > 255)
                    panning = 255;

                channel->panning = panning;
            }
        }
        else if (volcmd <= 202) // Glissando : mppuv_glissando
        {
            if (tick == 0)
                return period;

            const mm_byte vcmd_glissando_table[] = {
                0, 1, 4, 8, 16, 32, 64, 96, 128, 255
            };

            volcmd -= 193;

            mm_word glis = vcmd_glissando_table[volcmd];

            if (layer->flags & (1 << (C_FLAGS_GS - 1))) // Shared Gxx
            {
                if (glis == 0)
                    glis = channel->memory[MPP_IT_PORTAMEM];

                channel->memory[MPP_IT_PORTAMEM] = glis;
                channel->memory[MPP_GLIS_MEM] = glis;

                mm_byte mem = channel->memory[MPP_GLIS_MEM];

                return mppe_glis_backdoor_Wrapper(mem, channel, act_ch, layer, period);
            }
            else // Single Gxx
            {
                if (glis == 0)
                    glis = channel->memory[MPP_GLIS_MEM];

                channel->memory[MPP_GLIS_MEM] = glis;

                mm_byte mem = channel->memory[MPP_GLIS_MEM];

                return mppe_glis_backdoor_Wrapper(mem, channel, act_ch, layer, period);
            }
        }
        else if (volcmd <= 212) // Vibrato (Speed) : mppuv_vibrato
        {
            if (tick == 0)
                return period;

            // Set speed

            volcmd = volcmd - 203;
            if (volcmd != 0)
            {
                volcmd = volcmd << 2;
                channel->vibspd = volcmd;
            }

            return mppe_DoVibrato(volcmd, channel, layer);
        }
    }

    return period;
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

// An effect of 0 means custom behaviour, or disabled
IWRAM_CODE mm_word mpp_Channel_ExchangeMemory(mm_byte effect, mm_byte param,
                            mm_module_channel *channel, mpl_layer_information *layer)
{
    mm_byte table_entry;

    // Check flags for XM mode
    if (layer->flags & (1 << (C_FLAGS_XS - 1)))
    {
        const mm_byte mpp_effect_memmap_xm[] = {
            0, 0, 0, 0, 2, 3, 4, 0, 0, 5, 0, 6, 7, 0, 0, 8, 9, 10, 11, 0, 0, 0, 0, 12, 0, 0, 0,
            0, 0, 0, 13
        //  /, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q,   R, S, T, U, V,  W, X, Y, Z,
        //  0, 1, 2, 3
        };

        // XM effects
        table_entry = mpp_effect_memmap_xm[effect];
    }
    else
    {
        const mm_byte mpp_effect_memmap_it[] = {
            0, 0, 0, 0, 2, 3, 3, 0, 0, 4, 5, 2, 2, 0, 6, 7, 8, 9, 10, 11, 12, 0, 0, 13, 0, 14, 0
        //  /, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q,  R,  S,  T, U, V,  W, X,  Y, Z
        };

        // IT effects
        table_entry = mpp_effect_memmap_it[effect];
    }

    if (table_entry == 0)
        return param;

    if (param == 0)
    {
        // Load memory value
        channel->param = channel->memory[table_entry - 1];
        return channel->param;
    }
    else
    {
        // Save param to memory
        channel->memory[table_entry - 1] = param;
        return param;
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
// Note: This is also used for panning slide
mm_word mpph_VolumeSlide(int volume, mm_word param, mm_word tick, int max_volume,
                         mpl_layer_information *layer)
{
    if (layer->flags & (1 << (C_FLAGS_XS - 1))) // mpph_vs_XM
    {
        if (tick != 0)
        {
            int r3 = param >> 4;
            int r1 = param & 0xF;

            int new_val = volume + r3 - r1;

            if (new_val > max_volume)
                new_val = max_volume;
            if (new_val < 0)
                new_val = 0;

            volume = new_val;
        }

        return volume;
    }
    else // mpph_vs_IT
    {
        if (param == 0xF)
        {
            volume -= 0xF;
            if (volume < 0)
                return 0;

            return volume;
        }

        if (param == 0xF0)
        {
            if (tick != 0)
                return volume;

            volume += 0xF;
            if (volume > max_volume)
                return max_volume;

            return volume;
        }

        if ((param & 0xF) == 0) // Test for Dx0 : mpph_vs_add
        {
            if (tick == 0)
                return volume;

            volume += param >> 4;
            if (volume > max_volume)
                return max_volume;

            return volume;
        }

        if ((param >> 4) == 0) // Test for D0x : mpph_vs_sub
        {
            if (tick == 0)
                return volume;

            volume -= param & 0xF;
            if (volume < 0)
                return 0;

            return volume;
        }

        // Fine slides now... only slide on tick 0
        if (tick != 0)
            return volume;

        if ((param & 0xF) == 0xF) // Test for DxF
        {
            volume += param >> 4;
            if (volume > max_volume)
                return max_volume;

            return volume;
        }

        if ((param >> 4) == 0xF) // Test for DFx
        {
            volume -= param & 0xF;
            if (volume < 0)
                return 0;

            return volume;
        }

        return volume;
    }
}

mm_word mpph_VolumeSlide64(int volume, mm_word param, mm_word tick,
                           mpl_layer_information *layer)
{
    return mpph_VolumeSlide(volume, param, tick, 64, layer);
}

extern mm_sbyte mpp_TABLE_FineSineData[];

static mm_word mppe_DoVibrato(mm_word period, mm_module_channel *channel,
                              mpl_layer_information *layer)
{
    mm_byte position;

    if ((layer->oldeffects == 0) || (layer->tick != 0)) // Update effect
    {
        position = channel->vibspd + channel->vibpos; // Wrap to 8 bits
        channel->vibpos = position;
    }
    else
    {
        position = channel->vibpos;
    }

    mm_sword value = mpp_TABLE_FineSineData[position];
    mm_sword depth = channel->vibdep;

    value = (value * depth) >> 8;

    if (value < 0)
        return mpph_PitchSlide_Down(period, -value, layer);

    return mpph_PitchSlide_Up(period, value, layer);
}

// =============================================================================
//                                      EFFECTS
// =============================================================================

// EFFECT Axy: SET SPEED
void mppe_SetSpeed(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    if (param != 0)
        layer->speed = param;
}

// EFFECT Bxy: SET POSITION
void mppe_PositionJump(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    layer->pattjump = param;
}

// EFFECT Cxy: PATTERN BREAK
void mppe_PatternBreak(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    layer->pattjump_row = param;

    if (layer->pattjump == 255) // Check if pattjump is empty
        layer->pattjump = layer->position + 1;
}

// EFFECT Dxy: VOLUME SLIDE
void mppe_VolumeSlide(mm_word param, mm_module_channel *channel, mpl_layer_information *layer)
{
    channel->volume = mpph_VolumeSlide64(channel->volume, param, layer->tick, layer);
}

// EFFECT Hxy: Vibrato
mm_word mppe_Vibrato(mm_word param, mm_word period, mm_module_channel *channel,
                     mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return mppe_DoVibrato(period, channel, layer);

    mm_word x = param >> 4;
    mm_word y = param & 0xF;

    if (x != 0)
        channel->vibspd = x * 4;

    if (y != 0)
    {
        mm_word depth = y * 4;

        // if (OldEffects)
        //     depth <<= 1;
        channel->vibdep = depth << layer->oldeffects;

        return mppe_DoVibrato(period, channel, layer);
    }

    return period;
}

// EFFECT Jxy: Arpeggio
mm_word mppe_Arpeggio(mm_word param, mm_word period, mm_active_channel *act_ch,
                      mm_module_channel *channel, mpl_layer_information *layer)
{
    if (layer->tick == 0)
        channel->fxmem = 0;

    if (act_ch == NULL)
        return period;

    mm_word semitones;

    if (channel->fxmem > 1) // mppe_arp_2
    {
        channel->fxmem = 0; // Set next tick to 0
        semitones = param & 0xF; // Mask out low nibble of param
    }
    else if (channel->fxmem == 1) // mppe_arp_1
    {
        channel->fxmem = 2; // Set next tick to 2
        semitones = param >> 4; // Mask out high nibble of param
    }
    else // mppe_arp_0
    {
        // Do nothing! :)
        channel->fxmem = 1;
        return period;
    }

    // The assembly code had the following conditional, but the register used to
    // store the period was overwritten by mpph_LinearPitchSlide_Up() right
    // after jumping to it, even in the original assembly code before the C port
    // started. Is this a bug, or do we need to enable it?
    //
    // See if its >= 12. Double period if so... (set an octave higher)
    //if (semitones >= 12)
    //    period *= 2;

    semitones *= 16; // 16 hwords

    period = mpph_LinearPitchSlide_Up(period, semitones, layer);
    return period;
}

// EFFECT Kxy: Vibrato+Volume Slide
mm_word mppe_VibratoVolume(mm_word param, mm_word period, mm_module_channel *channel,
                           mpl_layer_information *layer)
{
    mm_word new_period = mppe_DoVibrato(period, channel, layer);

    mppe_VolumeSlide(param, channel, layer);

    return new_period;
}

// EFFECT Mxy: Set Channel Volume
void mppe_ChannelVolume(mm_word param, mm_module_channel *channel, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    if (param > 0x40) // Ignore command if parameter is greater than 0x40
        return;

    channel->cvolume = param;
}

// EFFECT Nxy: Channel Volume Slide
void mppe_ChannelVolumeSlide(mm_word param, mm_module_channel *channel, mpl_layer_information *layer)
{
    channel->cvolume = mpph_VolumeSlide64(channel->cvolume, param, layer->tick, layer);
}

// EFFECT Oxy Sample Offset
void mppe_SampleOffset(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    mpp_vars.sampoff = param;
}

// EFFECT Rxy: Tremolo
void mppe_Tremolo(mm_word param, mm_module_channel *channel, mpl_layer_information *layer)
{
    // X = speed, Y = depth

    if (layer->tick != 0)
    {
        // Get sine position
        mm_word position = channel->fxmem;

        mm_word speed = param >> 4;
        // Mask out SPEED, multiply by 4 to compensate for larger sine table
        position += speed * 4;

        // Save (value & 255)
        channel->fxmem = position;
    }

    mm_word position = channel->fxmem; // Get sine position
    mm_sword sine = mpp_TABLE_FineSineData[position]; // Load sine table value

    mm_word depth = param & 0xF;

    mm_sword result = (sine * depth) >> 6; // Sine * depth / 64

    if (layer->flags & (1 << (C_FLAGS_XS - 1)))
        result >>= 1;

    mpp_vars.volplus = result; // Set volume addition variable
}

// EFFECT Txy: Set Tempo / Tempo Slide
void mppe_SetTempo(mm_word param, mpl_layer_information *layer)
{
    if (param < 0x10) // 0x = Slide down
    {
        if (layer->tick == 0)
            return;

        int bpm = layer->bpm - param;

        if (bpm < 32)
            bpm = 32;

        mpp_setbpm(layer, bpm);
    }
    else if (param < 0x20) // 1x = Slide up
    {
        if (layer->tick == 0)
            return;

        int bpm = layer->bpm + (param & 0xF);

        if (bpm > 255)
            bpm = 255;

        mpp_setbpm(layer, bpm);
    }
    else // 2x+ = Set
    {
        if (layer->tick != 0)
            return;

        mpp_setbpm(layer, param);
    }
}

// EFFECT Uxy: Fine Vibrato
mm_word mppe_FineVibrato(mm_word param, mm_word period, mm_module_channel *channel,
                           mpl_layer_information *layer)
{
    if (layer->tick == 0)
    {
        mm_word x = param >> 4;
        mm_word y = param & 0xF;

        if (x != 0)
            channel->vibspd = x * 4;

        if (y != 0)
        {
            mm_word depth = y * 4;

            // if (OldEffects)
            //     depth <<= 1;
            channel->vibdep = depth << layer->oldeffects;
        }
    }

    return mppe_DoVibrato(period, channel, layer);
}

// EFFECT Vxy: Set Global Volume
void mppe_SetGlobalVolume(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    mm_word mask = (1 << (C_FLAGS_XS - 1)) | (1 << (C_FLAGS_LS - 1));

    mm_word maxvol;

    if (layer->flags & mask)
        maxvol = 64;
    else
        maxvol = 128;

    layer->gv = (param < maxvol) ? param : maxvol;
}

// EFFECT Wxy: Global Volume Slide
void mppe_GlobalVolumeSlide(mm_word param, mpl_layer_information *layer)
{
    mm_word maxvol;

    if (layer->flags & (1 << (C_FLAGS_XS - 1)))
        maxvol = 64;
    else
        maxvol = 128;

    layer->gv = mpph_VolumeSlide(layer->gv, param, layer->tick, maxvol, layer);
}

// EFFECT Xxy: Set Panning
void mppe_SetPanning(mm_word param, mm_module_channel *channel, mpl_layer_information *layer)
{
    if (layer->tick == 0)
        channel->panning = param;
}

// =============================================================================
//                                  EXTENDED EFFECTS
// =============================================================================

static void mppex_XM_FVolSlideUp(mm_word param, mm_module_channel *channel,
                                 mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    int volume = channel->volume + (param & 0xF);

    if (volume > 64)
        volume = 64;

    channel->volume = volume;
}

static void mppex_XM_FVolSlideDown(mm_word param, mm_module_channel *channel,
                                   mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    int volume = channel->volume - (param & 0xF);

    if (volume < 0)
        volume = 0;

    channel->volume = volume;
}

static void mppex_OldRetrig(mm_word param, mm_active_channel *act_ch,
                            mm_module_channel *channel, mpl_layer_information *layer)
{
    if (layer->tick == 0)
    {
        channel->fxmem = param & 0xF;
        return;
    }

    channel->fxmem--;
    if (channel->fxmem == 0)
    {
        channel->fxmem = param & 0xF;

        if (act_ch != NULL)
            act_ch->flags |= MCAF_START;
    }
}

static void mppex_FPattDelay(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    layer->fpattdelay = param & 0xF;
}

static void mppex_InstControl(mm_word param, mm_active_channel *act_ch,
                              mm_module_channel *channel, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    mm_word subparam = param & 0xF;

    if (subparam <= 2) // mppex_ic_pastnotes
    {
        // TODO
    }
    else if (subparam <= 6) // mppex_ic_nna
    {
        // Overwrite NNA
        channel->bflags = (channel->bflags & 0x3F) | ((subparam - 3) << 6);
    }
    else if (subparam <= 8) // mppex_ic_envelope
    {
        if (act_ch != NULL)
        {
            act_ch->flags &= ~(1 << 5);
            act_ch->flags |= (subparam - 7) << 5;
        }
    }
}

static void mppex_SetPanning(mm_word param, mm_module_channel *channel)
{
    channel->panning = param << 4;
}

static void mppex_SoundControl(mm_word param)
{
    if (param != 0x91)
        return;

    // Set surround
    // TODO
}

static void mppex_PatternLoop(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    mm_word subparam = param & 0xF;

    if (subparam == 0)
    {
        layer->ploop_row = layer->row;
        layer->ploop_adr = mpp_vars.pattread_p;
        return;
    }

    mm_word counter = layer->ploop_times; // Get pattern loop counter

    if (counter == 0)
    {
        layer->ploop_times = subparam; // Save parameter to counter
        layer->ploop_jump = 1; // Enable jump
    }
    else
    {
        // It is already active
        layer->ploop_times = counter - 1;
        if (layer->ploop_times != 0)
            layer->ploop_jump = 1; // Enable jump
    }
}

static void mppex_NoteCut(mm_word param, mm_module_channel *channel,
                          mpl_layer_information *layer)
{
    mm_word reference = param & 0xF;

    if (layer->tick != reference)
        return;

    channel->volume = 0;
}

static void mppex_NoteDelay(mm_word param, mpl_layer_information *layer)
{
    mm_word reference = param & 0xF;

    if (layer->tick >= reference)
        return;

    mpp_vars.notedelay = reference;
}

static void mppex_PatternDelay(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    if (layer->pattdelay == 0)
        layer->pattdelay = (param & 0xF) + 1;
}

static void mppex_SongMessage(mm_word param, mpl_layer_information *layer)
{
    if (layer->tick != 0)
        return;

    if (mmCallback != NULL)
        mmCallback(MPCB_SONGMESSAGE, param & 0xF);
}

// EFFECT Sxy: Extended Effects
void mppe_Extended(mm_word param, mm_active_channel *act_ch,
                   mm_module_channel *channel, mpl_layer_information *layer)
{
    mm_word subcmd = param >> 4;

    switch (subcmd)
    {
        case 0x0: // S0x
            mppex_XM_FVolSlideUp(param, channel, layer);
            break;
        case 0x1: // S1x
            mppex_XM_FVolSlideDown(param, channel, layer);
            break;
        case 0x2: // S2x
            mppex_OldRetrig(param, act_ch, channel, layer);
            break;
        case 0x3: // S3x
            // mppex_VibForm
            break;
        case 0x4: // S4x
            // mppex_TremForm
            break;
        case 0x5: // S5x
            // mppex_PanbForm
            break;

        case 0x6: // S6x
            mppex_FPattDelay(param, layer);
            break;
        case 0x7: // S7x
            mppex_InstControl(param, act_ch, channel, layer);
            break;
        case 0x8: // S8x
            mppex_SetPanning(param, channel);
            break;
        case 0x9: // S9x
            mppex_SoundControl(param);
            break;
        case 0xA: // SAx
            // mppex_HighOffset
            break;
        case 0xB: // SBx
            mppex_PatternLoop(param, layer);
            break;
        case 0xC: // SCx
            mppex_NoteCut(param, channel, layer);
            break;
        case 0xD: // SDx
            mppex_NoteDelay(param, layer);
            break;
        case 0xE: // SEx
            mppex_PatternDelay(param, layer);
            break;
        case 0xF: // SFx
            mppex_SongMessage(param, layer);
            break;

        default:
            break;
    }
}

// =============================================================================
//                                      OLD EFFECTS
// =============================================================================

// EFFECT 0xx: Set Volume
void mppe_SetVolume(mm_word param, mm_module_channel *channel, mpl_layer_information *layer)
{
    if (layer->tick == 0)
        channel->volume = param;
}

// EFFECT 1xx: Key Off
void mppe_KeyOff(mm_word param, mm_active_channel *act_ch, mpl_layer_information *layer)
{
    if (layer->tick != param)
        return;

    if (act_ch != NULL)
        act_ch->flags &= ~MCAF_KEYON;
}

// EFFECT 3xy: Old Tremor
// TODO: Is this used?
void mppe_OldTremor(mm_word param, mm_module_channel *channel, mpl_layer_information *layer)
{
    while(1);
    if (layer->tick == 0)
        return;

    int mem = channel->fxmem;
    if (mem == 0) // Old
    {
        channel->fxmem = mem - 1;
    }
    else // New
    {
        channel->bflags ^= 3 << 9;

        if (channel->bflags & (1 << 10))
            channel->fxmem = (param >> 4) + 1;
        else
            channel->fxmem = (param & 0xF) + 1;
    }

    if ((channel->bflags & (1 << 10)) == 0) // Cut note
        mpp_vars.volplus = -64;
}
