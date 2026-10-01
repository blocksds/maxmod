// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test a simple oneshot SFX and a looping one.

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

    mm_sword laser_id = mmGetSampleIdByName("laser2_1.wav");
    if (laser_id == -1)
    {
        printf("mmGetSampleIdByName(\"laser2_1.wav\") failed\n");
        goto error;
    }

    mm_sword helicopter_id = mmGetSampleIdByName("helicopter.wav");
    if (helicopter_id == -1)
    {
        printf("mmGetSampleIdByName(\"helicopter.wav\") failed\n");
        goto error;
    }

    WAV_FileStart(argv[2], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto error;

    mm_sfxhand handle;

    // Non-looping SFX

    handle = mmEffect(laser_id);
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

    generate_ms(600); // Wait until the end

    if (mmEffectActive(handle) == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Looping SFX

    handle = mmEffect(helicopter_id);
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

    generate_ms(1500); // Wait until the end of the original length and a bit more

    if (mmEffectActive(handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
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

    generate_ms(100);

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();

    return ret;
}
