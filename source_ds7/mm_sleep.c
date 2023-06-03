// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

// sleep functions are not needed!
#if 0

#include "maxmod7.h"
#include "mm_mixer_super.h"
#include "mm_main_ds.h"
#include "multiplatform_defs.h"

#define SELECTED_TIMER 0
#define BIAS_SLIDE_DELAY 64 // click is pretty muffled with this setting

/* ARM7 Only Functions */

/*
 * Enter Sleep:
 *   Disable updates [safely]
 *   Disable timer IRQ
 *   Disable sound
 *   Disable speakers
 *   Zero bias level
 */

/*
 * Exit Sleep:
 *   Reset bias level
 *   Enable speakers
 *   Enable timer IRQ
 *   Reset mixer [enables updates]
 */

// note: Sound bias level is not readable if the speakers are disabled
// (undocumented)

// Enter low power sleep mode
// Note: does not disable amplifier
void mmEnterSleep(void) {
    mmSuspendIRQ_t();
    
    // Disable updates
    TIMER_CR(SELECTED_TIMER) = 0;
    TIMER_DATA(SELECTED_TIMER) = 0;
    
    irqDisable(IRQ_TIMER(SELECTED_TIMER));
    
    mmRestoreIRQ_t();
    
    swiChangeSoundBias(0, BIAS_SLIDE_DELAY);
    
    // Disable sound
    REG_POWERCNT &= ~1;
}

// Exit low power sleep mode
void mmExitSleep(void) {
    // Enable sound
    REG_POWERCNT |= 1;
    
    swiChangeSoundBias(1, BIAS_SLIDE_DELAY);
    
    irqEnable(IRQ_TIMER(SELECTED_TIMER));
    
    mmSelectMode(mm_mixing_mode);
}

#endif
