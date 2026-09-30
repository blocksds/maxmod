// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

#include <stdlib.h>
#include <stdio.h>

#include <maxmod_headless.h>

#include "wav_utils.h"

#define SAMPLE_RATE (32 * 1024)

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Invalid number of arguments:\n");
        printf("\n");
        printf("Usage:\n");
        printf("\n");
        printf("    wav_renderer soundbank.bin song_name.mod output.wav\n");
        return -1;
    }

    // Initialize Maxmod

    if (!mmInitDefault(argv[1], 20, SAMPLE_RATE,
                       MM_OUTFMT_STEREO_U8)) // WAV expects unsigned 8-bit
    {
        printf("mmInitDefault() failed\n");
        return -1;
    }

    // Play the requested song until the end (or a timeout)

    mm_word module_id = mmGetModuleIdByName(argv[2]);
    if (module_id == -1)
    {
        printf("mmGetModuleIdByName() failed\n");
        mmEnd();
        return -1;
    }

    mmStart(module_id, MM_PLAY_ONCE);

    WAV_FileStart(argv[3], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
    {
        printf("WAV_FileIsOpen() failed\n");
        mmEnd();
        return -1;
    }

    int frames = 0;

    while (mmActive())
    {
#define SAMPLES (SAMPLE_RATE / 100)
        int8_t buffer[SAMPLES * 2];

        mmMix(buffer, SAMPLES);

        WAV_FileStream(buffer, sizeof(buffer));

        frames++;

        if (frames > 100 * 60 * 10) // 10 minutes limit
            break;
    }

    WAV_FileEnd();

    mmEnd();

    return 0;
}
