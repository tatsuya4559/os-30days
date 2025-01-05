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
  timerctl.timeout = 0;
}

void
inthandler20(int32_t *esp)
{
  // Notify PIC(Programmable Interrupt Controller) that IRQ-00 has been accepted.
  _io_out8(PIC0_OCW2, 0x60);

  timerctl.count++;
  if (timerctl.timeout > 0) {
    timerctl.timeout--;
    if (timerctl.timeout == 0) {
      fifo_enqueue(timerctl.bus, timerctl.data);
    }
  }
}

void
set_timer(uint32_t timeout, FIFO *bus, uint8_t data)
{
  // Disable interrupts because the timer configuration must be atomic.
  int eflags = _io_load_eflags();
  _io_cli();

  timerctl.timeout = timeout;
  timerctl.bus = bus;
  timerctl.data = data;

  // Restore interrupts.
  _io_store_eflags(eflags);
}
