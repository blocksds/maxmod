// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#include <nds.h>

#include "mm_main_ds.h"

static uint32_t previous_irq_state;

// Function to disable interrupts via the status register
void mmSuspendIRQ_t(void)
{
    uint32_t base_cpsr_state = getCPSR();
    previous_irq_state = base_cpsr_state & CPSR_FLAG_IRQ_DIS;
    setCPSR(base_cpsr_state | CPSR_FLAG_IRQ_DIS);
}

// Function to enable interrupts via the status register
void mmRestoreIRQ_t(void)
{
    setCPSR(previous_irq_state | (getCPSR() & (~CPSR_FLAG_IRQ_DIS)));
}
