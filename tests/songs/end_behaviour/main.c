// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test how the song behaves when it reaches the end with looping enabled or
// disabled.

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

    mm_sword panning_id = mmGetModuleIdByName("panning.mod");
    if (panning_id == -1)
    {
        printf("mmGetSampleIdByName(\"panning.mod\") failed\n");
        goto error;
    }

    WAV_FileStart(argv[2], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto error;

    // Play a module in the main layer. Loop enabled.
    // ----------------------------------------------

    if (mmStart(panning_id, MM_PLAY_LOOP) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    mmSetModuleTempo(2048); // 200% (make it go faster!)

    generate_ms(3000);

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    mmStop();

    // Play a module in the main layer. Loop disabled.
    // -----------------------------------------------

    if (mmStart(panning_id, MM_PLAY_ONCE) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    mmSetModuleTempo(2048); // 200% (make it go faster!)

    generate_ms(3000);

    if (mmActive() == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Play a module in the jingle layer. Loop enabled.
    // ------------------------------------------------

    if (mmJingleStart(panning_id, MM_PLAY_LOOP) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    mmSetJingleTempo(2048); // 200% (make it go faster!)

    generate_ms(3000);

    if (mmJingleActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    mmJingleStop();

    // Play a module in the jingle layer. Loop disabled.
    // -------------------------------------------------

    if (mmJingleStart(panning_id, MM_PLAY_ONCE) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    mmSetJingleTempo(2048); // 200% (make it go faster!)

    generate_ms(3000);

    if (mmJingleActive() == 1)
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
