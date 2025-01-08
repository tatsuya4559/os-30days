#include "nasmfunc.h"
#include "mtask.h"

Timer *mt_timer;
int32_t mt_tr;

void
mt_init(void)
{
  mt_timer = timer_alloc();
  // Omit calling timer_init
  timer_set_timeout(mt_timer, 2);
  mt_tr = 3 * 8;
}

void
mt_taskswitch(void)
{
  if (mt_tr == 3 * 8) {
    mt_tr = 4 * 8;
  } else {
    mt_tr = 3 * 8;
  }
  timer_set_timeout(mt_timer, 2);
  _farjmp(0, mt_tr);
}
