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

mm_word mpp_Process_VolumeCommand(mpl_layer_information*, mm_active_channel*, mm_module_channel*, mm_word);

// TODO: Convert these methods
mm_word mpp_Process_Effect_Wrapper(mpl_layer_information*, mm_active_channel*, mm_module_channel*, mm_word);

mm_word mpp_Update_ACHN_notest_Wrapper(mpl_layer_information *layer, mm_active_channel *act_ch,
                                       mm_module_channel *channel, mm_word period, mm_word ch);

void mpp_Channel_NewNote(mm_module_channel*, mpl_layer_information*);

mm_word mppe_DoVibrato(mm_word period, mm_module_channel *channel, mpl_layer_information *layer);

mm_word mpph_PitchSlide_Up(mm_word, mm_word, mpl_layer_information*);
mm_word mpph_LinearPitchSlide_Up(mm_word period, mm_word slide_value, mpl_layer_information *layer);
mm_word mpph_FinePitchSlide_Up(mm_word period, mm_word slide_value, mpl_layer_information *layer);

mm_word mpph_PitchSlide_Down(mm_word, mm_word, mpl_layer_information*);
mm_word mpph_LinearPitchSlide_Down(mm_word period, mm_word slide_value, mpl_layer_information *layer);
mm_word mpph_FinePitchSlide_Down(mm_word period, mm_word slide_value, mpl_layer_information *layer);

mm_word mppe_glis_backdoor_Wrapper(mm_word volcmd, mm_module_channel *channel,
                                   mm_active_channel *act_ch, mpl_layer_information *layer,
                                   mm_word period);

#endif // MM_MAS_ARM_H
