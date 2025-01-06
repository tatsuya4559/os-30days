#include "nasmfunc.h"
#include "int.h"
#include "timer.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

TimerCtl timerctl;

void
init_pit(void)
{
  // AL = 0x34; OUT(0x43, AL);
  // AL = lower 8 bits of interrupting frequency; OUT(0x40, AL);
  // AL = upper 8 bits of interrupting frequency; OUT(0x40, AL);
  _io_out8(PIT_CTRL, 0x34);
  // Interrupting frequency is the clock frequency divided by the set value
  // If the set value is 11932(0x2e9c in hexadecimal), the interrupting frequency
  // is about 100Hz.
  // 100Hz means the interrupt is called every 10 milliseconds.
  _io_out8(PIT_CNT0, 0x9c);
  _io_out8(PIT_CNT0, 0x2e);

  // Initialize timerctl
  timerctl.count = 0;
  timerctl.running_timers = NULL;
  for (int32_t i = 0; i < MAX_TIMERS; i++) {
    timerctl.underlying_timers[i].state = TIMER_UNUSED;
  }

  // Register a sentinel timer that never fires.
  Timer *sentinel = timer_alloc();
  timer_init(sentinel, NULL, 0);
  sentinel->fired_at = UINT32_MAX;
  sentinel->state = TIMER_RUNNING;
  timerctl.running_timers = sentinel;
}

Timer *
timer_alloc(void)
{
  for (int32_t i = 0; i < MAX_TIMERS; i++) {
    if (timerctl.underlying_timers[i].state == TIMER_UNUSED) {
      timerctl.underlying_timers[i].state = TIMER_ALLOCATED;
      return &timerctl.underlying_timers[i];
    }
  }
  return NULL;
}

void
timer_free(Timer *timer)
{
  timer->state = TIMER_UNUSED;
}

void
timer_init(Timer *timer, FIFO *bus, uint8_t data)
{
  timer->bus = bus;
  timer->data = data;
}

void
timer_set_timeout(Timer *timer, uint32_t timeout)
{
  _io_cli(); // Disable interrupts

  timer->fired_at = timeout + timerctl.count;
  timer->state = TIMER_RUNNING;

  // Insert the timer into the linked list of running timers.
  // We keep the list sorted so that inthandler works more efficiently.
  Timer *prev = NULL;
  Timer *curr = timerctl.running_timers;
  for (;;) {
    // Timer must be inserted before the sentinel.
    if (timer->fired_at < curr->fired_at) {
      if (prev == NULL) { // The timer is the first one.
        timer->next = timerctl.running_timers;
        timerctl.running_timers = timer;
      } else {
        timer->next = curr;
        prev->next = timer;
      }
      break;
    }
    prev = curr;
    curr = curr->next;
  }

  _io_sti(); // Enable interrupts
}

void
inthandler20(int32_t *esp)
{
  // Notify PIC(Programmable Interrupt Controller) that IRQ-00 has been accepted.
  _io_out8(PIC0_OCW2, 0x60);

  // FIXME: This OS cannot count time longer than MAX_UINT32 * 10ms.
  timerctl.count++;

  // Monitor the very next timer to be fired.
  if (timerctl.running_timers->fired_at > timerctl.count) {
    return;
  }

  // Fire the timer.
  // FIXME: I don't fire multiple timers simultaneously.
  Timer *timer = timerctl.running_timers;
  timer->state = TIMER_ALLOCATED;
  fifo_enqueue(timer->bus, timer->data);

  timerctl.running_timers = timer->next;
}
