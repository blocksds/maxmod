// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test playing module files from MAS files outside of a soundbank. Note that
// the headless platform needs a soundbank to be initialized. It compares it to
// the same sample played from a soundbank.

#include <stdio.h>
#include <stdlib.h>

#include <maxmod_headless.h>

#include "file.h"
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

    if (argc != 4)
    {
        printf("Invalid number of arguments.\n");
        return -1;
    }

    const char *soundbank = argv[1];
    const char *mas_file = argv[2];
    const char *wav_file = argv[3];

    printf("Soundbank:  %s\n", soundbank);
    printf("MAS file:   %s\n", mas_file);
    printf("WAV output: %s\n", wav_file);

    void *mas_data = NULL;
    size_t mas_data_size = 0;

    file_load(mas_file, &mas_data, &mas_data_size);
    if ((mas_data == NULL) || (mas_data_size == 0))
        return -1;

    if (!mmInitDefault(soundbank, 20, SAMPLE_RATE,
                       MM_OUTFMT_STEREO_U8)) // WAV expects unsigned 8-bit
    {
        printf("mmInitDefault() failed\n");
        goto error;
    }

    mm_sword laser_id = mmGetSampleIdByName("helicopter.wav");
    if (laser_id == -1)
    {
        printf("mmGetSampleIdByName(\"helicopter.wav\") failed\n");
        goto error;
    }

    WAV_FileStart(wav_file, SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto error;

    // Play a module file directly from a MAS file
    // -------------------------------------------

    mm_sound_effect effect_external =
    {
        .id = 0,
        .sample = mas_data,
        .rate = 1024,
        .handle = 0,
        .volume = 255,
        .panning = 128
    };

    mm_sound_effect effect_msl =
    {
        .id = laser_id,
        .sample = NULL,
        .rate = 1024,
        .handle = 0,
        .volume = 255,
        .panning = 128
    };

    mm_sfxhand handle = mmEffectEx(&effect_external);
    generate_ms(2000);
    mmEffectCancel(handle);
    generate_ms(200);

    mm_sfxhand handle2 = mmEffectEx(&effect_msl);
    generate_ms(2000);
    mmEffectCancel(handle2);
    generate_ms(200);

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();
    free(mas_data);

    return ret;
}
