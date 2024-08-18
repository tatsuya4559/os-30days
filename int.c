#include "nasmfunc.h"
#include "graphic.h"
#include "iolib.h"
#include "int.h"

#define PORT_KEYDAT 0x0060

FIFO keyfifo;

/* PS/2キーボードからの割り込み */
void
inthandler21(int *esp)
{
    _io_out8(PIC0_OCW2, 0x61); // IRQ-01受付完了をPICに通知
    Byte keycode = _io_in8(PORT_KEYDAT);
    fifo_enqueue(&keyfifo, keycode);
}

/* PS/2マウスからの割り込み */
void
inthandler2c(int *esp)
{
    BootInfo *binfo = (BootInfo *) ADR_BOOTINFO;
    boxfill8(binfo->vram, binfo->scrnx, COLOR_BLACK, 0, 0, 32 * 8 - 1, 15);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COLOR_WHITE, "INT 2C (IRQ-12) : PS/2 mouse");
    for (;;) {
        _io_hlt();
    }
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
