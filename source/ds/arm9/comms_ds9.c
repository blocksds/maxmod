// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stddef.h>
#include <stdint.h>

#include <nds.h>

#include <maxmod9.h>
#include <mm_mas.h>
#include <mm_types.h>

#include "ds/arm9/main_ds9.h"
#include "ds/common/comm_messages.h"

/***********************************************************************
 * Value32 format
 *
 * [cc] [mm] [bb] [aa]
 *
 * [mm] : ppmmmmmm, p = parameters, m = message type
 * [aa] : argument1, use if p >= 1
 * [bb] : argument2, use if p >= 2
 * [cc] : argument3, use if p == 3
 ***********************************************************************/

/***********************************************************************
 * Datamsg format
 *
 * First byte: Length of data
 * Following bytes: data
 ***********************************************************************/

#define EFFECT_CHANNELS         16
#define MAX_PARAM_WORDS         4
#define NO_HANDLES_AVAILABLE    0

// Flag used by the mmStreamBegin() and mmStreamEnd()
volatile mm_byte mm_stream_arm9_flag;

// Fifo channel to use for communications
mm_word mmFifoChannel;

mm_word sfx_bitmask;

mm_byte sfx_instances[EFFECT_CHANNELS];

// Send data via Datamsg
static void SendString(mm_word* values, int num_words)
{
    fifoSendDatamsg(mmFifoChannel, num_words * sizeof(mm_word), (unsigned char*)values);
}

static void SendCommand(mm_word id)
{
    mm_word buffer = (id << 8) | 1;

    SendString(&buffer, 1);
}

static void SendCommandByte(mm_word id, mm_byte arg)
{
    mm_word buffer = (arg << 16) | (id << 8) | 2;

    SendString(&buffer, 1);
}

static void SendCommandHword(mm_word id, mm_hword arg)
{
    mm_word buffer = (arg << 16) | (id << 8) | 3;

    SendString(&buffer, 1);
}

static void SendCommandHwordByte(mm_word id, mm_hword arg1, mm_byte arg2)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (arg1 << 16) | (id << 8) | 4;
    buffer[1] = arg2;

    SendString(buffer, 2);
}

static void SendCommandHwordByteByte(mm_word id, mm_hword arg1, mm_byte arg2, mm_byte arg3)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (arg1 << 16) | (id << 8) | 5;
    buffer[1] = arg2 | (arg3 << 8);

    SendString(buffer, 2);
}

// Send a memory bank to the ARM7
void mmSendBank(mm_word num_songs, mm_word num_samples, mm_addr bank_addr)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (num_songs << 16) | (MSG_BANK << 8) | 9;
    buffer[1] = (mm_word)bank_addr;
    buffer[2] = num_samples;

    SendString(buffer, 3);
}

// Lock channels to prevent use by maxmod
// TODO: Make this work for all channels, not just the PHYS ones.
// Requires changing the receiving end in arm7 to use 5 bytes.
void mmLockChannels(mm_word bitmask)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (bitmask << 24) | (1 << 16) | (MSG_SELCHAN << 8) | (6);
    buffer[1] = (bitmask >> 8);

    SendString(buffer, 2);
}

// Unlock channels to allow use by maxmod
// TODO: Make this work for all channels, not just the PHYS ones.
// Requires changing the receiving end in arm7 to use 5 bytes.
void mmUnlockChannels(mm_word bitmask)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (bitmask << 24) | (0 << 16) | (MSG_SELCHAN << 8) | (6);
    buffer[1] = (bitmask >> 8);

    SendString(buffer, 2);
}

// Start module playback
void mmStart(mm_word module_ID, mm_pmode mode)
{
    mmActiveStatus = 1;
    SendCommandHwordByteByte(MSG_START, module_ID, mode, MM_MAIN);
}

// Pause module playback
void mmPause(void)
{
    SendCommand(MSG_PAUSE);
}

// Resume module playback
void mmResume(void)
{
    SendCommand(MSG_RESUME);
}

// Stop module playback
void mmStop(void)
{
    SendCommandByte(MSG_STOP, MM_MAIN);
}

// Stop jingle playback
void mmJingleStop(void)
{
    SendCommandByte(MSG_STOP, MM_JINGLE);
}

// Set playback position
void mmSetPosition(mm_word position)
{
    SendCommandByte(MSG_POSITION, position);
}

// Start jingle
void mmJingleStart(mm_word module_ID, mm_pmode mode)
{
    SendCommandHwordByteByte(MSG_START, module_ID, mode, MM_JINGLE);
}

