// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nds.h>

#include <maxmod7.h>
#include <mm_mas.h>
#include <mm_msl.h>

#include "core/effect.h"
#include "core/mas.h"
#include "core/mixer.h"
#include "ds/arm7/main7.h"
#include "ds/arm7/mixer.h"

#define SWM_CHANNEL_1 6
#define SWM_CHANNEL_2 7

#define MIX_TIMER_NUMBER 0

#define SFRAC 10

mm_mixer_channel mm_mix_channels[MM_nDSCHANNELS];

mm_mode_enum mm_mixing_mode; // [0/1/2 = a/b/c] (MM_MODE_A/B/C)

// Reset all channels
static void mm_reset_channels(void)
{
    // Clear active channel data
    memset(mm_achannels, 0, sizeof(mm_active_channel) * NUM_CHANNELS);

    mm_module_channel *channels = mm_pchannels;

    // Reset channel allocation
    for (int i = 0; i < NUM_CHANNELS; i++)
        channels[i].alloc = 255;

    channels = &mm_schannels[0];

    // Reset channel allocation
    for (int i = 0; i < MP_SCHANNELS; i++)
        channels[i].alloc = 255;

    mmResetEffects();
}

static void mm_startup_wait(void)
{
    // Delay 12 samples (pcm startup time + extra)
    volatile int val = 1024 * (16 / 4);
    while (val--);
    // TODO: Replace this by a swiDelay()
}

static void EnableSound(void)
{
    // Full volume | Enable
    REG_SOUNDCNT |= (SOUND_ENABLE | SOUND_VOL(100)); // 100 out of 127
    // TODO: Fix this, it doesn't clear the volume before setting it with OR
}

static void ClearAllChannels(void)
{
    // reset hardware channels
    mm_word bitmask = mm_ch_mask;
    for (size_t i = 0; i < NUM_PHYS_CHANNELS; i++)
    {
        if (bitmask & (1 << i))
        {
            SCHANNEL_CR(i) = 0;
            SCHANNEL_SOURCE(i) = 0;
            SCHANNEL_TIMER(i) = 0;
            SCHANNEL_REPEAT_POINT(i) = 0;
            SCHANNEL_LENGTH(i) = 0;
        }
    }
}

static void DisableSWM(void)
{
    // Lock Software channels
    mmLockChannelsQuick(ALL_SOFT_CHANNELS_MASK);
    // Restore Stream channels
    mmUnlockChannelsQuick((1 << SWM_CHANNEL_1) | (1 << SWM_CHANNEL_2));
}

static void mmSetupModeB(void)
{
    // Disable timer
    TIMER_CR(MIX_TIMER_NUMBER) = 0;

    mmSetResolution(40960);

    mm_word bitmask = mm_ch_mask;
    for (int i = 0; i < NUM_PHYS_CHANNELS; i++)
    {
        if (bitmask & (1 << i))
        {
            SCHANNEL_CR(i) = 0xA8000000;
            SCHANNEL_SOURCE(i) = (uintptr_t)mm_mix_data.mix_data_b.output[i];
            SCHANNEL_TIMER(i) = 0xFE00;
            SCHANNEL_REPEAT_POINT(i) = 0;
            SCHANNEL_LENGTH(i) = MM_MIX_B_NUM_SAMPLES;
        }
    }

    // Reset output slice
    mm_output_slice = 0;

    EnableSound();

    mm_startup_wait();

    TIMER_DATA(MIX_TIMER_NUMBER) = 0xFF80;
    TIMER_CR(MIX_TIMER_NUMBER) = 0x00C3;
}

