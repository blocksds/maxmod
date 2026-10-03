// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test all functions related to the main song layer.

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

    // Play a module in the main layer and do some normal operations on it
    // -------------------------------------------------------------------

    if (mmStart(nb_roots_id, MM_PLAY_LOOP) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    // Get its position

    if ((mmGetPositionTick() != 0) || (mmGetPositionRow() != 10) ||
        (mmGetPosition() != 0))
    {
        printf("Line %d: Check failed (%u, %u, %u)\n", __LINE__,
               mmGetPositionTick(), mmGetPositionRow(), mmGetPosition());
        goto error;
    }

    // Pause it, it should become inactive

    mmPause();

    if (mmActive() == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(500);

    // Resume song, it should restart from the same position

    mmResume();

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if ((mmGetPositionTick() != 0) || (mmGetPositionRow() != 10) ||
        (mmGetPosition() != 0))
    {
        printf("Line %d: Check failed (%u, %u, %u)\n", __LINE__,
               mmGetPositionTick(), mmGetPositionRow(), mmGetPosition());
        goto error;
    }

    generate_ms(1000);

    // Test changing the volume

    mmPause();
    generate_ms(500);
    mmResume();

    mmSetModuleVolume(512); // 50%

    generate_ms(1000);

    mmSetModuleVolume(1024); // 100%

    generate_ms(1000);

    // Test changing the tempo

    mmPause();
    generate_ms(500);
    mmResume();

    mmSetModuleTempo(2048); // 200%

    generate_ms(1000);

    mmSetModuleTempo(1024); // 100%

    generate_ms(1000);

    // Test changing the pitch

    mmPause();
    generate_ms(500);
    mmResume();

    mmSetModulePitch(2048); // 200%

    generate_ms(1000);

    mmSetModulePitch(1024); // 100%

    generate_ms(1000);

    // Change the position a few times and see what happens

    mmPause();
    generate_ms(500);
    mmResume();

    mmSetPositionEx(6, 20);

    if ((mmGetPositionTick() != 0) || (mmGetPositionRow() != 20) ||
        (mmGetPosition() != 6))
    {
        printf("Line %d: Check failed (%u, %u, %u)\n", __LINE__,
               mmGetPositionTick(), mmGetPositionRow(), mmGetPosition());
        goto error;
    }

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    mmSetPositionEx(1, 20);

    if ((mmGetPositionTick() != 0) || (mmGetPositionRow() != 20) ||
        (mmGetPosition() != 1))
    {
        printf("Line %d: Check failed (%u, %u, %u)\n", __LINE__,
               mmGetPositionTick(), mmGetPositionRow(), mmGetPosition());
        goto error;
    }

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    mmSetPositionEx(1, 20);

    if ((mmGetPositionTick() != 0) || (mmGetPositionRow() != 20) ||
        (mmGetPosition() != 1))
    {
        printf("Line %d: Check failed (%u, %u, %u)\n", __LINE__,
               mmGetPositionTick(), mmGetPositionRow(), mmGetPosition());
        goto error;
    }

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    // Stop the song and check that it's silent

    mmStop();

    if (mmActive() == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(500);

    // Restart song. Try to go after the last pattern order
    // ----------------------------------------------------

    // If the song loops, it should go back to pattern order 0

    mmStart(nb_roots_id, MM_PLAY_LOOP);
    mmSetPositionEx(99, 0);

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if ((mmGetPositionTick() != 0) || (mmGetPositionRow() != 0) ||
        (mmGetPosition() != 0))
    {
        printf("Line %d: Check failed (%u, %u, %u)\n", __LINE__,
               mmGetPositionTick(), mmGetPositionRow(), mmGetPosition());
        goto error;
    }

    mmStop();

    // If the song doesn't loop, it should end

    mmStart(nb_roots_id, MM_PLAY_ONCE);
    mmSetPositionEx(99, 0);

    if (mmActive() == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Restart song. Try to go after the last row in a pattern
    // -------------------------------------------------------

    mmStart(nb_roots_id, MM_PLAY_LOOP);

    mmSetPositionEx(1, 99);

    // The pattern order should be changed, but the row should be set to 0

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if ((mmGetPositionTick() != 0) || (mmGetPositionRow() != 0) ||
        (mmGetPosition() != 1))
    {
        printf("Line %d: Check failed (%u, %u, %u)\n", __LINE__,
               mmGetPositionTick(), mmGetPositionRow(), mmGetPosition());
        goto error;
    }

    mmStop();

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();

    return ret;
}
