#include "fifo.h"
#include "nasmfunc.h"
#include "iolib.h"
#include "graphic.h"
#include "dsctbl.h"
#include "int.h"
#include "keyboard.h"
#include "mouse.h"
#include "memory.h"
#include "layer.h"
#include "timer.h"
#include "mtask.h"

#define MEMMAN_ADDR 0x003c0000

#define CLOSE_BUTTON_HEIGHT 14
#define CLOSE_BUTTON_WIDTH 16

#define ADR_BOOTINFO 0x00000ff0

typedef struct {
  uint8_t cyls, leds, vmode, reserve;
  short scrnx, scrny;
  uint8_t *vram;
} BootInfo;

void
make_window8(uint8_t *buf, int32_t xsize, int32_t ysize, char *title, bool_t active)
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
  uint8_t foreground_color = active ? COLOR_WHITE : COLOR_LIGHT_GRAY;
  uint8_t background_color = active ? COLOR_DARK_BLUE : COLOR_DARK_GRAY;
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        0,        xsize-1,  0);
  boxfill8(buf,  xsize,  COLOR_WHITE,       1,        1,        xsize-2,  1);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        0,        0,        ysize-1);
  boxfill8(buf,  xsize,  COLOR_WHITE,       1,        1,        1,        ysize-2);
  boxfill8(buf,  xsize,  COLOR_WHITE,       xsize-2,  1,        xsize-2,  ysize-2);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   xsize-1,  0,        xsize-1,  ysize-1);
  boxfill8(buf,  xsize,  COLOR_LIGHT_GRAY,  2,        2,        xsize-3,  ysize-3);
  boxfill8(buf,  xsize,  background_color,   3,        3,        xsize-4,  20);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        ysize-1,  xsize-1,  ysize-1);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   1,        ysize-2,  xsize-2,  ysize-2);
  putfonts8_asc(buf, xsize, 24, 4, foreground_color, title);
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