static void SetupSWM(void)
{
    // Reset output slice
    mm_output_slice = 0;

    TIMER_CR(MIX_TIMER_NUMBER) = 0;

    // Disable sound channel and clear loop points
    SCHANNEL_CR(SWM_CHANNEL_1) = 0;
    SCHANNEL_CR(SWM_CHANNEL_2) = 0;
    SCHANNEL_REPEAT_POINT(SWM_CHANNEL_1) = 0;
    SCHANNEL_REPEAT_POINT(SWM_CHANNEL_2) = 0;

    // Setup sources
    SCHANNEL_SOURCE(SWM_CHANNEL_1) = (uintptr_t)mm_mix_data.mix_data_c.mix_output[0];
    SCHANNEL_SOURCE(SWM_CHANNEL_2) = (uintptr_t)mm_mix_data.mix_data_c.mix_output[1];

    // Set sampling frequency
    SCHANNEL_TIMER(SWM_CHANNEL_1) = -768;
    SCHANNEL_TIMER(SWM_CHANNEL_2) = -768;

    // Set source length
    SCHANNEL_LENGTH(SWM_CHANNEL_1) = MM_SW_BUFFERLEN / 2;
    SCHANNEL_LENGTH(SWM_CHANNEL_2) = MM_SW_BUFFERLEN / 2;

    // Setup control
    SCHANNEL_CR(SWM_CHANNEL_1) = SOUND_VOL(0x7F) | SOUND_PAN(0) | SOUND_REPEAT | SOUND_FORMAT_16BIT | SCHANNEL_ENABLE;
    SCHANNEL_CR(SWM_CHANNEL_2) = SOUND_VOL(0x7F) | SOUND_PAN(0x7F) | SOUND_REPEAT | SOUND_FORMAT_16BIT | SCHANNEL_ENABLE;

    mm_startup_wait();

    TIMER_DATA(MIX_TIMER_NUMBER) = 0xFD60;
    TIMER_CR(MIX_TIMER_NUMBER) = 0x00C2;

    mmSetResolution(31170);

    // Lock Stream channels
    mmLockChannelsQuick((1 << SWM_CHANNEL_1) | (1 << SWM_CHANNEL_2));
    // Unlock Software channels
    mmUnlockChannelsQuick(ALL_SOFT_CHANNELS_MASK);
}

// Init mixer system & setup nds control
void mmMixerInit(void)
{
    // Default to mode A
    mmSelectMode(MM_MODE_A);
}

// Set channel volume
void mmMixerSetVolume(int channel, mm_word volume)
{
    mm_mix_channels[channel].cvol = volume;
}

// Set channel panning
void mmMixerSetPan(int channel, mm_byte panning)
{
    mm_mix_channels[channel].tpan = panning >> 1; // Discard one bit
}

// Set channel frequency
// Rate is 3.10 fixed point (value of 2048 will raise original pitch by 1 octave)
void mmMixerSetFreq(int channel, mm_word rate)
{
    mm_mas_ds_sample *sample = (mm_mas_ds_sample *)(mm_mix_channels[channel].samp + 0x2000000);
    mm_word freq = (sample->default_frequency * rate) >> 10;

    if (freq >= 0x1FFF)
        freq = 0x1FFF;

    mm_mix_channels[channel].freq = freq;
}

// Multiply channel frequency by a value
void mmMixerMulFreq(int channel, mm_word factor)
{
    mm_word freq = ((mm_mix_channels[channel].freq * factor) + (255 * 2)) >> 10;

    if (freq >= 0x1FFF)
        freq = 0x1FFF;

    mm_mix_channels[channel].freq = freq;
}

// Stop mixing channel
void mmMixerStopChannel(int channel)
{
    mm_mix_channels[channel].key_on = 0;
    mm_mix_channels[channel].samp = 0;
    mm_mix_channels[channel].tpan = 0;
}

#if 0
// Test active status of channel
// returns nonzero if active
mm_word mmMixerChannelActive(int channel)
{
    return mm_mix_channels[channel].samp;
}

// Set channel source
void mmMixerSetSource(int channel, mm_word p_sample)
{
    //mm_mix_channels[channel].tpan = (p_sample >> 24) - 2;
    mm_mix_channels[channel].samp = p_sample;
    mm_mix_channels[channel].key_on = 1;
}
#endif

