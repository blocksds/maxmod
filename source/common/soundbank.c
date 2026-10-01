// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026, Antonio Niño Díaz

#include <string.h>
#include <mm_msl.h>

#if defined(__GBA__)
#include "gba/main_gba.h"
#elif defined(__NDS__)
#include "ds/arm7/main_ds7.h"
#elif defined(__HEADLESS__)
#include "headless/main_headless.h"
#endif

static mm_sword mppGetIdByName(mm_word *ptr, const char *name)
{
    while (1)
    {
        mm_word entry = *ptr++;

        // End of list
        if (entry == 0)
            return -1;

        mm_sword id = entry & 0xFFFFFF;
        mm_word name_len = entry >> 24;

        if (strcmp(name, (const char *)ptr) == 0)
            return id;

        ptr += name_len / sizeof(mm_word);
    }
}

static const char *mppGetNameById(mm_word *ptr, mm_sword reference_id)
{
    while (1)
    {
        mm_word entry = *ptr++;

        // End of list
        if (entry == 0)
            return NULL;

        mm_sword id = entry & 0xFFFFFF;
        mm_word name_len = entry >> 24;

        if (id == reference_id)
            return (const char *)ptr;

        ptr += name_len / sizeof(mm_word);
    }
}

mm_sword mmGetSampleIdByName(const char *name)
{
    if (name == NULL)
        return -1;

    mm_word *ptr = mppGetSampleNameList();
    if (ptr == NULL)
        return -1;

    return mppGetIdByName(ptr, name);
}

mm_sword mmGetModuleIdByName(const char *name)
{
    if (name == NULL)
        return -1;

    mm_word *ptr = mppGetModuleNameList();
    if (ptr == NULL)
        return -1;

    return mppGetIdByName(ptr, name);
}

const char *mmGetSampleNameById(mm_sword id)
{
    mm_word *ptr = mppGetSampleNameList();
    if (ptr == NULL)
        return NULL;

    return mppGetNameById(ptr, id);
}

const char *mmGetModuleNameById(mm_sword id)
{
    mm_word *ptr = mppGetModuleNameList();
    if (ptr == NULL)
        return NULL;

    return mppGetNameById(ptr, id);
}
