// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_DS_ARM7_COMMS7_H__
#define MM_DS_ARM7_COMMS7_H__

#include <mm_types.h>

#include "ds/common/comm_messages.h"

mm_bool mmARM9msg(mm_byte cmd, mm_word value);
mm_bool mmSendUpdateToARM9(void);
void mmProcessComms(void);
void mmSetupComms(mm_word);

#endif // MM_DS_ARM7_COMMS7_H__
