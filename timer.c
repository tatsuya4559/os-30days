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
  for (int32_t i = 0; i < MAX_TIMERS; i++) {
    timerctl.timers[i].state = TIMER_UNUSED;
  }
}

Timer *
timer_alloc(void)
{
  for (int32_t i = 0; i < MAX_TIMERS; i++) {
    if (timerctl.timers[i].state == TIMER_UNUSED) {
      timerctl.timers[i].state = TIMER_ALLOCATED;
      return &timerctl.timers[i];
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
  timer->fired_at = timeout + timerctl.count;
  timer->state = TIMER_RUNNING;
}

void
inthandler20(int32_t *esp)
{
  // Notify PIC(Programmable Interrupt Controller) that IRQ-00 has been accepted.
  _io_out8(PIC0_OCW2, 0x60);

  timerctl.count++;
  for (int32_t i = 0; i < MAX_TIMERS; i++) {
    if (timerctl.timers[i].state == TIMER_RUNNING) {
      if (timerctl.timers[i].fired_at <= timerctl.count) {
        timerctl.timers[i].state = TIMER_ALLOCATED;
        fifo_enqueue(timerctl.timers[i].bus, timerctl.timers[i].data);
      }
    }
  }
}