// Select audio mode
void mmSelectMode(mm_mode_enum mode)
{
    int old_ime = enterCriticalSection();

    // reset mixer channels
    for (size_t i = 0; i < NUM_CHANNELS; i++)
        mmMixerStopChannel(i);

    ClearAllChannels();

    mm_reset_channels();

    // Clear volume and enable bit
    REG_SOUNDCNT &= ~(SOUND_ENABLE | SOUND_VOL(0xFF));

    memset(&mm_mix_data, 0, sizeof(mm_mix_data_ds));

    mm_mixing_mode = mode;

    switch (mode)
    {
        case MM_MODE_A: // Mode A: Hardware mixing
            DisableSWM();
            ClearAllChannels();
            // 256hz resolution
            mmSetResolution(40960);

            TIMER_DATA(MIX_TIMER_NUMBER) = 0xFF80;
            TIMER_CR(MIX_TIMER_NUMBER) = 0x00C3;

            EnableSound();
            break;

        case MM_MODE_B: // Mode B: Interpolated mixing
            DisableSWM();
            ClearAllChannels();
            mmSetupModeB();
            break;

        default:
        case MM_MODE_C: // Mode C: Extended mixing
            ClearAllChannels();
            SetupSWM();
            EnableSound();
            break;
    }

    leaveCriticalSection(old_ime);
}

// Update hardware data
//
// NOTE: Keep this function as Thumb so that SlideMixingLevels() can jump to it
// from Thumb mode in the ARM7.
void mmMixerPre(void)
{
    if (mm_mixing_mode == MM_MODE_A) // Full-hardware mode
    {
        // Do nothing
    }
    else if (mm_mixing_mode == MM_MODE_B) // Interpolated mode
    {
        // Update volume + panning

        mm_word channels = mm_ch_mask;

        for (int i = 0; i < NUM_PHYS_CHANNELS; i++)
        {
            if (channels & 1)
            {
                // Read shadow SOUNDCNT
                mm_word shadow = *(mm_word *)&(mm_mix_data.mix_data_b.shadow[i]);
                SCHANNEL_CR(i) = shadow | SOUND_REPEAT | SOUND_FORMAT_16BIT | SCHANNEL_ENABLE;
            }

            channels >>= 1;
            if (channels == 0)
                break;
        }
    }
    else // if (mm_mixing_mode == MM_MODE_C) // Extended mode
    {
        // Update everything

        mm_word channels = mm_ch_mask;

        for (int i = 0; i < NUM_PHYS_CHANNELS; i++)
        {
            if (channels & 1)
            {
                mmshadow_c_ds *shadow = &(mm_mix_data.mix_data_c.shadow[0]);

                if (shadow[i].src != 0)
                {
                    SCHANNEL_CR(i) = 0;

                    SCHANNEL_SOURCE(i) = shadow[i].src;
                    SCHANNEL_REPEAT_POINT(i) = shadow[i].pnt;
                    SCHANNEL_LENGTH(i) = shadow[i].len;

                    SCHANNEL_CR(i) = shadow[i].cnt;

                    shadow[i].src = 0;
                }

                SCHANNEL_TIMER(i) = shadow[i].tmr;

                //SCHANNEL_PAN(i) = shadow[i].cnt >> 16;
                SCHANNEL_CR(i) = shadow[i].cnt;
            }

            channels >>= 1;
            if (channels == 0)
                break;
        }
    }
}

// Slide volume and panning levels towards target levels for all channels.
static ARM_CODE void SlideMixingLevels(mm_word throttle)
{
    mm_mixer_channel *mix_ch = &mm_mix_channels[0];

    for (mm_word i = 0; i < MM_nDSCHANNELS; i++, mix_ch++)
    {
        // Slide volume

        // volume += (target - volume) * throttle
        mm_sword target_volume = mix_ch->vol;
        mm_sword volume = mix_ch->cvol;

        if (volume < target_volume)
        {
            // TODO: Is this ok? Shouldn't this check bounds as well?
            volume += ((target_volume - volume) >> 2);

            // volume += throttle
            // if (volume > target_volume)
            //     volume = target_volume;
        }
        else
        {
            volume -= throttle;
            if (volume < target_volume)
                volume = target_volume;
        }

        // volume += ((target_volume - volume) * throttle) >> 8;

        mix_ch->cvol = volume;

        // Slide panning

        // pan += (target - pan) * throttle
        mm_sword target_panning = mix_ch->tpan << 9;
        mm_sword panning = mix_ch->cpan;

        if (panning < target_panning)
        {
            panning += throttle;
            if (panning > target_panning)
                panning = target_panning;
        }
        else
        {
            panning -= throttle;
            if (panning < target_panning)
                panning = target_panning;
        }

        // panning += ((target_panning - panning) * throttle) >> 8;

        mix_ch->cpan = panning;
    }

    mmMixerPre();
}

