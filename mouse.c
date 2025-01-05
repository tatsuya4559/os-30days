#include "nasmfunc.h"
#include "keyboard.h"
#include "mouse.h"
#include "int.h"

FIFO *mousefifo;
int32_t mousedata0;

void
enable_mouse(
  FIFO *fifo,
  int32_t data0,
  MouseDecoder *mdec
)
{
  mousefifo = fifo;
  mousedata0 = data0;

  wait_KBC_sendready();
  _io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  _io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
  mdec->phase = MOUSE_DEC_PHASE_WAITING_0XFA;
}

int
mouse_decode(MouseDecoder *mdec, uint8_t data)
{
  switch (mdec->phase) {
    case MOUSE_DEC_PHASE_WAITING_0XFA:
      if (data == 0xfa) {
        mdec->phase = MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE;
      }
      return 0;
    case MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE:
      if ((data & 0xc8) == 0x08) {
        mdec->buf[0] = data;
        mdec->phase = MOUSE_DEC_PHASE_RECEIVED_2ND_BYTE;
      }
      return 0;
    case MOUSE_DEC_PHASE_RECEIVED_2ND_BYTE:
      mdec->buf[1] = data;
      mdec->phase = MOUSE_DEC_PHASE_RECEIVED_3RD_BYTE;
      return 0;
    case MOUSE_DEC_PHASE_RECEIVED_3RD_BYTE:
      mdec->buf[2] = data;
      mdec->phase = MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE;
      mdec->btn = mdec->buf[0] & 0x07; // extract lower 3 bits
      mdec->x = mdec->buf[1];
      mdec->y = mdec->buf[2];
      if ((mdec->buf[0] & 0x10) != 0) {
        mdec->x |= 0xffffff00;
      }
      if ((mdec->buf[0] & 0x20) != 0) {
        mdec->y |= 0xffffff00;
      }
      mdec->y = -mdec->y; // マウスではy方向の符号が画面と逆
      return 1;
    default:
      return -1; // unreachable
  }
}

/* PS/2マウスからの割り込み */
void
inthandler2c(int32_t *esp)
{
  _io_out8(PIC1_OCW2, 0x64); // IRQ-12受付完了をPIC1に通知
  _io_out8(PIC0_OCW2, 0x62); // IRQ-02受付完了をPIC0に通知
  int32_t data = _io_in8(PORT_KEYDAT);
  fifo_enqueue(mousefifo, data + mousedata0);
}

