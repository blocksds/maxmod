// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include "mm_main_ds.h"
#include "multiplatform_defs.h"
#include "useful_qualifiers.h"

static uint32_t previous_irq_state;

// Function to disable interrupts via the status register
void mmSuspendIRQ_t() {
    previous_irq_state = getCPSR() & CPSR_FLAG_IRQ_DIS;
    setCPSR(previous_irq_state | CPSR_FLAG_IRQ_DIS);
}

// Function to enable interrupts via the status register
void mmRestoreIRQ_t() {
    setCPSR(previous_irq_state | (getCPSR() & (~CPSR_FLAG_IRQ_DIS)));
}
