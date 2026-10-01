// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test the effect of releasing channels.

#include <stdio.h>

#include <maxmod_headless.h>

#include "wav_utils.h"

#define NUM_CHANNELS 5
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

    if (!mmInitDefault(argv[1], NUM_CHANNELS, SAMPLE_RATE,
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

    mm_sfxhand helicopter_handle;
    mm_sfxhand laser_handle;

    // First, test that a non-released SFX can't be cancelled automatically
    // --------------------------------------------------------------------

    // Start sound that can't be cancelled
    helicopter_handle = mmEffect(helicopter_id);
    if (helicopter_handle == MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(100);

    // Fill the other channels
    for (int i = 0; i < NUM_CHANNELS - 1; i++)
    {
        laser_handle = mmEffect(laser_id);
        if (laser_handle == MM_SFXHAND_INVALID)
        {
            printf("Line %d: Check failed (iteration %d)\n", __LINE__, i);
            goto error;
        }
        generate_ms(5);
    }

    // Try to play another sound effect and fail
    laser_handle = mmEffect(laser_id);
    if (laser_handle != MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }
    generate_ms(5);

    // The original effect should still be active
    if (mmEffectActive(helicopter_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Wait for all other effects to end
    generate_ms(1000);

    // We should be able to cancel the first effect
    if (mmEffectCancel(helicopter_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(100);

    // Second, test that a released SFX can be cancelled automatically
    // ---------------------------------------------------------------

    // Start the original effect that we want to test
    helicopter_handle = mmEffect(helicopter_id);
    if (helicopter_handle == MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(100);

    // Fill the other channels
    for (int i = 0; i < NUM_CHANNELS - 1; i++)
    {
        laser_handle = mmEffect(laser_id);
        if (laser_handle == MM_SFXHAND_INVALID)
        {
            printf("Line %d: Check failed (iteration %d)\n", __LINE__, i);
            goto error;
        }
        generate_ms(5);
    }

    // The original effect should still be playing
    if (mmEffectActive(helicopter_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Release the original effect. It's handle becomes invalid
    if (mmEffectRelease(helicopter_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // We can't cancel the effect
    if (mmEffectCancel(helicopter_handle) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // We shouldn't be able to check if it's active or not
    if (mmEffectActive(helicopter_handle) == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(50);

    // Try to play another sound effect and replace the released effect
    laser_handle = mmEffect(laser_id);
    if (laser_handle == MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectActive(laser_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(50);

    // At this point, the original effect should have been cancelled, but we
    // can't check because the handle is invalid (check the WAV file).

    // Any other effect should fail because there are no more released channels.

    laser_handle = mmEffect(laser_id);
    if (laser_handle != MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    // Third, test that a released SFX can be cancelled with mmEffectCancelAll()
    // -------------------------------------------------------------------------

    helicopter_handle = mmEffect(helicopter_id);
    if (helicopter_handle == MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }
    if (mmEffectActive(helicopter_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(100);

    laser_handle = mmEffect(laser_id);
    if (laser_handle == MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }
    if (mmEffectActive(laser_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(100);

    // Release one of the effects, this invalidates its handle
    if (mmEffectRelease(helicopter_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Cancel all released and non-released effects, invalidate all handles
    mmEffectCancelAll();

    // We can't cancel either effect because both handles are invalid
    if (mmEffectCancel(helicopter_handle) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectCancel(laser_handle) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // We can't check here that the effects are actually cancelled, check the
    // output in the WAV file to verify that they have stopped.

    generate_ms(1000);

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();

    return ret;
}
