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
#include "timer.h"

#define MEMMAN_ADDR 0x003c0000

#define CLOSE_BUTTON_HEIGHT 14
#define CLOSE_BUTTON_WIDTH 16

void
make_window8(Byte *buf, int xsize, int ysize, char *title)
{
  static char closebtn[CLOSE_BUTTON_HEIGHT][CLOSE_BUTTON_WIDTH] = {
    "OOOOOOOOOOOOOOO@",
    "OQQQQQQQQQQQQQ$@",
    "OQQQQQQQQQQQQQ$@",
    "OQQQ@@QQQQ@@QQ$@",
    "OQQQQ@@QQ@@QQQ$@",
    "OQQQQQ@@@@QQQQ$@",
    "OQQQQQQ@@QQQQQ$@",
    "OQQQQQ@@@@QQQQ$@",
    "OQQQQ@@QQ@@QQQ$@",
    "OQQQ@@QQQQ@@QQ$@",
    "OQQQQQQQQQQQQQ$@",
    "OQQQQQQQQQQQQQ$@",
    "O$$$$$$$$$$$$$$@",
    "@@@@@@@@@@@@@@@@",
  };
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        0,        xsize-1,  0);
  boxfill8(buf,  xsize,  COLOR_WHITE,       1,        1,        xsize-2,  1);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        0,        0,        ysize-1);
  boxfill8(buf,  xsize,  COLOR_WHITE,       1,        1,        1,        ysize-2);
  boxfill8(buf,  xsize,  COLOR_WHITE,       xsize-2,  1,        xsize-2,  ysize-2);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   xsize-1,  0,        xsize-1,  ysize-1);
  boxfill8(buf,  xsize,  COLOR_LIGHT_GRAY,  2,        2,        xsize-3,  ysize-3);
  boxfill8(buf,  xsize,  COLOR_DARK_BLUE,   3,        3,        xsize-4,  20);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        ysize-1,  xsize-1,  ysize-1);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   1,        ysize-2,  xsize-2,  ysize-2);
  putfonts8_asc(buf, xsize, 24, 4, COLOR_WHITE, title);
  for (int y = 0; y < CLOSE_BUTTON_HEIGHT; y++) {
    for (int x = 0; x < CLOSE_BUTTON_WIDTH; x++) {
      char c = closebtn[y][x];
      switch (c) {
        case '@':
          c = COLOR_BLACK;
          break;
        case '$':
          c = COLOR_DARK_GRAY;
          break;
        case 'Q':
          c = COLOR_LIGHT_GRAY;
          break;
        default:
          c = COLOR_WHITE;
          break;
      }
      buf[(5+y)*xsize + (xsize-21+x)] = c;
    }
  }
}

#define KEY_BUF_SIZE 32
#define MOUSE_BUF_SIZE 128


void
hari_main(void)
{
  BootInfo *binfo = (BootInfo *) ADR_BOOTINFO;
  Byte keybuf[KEY_BUF_SIZE], mousebuf[MOUSE_BUF_SIZE];
  MouseDecoder mouse_decoder;
  MemoryManager *memman = (MemoryManager *) MEMMAN_ADDR;
  LayerCtl *layerctl;
  Layer *layer_back, *layer_mouse, *layer_win;
  Byte *buf_back, buf_mouse[256], *buf_win;

  init_gdtidt();
  init_pic();
  _io_sti();

  fifo_init(&keyfifo, KEY_BUF_SIZE, keybuf);
  fifo_init(&mousefifo, MOUSE_BUF_SIZE, mousebuf);

  init_pit();

  _io_out8(PIC0_IMR, 0xf9); // PIC1とキーボードを許可
  _io_out8(PIC1_IMR, 0xef); // マウスを許可

  init_keyboard();
  enable_mouse(&mouse_decoder);

  unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
  memman_init(memman);
  memman_free(memman, 0x00001000, 0x0009e000);
  memman_free(memman, 0x00400000, memtotal - 0x00400000);

  init_palette();
  layerctl = layerctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
  layer_back = layer_alloc(layerctl);
  layer_mouse = layer_alloc(layerctl);
  layer_win = layer_alloc(layerctl);
  buf_back = (Byte *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
  buf_win = (Byte *) memman_alloc_4k(memman, 160 * 52);
  layer_setbuf(layer_back, buf_back, binfo->scrnx, binfo->scrny, -1);
  layer_setbuf(layer_mouse, buf_mouse, 16, 16, 99);
  layer_setbuf(layer_win, buf_win, 160, 52, -1);
  init_screen8(buf_back, binfo->scrnx, binfo->scrny);
  init_mouse_cursor8(buf_mouse, COLOR_TRANSPARENT);
  make_window8(buf_win, 160, 52, "counter");

  layer_slide(layer_back, 0, 0);
  int mx = (binfo->scrnx - 16) / 2;
  int my = (binfo->scrny - 28 - 16) / 2;
  layer_slide(layer_mouse, mx, my);
  layer_slide(layer_win, 80, 72);
  layer_updown(layer_back, 0);
  layer_updown(layer_win, 1);
  layer_updown(layer_mouse, 2);

  char s0[20];
  sprintf(s0, "(%d, %d)", mx, my);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COLOR_WHITE, s0);

  sprintf(s0, "memory %dMB   free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COLOR_WHITE, s0);

  layer_refresh(layer_back, 0, 0, binfo->scrnx, 48);

  Byte keycode;
  char s[4];
  int count = 0;
  for (;;) {
    count++;
    sprintf(s0, "%d", count);
    boxfill8(buf_win, 160, COLOR_LIGHT_GRAY, 40, 28, 119, 43);
    putfonts8_asc(buf_win, 160, 40, 28, COLOR_WHITE, s0);
    layer_refresh(layer_win, 40, 28, 120, 44);

    _io_cli(); // 割り込み禁止
    if (keyfifo.len == 0 && mousefifo.len == 0) {
      _io_sti();
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
      if (mouse_decode(&mouse_decoder, keycode) != 0) {
        sprintf(s0, "[lcr %d %d]", mouse_decoder.x, mouse_decoder.y);
        if ((mouse_decoder.btn & 0x01) != 0) {
          s0[1] = 'L';
        }
        if ((mouse_decoder.btn & 0x02) != 0) {
          s0[3] = 'R';
        }
        if ((mouse_decoder.btn & 0x04) != 0) {
          s0[2] = 'C';
        }
        boxfill8(buf_back, binfo->scrnx, COLOR_DARK_CYAN, 32, 16, 32 + 15*8 - 1, 31);
        putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COLOR_WHITE, s0);
        layer_refresh(layer_back, 32, 16, 32 + 15*8, 32);

        // move mouse cursor
        mx += mouse_decoder.x;
        my += mouse_decoder.y;
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
