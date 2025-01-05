#pragma once

#include "types.h"
#include "fifo.h"

#define MAX_TIMERS 500

typedef enum {
  TIMER_UNUSED,
  TIMER_ALLOCATED,
  TIMER_RUNNING,
} TimerState;

typedef struct {
  TimerState state;
  /**
   * When count reaches this value, the data is pushed into the bus.
   */
  uint32_t fired_at;
  FIFO *bus;
  uint8_t data;
} Timer;

typedef struct {
  /**
   * The number of interrupts that have occurred since the system started.
   */
  uint32_t count;
  uint32_t next_fired_at;
  uint32_t num_running_timers;
  Timer *running_timers[MAX_TIMERS];
  Timer underlying_timers[MAX_TIMERS];
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

Timer *timer_alloc(void);
void timer_free(Timer *timer);
void timer_init(Timer *timer, FIFO *bus, uint8_t data);
/**
 * Set the timeout in centiseconds.
 */
void timer_set_timeout(Timer *timer, uint32_t timeout);