#define SLIDE_THROTTLE 6144 //45

static ARM_CODE mm_word translateVolume(mm_word volume) // volume = 0..65535
{
    mm_word shift_data = mmVolumeDivTable[volume >> (7 + 5)];
    //mm_word shift_level = mmVolumeTable[(volume + (16 << (7 + 5))) >> (7 + 5)];
    mm_word shift_level = mmVolumeShiftTable[volume >> (7 + 5)];

    shift_level += 5; // Add this to the table values instead

    return (shift_data << 8) | (volume >> shift_level);
}

#define CLK_DIV 524288 // VALUE = 16777216 * CLK / 512 / 32

static ARM_CODE void mmMixA(void)
{
    mm_word ch_mask = mm_ch_mask & 0xFFFF; // 16 channels only
    mm_mixer_channel *mix_ch = &mm_mix_channels[0];

    for (int channel = 0; channel < 16; channel++, mix_ch++)
    {
        if ((ch_mask & BIT(channel)) == 0)
            continue;

        if (mix_ch->samp == 0) // 0 = channel is disabled
        {
            SCHANNEL_CR(channel) = 0; // Clear channel and skip to next
            continue;
        }

        // Get mainram address
        mm_mas_ds_sample *sample = (mm_mas_ds_sample *)(mix_ch->samp + 0x2000000);

        if (mix_ch->key_on) // If KEY-ON is cleared, continue activity
        {
            // start new note
            //------------------------

            mix_ch->key_on = 0; // Clear start bit

            // TODO: The original only reads one byte instead of 32, is that a bug?
            mm_word offset = (mm_byte)mix_ch->read; // Read sample offset

            if (offset != 0)
            {
                // Test sample format
                if (sample->format == 0)
                    offset = offset << (8 - 2); // 8-bit = LSL 0
                if (sample->format == 1)
                    offset = offset << (9 - 2); // 16-bit = LSL 1
                else
                    offset = 0; // ADPCM/other = invalid
            }

            mm_byte *sampledata = (mm_byte *)sample->point; // get sampledata pointer
            if (sampledata == NULL)
                sampledata = sample->data;

            sampledata += (offset << 2); // add sample offset (in bytes)

            mm_byte rep_mode = sample->repeat_mode; // check repeat mode

            if (rep_mode == 1) // mma_looping | 1 == forward loop
            {
                mm_sword lstart = sample->loop_start; // Get loopstart position

                lstart -= offset; // Subtract sample offset

                if (lstart < 0)
                {
                    sampledata += lstart << 2; // if result goes negative than clamp values
                    lstart = 0;
                }

                // write CNT=0,SAD=SAD,TMR=0,PNT=PNT,LEN=LEN
                SCHANNEL_CR(channel) = 0;
                SCHANNEL_SOURCE(channel) = (mm_word)sampledata;
                SCHANNEL_TIMER(channel) = 0;
                SCHANNEL_REPEAT_POINT(channel) = lstart;
                SCHANNEL_LENGTH(channel) = sample->loop_length;
            }
            else // mma_notlooping
            {
                mm_word len = sample->length; // read length

                mm_sword remaining_len = len - offset; // subtract sample offset

                if (remaining_len < 0)
                {
                    sampledata -= offset << 2; // clamp negative results
                    remaining_len = len;
                }

                // write CNT=0,SAD=SAD,TMR=0,PNT=0,LEN=LEN
                SCHANNEL_CR(channel) = 0;
                SCHANNEL_SOURCE(channel) = (mm_word)sampledata;
                SCHANNEL_TIMER(channel) = 0;
                SCHANNEL_REPEAT_POINT(channel) = 0;
                SCHANNEL_LENGTH(channel) = remaining_len;
            }

            // mma_copy_levels

            // Set direct volume levels on key-on
            mix_ch->cvol = mix_ch->vol;
            mix_ch->cpan = mix_ch->tpan << 9;

            SCHANNEL_CR(channel) = SCHANNEL_ENABLE |
                (sample->repeat_mode | (sample->format << 2)) << 27;
        }
        else
        {
            // Continue channel
            // ----------------

            // Check if sound has ended
            if ((SCHANNEL_CR(channel) & SCHANNEL_ENABLE) == 0)
            {
                mix_ch->samp = 0;
                // TODO: The original code clears panning as well (32 bits at
                // once). Is this a problem? Should we clear it or not?
                mix_ch->tpan = 0;
                mix_ch->key_on = 0;
                continue;
            }
        }

        // mma_started
        // -----------

        // Set timer
        if (mix_ch->freq == 0)
            SCHANNEL_TIMER(channel) = 0;
        else
            SCHANNEL_TIMER(channel) = -(CLK_DIV / mix_ch->freq);

        // Set volume levels
        *(mm_hword*)&SCHANNEL_CR(channel) = translateVolume(mix_ch->cvol);

        // Set panning levels. Use top 7 bits.
        SCHANNEL_PAN(channel) = mix_ch->cpan >> 9;
    }
}