// Set module volume
void mmSetModuleVolume(mm_word vol)
{
    SendCommandHwordByte(MSG_MASTERVOL, vol, MM_MAIN);
}

// Set jingle volume
void mmSetJingleVolume(mm_word vol)
{
    SendCommandHwordByte(MSG_MASTERVOL, vol, MM_JINGLE);
}

// Set master tempo
void mmSetModuleTempo(mm_word tempo)
{
    SendCommandHword(MSG_MASTERTEMPO, tempo);
}

// Set master pitch
void mmSetModulePitch(mm_word pitch)
{
    SendCommandHword(MSG_MASTERPITCH, pitch);
}

// Set master effect volume
void mmSetEffectsVolume(mm_word vol)
{
    SendCommandHword(MSG_MASTEREFFECTVOL, vol);
}

// Open audio stream
void mmStreamBegin(mm_word wave_memory, mm_hword clks, mm_hword len, mm_byte format)
{
    mm_stream_arm9_flag = 0;

    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (((mm_word)wave_memory) << 16) | (MSG_OPENSTREAM << 8) | (10);
    buffer[1] = (((mm_word)wave_memory) >> 16) | (clks << 16);
    buffer[2] = len | (format << 16);

    SendString(buffer, 3);

    while (mm_stream_arm9_flag == 0);
}

// Close audio stream
void mmStreamEnd(void)
{
    mm_stream_arm9_flag = 0;

    SendCommand(MSG_CLOSESTREAM);

    while (mm_stream_arm9_flag == 0);
}

// Select audio mode
void mmSelectMode(mm_mode_enum mode)
{
    SendCommandByte(MSG_SELECTMODE, mode);
}

// Return effect handle
// NO_HANDLES_AVAILABLE = no channels available
static mm_sfxhand mmCreateEffectHandle(void)
{
    int i = 0;

    // Search for free channel
    for (; i < EFFECT_CHANNELS; i++)
    {
        if (!((sfx_bitmask >> i) & 1))
            break;
    }

    if (i >= EFFECT_CHANNELS)
        return NO_HANDLES_AVAILABLE;

    // Disable IRQ, prevent interruptions while updating the bitmask
    int old_ime = enterCriticalSection();

    sfx_bitmask |= 1 << i;

    // Re-enable IRQ
    leaveCriticalSection(old_ime);

    sfx_instances[i] += 1;

    return (sfx_instances[i] << 8) | (i + 1);
}

// Returns same handle, or a newer valid handle
static mm_sfxhand mmValidateEffectHandle(mm_sfxhand handle)
{
    mm_byte instance_num = (handle & 0xFF) - 1;

    if ((instance_num < EFFECT_CHANNELS) && (sfx_instances[instance_num] == ((handle >> 8) & 0xFF)))
        return handle;

    return mmCreateEffectHandle();
}

// Play sound effect, default parameters
mm_sfxhand mmEffect(mm_word sample_ID)
{
    mm_word buffer[MAX_PARAM_WORDS];

    mm_sfxhand handle = mmCreateEffectHandle();

    if (handle != NO_HANDLES_AVAILABLE)
    {
        buffer[0] = (sample_ID << 16) | (MSG_EFFECT << 8) | (5);
        buffer[1] = handle;

        SendString(buffer, 2);
    }

    return handle;
}

// Set effect volume
// volume should be a byte... :/
void mmEffectVolume(mm_sfxhand handle, mm_word volume)
{
    SendCommandHwordByte(MSG_EFFECTVOL, handle, volume);
}

// Set effect panning
void mmEffectPanning(mm_sfxhand handle, mm_byte panning)
{
    SendCommandHwordByte(MSG_EFFECTPAN, handle, panning);
}

// Set effect playback rate
void mmEffectRate(mm_sfxhand handle, mm_word rate)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (handle << 16) | (MSG_EFFECTRATE << 8) | (5);
    buffer[1] = rate;

    SendString(buffer, 2);
}

// Scale effect playback rate by some factor
void mmEffectScaleRate(mm_sfxhand handle, mm_word factor)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (handle << 16) | (MSG_EFFECTMULRATE << 8) | (5);
    buffer[1] = factor;

    SendString(buffer, 2);
}

// Release sound effect
void mmEffectRelease(mm_sfxhand handle)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (handle << 24) | (1 << 16) | (MSG_EFFECTOPT << 8) | (4);
    buffer[1] = (handle >> 8);

    SendString(buffer, 2);
}