static
void
make_textbox8(Layer *layer, int32_t x0, int32_t y0, int32_t width, int32_t height, int32_t color)
{
  int32_t x1 = x0 + width;
  int32_t y1 = y0 + height;
  boxfill8(layer->buf, layer->bxsize, COLOR_DARK_GRAY, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
  boxfill8(layer->buf, layer->bxsize, COLOR_DARK_GRAY, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
  boxfill8(layer->buf, layer->bxsize, COLOR_WHITE, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
  boxfill8(layer->buf, layer->bxsize, COLOR_WHITE, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
  boxfill8(layer->buf, layer->bxsize, COLOR_BLACK, x0 - 1, y0 - 2, x1, y0 - 2);
  boxfill8(layer->buf, layer->bxsize, COLOR_BLACK, x0 - 2, y0 - 2, x0 - 2, y1);
  boxfill8(layer->buf, layer->bxsize, COLOR_LIGHT_GRAY, x0 - 2, y1 + 1, x1, y1 + 1);
  boxfill8(layer->buf, layer->bxsize, COLOR_LIGHT_GRAY, x1 + 1, y0 - 2, x1 + 1, y1);
  boxfill8(layer->buf, layer->bxsize, color, x0 - 1, y0 - 1, x1, y1);
}

#define FIFO_BUF_SIZE 128

enum {
  EVENT_CURSOR_OFF,
  EVENT_CURSOR_ON,
  EVENT_PRINT,
  EVENT_KEYBOARD_INPUT = 256,
  EVENT_MOUSE_INPUT = 512,
};

static char keytable[0x54] = {
  0,   0,
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
  0,   0,
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
  0, 0,
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 
  0, '\\',
  'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/',
  0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

void
task_b_main(Layer *layer_back)
{
  FIFO fifo;
  int32_t fifo_buf[FIFO_BUF_SIZE];
  fifo_init(&fifo, FIFO_BUF_SIZE, fifo_buf, NULL);

  Timer *print_timer = timer_alloc();
  timer_init(print_timer, &fifo, EVENT_PRINT);
  timer_set_timeout(print_timer, 1);

  int32_t count = 0;
  int32_t count0 = 0;
  char s[11];
  for (;;) {
    count++;

    _io_cli();
    if (fifo.len == 0) {
      _io_stihlt();
      continue;
    }
    int32_t event = fifo_dequeue(&fifo);
    _io_sti();
    switch (event) {
    case EVENT_PRINT:
      sprintf(s, "%d", count - count0);
      print_on_layer(layer_back, 24, 28, COLOR_LIGHT_GRAY, COLOR_WHITE, s, 10);
      count0 = count;
      timer_set_timeout(print_timer, 100);
      break;
    }
  }
}

void
hari_main(void)
{
  /* Get boot information */
  BootInfo *binfo = (BootInfo *) ADR_BOOTINFO;

  /* Initialize IDT, PIC, and PIT */
  init_gdtidt();
  init_pic();
  _io_sti(); /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
  init_pit();

  /* Initialize memory manager */
  MemoryManager *mem_manager = (MemoryManager *) MEMMAN_ADDR;
  uint32_t total_mem_size = memtest(0x00400000, 0xbfffffff);
  memman_init(mem_manager);
  memman_free(mem_manager, 0x00001000, 0x0009e000);
  memman_free(mem_manager, 0x00400000, total_mem_size - 0x00400000);

  /* Initialize Bus */
  FIFO fifo;
  int32_t fifo_buf[FIFO_BUF_SIZE];
  fifo_init(&fifo, FIFO_BUF_SIZE, fifo_buf, NULL);

  /* Initialize Devices */
  MouseDecoder mouse_decoder;
  LayerController *layerctl;
  _io_out8(PIC0_IMR, 0xf8); // PITとPIC1とキーボードを許可(11111000)
  _io_out8(PIC1_IMR, 0xef); // マウスを許可
  init_keyboard(&fifo, EVENT_KEYBOARD_INPUT);
  enable_mouse(&fifo, EVENT_MOUSE_INPUT, &mouse_decoder);

  /* Initialize Multi-task Controller */
  Task *task_a = task_init(mem_manager);
  fifo.metadata = task_a;
  Task *task_b[3];

  /* Initialize Layers */
  Layer *layer_back;
  Layer *layer_mouse;
  Layer *layer_win;
  Layer *layer_win_b[3];
  uint8_t *background_layer_buf;
  uint8_t mouse_layer_buf[256];
  uint8_t *window_layer_buf;
  uint8_t *window_layer_buf_b;

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
  make_window8(window_layer_buf, 160, 52, "window", TRUE);
  make_textbox8(layer_win, 8, 28, 144, 16, COLOR_WHITE);

  // task B
  for (int32_t i = 0; i < 3; i++) {
    layer_win_b[i] = layer_alloc(layerctl);
    window_layer_buf_b = (uint8_t *) memman_alloc_4k(mem_manager, 144 * 52);
    layer_setbuf(layer_win_b[i], window_layer_buf_b, 144, 52, -1);
    char title[10];
    sprintf(title, "task_b%d", i);
    make_window8(window_layer_buf_b, 144, 52, title, FALSE);
    task_b[i] = task_alloc();
    // Substract 8 bytes from the end of the allocated memory to store an int32_t argument.
    // |---------------------|<- int32_t(4 byte) ->|
    // ^                     ^                     ^
    // ESP                   ESP+4                 End of the segment
    int32_t task_b_esp = memman_alloc_4k(mem_manager, 64 * 1024) + 64 * 1024 - 8;
    task_b[i]->tss.eip = (int32_t) &task_b_main;
    task_b[i]->tss.esp = task_b_esp;
    task_b[i]->tss.es = 1 * 8;
    task_b[i]->tss.cs = 2 * 8;
    task_b[i]->tss.ss = 1 * 8;
    task_b[i]->tss.ds = 1 * 8;
    task_b[i]->tss.fs = 1 * 8;
    task_b[i]->tss.gs = 1 * 8;
    // Arg1 should be at the address of [Stack Pointer + 4 Byte]
    *((int32_t *) (task_b_esp + 4)) = (int32_t) layer_win_b[i];
    task_run(task_b[i], i + 1);
  }

  // Draw cursor
  int32_t cursor_x = 8;
  int32_t cursor_c = COLOR_WHITE;
  boxfill8(layer_win->buf, layer_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
  layer_refresh(layer_win, cursor_x, 28, cursor_x + 8, 44);

  // Draw layers
  layer_slide(layer_back, 0, 0);
  int32_t mx = (binfo->scrnx - 16) / 2;
  int32_t my = (binfo->scrny - 28 - 16) / 2;
  layer_slide(layer_mouse, mx, my);
  layer_slide(layer_win, 8, 56);
  layer_slide(layer_win_b[0], 168, 56);
  layer_slide(layer_win_b[1], 8, 116);
  layer_slide(layer_win_b[2], 168, 116);
  layer_updown(layer_back, 0);
  layer_updown(layer_win_b[0], 1);
  layer_updown(layer_win_b[1], 2);
  layer_updown(layer_win_b[2], 3);
  layer_updown(layer_win, 4);
  layer_updown(layer_mouse, 1000);

  /* Timer */
  Timer *cursor_timer = timer_alloc();
  timer_init(cursor_timer, &fifo, EVENT_CURSOR_ON);
  timer_set_timeout(cursor_timer, 50);

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
      task_sleep(task_a);
      _io_stihlt();
      continue;
    }

    int32_t event = fifo_dequeue(&fifo);
    _io_sti(); // 割り込み禁止解除

    if (EVENT_KEYBOARD_INPUT <= event && event < EVENT_MOUSE_INPUT) {
      int32_t keycode = event - EVENT_KEYBOARD_INPUT;
      sprintf(s0, "%x", keycode);
      print_on_layer(layer_back, 0, 16, COLOR_DARK_CYAN, COLOR_WHITE, s0, 2);
      if (keycode < 0x54) {
        char c;
        if ((c = keytable[keycode]) != 0) {
          s[0] = c;
          s[1] = '\0';
          print_on_layer(layer_win, cursor_x, 28, COLOR_WHITE, COLOR_BLACK, s, 1);
          cursor_x += 8;
        }
      }
      if (keycode == 0x0e && cursor_x > 8) { // Backspace
        print_on_layer(layer_win, cursor_x, 28, COLOR_WHITE, COLOR_BLACK, " ", 1);
        cursor_x -= 8;
      }
      // Draw cursor
      boxfill8(layer_win->buf, layer_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
      layer_refresh(layer_win, cursor_x, 28, cursor_x + 8, 44);
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

        if ((mouse_decoder.btn & 0x01) != 0) {
          // Move the window if draged with left button pressed.
          layer_slide(layer_win, mx - 80, my - 8);
        }
      }
    } else {
      switch (event) {
        case EVENT_CURSOR_ON:
          cursor_c = COLOR_BLACK;
          timer_init(cursor_timer, &fifo, EVENT_CURSOR_OFF);
          break;
        case EVENT_CURSOR_OFF:
          cursor_c = COLOR_WHITE;
          timer_init(cursor_timer, &fifo, EVENT_CURSOR_ON);
          break;
      }
      timer_set_timeout(cursor_timer, 50);
      // Draw cursor
      boxfill8(layer_win->buf, layer_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
      layer_refresh(layer_win, cursor_x, 28, cursor_x + 8, 44);
    }
  }
}
