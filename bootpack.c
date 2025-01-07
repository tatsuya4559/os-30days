#include "fifo.h"
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
make_window8(uint8_t *buf, int32_t xsize, int32_t ysize, char *title)
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
  for (int32_t y = 0; y < CLOSE_BUTTON_HEIGHT; y++) {
    for (int32_t x = 0; x < CLOSE_BUTTON_WIDTH; x++) {
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

#define FIFO_BUF_SIZE 128

enum {
  EVENT_CURSOR_OFF,
  EVENT_CURSOR_ON,
  EVENT_THREE_SEC_ELAPSED = 3,
  EVENT_TEN_SEC_ELAPSED = 10,
  EVENT_KEYBOARD_INPUT = 256,
  EVENT_MOUSE_INPUT = 512,
};

void
hari_main(void)
{
  BootInfo *binfo = (BootInfo *) ADR_BOOTINFO;
  MouseDecoder mouse_decoder;
  MemoryManager *mem_manager = (MemoryManager *) MEMMAN_ADDR;
  LayerController *layerctl;

  FIFO fifo;
  int32_t fifo_buf[FIFO_BUF_SIZE];
  fifo_init(&fifo, FIFO_BUF_SIZE, fifo_buf);

  Layer *layer_back;
  Layer *layer_mouse;
  Layer *layer_win;

  uint8_t *background_layer_buf;
  uint8_t mouse_layer_buf[256];
  uint8_t *window_layer_buf;

  init_gdtidt();
  init_pic();
  _io_sti();

  init_pit();

  Timer *timer = timer_alloc();
  timer_init(timer, &fifo, EVENT_TEN_SEC_ELAPSED);
  Timer *timer2 = timer_alloc();
  timer_init(timer2, &fifo, EVENT_THREE_SEC_ELAPSED);
  Timer *timer3 = timer_alloc();
  timer_init(timer3, &fifo, EVENT_CURSOR_ON);
  timer_set_timeout(timer, 1000);
  timer_set_timeout(timer2, 300);
  timer_set_timeout(timer3, 50);

  _io_out8(PIC0_IMR, 0xf8); // PITとPIC1とキーボードを許可(11111000)
  _io_out8(PIC1_IMR, 0xef); // マウスを許可

  init_keyboard(&fifo, EVENT_KEYBOARD_INPUT);
  enable_mouse(&fifo, EVENT_MOUSE_INPUT, &mouse_decoder);

  uint32_t total_mem_size = memtest(0x00400000, 0xbfffffff);
  memman_init(mem_manager);
  memman_free(mem_manager, 0x00001000, 0x0009e000);
  memman_free(mem_manager, 0x00400000, total_mem_size - 0x00400000);

  init_palette();
  layerctl = layerctl_init(mem_manager, binfo->vram, binfo->scrnx, binfo->scrny);
  layer_back = layer_alloc(layerctl);
  layer_mouse = layer_alloc(layerctl);
  layer_win = layer_alloc(layerctl);
  background_layer_buf = (uint8_t *) memman_alloc_4k(mem_manager, binfo->scrnx * binfo->scrny);
  window_layer_buf = (uint8_t *) memman_alloc_4k(mem_manager, 160 * 52);
  layer_setbuf(layer_back, background_layer_buf, binfo->scrnx, binfo->scrny, -1);
  layer_setbuf(layer_mouse, mouse_layer_buf, 16, 16, 99);
  layer_setbuf(layer_win, window_layer_buf, 160, 52, -1);
  init_screen8(background_layer_buf, binfo->scrnx, binfo->scrny);
  init_mouse_cursor8(mouse_layer_buf, COLOR_TRANSPARENT);
  make_window8(window_layer_buf, 160, 52, "window");

  layer_slide(layer_back, 0, 0);
  int32_t mx = (binfo->scrnx - 16) / 2;
  int32_t my = (binfo->scrny - 28 - 16) / 2;
  layer_slide(layer_mouse, mx, my);
  layer_slide(layer_win, 80, 72);
  layer_updown(layer_back, 0);
  layer_updown(layer_win, 1);
  layer_updown(layer_mouse, 2);

  char s0[20];
  sprintf(s0, "(%d, %d)", mx, my);
  putfonts8_asc(background_layer_buf, binfo->scrnx, 0, 0, COLOR_WHITE, s0);

  sprintf(s0, "memory %dMB   free: %dKB", total_mem_size / (1024 * 1024), memman_total(mem_manager) / 1024);
  putfonts8_asc(background_layer_buf, binfo->scrnx, 0, 32, COLOR_WHITE, s0);

  layer_refresh(layer_back, 0, 0, binfo->scrnx, 48);

  char s[4];
  for (;;) {
    _io_cli(); // 割り込み禁止
    if (fifo.len == 0) {
      _io_stihlt();
      continue;
    }

    int32_t event = fifo_dequeue(&fifo);
    _io_sti(); // 割り込み禁止解除

    if (EVENT_KEYBOARD_INPUT <= event && event < EVENT_MOUSE_INPUT) {
      int32_t keycode = event - EVENT_KEYBOARD_INPUT;
      sprintf(s0, "%x", keycode);
      print_on_layer(layer_back, 0, 16, COLOR_DARK_CYAN, COLOR_WHITE, s0, 2);
      if (keycode == 0x1e) {
        print_on_layer(layer_win, 40, 28, COLOR_LIGHT_GRAY, COLOR_BLACK, "A", 1);
      }
    } else if (EVENT_MOUSE_INPUT <= event) {
      int32_t keycode = event - EVENT_MOUSE_INPUT;
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
        print_on_layer(layer_back, 32, 16, COLOR_DARK_CYAN, COLOR_WHITE, s0, 15);

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
        print_on_layer(layer_back, 0, 0, COLOR_DARK_CYAN, COLOR_WHITE, s, 10);
        layer_slide(layer_mouse, mx, my);
      }
    } else if (event == EVENT_THREE_SEC_ELAPSED) {
      print_on_layer(layer_back, 0, 80, COLOR_DARK_CYAN, COLOR_WHITE, "3[sec]", 6);
    } else if (event == EVENT_TEN_SEC_ELAPSED) {
      print_on_layer(layer_back, 0, 64, COLOR_DARK_CYAN, COLOR_WHITE, "10[sec]", 7);
    } else if (event == EVENT_CURSOR_ON) {
      timer_init(timer3, &fifo, EVENT_CURSOR_OFF);
      boxfill8(background_layer_buf, binfo->scrnx, COLOR_DARK_CYAN, 8, 96, 15, 111);
      timer_set_timeout(timer3, 50);
      layer_refresh(layer_back, 8, 96, 16, 112);
    } else if (event == EVENT_CURSOR_OFF) {
      timer_init(timer3, &fifo, EVENT_CURSOR_ON);
      boxfill8(background_layer_buf, binfo->scrnx, COLOR_WHITE, 8, 96, 15, 111);
      timer_set_timeout(timer3, 50);
      layer_refresh(layer_back, 8, 96, 16, 112);
    }
  }
}
