#include "nasmfunc.h"
#include "font.h"
#include "graphic.h"
#include "strutil.h"

static
void
set_palette(int32_t start, int32_t end, uint8_t *rgb)
{
  int32_t i, eflags;
  eflags = _io_load_eflags(); // load interrupt flag
  _io_cli(); // set interrupt flag to 0; forbid interrupting
  _io_out8(0x03c8, start);
  for (i = start; i <= end; i++) {
    _io_out8(0x03c9, rgb[0] / 4);
    _io_out8(0x03c9, rgb[1] / 4);
    _io_out8(0x03c9, rgb[2] / 4);
    rgb += 3;
  }
  _io_store_eflags(eflags); // recover interrupt flag
  return;
}

void
init_palette(void)
{
  // 普通に初期値を書くと要素数だけ代入文にコンパイルされる
  // 非効率なのでstatic宣言することで一発で初期化できる
  static uint8_t table_rgb[16 * 3] = {
    0x00, 0x00, 0x00, // 0: black
    0xff, 0x00, 0x00, // 1: light red
    0x00, 0xff, 0x00, // 2: light green
    0xff, 0xff, 0x00, // 3: light yellow
    0x00, 0x00, 0xff, // 4: light blue
    0xff, 0x00, 0xff, // 5: light purple
    0x00, 0xff, 0xff, // 6: light cyan
    0xff, 0xff, 0xff, // 7: white
    0xc6, 0xc6, 0xc6, // 8: light gray
    0x84, 0x00, 0x00, // 9: dark red
    0x00, 0x84, 0x00, // 10: dark green
    0x84, 0x84, 0x00, // 11: dark yellow
    0x00, 0x00, 0x84, // 12: dark blue
    0x84, 0x00, 0x84, // 13: dark purple
    0x00, 0x84, 0x84, // 14: dark cyan
    0x84, 0x84, 0x84, // 15: dark gray
  };
  set_palette(0, 15, table_rgb);
  return;
}

#define CURSOR_HEIGHT 16
#define CURSOR_WIDTH 16

void
init_mouse_cursor8(uint8_t *mouse, uint8_t background_color)
{
  static char cursor[CURSOR_HEIGHT][CURSOR_WIDTH] = {
    "**************..",
    "*ooooooooooo*...",
    "*oooooooooo*....",
    "*ooooooooo*.....",
    "*oooooooo*......",
    "*ooooooo*.......",
    "*ooooooo*.......",
    "*oooooooo*......",
    "*oooo**ooo*.....",
    "*ooo*..*ooo*....",
    "*oo*....*ooo*...",
    "*o*......*ooo*..",
    "**........*ooo*.",
    "*..........*ooo*",
    "............*oo*",
    ".............***",
  };

  int32_t x, y;
  for (y=0; y<CURSOR_HEIGHT; y++) {
    for (x=0; x<CURSOR_WIDTH; x++) {
      switch (cursor[y][x]) {
        case '*':
          mouse[y*CURSOR_WIDTH + x] = COLOR_BLACK;
          break;
        case 'o':
          mouse[y*CURSOR_WIDTH + x] = COLOR_WHITE;
          break;
        case '.':
          mouse[y*CURSOR_WIDTH + x] = background_color;
          break;
      }
    }
  }
}

static
void
putfont8(uint8_t *vram, int32_t xsize, int32_t x, int32_t y, uint8_t c, uint8_t *font)
{
  uint8_t *p, d;
  for (int32_t i = 0; i < 16; i++) {
    p = vram + (y + i) * xsize + x;
    d = font[i];
    if ((d & 0x80) != 0) { p[0] = c; }
    if ((d & 0x40) != 0) { p[1] = c; }
    if ((d & 0x20) != 0) { p[2] = c; }
    if ((d & 0x10) != 0) { p[3] = c; }
    if ((d & 0x08) != 0) { p[4] = c; }
    if ((d & 0x04) != 0) { p[5] = c; }
    if ((d & 0x02) != 0) { p[6] = c; }
    if ((d & 0x01) != 0) { p[7] = c; }
  }
}

