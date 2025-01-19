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
#include "strutil.h"

/* #define DEBUG */
#define MEMMAN_ADDR 0x003c0000
#define ADR_BOOTINFO 0x00000ff0
#define ADR_DISKIMG  0x00100000
#define MAX_FILES 224
#define KEYCMD_LED 0xed
#define FIFO_BUF_SIZE 128
#define NUM_DISK_SECTORS 2880

// Console width must be a multiple of 8
// Console height must be a multiple of 16
/* #define CONSOLE_WIDTH 240 */
/* #define CONSOLE_HEIGHT 128 */
#define CONSOLE_WIDTH 632
#define CONSOLE_HEIGHT 320

typedef struct {
  uint8_t cyls, leds, vmode, reserve;
  short scrnx, scrny;
  uint8_t *vram;
} BootInfo;

typedef struct {
  uint8_t name[8], ext[3], type;
  uint8_t reserve[10];
  uint16_t time, date, clustno;
  uint32_t size;
} FileInfo;

/**
 * Convert finfo to normalized filename.
 *
 * @param finfo file information
 * @param buf buffer to store the normalized filename.
 *        The buffer must be at least 13 bytes long.
 */
static
void
file_normalized_name(const FileInfo *finfo, char *buf)
{
  int32_t idx = 0;
  for (int32_t i = 0; i < 8; i++) {
    if (finfo->name[i] == ' ') {
      break;
    }
    buf[idx++] = finfo->name[i];
  }
  buf[idx++] = '.';
  for (int32_t i = 0; i < 3; i++) {
    if (finfo->ext[i] == ' ') {
      break;
    }
    buf[idx++] = finfo->ext[i];
  }
  buf[idx] = '\0';
  if (str_equal(buf, ".")) {
    buf[0] = '\0';
  }
}

static
void
file_readfat(uint8_t *fat, uint8_t *img)
{
  // Read 3 bytes and decompress to 2 bytes like below:
  // 1A 2B 3C -> B1A 3C2
  for (int32_t i = 0, j = 0; i < NUM_DISK_SECTORS; i += 2) {
    fat[i + 0] = (img[j + 0]) | (img[j + 1] << 8) & 0xfff;
    fat[i + 1] = (img[j + 1] >> 4) | (img[j + 2] << 4) & 0xfff;
    j += 3;
  }
}

static
void
file_load(uint16_t clustno, uint32_t size, uint8_t *buf, uint8_t *fat, uint8_t *img)
{
  while (size > 0) {
    uint32_t batch_size = size > 512 ? 512 : size;
    for (uint32_t i = 0; i < batch_size; i++) {
      buf[i] = img[clustno * 512 + i];
    }
    size -= batch_size;
    buf += batch_size;
    clustno = fat[clustno];
  }
}

enum {
  EVENT_CURSOR_OFF,
  EVENT_CURSOR_ON,
  EVENT_KEYBOARD_INPUT = 256,
  EVENT_MOUSE_INPUT = 512,
};

static char keytable[0x80] = {
  0,   0,
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
  0,   0,
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
  0, 0,
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
  0, '\\',
  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
  0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
};

static char shifted_keytable[0x80] = {
  0,   0,
  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
  0x08, 0,
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
  0x0a, 0,
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
  0, '|',
  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
  0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, '_', 0, 0, 0, 0, 0, 0, 0, 0, 0, '|', 0, 0
};

static
char
keycode_to_char(int32_t keycode, bool_t with_shift, bool_t with_capslock)
{
  char c;
  if (with_shift) {
    c = shifted_keytable[keycode];
  } else {
    c = keytable[keycode];
  }
  if (with_capslock) {
    // Invert case
    if ('a' <= c && c <= 'z') {
      c -= 0x20;
    } else if ('A' <= c && c <= 'Z') {
      c += 0x20;
    }
  }
  return c;
}

static
int32_t
console_newline(int32_t cursor_y, Layer *layer)
{
  if (cursor_y < 28 + CONSOLE_HEIGHT - 16) {
    cursor_y += 16;
  } else {
    // scroll
    for (int32_t y = 28; y < 28 + CONSOLE_HEIGHT - 16; y++) {
      for (int32_t x = 8; x < 8 + CONSOLE_WIDTH; x++) {
        layer->buf[x + y * layer->bxsize] = layer->buf[x + (y + 16) * layer->bxsize];
      }
    }
    for (int32_t y = 28 + CONSOLE_HEIGHT - 16; y < 28 + CONSOLE_HEIGHT; y++) {
      for (int32_t x = 8; x < 8 + CONSOLE_WIDTH; x++) {
        layer->buf[x + y * layer->bxsize] = COLOR_BLACK;
      }
    }
    layer_refresh(layer, 8, 28, 8 + CONSOLE_WIDTH, 28 + CONSOLE_HEIGHT);
  }
  return cursor_y;
}

