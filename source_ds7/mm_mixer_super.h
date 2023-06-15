#ifndef MM_MIXER_SUPER_H__
#define MM_MIXER_SUPER_H__

#include "mm_types.h"

void mmMixerInit(void);
void mmMixerMix(void);

extern mm_byte mm_mixing_mode;
extern mm_byte mmVolumeDivTable[];
extern mm_byte mmVolumeShiftTable[];

#endif
