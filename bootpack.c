#include "nasmfunc.h"
#include "common.h"
#include "iolib.h"
#include "graphic.h"
#include "dsctbl.h"
#include "int.h"

#define  PORT_KEYDAT           0x0060
#define  PORT_KEYSTA           0x0064
#define  PORT_KEYCMD           0x0064
#define  KEYSTA_SEND_NOTREADY  0x02
#define  KEYCMD_WRITE_MODE     0x60
#define  KEYCMD_SENDTO_MOUSE   0xd4
#define  MOUSECMD_ENABLE       0xf4
#define  KBC_MODE              0x47

static
void
wait_KBC_sendready(void)
{
    for (;;) {
        if ((_io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            return;
        }
    }
}

static
void
init_keyboard(void)
{
    wait_KBC_sendready();
    _io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    _io_out8(PORT_KEYDAT, KBC_MODE);
}

static
void
enable_mouse(void)
{
    wait_KBC_sendready();
    _io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    _io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
}

static
void
set_screen(BootInfo *binfo)
{
    Byte *vram = binfo->vram;

    boxfill8(vram, binfo->scrnx, COLOR_DARK_CYAN, 0, 0, binfo->scrnx - 1, binfo->scrny - 29);
    boxfill8(vram, binfo->scrnx, COLOR_LIGHT_GRAY, 0, binfo->scrny - 28, binfo->scrnx - 1, binfo->scrny - 28);
    boxfill8(vram, binfo->scrnx, COLOR_WHITE, 0, binfo->scrny - 27, binfo->scrnx - 1, binfo->scrny - 27);
    boxfill8(vram, binfo->scrnx, COLOR_LIGHT_GRAY, 0, binfo->scrny - 26, binfo->scrnx - 1, binfo->scrny - 1);

    boxfill8(vram, binfo->scrnx, COLOR_WHITE, 3, binfo->scrny - 24, 59, binfo->scrny - 24);
    boxfill8(vram, binfo->scrnx, COLOR_WHITE, 2, binfo->scrny - 24, 2, binfo->scrny - 4);
    boxfill8(vram, binfo->scrnx, COLOR_DARK_GRAY, 3, binfo->scrny - 4, 59, binfo->scrny - 4);
    boxfill8(vram, binfo->scrnx, COLOR_DARK_GRAY, 59, binfo->scrny - 23, 59, binfo->scrny - 5);
    boxfill8(vram, binfo->scrnx, COLOR_BLACK, 2, binfo->scrny - 3, 59, binfo->scrny - 3);
    boxfill8(vram, binfo->scrnx, COLOR_BLACK, 60, binfo->scrny - 24, 60, binfo->scrny - 3);

    boxfill8(vram, binfo->scrnx, COLOR_DARK_GRAY, binfo->scrnx - 47, binfo->scrny - 24, binfo->scrnx - 4, binfo->scrny - 24);
    boxfill8(vram, binfo->scrnx, COLOR_DARK_GRAY, binfo->scrnx - 47, binfo->scrny - 23, binfo->scrnx - 47, binfo->scrny - 4);
    boxfill8(vram, binfo->scrnx, COLOR_WHITE, binfo->scrnx - 47, binfo->scrny - 3, binfo->scrnx - 4, binfo->scrny - 3);
    boxfill8(vram, binfo->scrnx, COLOR_WHITE, binfo->scrnx - 3, binfo->scrny - 24, binfo->scrnx - 3, binfo->scrny - 3);
}

static
void
show_message(BootInfo *binfo)
{
    Byte *vram = binfo->vram;

    putfonts8_asc(vram, binfo->scrnx, 40, 40, COLOR_WHITE, "Hello World!");

    char s[20];
    sprintf(s, "scrnx = %d", binfo->scrnx);
    putfonts8_asc(vram, binfo->scrnx, 40, 80, COLOR_WHITE, s);

    int mx, my;
    Byte mcursor[256];
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COLOR_DARK_CYAN);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COLOR_WHITE, s);
}

void
hari_main(void)
{
    init_gdtidt();
    init_pic();
    _io_sti();

    Byte keybuf[32], mousebuf[128];
    fifo_init(&keyfifo, 32, keybuf);
    fifo_init(&mousefifo, 128, mousebuf);

    _io_out8(PIC0_IMR, 0xf9); // PIC1とキーボードを許可
    _io_out8(PIC1_IMR, 0xef); // マウスを許可

    init_keyboard();

    init_palette();
    BootInfo *binfo = (BootInfo *) ADR_BOOTINFO;
    set_screen(binfo);
    show_message(binfo);

    enable_mouse();

    Byte keycode, s[4];
    for (;;) {
        _io_cli(); // 割り込み禁止
        if (keyfifo.len == 0 && mousefifo.len == 0) {
            _io_stihlt();
        } else if (keyfifo.len != 0) {
            keycode = fifo_dequeue(&keyfifo);
            _io_sti(); // 割り込み禁止解除
            sprintf(s, "%x", keycode);
            boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COLOR_WHITE, s);
        } else if (mousefifo.len != 0) {
            keycode = fifo_dequeue(&mousefifo);
            _io_sti();
            sprintf(s, "%x", keycode);
            boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, 32, 16, 47, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COLOR_WHITE, s);
        }
    }
}
