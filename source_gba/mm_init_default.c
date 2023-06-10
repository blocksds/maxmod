// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <stdint.h>
#include <stdlib.h>

#include "maxmod.h"

#include "mp_mas_structs.h"

#define mixlen 1056 // 16 KHz

static uint32_t mixbuffer[mixlen / sizeof(uint32_t)];

void mmInitDefault(mm_addr soundbank, mm_word number_of_channels)
{
    // Allocate buffer
    size_t size_of_channel = sizeof(mm_module_channel) + sizeof(mm_active_channel) + sizeof(mm_mixer_channel);
    size_t size_of_buffer = mixlen + (number_of_channels * size_of_channel);
    mm_addr buffer = malloc(size_of_buffer);

    // The original library didn't check the return value of malloc... :/
    if(buffer == NULL)
        return;

    // Split up buffer
    mm_addr wave_memory, module_channels, active_channels, mixing_channels;

    wave_memory = buffer;
    module_channels = (mm_addr)(((mm_word)wave_memory) + mixlen);
    active_channels = (mm_addr)(((mm_word)module_channels) + (number_of_channels * sizeof(mm_module_channel)));
    mixing_channels = (mm_addr)(((mm_word)active_channels) + (number_of_channels * sizeof(mm_active_channel)));

    mm_gba_system setup =
    {
        .mixing_mode = 3,
        .mod_channel_count = number_of_channels,
        .mix_channel_count = number_of_channels,
        .module_channels = module_channels,
        .active_channels = active_channels,
        .mixing_channels = mixing_channels,
        .mixing_memory = (mm_addr)&mixbuffer[0],
        .wave_memory = wave_memory,
        .soundbank = soundbank
    };

    mmInit(&setup);
}
