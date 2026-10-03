// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test all functions related to the jingle song layer.

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

    mm_sword nb_roots_id = mmGetModuleIdByName("nb_roots.mod");
    if (nb_roots_id == -1)
    {
        printf("mmGetSampleIdByName(\"nb_roots.mod\") failed\n");
        goto error;
    }

    WAV_FileStart(argv[2], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto error;

    // Play a module in the jingle layer and do some normal operations on it
    // ---------------------------------------------------------------------

    if (mmJingleStart(nb_roots_id, MM_PLAY_LOOP) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmJingleActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    // Pause it, it should become inactive

    mmJinglePause();

    if (mmJingleActive() == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(500);

    // Resume song, it should restart from the same position

    mmJingleResume();

    if (mmJingleActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    // Test changing the volume

    mmJinglePause();
    generate_ms(500);
    mmJingleResume();

    mmSetJingleVolume(512); // 50%

    generate_ms(1000);

    mmSetJingleVolume(1024); // 100%

    generate_ms(1000);

    // Test changing the tempo

    mmJinglePause();
    generate_ms(500);
    mmJingleResume();

    mmSetJingleTempo(2048); // 200%

    generate_ms(1000);

    mmSetJingleTempo(1024); // 100%

    generate_ms(1000);

    // Test changing the pitch

    mmJinglePause();
    generate_ms(500);
    mmJingleResume();

    mmSetJinglePitch(2048); // 200%

    generate_ms(1000);

    mmSetJinglePitch(1024); // 100%

    generate_ms(1000);

    // Stop the song and check that it's silent

    mmJingleStop();

    if (mmJingleActive() == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(500);

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();

    return ret;
}
