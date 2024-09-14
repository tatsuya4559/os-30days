#include "nasmfunc.h"
#include "common.h"
#include "iolib.h"
#include "graphic.h"
#include "dsctbl.h"
#include "int.h"
#include "keyboard.h"
#include "mouse.h"
#include "memory.h"
#include "layer.h"

#define MEMMAN_ADDR 0x003c0000

void
hari_main(void)
{
    init_gdtidt();
    init_pic();
    _io_sti();

    BootInfo *binfo = (BootInfo *) ADR_BOOTINFO;
    MemoryManager *memman = (MemoryManager *) MEMMAN_ADDR;

    Byte keybuf[32], mousebuf[128];
    fifo_init(&keyfifo, 32, keybuf);
    fifo_init(&mousefifo, 128, mousebuf);
    MouseDec mdec;

    _io_out8(PIC0_IMR, 0xf9); // PIC1とキーボードを許可
    _io_out8(PIC1_IMR, 0xef); // マウスを許可

    init_keyboard();
    enable_mouse(&mdec);

    unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    LayerCtl *layerctl;
    Layer *layer_back, *layer_mouse;
    Byte *buf_back, buf_mouse[256];

    init_palette();
    layerctl = layerctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    layer_back = layer_alloc(layerctl);
    layer_mouse = layer_alloc(layerctl);
    buf_back = (Byte *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    layer_setbuf(layer_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    layer_setbuf(layer_mouse, buf_mouse, 16, 16, 99);
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, COLOR_TRANSPARENT);
    layer_slide(layer_back, 0, 0);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    layer_slide(layer_mouse, mx, my);
    layer_updown(layer_back, 0);
    layer_updown(layer_mouse, 1);

    char s0[20];
    sprintf(s0, "(%d, %d)", mx, my);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COLOR_WHITE, s0);

    sprintf(s0, "memory %dMB   free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COLOR_WHITE, s0);

    layer_refresh(layer_back, 0, 0, binfo->scrnx, 48);

    Byte keycode;
    char s[4];
    for (;;) {
        _io_cli(); // 割り込み禁止
        if (keyfifo.len == 0 && mousefifo.len == 0) {
            _io_stihlt();
        } else if (keyfifo.len != 0) {
            keycode = fifo_dequeue(&keyfifo);
            _io_sti(); // 割り込み禁止解除
            sprintf(s0, "%x", keycode);
            boxfill8(buf_back, binfo->scrnx, COLOR_DARK_CYAN, 0, 16, 15, 31);
            putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COLOR_WHITE, s0);
            layer_refresh(layer_back, 0, 16, 16, 32);
        } else if (mousefifo.len != 0) {
            keycode = fifo_dequeue(&mousefifo);
            _io_sti();
            if (mouse_decode(&mdec, keycode) != 0) {
                sprintf(s0, "[lcr %d %d]", mdec.x, mdec.y);
                if ((mdec.btn & 0x01) != 0) {
                    s0[1] = 'L';
                }
                if ((mdec.btn & 0x02) != 0) {
                    s0[3] = 'R';
                }
                if ((mdec.btn & 0x04) != 0) {
                    s0[2] = 'C';
                }
                boxfill8(buf_back, binfo->scrnx, COLOR_DARK_CYAN, 32, 16, 32 + 15*8 - 1, 31);
                putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COLOR_WHITE, s0);
                layer_refresh(layer_back, 32, 16, 32 + 15*8, 32);

                // move mouse cursor
                mx += mdec.x;
                my += mdec.y;
                if (mx < 0) {
                    mx = 0;
                }
                if (my < 0) {
                    my = 0;
                }
                if (mx > binfo->scrnx - 1) {
                    mx = binfo->scrnx - 1;
                }
                if (my > binfo->scrny - 1) {
                    my = binfo->scrny - 1;
                }
                sprintf(s, "(%d, %d)", mx, my);
                boxfill8(buf_back, binfo->scrnx, COLOR_DARK_CYAN, 0, 0, 79, 15); // hide coordinates
                putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COLOR_WHITE, s); // show coordinates
                layer_refresh(layer_back, 0, 0, 80, 16);
                layer_slide(layer_mouse, mx, my);
            }
        }
    }
}
