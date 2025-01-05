#include "nasmfunc.h"
#include "int.h"

#define PORT_KEYDAT 0x0060

FIFO keyfifo, mousefifo;

/* PS/2キーボードからの割り込み */
void
inthandler21(int32_t *esp)
{
  _io_out8(PIC0_OCW2, 0x61); // IRQ-01受付完了をPICに通知
  uint8_t keycode = _io_in8(PORT_KEYDAT);
  fifo_enqueue(&keyfifo, keycode);
}

/* PS/2マウスからの割り込み */
void
inthandler2c(int32_t *esp)
{
  uint8_t data;
  _io_out8(PIC1_OCW2, 0x64); // IRQ-12受付完了をPIC1に通知
  _io_out8(PIC0_OCW2, 0x62); // IRQ-02受付完了をPIC0に通知
  data = _io_in8(PORT_KEYDAT);
  fifo_enqueue(&mousefifo, data);
}

void
init_pic(void)
{
  _io_out8(PIC0_IMR,  0xff  ); /* すべての割り込みを受け付けない */
  _io_out8(PIC1_IMR,  0xff  ); /* すべての割り込みを受け付けない */

  _io_out8(PIC0_ICW1, 0x11  ); /* エッジトリガモード */
  _io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7は、INT20-27で受ける */
  _io_out8(PIC0_ICW3, 1 << 2); /* PIC1はIRQ2にて接続 */
  _io_out8(PIC0_ICW4, 0x01  ); /* ノンバッファモード */

  _io_out8(PIC1_ICW1, 0x11  ); /* エッジトリガモード */
  _io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15は、INT28-2fで受ける */
  _io_out8(PIC1_ICW3, 2     ); /* PIC1はIRQ2にて接続 */
  _io_out8(PIC1_ICW4, 0x01  ); /* ノンバッファモード */

  _io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外は全て禁止 */
  _io_out8(PIC1_IMR,  0xff  ); /* 11111111 全ての割り込みを受け付けない */
}
