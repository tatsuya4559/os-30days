#include "common.h"
#include "nasmfunc.h"
#include "keyboard.h"
#include "int.h"

FIFO *keyfifo;
int32_t keydata0;

void
wait_KBC_sendready(void)
{
  for (;;) {
    if ((_io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
      return;
    }
  }
}

void
init_keyboard(FIFO *fifo, int32_t data0)
{
  keyfifo = fifo;
  keydata0 = data0;

  wait_KBC_sendready();
  _io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_KBC_sendready();
  _io_out8(PORT_KEYDAT, KBC_MODE);
}

/* PS/2キーボードからの割り込み */
void
inthandler21(int32_t *esp)
{
  _io_out8(PIC0_OCW2, 0x61); // IRQ-01受付完了をPICに通知
  int32_t keycode = _io_in8(PORT_KEYDAT);
  fifo_enqueue(keyfifo, keycode + keydata0);
}