// Stop sound effect
void mmEffectCancel(mm_sfxhand handle)
{
    mm_word buffer[MAX_PARAM_WORDS];

    buffer[0] = (handle << 24) | (0 << 16) | (MSG_EFFECTOPT << 8) | (4);
    buffer[1] = (handle >> 8);

    SendString(buffer, 2);
}

// Play sound effect, parameters supplied
mm_sfxhand mmEffectEx(mm_sound_effect *sound)
{
    mm_word buffer[MAX_PARAM_WORDS];
    mm_sfxhand handle = sound->handle;

    if (handle == NO_HANDLES_AVAILABLE)
        handle = mmCreateEffectHandle();
    else
        handle = mmValidateEffectHandle(handle);

    if (handle == NO_HANDLES_AVAILABLE)
        return handle;

    buffer[0] = (((mm_word)sound->sample) << 16) | (MSG_EFFECTEX << 8) | (11);
    buffer[1] = (sound->rate << 16) | (((mm_word)sound->sample) >> 16);
    buffer[2] = (sound->panning << 24) | (sound->volume << 16) | (handle & 0xFFFF);

    SendString(buffer, 3);

    return handle;
}

// Enable reverb system
void mmReverbEnable(void)
{
    SendCommand(MSG_REVERBENABLE);
}

// Disable reverb system
void mmReverbDisable(void)
{
    SendCommand(MSG_REVERBDISABLE);
}

// Configure reverb system
// TODO: Maybe migrate to a more easy-to-mantain config?
void mmReverbConfigure(mm_reverb_cfg* config)
{
    mm_word buffer[MAX_PARAM_WORDS];
    mm_byte* buffer_byte = (mm_byte*)buffer;

    size_t bytes = 4;

    if (config->flags & MMRF_MEMORY)
    {
        for (size_t i = 0; i < sizeof(config->memory); i++)
            buffer_byte[bytes + i] = (((mm_word)config->memory) >> (8 * i)) & 0xFF;
        bytes += sizeof(config->memory);
    }

    if (config->flags & MMRF_DELAY)
    {
        for (size_t i = 0; i < sizeof(config->delay); i++)
            buffer_byte[bytes + i] = (config->delay >> (8 * i)) & 0xFF;
        bytes += sizeof(config->delay);
    }

    if (config->flags & MMRF_RATE)
    {
        for (size_t i = 0; i < sizeof(config->rate); i++)
            buffer_byte[bytes + i] = (config->rate >> (8 * i)) & 0xFF;
        bytes += sizeof(config->rate);
    }

    if (config->flags & MMRF_FEEDBACK)
    {
        for (size_t i = 0; i < sizeof(config->feedback); i++)
            buffer_byte[bytes + i] = (config->feedback >> (8 * i)) & 0xFF;
        bytes += sizeof(config->feedback);
    }

    if (config->flags & MMRF_PANNING)
    {
        for (size_t i = 0; i < sizeof(config->panning); i++)
            buffer_byte[bytes + i] = (config->panning >> (8 * i)) & 0xFF;
        bytes += sizeof(config->panning);
    }

    buffer[0] = (config->flags << 16) | (MSG_REVERBCFG << 8) | (bytes - 1);

    SendString(buffer, (bytes + 3) >> 2);
}

// Enable reverb output
void mmReverbStart(mm_reverbch channels)
{
    SendCommandByte(MSG_REVERBSTART, channels);
}

// Disable reverb output
void mmReverbStop(mm_reverbch channels)
{
    SendCommandByte(MSG_REVERBSTOP, channels);
}

// Cancel all sound effects
void mmEffectCancelAll(void)
{
    SendCommand(MSG_EFFECTCANCELALL);
}

// Default maxmod message receiving code
static void mmReceiveMessage(uint32_t value32, void *userdata)
{
    (void)userdata;

    mm_word cmd = value32 >> 20;

    if (cmd == MSG_ARM7_SONG_EVENT)
    {
        if (mmCallback != NULL)
            mmCallback(value32 & 0xFF, (value32 >> 8) & 0xFF);
    }
    else if (cmd == MSG_ARM7_STREAM_READY)
    {
        mm_stream_arm9_flag = 1;
    }
    else if (cmd == MSG_ARM7_UPDATE)
    {
        sfx_bitmask &= ~(value32 & 0xFFFF);
        mmActiveStatus = (value32 >> 16) & 1;
    }
}

// ARM9 Communication Setup
void mmSetupComms(mm_word channel)
{
    mmFifoChannel = channel;
    fifoSetValue32Handler(channel, mmReceiveMessage, 0);
}