static
void
console_task_main(Layer *layer, uint32_t total_mem_size)
{
  Task *self = task_now();
  MemoryManager *memman = (MemoryManager *) MEMMAN_ADDR;
  FileInfo *finfo = (FileInfo *) (ADR_DISKIMG + 0x002600);

  uint8_t *fat = (uint8_t *) memman_alloc_4k(memman, NUM_DISK_SECTORS * 4);
  file_readfat(fat, (uint8_t *) (ADR_DISKIMG + 0x000200));

  int32_t fifo_buf[FIFO_BUF_SIZE];
  fifo_init(&self->fifo, FIFO_BUF_SIZE, fifo_buf, self);

  Timer *timer = timer_alloc();
  timer_init(timer, &self->fifo, 1);
  timer_set_timeout(timer, 50);

  const int32_t cmdline_len = 30;
  char cmdline[cmdline_len];
  char s[cmdline_len]; // printing buffer

  int32_t cursor_x = 16;
  int32_t cursor_y = 28;
  int32_t cursor_c = COLOR_WHITE;

  print_on_layer(layer, 8, cursor_y, COLOR_BLACK, cursor_c, ">");

  int32_t event;
  for (;;) {
    _io_cli();

    if (self->fifo.len == 0) {
      task_sleep(self);
      _io_sti();
      continue;
    }

    event = fifo_dequeue(&self->fifo);
    _io_sti();

    // Cursor blink event
    if (event <= 1) {
      switch (event) {
      case 0:
        timer_init(timer, &self->fifo, 1);
        cursor_c = COLOR_BLACK;
        break;
      case 1:
        timer_init(timer, &self->fifo, 0);
        cursor_c = COLOR_WHITE;
        break;
      }
      timer_set_timeout(timer, 50);
      if (!layer_is_active(layer)) {
        cursor_c = COLOR_BLACK;
      }
      boxfill8(layer->buf, layer->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
      layer_refresh(layer, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
    }

    // Keyboard input event
    if (256 <= event && event <= 511) {
      char c = event - 256;
      switch (c) {
      case 0: // Null character
        break;
      case 0x0e: // Backspace
        if (cursor_x > 16) {
          print_on_layer(layer, cursor_x, cursor_y, COLOR_BLACK, COLOR_WHITE, " ");
          cursor_x -= 8;
        }
        break;
      case 0x1c: // Enter
        // erase cursor
        print_on_layer(layer, cursor_x, cursor_y, COLOR_BLACK, COLOR_WHITE, " ");
        cmdline[cursor_x / 8 - 2] = '\0';
        cursor_y = console_newline(cursor_y, layer);

        if (str_equal(cmdline, "mem")) { // mem command
          sprintf(s, "total %dMB", total_mem_size / (1024 * 1024));
          print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, s);
          cursor_y = console_newline(cursor_y, layer);
          sprintf(s, "free %dKB", memman_total(memman) / 1024);
          print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, s);
          cursor_y = console_newline(cursor_y, layer);
          cursor_y = console_newline(cursor_y, layer);
        } else if (str_equal(cmdline, "cls")) { // clear screen
          for (int32_t y = 28; y < 28 + CONSOLE_HEIGHT; y++) {
            for (int32_t x = 8; x < 8 + CONSOLE_WIDTH; x++) {
              layer->buf[x + y * layer->bxsize] = COLOR_BLACK;
            }
          }
          layer_refresh(layer, 8, 28, 8 + CONSOLE_WIDTH, 28 + CONSOLE_HEIGHT);
          cursor_y = 28;
        } else if (str_equal(cmdline, "dir")) {
          for (int32_t i = 0; i < MAX_FILES; i++) {
            if (finfo[i].name == 0x00) {
              // If the first byte of the name is 0x00, there are no more files.
              break;
            }
            if (finfo[i].name == 0xe5) {
              // If the first byte of the name is 0xe5, the file is deleted.
              continue;
            }
            if (finfo[i].type == 0x00 || finfo[i].type == 0x20) {
              char filename[13];
              file_normalized_name(&finfo[i], filename);
              if (str_equal(filename, "")) {
                continue;
              }
              sprintf(s, "%s %d", filename, finfo[i].size);
              print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, s);
              cursor_y = console_newline(cursor_y, layer);
            }
          }
        } else if (str_has_prefix(cmdline, "type ")) {
          char *input_filename = str_to_upper(str_trim_prefix(cmdline, "type "));

          // Search for the file
          char filename[13];
          int32_t idx;
          for (idx = 0; idx < MAX_FILES; idx++) {
            file_normalized_name(&finfo[idx], filename);
            if (str_equal(filename, input_filename)) {
              break;
            }
          }
          bool_t found = idx < MAX_FILES;

          if (found) {
            print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, filename);
            cursor_y = console_newline(cursor_y, layer);

            uint8_t *buf = (uint8_t *) memman_alloc_4k(memman, finfo[idx].size);
            file_load(finfo[idx].clustno, finfo[idx].size, buf, fat, (uint8_t *) (ADR_DISKIMG + 0x003e00));

            // Print the file content
            cursor_x = 8;
            for (int32_t p = 0; p < finfo[idx].size; p++) {
              if (buf[p] == '\n') {
                cursor_x = 8;
                cursor_y = console_newline(cursor_y, layer);
                continue;
              }
              if (buf[p] == '\t') {
                const int32_t tabstop = 8;
                do {
                  print_on_layer(layer, cursor_x, cursor_y, COLOR_BLACK, COLOR_WHITE, " ");
                  cursor_x += 8;
                } while ((cursor_x / 8) % tabstop != 1);
                continue;
              }
              s[0] = buf[p];
              s[1] = '\0';
              print_on_layer(layer, cursor_x, cursor_y, COLOR_BLACK, COLOR_WHITE, s);
              cursor_x += 8;
              if (cursor_x == 8 + CONSOLE_WIDTH) {
                cursor_x = 8;
                cursor_y = console_newline(cursor_y, layer);
              }
            }
            memman_free_4k(memman, (uint32_t) buf, finfo[idx].size);
          } else {
            print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, "File not found.");
            cursor_y = console_newline(cursor_y, layer);
          }
          cursor_y = console_newline(cursor_y, layer);
        } else if (!str_equal(cmdline, "")) { // not a command but a string
          print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, "Bad command.");
          cursor_y = console_newline(cursor_y, layer);
          cursor_y = console_newline(cursor_y, layer);
        }

        // Draw a prompt
        print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, ">");
        cursor_x = 16;
        break;
      default:
        if (cursor_x < CONSOLE_WIDTH) {
          s[0] = c;
          s[1] = '\0';
          cmdline[cursor_x / 8 - 2] = c;
          print_on_layer(layer, cursor_x, cursor_y, COLOR_BLACK, COLOR_WHITE, s);
          cursor_x += 8;
        }
        break;
      }

      // Draw cursor
      if (!layer_is_active(layer)) {
        cursor_c = COLOR_BLACK;
      }
      boxfill8(layer->buf, layer->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, 43);
      layer_refresh(layer, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
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
  MemoryManager *memman = (MemoryManager *) MEMMAN_ADDR;
  uint32_t total_mem_size = memtest(0x00400000, 0xbfffffff);
  memman_init(memman);
  memman_free(memman, 0x00001000, 0x0009e000);
  memman_free(memman, 0x00400000, total_mem_size - 0x00400000);

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
  Task *task_a = task_init(memman);
  fifo.metadata = task_a;
  task_run(task_a, 1, 0);

  /* Initialize Layers */
  Layer *layer_back;
  Layer *layer_mouse;
  Layer *layer_win;
  uint8_t *background_layer_buf;
  uint8_t mouse_layer_buf[256];
  uint8_t *window_layer_buf;

  init_palette();
  layerctl = layerctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
  layer_back = layer_alloc(layerctl);
  layer_mouse = layer_alloc(layerctl);
  layer_win = layer_alloc(layerctl);
  layer_activate(layer_win);
  background_layer_buf = (uint8_t *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
  window_layer_buf = (uint8_t *) memman_alloc_4k(memman, 160 * 52);
  layer_setbuf(layer_back, background_layer_buf, binfo->scrnx, binfo->scrny, -1);
  layer_setbuf(layer_mouse, mouse_layer_buf, 16, 16, 99);
  layer_setbuf(layer_win, window_layer_buf, 160, 52, -1);
  init_screen8(background_layer_buf, binfo->scrnx, binfo->scrny);
  init_mouse_cursor8(mouse_layer_buf, COLOR_TRANSPARENT);
  make_window8(window_layer_buf, 160, 52, "window", TRUE);
  make_textbox8(layer_win, 8, 28, 144, 16, COLOR_WHITE);

  // console task
  Layer *console_layer = layer_alloc(layerctl);
  uint8_t *console_layer_buf = (uint8_t *) memman_alloc_4k(memman, (CONSOLE_WIDTH + 16) * (CONSOLE_HEIGHT + 37));
  layer_setbuf(console_layer, console_layer_buf, CONSOLE_WIDTH + 16, CONSOLE_HEIGHT + 37, -1);
  make_window8(console_layer_buf, CONSOLE_WIDTH + 16, CONSOLE_HEIGHT + 37, "console", FALSE);
  make_textbox8(console_layer, 8, 28, CONSOLE_WIDTH, CONSOLE_HEIGHT, COLOR_BLACK);
  Task *console_task = task_alloc();
  // Substract 8 bytes from the end of the allocated memory to store an int32_t argument.
  // |---------------------|<- int32_t(4 byte) ->|
  // ^                     ^                     ^
  // ESP                   ESP+4                 End of the segment
  console_task->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
  console_task->tss.eip = (int32_t) &console_task_main;
  console_task->tss.es = 1 * 8;
  console_task->tss.cs = 2 * 8;
  console_task->tss.ss = 1 * 8;
  console_task->tss.ds = 1 * 8;
  console_task->tss.fs = 1 * 8;
  console_task->tss.gs = 1 * 8;
  // Arg1 should be at the address of [Stack Pointer + 4 Byte]
  *((int32_t *) (console_task->tss.esp + 4)) = (int32_t) console_layer;
  *((uint32_t *) (console_task->tss.esp + 8)) = total_mem_size;
  task_run(console_task, 2, 2); // level 2, priority 2

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
  layer_slide(console_layer, 32, 200);

  layer_updown(layer_back, 0);
  layer_updown(console_layer, 4);
  layer_updown(layer_win, 5);
  layer_updown(layer_mouse, 1000);

  /* Timer */
  Timer *cursor_timer = timer_alloc();
  timer_init(cursor_timer, &fifo, EVENT_CURSOR_ON);
  timer_set_timeout(cursor_timer, 50);

  char s[20];
#ifdef DEBUG
  sprintf(s, "(%d, %d)", mx, my);
  print_on_layer(layer_back, 0, 0, COLOR_DARK_CYAN, COLOR_WHITE, s);

  sprintf(s, "memory %dMB   free: %dKB", total_mem_size / (1024 * 1024), memman_total(memman) / 1024);
  print_on_layer(layer_back, 0, 32, COLOR_DARK_CYAN, COLOR_WHITE, s);
#endif

  layer_refresh(layer_back, 0, 0, binfo->scrnx, 48);

  bool_t shift_pressed = FALSE;

  // forth bit of leds indicates ScallLock
  // fifth bit of leds indicates NumLock
  // sixth bit of leds indicates CapsLock
  uint8_t key_leds = (binfo->leds >> 4) & 0b111;
  bool_t caps_locked = key_leds & 0b100 != 0;
  int32_t keycmd_wait = -1;

  FIFO keycmd;
  int32_t keycmd_buf[FIFO_BUF_SIZE];
  fifo_init(&keycmd, FIFO_BUF_SIZE, keycmd_buf, 0);
  fifo_enqueue(&keycmd, KEYCMD_LED);
  fifo_enqueue(&keycmd, key_leds);

MAIN_LOOP:
  for (;;) {
    if (keycmd.len > 0 && keycmd_wait < 0) {
      keycmd_wait = fifo_dequeue(&keycmd);
      wait_KBC_sendready();
      _io_out8(PORT_KEYDAT, keycmd_wait);
    }

    _io_cli(); // 割り込み禁止
    if (fifo.len == 0) {
      task_sleep(task_a);
      _io_sti();
      continue;
    }

    int32_t event = fifo_dequeue(&fifo);
    _io_sti(); // 割り込み禁止解除

    if (EVENT_KEYBOARD_INPUT <= event && event < EVENT_MOUSE_INPUT) {
      int32_t keycode = event - EVENT_KEYBOARD_INPUT;
#ifdef DEBUG
      sprintf(s, "%x", keycode);
      print_on_layer(layer_back, 0, 16, COLOR_DARK_CYAN, COLOR_WHITE, s);
#endif

      switch (keycode) {
      /* Lock keys */
      case 0x3a: // CapsLock
        key_leds ^= 0b100;
        fifo_enqueue(&keycmd, KEYCMD_LED);
        fifo_enqueue(&keycmd, key_leds);
        break;
      case 0x45: // NumLock
        key_leds ^= 0b010;
        fifo_enqueue(&keycmd, KEYCMD_LED);
        fifo_enqueue(&keycmd, key_leds);
        break;
      case 0x46: // ScrollLock
        key_leds ^= 0b001;
        fifo_enqueue(&keycmd, KEYCMD_LED);
        fifo_enqueue(&keycmd, key_leds);
        break;
      case 0xfa: // LED status ok
        keycmd_wait = -1;
        break;
      case 0xfe: // LED status error
        wait_KBC_sendready();
        _io_out8(PORT_KEYDAT, keycmd_wait);
        break;

      /* Shift keys */
      case 0x2a: // Left shift pressed
      case 0x36: // Right shift pressed
        shift_pressed = TRUE;
        goto MAIN_LOOP;
      case 0xaa: // Left shift released
      case 0xb6: // Right shift released
        shift_pressed = FALSE;
        goto MAIN_LOOP;

      /* Task switch keys */
      case 0x0f: // Tab
        if (layer_is_active(layer_win)) {
          make_window_title(window_layer_buf, layer_win->bxsize, "task_a", FALSE);
          make_window_title(console_layer_buf, console_layer->bxsize, "console", TRUE);
          layer_activate(console_layer);
        } else {
          make_window_title(window_layer_buf, layer_win->bxsize, "task_a", TRUE);
          make_window_title(console_layer_buf, console_layer->bxsize, "console", FALSE);
          layer_activate(layer_win);
        }
        layer_refresh(layer_win, 0, 0, layer_win->bxsize, 21);
        layer_refresh(console_layer, 0, 0, console_layer->bxsize, 21);
        break;

      /* Char keys */
      case 0x0e: // Backspace
        if (layer_is_active(layer_win)) {
          if (cursor_x > 8) {
            print_on_layer(layer_win, cursor_x, 28, COLOR_WHITE, COLOR_BLACK, " ");
            cursor_x -= 8;
          }
        } else if (layer_is_active(console_layer)) {
          fifo_enqueue(&console_task->fifo, keycode + 256);
        }
        break;
      case 0x1c: // Enter
        if (layer_is_active(console_layer)) {
          fifo_enqueue(&console_task->fifo, keycode + 256);
        }
        break;

      default: // Print a char
        if (keycode < 0x54) {
          char c = keycode_to_char(keycode, shift_pressed, caps_locked);
          if (layer_is_active(layer_win)) {
            if (c != 0) {
              s[0] = c;
              s[1] = '\0';
              print_on_layer(layer_win, cursor_x, 28, COLOR_WHITE, COLOR_BLACK, s);
              cursor_x += 8;
            }
          } else if (layer_is_active(console_layer)) {
            // Add 256 to avoid conflict with curosor event.
            fifo_enqueue(&console_task->fifo, c + 256);
          }
        }
        break;
      }

      // Draw cursor
      if (!layer_is_active(layer_win)) {
        cursor_c = COLOR_WHITE;
      }
      boxfill8(layer_win->buf, layer_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
      layer_refresh(layer_win, cursor_x, 28, cursor_x + 8, 44);

    } else if (EVENT_MOUSE_INPUT <= event) {
      int32_t keycode = event - EVENT_MOUSE_INPUT;
      if (mouse_decode(&mouse_decoder, keycode) != 0) {
#ifdef DEBUG
        sprintf(s, "[lcr %d %d]", mouse_decoder.x, mouse_decoder.y);
        if ((mouse_decoder.btn & 0x01) != 0) {
          s[1] = 'L';
        }
        if ((mouse_decoder.btn & 0x02) != 0) {
          s[3] = 'R';
        }
        if ((mouse_decoder.btn & 0x04) != 0) {
          s[2] = 'C';
        }
        print_on_layer(layer_back, 32, 16, COLOR_DARK_CYAN, COLOR_WHITE, s);
#endif

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
#ifdef DEBUG
        sprintf(s, "(%d, %d)", mx, my);
        print_on_layer(layer_back, 0, 0, COLOR_DARK_CYAN, COLOR_WHITE, s);
#endif
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
      if (!layer_is_active(layer_win)) {
        cursor_c = COLOR_WHITE;
      }
      boxfill8(layer_win->buf, layer_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
      layer_refresh(layer_win, cursor_x, 28, cursor_x + 8, 44);
    }
  }
}
