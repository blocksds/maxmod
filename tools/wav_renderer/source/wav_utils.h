// SPDX-License-Identifier: ISC
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef WAV_UTILS_H__
#define WAV_UTILS_H__

#include <stddef.h>
#include <stdint.h>

void WAV_FileStart(const char *path, uint32_t sample_rate);
void WAV_FileEnd(void);

int WAV_FileIsOpen(void);

void WAV_FileStream(void *buffer, size_t size);

#endif // WAV_UTILS_H__
