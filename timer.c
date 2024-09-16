#include "nasmfunc.h"
#include "int.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

void
init_pit(void)
{
  _io_out8(PIT_CTRL, 0x34);
  // interrupting frequency: 1193182 / data
  // 100Hz: 1193182 / 100 = 11932
  // 11932 = 0x2e9c
  _io_out8(PIT_CNT0, 0x9c);
  _io_out8(PIT_CNT0, 0x2e);
}

void
inthandler20(int *esp)
{
  _io_out8(PIC0_OCW2, 0x60);
}
