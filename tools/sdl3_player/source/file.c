// SPDX-License-Identifier: ISC
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdio.h>
#include <stdlib.h>

void file_load(const char *filename, void **buffer, size_t *size_)
{
    FILE *f = fopen(filename, "rb");
    size_t size;

    *buffer = NULL;
    if (size_)
        *size_ = 0;

    if (f == NULL)
    {
        printf("File couldn't be opened: %s\n", filename);
        return;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    if (size_)
        *size_ = size;

    if (size == 0)
    {
        printf("File size is 0: %s\n", filename);
        fclose(f);
        return;
    }

    rewind(f);
    *buffer = malloc(size);
    if (*buffer == NULL)
    {
        printf("Not enought memory to load file: %s\n", filename);
        fclose(f);
        return;
    }

    if (fread(*buffer, size, 1, f) != 1)
    {
        printf("Error while reading file: %s\n", filename);
        fclose(f);
        free(*buffer);
        return;
    }

    fclose(f);
}
