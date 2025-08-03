// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <nds.h>

#include <maxmod7.h>
#include <mm_mas.h>

#include "core/effect.h"
#include "core/mas.h"
#include "core/mixer.h"
#include "core/mas_structs.h"
#include "ds/arm7/comms_ds7.h"
#include "ds/arm7/main_ds7.h"
#include "ds/arm7/mixer.h"

static mm_word mmEventForwarder(mm_word, mm_word);
static void StopActiveChannel(mm_word);

#define NUM_CHANNELS 32
#define NUM_PHYS_CHANNELS 16

#define INITIALIZED_VALUE 42
#define BASE_VOLUME 0x400
#define BASE_TEMPO 0x400
#define BASE_PITCH 0x400

extern mm_mode_enum mm_mixing_mode;

// Number of modules in sound bank
mm_word mmModuleCount;

// Number of samples in sound bank
mm_word mmSampleCount;

// This is a pointer to an array of pointers. Each pointer points to a module in
// MAS format stored in main RAM, or to NULL if the module hasn't been loaded
// by mmLoad().
mm_addr *mmModuleBank;

// Same as mmModuleBank, but for samples instead of modules. The MAS files it
// points to only contain one sample.
//
// mmSampleBank should start right after the end mmModuleBank. However, only the
// bottom 24 bits hold the address (minus 0x2000000). The top 8 bit are the
// number of times that the sample has been requested to be loaded (in case a
// sample is used by multiple modules).
mm_word *mmSampleBank;

// Memory for module/active channels for NDS system
mm_module_channel mm_rds_pchannels[NUM_CHANNELS];
mm_active_channel mm_rds_achannels[NUM_CHANNELS];

// Variable that will be 'true' if we are ready for playback
static mm_byte mmInitialized;

// Returns true if the system is ready for playback
mm_bool mmIsInitialized(void)
{
    return mmInitialized == INITIALIZED_VALUE;
}

static void mmInitialize(mm_bool do_initialize)
{
    if (do_initialize)
        mmInitialized = INITIALIZED_VALUE;
    else
        mmInitialized = 0;
}

// Unlock audio channels so they can be used by the sequencer.
// Does not prevent this from working for MM_MODE_B.
void mmUnlockChannelsQuick(mm_word mask)
{
    mm_ch_mask |= mask;
}

// Unlock audio channels so they can be used by the sequencer.
void mmUnlockChannels(mm_word mask)
{
    // Can NOT unlock channels in mode b
    if (mm_mixing_mode == MM_MODE_B)
        return;

    mmUnlockChannelsQuick(mask);
}

// Lock audio channels to prevent the sequencer from using them.
// Does not Stop the channels themselves.
void mmLockChannelsQuick(mm_word mask)
{
     mm_ch_mask &= ~mask;
}

// Lock audio channels to prevent the sequencer from using them.
void mmLockChannels(mm_word mask)
{
    mmLockChannelsQuick(mask);

    // It stops them, even if they were already locked
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        if (mask & (1 << i))
            StopActiveChannel(i);
    }
}

// Stop active channel
static void StopActiveChannel(mm_word index)
{
    mm_module_channel *channels;
    mm_word num_channels;

    mm_mixer_channel *mix_ch = &mm_mix_channels[index];

    mix_ch->key_on = 0;
    mix_ch->samp = 0;
    mix_ch->tpan = 0;
    mix_ch->vol = 0;
    mix_ch->cvol = 0;

    // There are more channels than physical sound channels,
    // but there was no check for that in the asm... :/
    if (index < NUM_PHYS_CHANNELS)
        SCHANNEL_CR(index) = 0;

    mm_active_channel *act_ch = &mm_achannels[index];
    mm_byte prev_flags = act_ch->flags;
    act_ch->flags = 0;
    act_ch->type = 0;

    if (prev_flags & MCAF_EFFECT)
        return;

    if (prev_flags & MCAF_SUB)
    {
        channels = &mm_schannels[0];
        num_channels = MP_SCHANNELS;
    }
    else
    {
        channels = mm_pchannels;
        num_channels = mm_num_mch;
    }

    // This was 99.9999% bugged, as it did not iterate, but just checked the same index.
    for (mm_word i = 0; i < num_channels; i++)
    {
        if (channels[i].alloc == index)
        {
            channels[i].alloc = NO_CHANNEL_AVAILABLE;
            return;
        }
    }
}

// Install ARM7 system
void mmInstall(int fifo_channel)
{
    mmInitialize(0);
    mmSetupComms(fifo_channel);
}

// Initialize system
void mmInit7(void)
{
    irqSet(IRQ_TIMER0, mmFrame);
    irqEnable(IRQ_TIMER0);

    // Init volumes
    mmSetModuleVolume(BASE_VOLUME);
    mmSetJingleVolume(BASE_VOLUME);
    mmSetEffectsVolume(BASE_VOLUME);

    mmUnlockChannels(ALL_PHYS_CHANNELS_MASK);

    // Setup channel addresses
    mm_achannels = mm_rds_achannels;
    mm_pchannels = mm_rds_pchannels;
    mm_num_mch = NUM_CHANNELS;
    mm_num_ach = NUM_CHANNELS;

    mmSetModuleTempo(BASE_TEMPO);
    mmSetModulePitch(BASE_PITCH);

    mmResetEffects();

    // Setup Mixer
    mmMixerInit();

    // Forward events
    mmSetEventHandler(mmEventForwarder);

    // Mark Maxmod as initialized so that mmFrame() can start mixing
    mmInitialize(1);
}

// Routine function
void mmFrame(void)
{
    if (mmIsInitialized())
    {
        mmMixerPre(); // critical timing
        REG_IME = 1;
        mmUpdateEffects(); // update sound effects
        mmPulse(); // update module playback
        mmMixerMix(); // update audio
        mmSendUpdateToARM9();
    }

    mmProcessComms();
}

// Forward event to arm9
static mm_word mmEventForwarder(mm_word msg, mm_word param)
{
    return mmARM9msg(MSG_ARM7_SONG_EVENT, msg | (param << 8));
}

// Load memory bank address
void mmGetMemoryBank(mm_word n_songs, mm_word n_samples, mm_addr bank)
{
    mmModuleCount = n_songs;
    mmSampleCount = n_samples;

    // In the memory bank the sample bank goes right after the module bank
    mmModuleBank = bank;
    mmSampleBank = bank + (n_songs * sizeof(mm_addr));

    // Initialize system
    mmInit7();
}

mm_word mmGetModuleCount(void)
{
    return mmModuleCount;
}

mm_word mmGetSampleCount(void)
{
    return mmSampleCount;
}
