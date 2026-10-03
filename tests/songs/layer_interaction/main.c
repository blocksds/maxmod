// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Make sure that functions that affect the jingle/main layer don't affect the
// other layer.

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

    // Play a module in the jingle layer and try to affect it with main layer functions
    // --------------------------------------------------------------------------------

    mmSetJingleVolume(1024); // 100%
    mmSetJinglePitch(1024); // 100%
    if (mmJingleStart(nb_roots_id, MM_PLAY_LOOP) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmActive() == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmJingleActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    mmPause();
    mmStop();

    mmSetModuleVolume(128); // 12.5%
    mmSetModulePitch(2048); // 200%

    mmStop();

    mmSetPositionEx(2, 2);

    generate_ms(1000);

    // We test the jingle first because after we play any song in the main layer
    // the values of the position will have been modified and we aren't able to
    // check if they have been modified by the jingle or the main layer. By
    // checking them now, the only possible reason why they may have changed is
    // that the jingle layer has changed them.
    if ((mmGetPositionTick() != 0) || (mmGetPositionRow() != 0) ||
        (mmGetPosition() != 0))
    {
        printf("Line %d: Check failed (%u, %u, %u)\n", __LINE__,
               mmGetPositionTick(), mmGetPositionRow(), mmGetPosition());
        goto error;
    }

    // Done

    mmJingleStop();

    generate_ms(1000);

    // Play a module in the main layer and try to affect it with jingle functions
    // ---------------------------------------------------------------------------

    mmSetModuleVolume(1024); // 100%
    mmSetModulePitch(1024); // 100%
    if (mmStart(nb_roots_id, MM_PLAY_LOOP) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmJingleActive() == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmActive() == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    mmJinglePause();
    mmJingleStop();

    mmSetJingleVolume(128); // 12.5%
    mmSetJinglePitch(2048); // 200%

    mmJingleStop();

    generate_ms(1000);

    // Done

    mmStop();

    generate_ms(1000);

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();

    return ret;
}
