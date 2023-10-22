#ifndef MM_MIXER_SUPER_H__
#define MM_MIXER_SUPER_H__

#include "mm_types.h"
#include "mm_mixer_ds_types.h"

void mmMixerInit(void);
void mmMixerMix(void);

extern mm_byte mm_output_slice;
extern mm_mode_enum mm_mixing_mode;
extern mm_byte mmVolumeDivTable[];
extern mm_byte mmVolumeShiftTable[];
extern mm_mix_data_ds mm_mix_data;

#endif
