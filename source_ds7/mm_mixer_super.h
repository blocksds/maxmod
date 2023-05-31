#ifndef MM_MIXER_SUPER_H__
#define MM_MIXER_SUPER_H__

#include "mm_types.h"

void mmMixerInit(void);
void mmMixerMix(void);
void mmMixerPre(void);

extern mm_byte mmVolumeDivTable[];
extern mm_byte mmVolumeShiftTable[];

#endif
