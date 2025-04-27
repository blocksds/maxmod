// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_MAS_ARM_H
#define MM_MAS_ARM_H

#include "mm_types.h"
#include "mp_mas_structs.h"

extern mpl_layer_information mmLayerMain;
extern mpl_layer_information mmLayerSub;

extern mpl_layer_information *mpp_layerp;
extern mpv_active_information mpp_vars;
extern mm_addr mpp_channels;
extern mm_word mpp_resolution;

extern mm_word mm_mastertempo;
extern mm_word mm_masterpitch;

extern mm_byte mpp_nchannels;
extern mm_byte mpp_clayer;
extern mm_active_channel *mm_achannels;
extern mm_addr mm_pchannels;
extern mm_word mm_num_mch;
extern mm_word mm_num_ach;
extern mm_module_channel mm_schannels[]; // [MP_SCHANNELS]

void mmSetResolution(mm_word);
void mmPulse(void);
void mppUpdateSub(void);
void mppProcessTick(void);

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
