// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

#include <stdlib.h>
#include <stdio.h>

#include <maxmod_headless.h>

#include "file.h"
#include "wav_utils.h"

#define SAMPLE_RATE (32 * 1024)

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Invalid number of arguments\n");
        return -1;
    }

    // Load file

    void *soundbank_buffer = NULL;
    size_t soundbank_size;

    file_load(argv[1], &soundbank_buffer, &soundbank_size);
    if (soundbank_size == 0)
        goto cleanup;

    // Play music until the song ends, while saving it to a WAV

    if (!mmInitDefault(soundbank_buffer, 20, SAMPLE_RATE))
    {
        printf("mmInitDefault() failed\n");
        goto cleanup;
    }

    mmStart(0, MM_PLAY_ONCE);

    WAV_FileStart(argv[2], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto cleanup;

    int frames = 0;

    while (mmActive())
    {
#define SAMPLES (SAMPLE_RATE / 60)
        int8_t buffer[SAMPLES * 2];

        mmFrame(buffer, SAMPLES);

        for (int i = 0; i < SAMPLES * 2; i++)
            buffer[i] += 128; // Make it unsigned

        WAV_FileStream(buffer, sizeof(buffer));

        frames++;

        if (frames > 60 * 60 * 10) // 10 minutes limit
            break;
    }

    WAV_FileEnd();

    mmEnd();

cleanup:
    free(soundbank_buffer);
    return 0;
}
