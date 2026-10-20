// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

#define DATA_ID 0x61746164
#define FMT_ID  0x20746d66
#define RIFF_ID 0x46464952
#define WAVE_ID 0x45564157

// This struct is good enough to read files saved by the other tools in the
// repository, but not enough to read any WAV file.
typedef struct
{
    // RIFF header
    uint32_t chunk_id;          // "RIFF" == 0x46464952
    uint32_t chunk_size;        // File size - size of (chunk_id + chunk_size)
    uint32_t format;            // "WAVE" == 0x45564157

    // "fmt " subchunk
    uint32_t subchunk_1_id;     // "fmt " == 0x20746D66
    uint32_t subchunk_1_size;   // 16 for PCM
    uint16_t audio_format;      // PCM = 1
    uint16_t num_channels;      // Mono = 1, Stereo = 2
    uint32_t sample_rate;       // Samples per second
    uint32_t byte_rate;         // sample_rate * num_channels * bits_per_sample / 8
    uint16_t block_align;       // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample;   // 8 bits = 8, 16 bits = 16, etc.

    // "data" subchunk
    uint32_t subchunk_2_id;     // "data" == 0x61746164
    uint32_t subchunk_2_size;   // Size of data following this entry
    // uint8_t data[]           // Sound data is stored here
}
wav_header_t;

bool check_header(const wav_header_t *header)
{
    if (header->chunk_id != RIFF_ID)
    {
        printf("Wrong RIFF ID %lx\n", header->chunk_id);
        return false;
    }

    if (header->format != WAVE_ID)
    {
        printf("Wrong WAVE ID %lx\n", header->format);
        return false;
    }

    if (header->subchunk_1_id != FMT_ID)
    {
        printf("Wrong FMT ID %lx\n", header->subchunk_1_id);
        return false;
    }

    if (header->subchunk_2_id != DATA_ID)
    {
        printf("Wrong subchunk 2 ID %lx\n", header->subchunk_2_id);
        return false;
    }

    if (header->audio_format != 1)
    {
        printf("Not a PCM WAV\n");
        return false;
    }

    if (header->num_channels != 2)
    {
        printf("Not a stereo WAV\n");
        return false;
    }

    if (header->bits_per_sample != 8)
    {
        printf("Not a 8 BPP WAV\n");
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Invalid number of arguments:\n");
        printf("\n");
        printf("Usage:\n");
        printf("\n");
        printf("    wav_diff file_1.wav file_2.wav\n");
        return -1;
    }

    int ret = -1;

    const char *filename_1 = argv[1];
    const char *filename_2 = argv[2];

    void *filedata_1 = NULL;
    size_t filesize_1 = 0;
    void *filedata_2 = NULL;
    size_t filesize_2 = 0;

    file_load(filename_1, &filedata_1, &filesize_1);
    file_load(filename_2, &filedata_2, &filesize_2);

    if ((filedata_1 == NULL) || (filedata_2 == NULL))
    {
        printf("Failed to load WAV files...\n");
        goto cleanup;
    }

    if (filesize_1 <= sizeof(wav_header_t))
    {
        printf("File is too small: %s\n", filename_1);
        goto cleanup;
    }

    if (filesize_2 <= sizeof(wav_header_t))
    {
        printf("File is too small: %s\n", filename_2);
        goto cleanup;
    }

    if (memcmp(filedata_1, filedata_2, sizeof(wav_header_t)) != 0)
    {
        printf("WAV headers don't match!\n");
        goto cleanup;
    }

    const wav_header_t *header = filedata_1;

    if (check_header(header) == false)
        goto cleanup;

    size_t data_size = header->subchunk_2_size;
    uint8_t *data_1 = (uint8_t*)((uintptr_t)filedata_1 + sizeof(wav_header_t));
    uint8_t *data_2 = (uint8_t*)((uintptr_t)filedata_2 + sizeof(wav_header_t));

    bool waveform_matches = true;

    // We don't want to report every mismatched sample, we want to group time
    // ranges where there are mismatches. Once there is a mismatch, we need to
    // find at least 1/5th of a second that matches to report that period of
    // time as an individual range.
    const size_t samples_threshold = header->byte_rate / 5; // 1/5th of a second

    size_t samples_since_last_change = samples_threshold;
    bool previously_matching = true;

    // Keep track of the time range that has the mismatch
    size_t start_of_mismatch = 0;
    size_t end_of_mismatch = 0;

    size_t total_mismatched_samples = 0;

    for (size_t i = 0; i < data_size; i += 2)
    {
        bool match = (data_1[i] == data_2[i]) && (data_1[i + 1] == data_2[i + 1]);

        if (!match)
            end_of_mismatch = i;

        if (previously_matching)
        {
            if (!match)
            {
                // Switch to "not matching" state very easily
                previously_matching = false;
                samples_since_last_change = 0;
                start_of_mismatch = i;
            }
        }
        else
        {
            if (match)
            {
                // Switch to "matching" state only after some time has passed
                // without any errors.
                samples_since_last_change++;
                if (samples_since_last_change >= samples_threshold)
                {
                    previously_matching = true;
                    printf("Mismatch: %.2f s to %.2f s\n",
                           (float)start_of_mismatch / header->byte_rate,
                           (float)end_of_mismatch / header->byte_rate);

                    total_mismatched_samples += end_of_mismatch - start_of_mismatch;
                }
            }
        }
    }

    if (previously_matching == false)
    {
        printf("Mismatch: %.2f s to %.2f s (end)\n",
               (float)start_of_mismatch / header->byte_rate,
               (float)data_size / header->byte_rate);

        total_mismatched_samples += end_of_mismatch - data_size;
    }

    if (total_mismatched_samples > 0)
    {
        printf("Waveform match: %.2f%% (%.2f / %.2f seconds)\n",
               100.f * total_mismatched_samples / data_size,
               (float)(data_size - total_mismatched_samples) / header->byte_rate,
               (float)data_size / header->byte_rate);
    }
    else
    {
        ret = 0;
    }

cleanup:
    free(filedata_1);
    free(filedata_2);
    return ret;
}
