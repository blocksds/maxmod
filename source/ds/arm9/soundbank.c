// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025-2026, Antonio Niño Díaz

#include <stdio.h>
#include <stdlib.h>

#include <maxmod9.h>
#include <mm_mas.h>
#include <mm_types.h>
#include <mm_msl.h>

#include "ds/arm9/main_ds9.h"

#define MM_FILENAME_SIZE 64

// Pointer to the sound bank when it's stored in RAM. Not used when the sound
// bank is in the filesystem.
static msl_head *mmsAddress;

// This pointer is the soundbank being used by Maxmod when it's stored in the
// filesystem. Not used when the sound bank is in RAM.
// TODO: If mmEnd() is ever implemented, this needs to be closed.
static FILE *mmsFile;

// Default soundbank handler (memory)
static mm_word mmsHandleMemoryOp(mm_word msg, mm_word param)
{
    mm_word retval = 0;

    switch (msg)
    {
        case MMCB_SONGREQUEST:
            if (param < mmsAddress->head_data.moduleCount)
            {
                mm_word index = param + mmsAddress->head_data.sampleCount;
                retval = ((mm_word)mmsAddress->sampleTable[index]) + ((mm_word)mmsAddress);
            }
            break;

        case MMCB_SAMPREQUEST:
            if (param < mmsAddress->head_data.sampleCount)
            {
                mm_word index = param;
                retval = ((mm_word)mmsAddress->sampleTable[index]) + ((mm_word)mmsAddress);
            }
            break;

        default:
            break;
    }

    return retval;
}

// Load a file from the soundbank and return memory pointer, if it succeeded
static mm_word mmLoadDataFromSoundBank(mm_word index, mm_word command)
{
    if (mmsFile == NULL)
        return 0;

    if (command == 0) // Load module
    {
        if (index >= mmGetModuleCount())
            return 0;

        // The modules array starts right after the samples array
        index += mmGetSampleCount();
    }
    else if (command == 1) // Load samples
    {
        if (index >= mmGetSampleCount())
            return 0;
    }

    if (fseek(mmsFile, sizeof(msl_head_data) + (index * sizeof(mm_addr)), SEEK_SET) != 0)
        return 0;

    mm_word offset = 0;

    if (fread(&offset, sizeof(mm_word), 1, mmsFile) == 0)
        return 0;

    if (fseek(mmsFile, offset, SEEK_SET) != 0)
        return 0;

    mm_word size = 0;

    if (fread(&size, sizeof(mm_word), 1, mmsFile) == 0)
        return 0;

    size += sizeof(mm_mas_prefix);

    mm_byte *data = malloc(size);

    if (data == NULL)
        return 0;

    if (fseek(mmsFile, offset, SEEK_SET) != 0)
        return 0;

    if (fread(data, sizeof(mm_byte), size, mmsFile) != size)
        return 0;

    return (mm_word)data;
}

// Default soundbank handler (filesystem)
static mm_word mmsHandleFileOp(mm_word msg, mm_word param)
{
    mm_word retval = 0;

    switch (msg)
    {
        case MMCB_SONGREQUEST:
            retval = mmLoadDataFromSoundBank(param, 0);
            break;

        case MMCB_SAMPREQUEST:
            retval = mmLoadDataFromSoundBank(param, 1);
            break;

        case MMCB_DELETESONG:
        case MMCB_DELETESAMPLE:
            free((mm_addr)param);
            break;

        default:
            break;
    }

    return retval;
}

// Setup default handler for a soundbank loaded in memory
void mmSoundBankInMemory(mm_addr address)
{
    mmsAddress = (msl_head *)address;

    mmSetCustomSoundBankHandler(mmsHandleMemoryOp);
}

// Setup default handler for a soundbank file
mm_bool mmSoundBankInFiles(const char *filename)
{
    mmsFile = fopen(filename, "rb");
    if (mmsFile == NULL)
        return false;

    mmSetCustomSoundBankHandler(mmsHandleFileOp);
    return true;
}

// Setup default handler for a soundbank file
void mmSetCustomSoundBankHandler(mm_callback p_loader)
{
    mmcbMemory = p_loader;
}
