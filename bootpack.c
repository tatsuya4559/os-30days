#include "fifo.h"
#include "nasmfunc.h"
#include "graphic.h"
#include "dsctbl.h"
#include "int.h"
#include "keyboard.h"
#include "iolib.h"
#include "mouse.h"
#include "memory.h"
#include "layer.h"
#include "timer.h"
#include "mtask.h"
#include "console.h"

/* #define DEBUG */
#define ADR_BOOTINFO 0x00000ff0
#define KEYCMD_LED 0xed

typedef struct {
  uint8_t cyls, leds, vmode, reserve;
  short scrnx, scrny;
  uint8_t *vram;
} BootInfo;

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
