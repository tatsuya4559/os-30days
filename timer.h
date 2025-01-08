#pragma once

#include "types.h"
#include "fifo.h"

#define MAX_TIMERS 500

typedef enum {
  TIMER_UNUSED,
  TIMER_ALLOCATED,
  TIMER_RUNNING,
} TimerState;

typedef struct Timer_tag {
  TimerState state;
  /**
   * When count reaches this value, the data is pushed into the bus.
   */
  uint32_t fired_at;
  FIFO *bus;
  int32_t data;
  /**
   * The next timer in the linked list.
   *
   * HACK: I embedded this field here to avoid dynamic memory allocation.
   */
  struct Timer_tag *next;
} Timer;

/**
 * Initialize PIT.
 * PIT stands for Programmable Interval Timer.
 */
void init_pit(void);

/**
 * Interrupt handler for IRQ0.
 */
void inthandler20(int32_t *esp);

Timer *timer_alloc(void);
void timer_init(Timer *timer, FIFO *bus, uint8_t data);
void timer_free(Timer *timer);
/**
 * Set the timeout in centiseconds.
 */
void timer_set_timeout(Timer *timer, uint32_t timeout);