static
void
putfonts8_asc(uint8_t *vram, int32_t xsize, int32_t x, int32_t y, uint8_t c, char *string)
{
  for (char *ch = string; *ch != '\0'; ch++) {
    putfont8(vram, xsize, x, y, c, fonts[*ch]);
    x += 8;
  }
}

void
boxfill8(uint8_t *vram, int32_t xsize, uint8_t c, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  int32_t x, y;
  for (y = y0; y <= y1; y++) {
    for (x = x0; x <= x1; x++) {
      vram[x + y * xsize] = c;
    }
  }
}

void
init_screen8(uint8_t *vram, int32_t x, int32_t y)
{
  boxfill8(vram, x, COLOR_DARK_CYAN, 0, 0, x - 1, y - 29);
  boxfill8(vram, x, COLOR_LIGHT_GRAY, 0, y - 28, x - 1, y - 28);
  boxfill8(vram, x, COLOR_WHITE, 0, y - 27, x - 1, y - 27);
  boxfill8(vram, x, COLOR_LIGHT_GRAY, 0, y - 26, x - 1, y - 1);

  boxfill8(vram, x, COLOR_WHITE, 3, y - 24, 59, y - 24);
  boxfill8(vram, x, COLOR_WHITE, 2, y - 24, 2, y - 4);
  boxfill8(vram, x, COLOR_DARK_GRAY, 3, y - 4, 59, y - 4);
  boxfill8(vram, x, COLOR_DARK_GRAY, 59, y - 23, 59, y - 5);
  boxfill8(vram, x, COLOR_BLACK, 2, y - 3, 59, y - 3);
  boxfill8(vram, x, COLOR_BLACK, 60, y - 24, 60, y - 3);

  boxfill8(vram, x, COLOR_DARK_GRAY, x - 47, y - 24, x - 4, y - 24);
  boxfill8(vram, x, COLOR_DARK_GRAY, x - 47, y - 23, x - 47, y - 4);
  boxfill8(vram, x, COLOR_WHITE, x - 47, y - 3, x - 4, y - 3);
  boxfill8(vram, x, COLOR_WHITE, x - 3, y - 24, x - 3, y - 3);
}

void
print_on_layer(Layer *layer, int32_t x, int32_t y, uint8_t bg_color, uint8_t fg_color, char *s)
{
  int32_t s_len = str_len(s);
  boxfill8(layer->buf, layer->bxsize, bg_color, x, y, x + s_len * 8 - 1, y + FONT_HEIGHT - 1);
  putfonts8_asc(layer->buf, layer->bxsize, x, y, fg_color, s);
  layer_refresh(layer, x, y, x + s_len * 8, y + FONT_HEIGHT);
}

#define CLOSE_BUTTON_HEIGHT 14
#define CLOSE_BUTTON_WIDTH 16

void
make_window_title(uint8_t *buf, int32_t xsize, char *title, bool_t active)
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
  boxfill8(buf,  xsize,  background_color,   3,        3,        xsize-4,  20);
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

void
make_window8(uint8_t *buf, int32_t xsize, int32_t ysize, char *title, bool_t active)
{
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        0,        xsize-1,  0);
  boxfill8(buf,  xsize,  COLOR_WHITE,       1,        1,        xsize-2,  1);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        0,        0,        ysize-1);
  boxfill8(buf,  xsize,  COLOR_WHITE,       1,        1,        1,        ysize-2);
  boxfill8(buf,  xsize,  COLOR_WHITE,       xsize-2,  1,        xsize-2,  ysize-2);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   xsize-1,  0,        xsize-1,  ysize-1);
  boxfill8(buf,  xsize,  COLOR_LIGHT_GRAY,  2,        2,        xsize-3,  ysize-3);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   0,        ysize-1,  xsize-1,  ysize-1);
  boxfill8(buf,  xsize,  COLOR_DARK_GRAY,   1,        ysize-2,  xsize-2,  ysize-2);
  make_window_title(buf, xsize, title, active);
}

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

