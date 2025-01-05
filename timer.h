#pragma once

#include "types.h"

typedef struct {
  /**
   * The number of interrupts that have occurred since the system started.
   */
  uint32_t count;
} TimerCtl;

extern TimerCtl timerctl;

/**
 * Initialize PIT.
 * PIT stands for Programmable Interval Timer.
 */
void init_pit(void);

/**
 * Interrupt handler for IRQ0.
 */
void inthandler20(int32_t *esp);
