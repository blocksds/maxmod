// SPDX-License-Identifier: ISC
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef FILE_H__
#define FILE_H__

#include <stddef.h>

void file_load(const char *filename, void **buffer, size_t *size_);

#endif // FILE_H__
