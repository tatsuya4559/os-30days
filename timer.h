#pragma once

#include "types.h"
#include "fifo.h"

typedef struct {
  /**
   * The number of interrupts that have occurred since the system started.
   */
  uint32_t count;
  /**
   * The remaining time until timeout.
   * When this value reaches 0, the data is pushed into the bus.
   */
  uint32_t timeout;
  FIFO *bus;
  uint8_t data;
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

/**
 * Set the timer.
 * After the specified time has passed, the data is pushed into the bus.
 *
 * @param timeout The time until the data is pushed into the bus.
 * @param bus The bus to push the data into.
 * @param data The data to push into the bus.
 */
void set_timer(uint32_t timeout, FIFO *bus, uint8_t data);
