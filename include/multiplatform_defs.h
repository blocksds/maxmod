// SPDX-License-Identifier: ISC
//
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MULTIPLATFORM_DEFS__
#define MULTIPLATFORM_DEFS__

#include <mm_types.h>

#ifdef SYS_NDS

#include <nds.h>
//#define REG_SOUNDXCNT ((volatile mmsoundcnt_ds*)(0x4000400))

#endif
#ifdef SYS_GBA

//#include <gba.h>
#define REG_IME *((volatile mm_hword*)(0x4000208))

#endif


#endif
