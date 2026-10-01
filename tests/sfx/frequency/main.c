// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test that modifies the frequency of a SFX while it is being played.

#include <stdio.h>

#include <maxmod_headless.h>

#include "wav_utils.h"

#define SAMPLE_RATE 15768 // Default GBA frequency

void generate_ms(unsigned int ms)
{
    unsigned int samples = (SAMPLE_RATE * ms) / 1000;

    int8_t buffer[samples * 2];

    mmMix(buffer, samples);

    WAV_FileStream(buffer, sizeof(buffer));
}

int main(int argc, char *argv[])
{
    int ret = -1;

    if (argc != 3)
    {
        printf("Invalid number of arguments.\n");
        return -1;
    }

    if (!mmInitDefault(argv[1], 20, SAMPLE_RATE,
                       MM_OUTFMT_STEREO_U8)) // WAV expects unsigned 8-bit
    {
        printf("mmInitDefault() failed\n");
        return -1;
    }

    mm_sword sine_id = mmGetSampleIdByName("sine.wav");
    if (sine_id == -1)
    {
        printf("mmGetSampleIdByName(\"sine.wav\") failed\n");
        goto error;
    }

    WAV_FileStart(argv[2], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto error;

    mm_sfxhand handle;

    // sine.wav has loop information, so it should loop

    handle = mmEffect(sine_id);
    if (handle == MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectActive(handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(500);

    for (uint32_t i = 1; i < 32; i++)
    {
        mmEffectRate(handle, (i * 1024) / 16); // Limits are 512 - 2048
        generate_ms(100);
    }

    if (mmEffectCancel(handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectActive(handle) == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();

    return ret;
}
