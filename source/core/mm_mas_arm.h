// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_MAS_ARM_H
#define MM_MAS_ARM_H

#include "mm_types.h"
#include "mp_mas_structs.h"

mm_word mmAllocChannel(void);
void mmUpdateChannel_T0(mm_module_channel*, mpl_layer_information*, mm_byte);
void mmUpdateChannel_TN(mm_module_channel*, mpl_layer_information*);
mm_word mmGetPeriod(mpl_layer_information*, mm_word, mm_byte);
void mmReadPattern(mpl_layer_information*);

extern mm_word IT_PitchTable[];
extern mm_hword ST3_FREQTABLE[];

// TODO: Convert these methods
mm_word mpp_Process_VolumeCommand_Wrapper(mpl_layer_information*, mm_active_channel*, mm_module_channel*, mm_word);
mm_word mpp_Process_Effect_Wrapper(mpl_layer_information*, mm_active_channel*, mm_module_channel*, mm_word);
mm_word mpp_Update_ACHN_notest_Wrapper(mpl_layer_information*, mm_active_channel*, mm_module_channel*, mm_word);
void mpp_Channel_NewNote(mm_module_channel*, mpl_layer_information*);

#endif // MM_MAS_ARM_H
