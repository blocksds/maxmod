// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include <stdio.h>
#include <stdlib.h>

#include <maxmod9.h>
#include <mm_mas.h>
#include <mm_types.h>
#include <mm_msl.h>

#include "ds/arm9/main9.h"

#ifndef MM_FILENAME_SIZE
#define MM_FILENAME_SIZE 64
#endif
#define FILENAME_EOS '\0'
#define NOT_PROCESSED_RETVAL 0
#define FILE_PREFIX_SIZE 8

static mm_word mmsHandleMemoryOp(mm_word, mm_word);
static mm_word mmsHandleFileOp(mm_word, mm_word);
static mm_word mmLoadDataFromSoundBank(mm_word, mm_word);
static mm_word failLoadData(FILE*);

msl_head* mmsAddress;
char mmsFile[MM_FILENAME_SIZE + 1];

// Setup default handler for a soundbank loaded in memory
void mmSoundBankInMemory(mm_addr address)
{
    mmsAddress = (msl_head*)address;

    mmSetCustomSoundBankHandler(mmsHandleMemoryOp);
}

// Setup default handler for a soundbank file
void mmSoundBankInFiles(const char *filename)
{
    int i = 0;
    // Store filename
    for (; i < MM_FILENAME_SIZE; i++)
    {
        if (filename[i] == FILENAME_EOS)
            break;
        mmsFile[i] = filename[i];
    }
    mmsFile[i] = FILENAME_EOS;

    mmSetCustomSoundBankHandler(mmsHandleFileOp);
}

// Setup default handler for a soundbank file
void mmSetCustomSoundBankHandler(mm_callback p_loader)
{
    mmcbMemory = p_loader;
}

// Default soundbank handler (memory)
static mm_word mmsHandleMemoryOp(mm_word msg, mm_word param)
{
    mm_word retval = NOT_PROCESSED_RETVAL;

    switch (msg)
    {
        case MMCB_SONGREQUEST:
            if (param < mmsAddress->head_data.moduleCount)
                retval = ((mm_word)mmsAddress->sampleTable[param + mmsAddress->head_data.sampleCount]) + ((mm_word)mmsAddress);
            break;
        case MMCB_SAMPREQUEST:
            if (param < mmsAddress->head_data.sampleCount)
                retval = ((mm_word)mmsAddress->sampleTable[param]) + ((mm_word)mmsAddress);
            break;
        default:
            break;
    }

    return retval;
}

// Default soundbank handler (filesystem)
static mm_word mmsHandleFileOp(mm_word msg, mm_word param)
{
    mm_word retval = NOT_PROCESSED_RETVAL;

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

// Failure handling function for mmLoadDataFromSoundBank
static mm_word failLoadData(FILE *fp)
{
    fclose(fp);
    return NOT_PROCESSED_RETVAL;
}

// Load a file from the soundbank and return memory pointer, if it succeeded
static mm_word mmLoadDataFromSoundBank(mm_word index, mm_word command)
{
    FILE *fp = fopen(mmsFile, "rb");

    if (fp == NULL)
        return NOT_PROCESSED_RETVAL;

    msl_head_data head_data;

    if (fread(&head_data, sizeof(msl_head_data), 1, fp) == 0)
        return failLoadData(fp);

    if (command == 0)
    {
        if (index >= head_data.moduleCount)
            return failLoadData(fp);
        index += head_data.sampleCount;
    }
    else if (command == 1)
    {
        if (index >= head_data.sampleCount)
            return failLoadData(fp);
    }

    if (fseek(fp, sizeof(msl_head_data) + (index * sizeof(mm_addr)), SEEK_SET) != 0)
        return failLoadData(fp);

    mm_addr offset = 0;

    if (fread(&offset, sizeof(mm_addr), 1, fp) == 0)
        return failLoadData(fp);

    if (fseek(fp, (mm_word)offset, SEEK_SET) != 0)
        return failLoadData(fp);

    mm_word size = 0;

    if (fread(&size, sizeof(mm_word), 1, fp) == 0)
        return failLoadData(fp);

    size += FILE_PREFIX_SIZE;

    mm_byte *data = malloc(size);

    if (data == NULL)
        return failLoadData(fp);

    if (fseek(fp, (mm_word)offset, SEEK_SET) != 0)
        return failLoadData(fp);

    if (fread(data, sizeof(mm_byte), size, fp) != size)
        return failLoadData(fp);

    fclose(fp);

    return (mm_word)data;
}
