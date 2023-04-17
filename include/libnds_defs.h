#ifndef LIBNDS_DEFS__
#define LIBNDS_DEFS__

#include "mm_types.h"

#define REG_IME *((volatile mm_hword*)(0x4000208))
#define REG_SOUNDXCNT ((volatile mmsoundcnt_ds*)(0x4000400))

void irqSet(mm_word, mm_voidfunc);
void irqEnable(mm_word);

#endif
