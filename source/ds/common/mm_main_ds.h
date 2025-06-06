// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_MAIN_DS_H
#define MM_MAIN_DS_H

#include "mm_types.h"

extern mm_addr mmModuleBank;
extern mm_addr mmSampleBank;

void mmSuspendIRQ_t(void);
void mmRestoreIRQ_t(void);

#endif // MM_MAIN_DS_H