void mmbZerofillBuffer(mm_addr buffer);
void mmbResampleData(mm_addr dest, mm_word do_zero_padding, mm_word *shadow,
                     mm_mixer_channel *mix_ch);

static mm_addr mmb_getdest(mm_word channel)
{
    // Calc destination address...
    mm_byte *dest = &(mm_mix_data.mix_data_b.output[0][0]) + (channel * 512);

    mm_word slice = mm_output_slice;

    return dest + (slice << 8); // add output slice offset
}

static void ARM_CODE mmMixB(void)
{
    mm_word ch_mask = mm_ch_mask & 0xFFFF; // Clear top 16 bits (only 16 channels)
    mm_mixer_channel *mix_ch = &mm_mix_channels[0];

    DMA1_DEST = (mm_word)&(mm_mix_data.mix_data_b.fetch[0]);

    // Get mode B shadow data (word pointer, don't access individual fields)
    mm_word *shadow = (mm_word *)&(mm_mix_data.mix_data_b.shadow[0]);

    for (int i = 0; i < 16; i++, shadow++, mix_ch++)
    {
        if ((ch_mask & BIT(i)) == 0)
            continue;

        if (mix_ch->samp == 0) // Check if channel is disabled
        {
            *shadow = 64 << 16; // write pan=center, vol=silent
            mmbZerofillBuffer(mmb_getdest(i)); // zero wavebuffer
            continue;
        }

        // The channel is active

        mm_byte do_zero_padding;

        if (mix_ch->key_on == 1)
        {
            // New note
            mix_ch->key_on = 0; // clear start bit

            mm_word vol = mix_ch->vol;
            mm_word pan = mix_ch->tpan;

            mix_ch->cvol = vol | (pan << (16 + 9));

            // **todo: CLIP sample offset
            mix_ch->read = mix_ch->read << (8 + SFRAC);

            do_zero_padding = 1; // add zero padding
        }
        else
        {
            do_zero_padding = 0;
        }

        // Do volume ramping
        {
            // get volume+shift value
            mm_word volume = translateVolume(mix_ch->cvol);

            // assemble volume|shift|panning
            mm_hword pan = mix_ch->cpan >> 9;

            // -write to shadow
            *shadow = volume | (pan << 16);
        }

        mm_addr dest = mmb_getdest(i); // fill wave buffer

        mmbResampleData(dest, do_zero_padding, shadow, mix_ch);
    }

    // Swap mixing slice
    mm_output_slice ^= 1;
}

void mmMixC(mm_byte *volume_table, mm_word ch_mask, mm_mixer_channel *mix_ch);

ARM_CODE void mmMixerMix(void)
{
    // Do volume ramping
    SlideMixingLevels(SLIDE_THROTTLE);

    if (mm_mixing_mode == MM_MODE_A)
    {
        mmMixA();
    }
    else if (mm_mixing_mode == MM_MODE_B)
    {
        mmMixB();
    }
    else // if (mm_mixing_mode == MM_MODE_C)
    {
        extern mm_byte mmVolumeTable[];

        mm_byte *volume_table = &mmVolumeTable[0];
        mm_word ch_mask = mm_ch_mask;
        mm_mixer_channel *mix_ch = &mm_mix_channels[0];

        mmMixC(volume_table, ch_mask, mix_ch);
    }
}
