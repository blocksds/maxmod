// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test volume and panning functions.

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

    mm_sword helicopter_id = mmGetSampleIdByName("helicopter.wav");
    if (helicopter_id == -1)
    {
        printf("mmGetSampleIdByName(\"helicopter.wav\") failed\n");
        goto error;
    }

    mm_sword laser_id = mmGetSampleIdByName("laser2_1.wav");
    if (laser_id == -1)
    {
        printf("mmGetSampleIdByName(\"laser2_1.wav\") failed\n");
        goto error;
    }

    WAV_FileStart(argv[2], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto error;

    mm_sfxhand handle;

    // Per-SFX volume

    handle = mmEffect(laser_id);
    generate_ms(200);
    mmEffectCancel(handle);

    handle = mmEffect(laser_id);
    mmEffectVolume(handle, 128); // 50%
    generate_ms(200);
    mmEffectCancel(handle);

    generate_ms(100);

    // Global SFX volume

    mmSetEffectsVolume(1024); // 100%
    handle = mmEffect(laser_id);
    generate_ms(200);
    mmEffectCancel(handle);

    mmSetEffectsVolume(512); // 50%
    handle = mmEffect(laser_id);
    generate_ms(200);
    mmEffectCancel(handle);

    mmSetEffectsVolume(1024); // 100%
    generate_ms(100);

    // Panning

    handle = mmEffect(laser_id);
    mmEffectPanning(handle, 0); // Left
    generate_ms(200);
    mmEffectCancel(handle);

    handle = mmEffect(laser_id);
    mmEffectPanning(handle, 128); // Center
    generate_ms(200);
    mmEffectCancel(handle);

    handle = mmEffect(laser_id);
    mmEffectPanning(handle, 255); // Right
    generate_ms(200);
    mmEffectCancel(handle);

    generate_ms(100);

    handle = mmEffect(helicopter_id);
    for (int i = 0; i < 256; i++) // 256 not included
    {
        mmEffectPanning(handle, i); // Right
        generate_ms(5);
    }
    mmEffectCancel(handle);

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();

    return ret;
}
