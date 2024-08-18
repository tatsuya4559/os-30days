#include "nasmfunc.h"
#include "common.h"
#include "iolib.h"
#include "graphic.h"
#include "dsctbl.h"
#include "int.h"

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

    init_palette();

    BootInfo *binfo = (BootInfo *) ADR_BOOTINFO;
    set_screen(binfo);
    show_message(binfo);

    _io_out8(PIC0_IMR, 0xf9); // PIC1とキーボードを許可
    _io_out8(PIC1_IMR, 0xef); // マウスを許可

    Byte keycode, s[4];
    for (;;) {
        _io_cli(); // 割り込み禁止
        if (keybuf.len == 0) {
            _io_stihlt();
        } else {
            keycode = keybuf_dequeue(&keybuf);
            _io_sti(); // 割り込み禁止解除
            sprintf(s, "%x", keycode);
            boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COLOR_WHITE, s);
        }
    }
}
