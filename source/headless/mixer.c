// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021-2026, Antonio Niño Díaz

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <maxmod_headless.h>
#include <mm_mas.h>

#include "headless/main_headless.h"
#include "headless/mixer.h"

mm_mixer_channel *mm_mix_channels;

mm_word mm_bpmdv;

static mm_word mm_ratescale;

static mm_word mm_mixch_count;

// Set channel volume
void mmMixerSetVolume(int channel, mm_word volume)
{
    mm_mix_channels[channel].vol = volume;
}

// Set channel panning
void mmMixerSetPan(int channel, mm_byte panning)
{
    mm_mix_channels[channel].pan = panning;
}

// Scale mixing frequency
void mmMixerMulFreq(int channel, mm_word factor)
{
    mm_word freq = mm_mix_channels[channel].freq;

    freq = (freq * factor) >> 10;

    mm_mix_channels[channel].freq = freq;
}

// Stop mixing channel
void mmMixerStopChannel(int channel)
{
    // Set MSB (disable) of source
    mm_mix_channels[channel].src = MIXCH_HEADLESS_SRC_STOPPED;
}

// Set channel read position
void mmMixerSetRead(int channel, mm_word value)
{
    // Store new offset
    mm_mix_channels[channel].read = value;
}

// Set channel mixing rate
void mmMixerSetFreq(int channel, mm_word rate)
{
    mm_mix_channels[channel].freq = rate << 2;
}

// Initialize mixer
void mmMixerInit(mm_headless_system *setup)
{
    mm_mixch_count = setup->mix_channel_count;

    mm_mix_channels = setup->mixing_channels;

    // 15768 * 16384 / rate
    mm_ratescale = 15768 * 16384 / setup->sample_rate;

    // rate * 2.5
    mm_bpmdv = setup->sample_rate * 2.5;

    // Disable mixing channels

    mm_mixer_channel *mix_ch = &mm_mix_channels[0];

    for (mm_word i = 0; i < mm_mixch_count; i++)
        mix_ch[i].src = MIXCH_HEADLESS_SRC_STOPPED;
}

void mmMixerEnd(void)
{
    // Nothing to do
}

void mmMixerMix(mm_addr wave_buffer, mm_word samples_count)
{
    if (samples_count == 0)
        return;

    // Part 0. Initialization
    // ----------------------

    // Allocate left and right samples for stereo
    mm_sword *mm_mixbuffer = calloc(samples_count, sizeof(mm_sword) * 2);
    if (mm_mixbuffer == NULL)
        return;

    // Begin mixing routine
    // --------------------

    for (mm_word ch = 0; ch < mm_mixch_count; ch++)
    {
        mm_mixer_channel *rchan = &mm_mix_channels[ch];

        if (rchan->src & MIXCH_HEADLESS_SRC_STOPPED)
            continue;

        // Part 1: Calculations
        // --------------------

        // Read frequency

        mm_word rfreq = rchan->freq;
        if (rfreq == 0)
            continue;

        rfreq = (rfreq * mm_ratescale) >> 14;

        // Calculate volume of right and left speakers

        mm_word rvol = rchan->vol; // volume = 0-255
        if (rvol == 0)
            continue;

        // pan = 0-255

        mm_sword volL = (256 - rchan->pan) * rvol; // right volume = (vol*pan)
        mm_sword volR = rchan->pan * rvol; // left volume = (256-pan) * vol
        // volL and volR go from 0 to 256*256

        // Part 2: Mixing
        // --------------

        mm_mas_gba_sample *sample = (mm_mas_gba_sample *)(rchan->src - sizeof(mm_mas_gba_sample));

        // Fetch samples from the waveform at the current playback frequency and
        // add them to mm_mixbuffer

        mm_word rread = rchan->read;

        for (mm_word i = 0; i < samples_count; i++)
        {
            mm_sword val = sample->data[rread >> MP_SAMPFRAC];
            rread += rfreq;

            // The waveform stored in the soundbank is unsigned, make it signed
            // so that it's easier to operate on it.
            val -= 128;

            mm_mixbuffer[i * 2 + 0] += val * volL;
            mm_mixbuffer[i * 2 + 1] += val * volR;

            // Check if we've reached the end of the sample
            if (rread >= (sample->length << MP_SAMPFRAC))
            {
                // The sample doesn't loop, stop it
                if (sample->loop_length == 0xFFFFFFFF)
                {
                    rchan->src = MIXCH_HEADLESS_SRC_STOPPED;
                    break;
                }

                rread -= sample->loop_length << MP_SAMPFRAC;
            }
        }

        rchan->read = rread;
    }

    // Part 3. Post-processing
    // -----------------------

    // Copy mm_mixbuffer to the output buffer interleaving left and right samples.
    mm_sbyte *pwrite = wave_buffer;

    for (mm_word i = 0; i < samples_count; i++)
    {
        mm_sword sampleL = mm_mixbuffer[i * 2 + 0];
        mm_sword sampleR = mm_mixbuffer[i * 2 + 1];

        // Divide by the max volume, max panning, and a bit extra because of having
        // multiple channels
        sampleL /= 256 * 256 * 4;
        sampleR /= 256 * 256 * 4;

        if (sampleL > 127)
            sampleL = 127;
        if (sampleL < -128)
            sampleL = -128;

        if (sampleR > 127)
            sampleR = 127;
        if (sampleR < -128)
            sampleR = -128;

        *pwrite++ = sampleL;
        *pwrite++ = sampleR;
    }

    free(mm_mixbuffer);
}
