// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

// Test calling SFX functions with invalid arguments.

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

    WAV_FileStart(argv[2], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto error;

    mm_sfxhand laser_handle;

    // Try to play effects that don't exist
    // ------------------------------------

    // The last valid ID is `mmGetSampleCount() - 1`
    laser_handle = mmEffect(mmGetSampleCount());
    if (laser_handle != MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    laser_handle = mmEffect(UINT32_MAX);
    if (laser_handle != MM_SFXHAND_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Try to get invalid IDs and names from the soundbank dictionary
    // --------------------------------------------------------------

    // mmGetSampleIdByName()

    if (mmGetSampleIdByName("laser2_1.wa") != -1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmGetSampleIdByName("laser2_1.wavv") != -1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmGetSampleIdByName(NULL) != -1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmGetSampleIdByName("") != -1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // mmGetModuleIdByName()

    if (mmGetModuleIdByName("arpeggio.mo") != -1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmGetModuleIdByName("arpeggio.mods") != -1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmGetModuleIdByName(NULL) != -1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmGetModuleIdByName("") != -1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // mmGetSampleNameById();

    if (mmGetSampleNameById(UINT32_MAX) != NULL)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // The maximum valid ID is `mmGetSampleCount() - 1`
    if (mmGetSampleNameById(mmGetSampleCount()) != NULL)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // mmGetModuleNameById();

    if (mmGetModuleNameById(UINT32_MAX) != NULL)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // The maximum valid ID is `mmGetSampleCount() - 1`
    if (mmGetModuleNameById(mmGetModuleCount()) != NULL)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Play a non-looping effect, wait until it ends, try to modify it
    // ---------------------------------------------------------------

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

    generate_ms(800);

    if (mmEffectActive(laser_handle) == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Call functions that should fail

    if (mmEffectVolume(laser_handle, 1024) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectPanning(laser_handle, 128) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectRate(laser_handle, 1024) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectScaleRate(laser_handle, 1024) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectRelease(laser_handle ) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectCancel(laser_handle ) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    // Play an effect, stop it, try to modify it
    // -----------------------------------------

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

    if (mmEffectCancel(laser_handle ) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    // Call functions that should fail

    if (mmEffectVolume(laser_handle, 1024) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectPanning(laser_handle, 128) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectRate(laser_handle, 1024) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectScaleRate(laser_handle, 1024) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectRelease(laser_handle ) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    if (mmEffectCancel(laser_handle ) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto error;
    }

    generate_ms(1000);

    // Test volumes outside of valid ranges (they should be clamped)
    // -------------------------------------------------------------

    // This can't really be tested from code, check the WAV file

    // Reference
    laser_handle = mmEffect(laser_id);
    generate_ms(500);
    mmEffectCancel(laser_handle);

    laser_handle = mmEffect(laser_id);
    mmEffectVolume(laser_handle, 5000);
    generate_ms(500);
    mmEffectCancel(laser_handle);

    mmSetEffectsVolume(5000);
    laser_handle = mmEffect(laser_id);
    generate_ms(500);
    mmEffectCancel(laser_handle);
    mmSetEffectsVolume(1024);

    generate_ms(1000);

    // Test panning outside of valid ranges (they should be clamped)
    // -------------------------------------------------------------

    // Note: It isn't possible to set panning values outside of the valud range.
    // The panning argument is a byte, and the valid range is 0-255.

    ret = 0;
error:

    WAV_FileEnd();
    mmEnd();

    return ret;
}
