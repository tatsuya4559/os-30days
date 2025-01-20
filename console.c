#include "dsctbl.h"
#include "graphic.h"
#include "mtask.h"
#include "file.h"
#include "nasmfunc.h"
#include "iolib.h"
#include "strutil.h"
#include "dsctbl.h"
#include "console.h"

#define ADR_DISKIMG  0x00100000
#define CMDLINE_LEN 30

FileInfo *finfo = (FileInfo *) (ADR_DISKIMG + 0x002600);
MemoryManager *memman = (MemoryManager *) MEMMAN_ADDR;
SegmentDescriptor *gdt = (SegmentDescriptor *) ADR_GDT;

typedef struct {
  Layer *layer;
  int32_t cursor_x, cursor_y, cursor_c;
} Console;

static
void
cons_newline(Console *cons)
{
  if (cons->cursor_y < 28 + CONSOLE_HEIGHT - 16) {
    cons->cursor_y += 16;
  } else {
    // scroll
    for (int32_t y = 28; y < 28 + CONSOLE_HEIGHT - 16; y++) {
      for (int32_t x = 8; x < 8 + CONSOLE_WIDTH; x++) {
        cons->layer->buf[x + y * cons->layer->bxsize] = cons->layer->buf[x + (y + 16) * cons->layer->bxsize];
      }
    }
    for (int32_t y = 28 + CONSOLE_HEIGHT - 16; y < 28 + CONSOLE_HEIGHT; y++) {
      for (int32_t x = 8; x < 8 + CONSOLE_WIDTH; x++) {
        cons->layer->buf[x + y * cons->layer->bxsize] = COLOR_BLACK;
      }
    }
    layer_refresh(cons->layer, 8, 28, 8 + CONSOLE_WIDTH, 28 + CONSOLE_HEIGHT);
  }
}

static
void
cons_putchar(Console *cons, char c, bool_t move_cursor)
{
  if (c == '\n') {
    cons->cursor_x = 8;
    cons_newline(cons);
    return;
  }
  if (c == '\t') {
    const int32_t tabstop = 8;
    do {
      print_on_layer(cons->layer, cons->cursor_x, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, " ");
      cons->cursor_x += 8;
    } while ((cons->cursor_x / 8) % tabstop != 1);
    return;
  }
  char s[2] = {c, '\0'};
  print_on_layer(cons->layer, cons->cursor_x, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, s);
  if (move_cursor) {
    cons->cursor_x += 8;
    if (cons->cursor_x >= 8 + CONSOLE_WIDTH) {
      cons->cursor_x = 8;
      cons_newline(cons);
    }
  }
}

static
void
cmd_mem(Console *cons, uint32_t total_mem_size)
{
  MemoryManager *memman = (MemoryManager *) MEMMAN_ADDR;
  char s[CMDLINE_LEN];
  sprintf(s, "total %dMB", total_mem_size / (1024 * 1024));
  print_on_layer(cons->layer, 8, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, s);
  cons_newline(cons);
  sprintf(s, "free %dKB", memman_total(memman) / 1024);
  print_on_layer(cons->layer, 8, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, s);
  cons_newline(cons);
  cons_newline(cons);
}

static
void
cmd_cls(Console *cons)
{
  for (int32_t y = 28; y < 28 + CONSOLE_HEIGHT; y++) {
    for (int32_t x = 8; x < 8 + CONSOLE_WIDTH; x++) {
      cons->layer->buf[x + y * cons->layer->bxsize] = COLOR_BLACK;
    }
  }
  layer_refresh(cons->layer, 8, 28, 8 + CONSOLE_WIDTH, 28 + CONSOLE_HEIGHT);
  cons->cursor_y = 28;
}

static
void
cmd_dir(Console *cons)
{
  char s[CMDLINE_LEN];
  FileInfoIterator iter = {
    .files = finfo,
    .index = 0,
  };
  FileInfo *file;
  while ((file = next_file(&iter)) != NULL) {
    if (file->type == 0x00 || file->type == 0x20) {
      char filename[13];
      file_normalized_name(file, filename);
      sprintf(s, "%s %d", filename, file->size);
      print_on_layer(cons->layer, 8, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, s);
      cons_newline(cons);
    }
  }
}

static
void
cmd_type(Console *cons, uint8_t *fat, char *cmdline)
{
  char *filename = str_to_upper(str_trim_prefix(cmdline, "type "));
  FileInfo *target_file = search_file(finfo, filename);
  char s[CMDLINE_LEN];

  if (target_file != NULL) {
    print_on_layer(cons->layer, 8, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, filename);
    cons_newline(cons);

    uint8_t *buf = (uint8_t *) memman_alloc_4k(memman, target_file->size);
    file_load(target_file->clustno, target_file->size, buf, fat, (uint8_t *) (ADR_DISKIMG + 0x003e00));

    // Print the file content
    cons->cursor_x = 8;
    for (int32_t p = 0; p < target_file->size; p++) {
      cons_putchar(cons, buf[p], TRUE);
    }
    memman_free_4k(memman, (uint32_t) buf, target_file->size);
  } else {
    print_on_layer(cons->layer, 8, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, "File not found.");
    cons_newline(cons);
  }
  cons_newline(cons);
}

static
void
cmd_hlt(Console *cons, uint8_t *fat)
{
  FileInfo *hlt_file = search_file(finfo, "HLT.HRB");
  if (hlt_file == NULL) {
    print_on_layer(cons->layer, 8, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, "hlt command not found.");
    cons_newline(cons);
  } else {
    uint8_t *buf = (uint8_t *) memman_alloc_4k(memman, hlt_file->size);
    file_load(hlt_file->clustno, hlt_file->size, buf, fat, (uint8_t *) (ADR_DISKIMG + 0x003e00));
    // We use 1003 segment since 1~2 are used in dsctbl.c and 3~1002 in mtask.c
    set_segmdesc(gdt + 1003, hlt_file->size - 1, (uint32_t) buf, AR_CODE32_ER);
    _farjmp(0, 1003 * 8);
    memman_free_4k(memman, (uint32_t) buf, hlt_file->size);
  }
  cons_newline(cons);
}

static
void
cons_run_cmd(Console *cons, char *cmdline, uint32_t total_mem_size, uint8_t *fat)
{
  if (str_equal(cmdline, "mem")) { // mem command
    cmd_mem(cons, total_mem_size);
  } else if (str_equal(cmdline, "cls")) { // clear screen
    cmd_cls(cons);
  } else if (str_equal(cmdline, "dir")) {
    cmd_dir(cons);
  } else if (str_has_prefix(cmdline, "type ")) {
    cmd_type(cons, fat, cmdline);
  } else if (str_equal(cmdline, "hlt")) {
    cmd_hlt(cons, fat);
  } else if (!str_equal(cmdline, "")) { // not a command but a string
    print_on_layer(cons->layer, 8, cons->cursor_y, COLOR_BLACK, COLOR_WHITE, "Bad command.");
    cons_newline(cons);
    cons_newline(cons);
  }
}

void
console_task_main(Layer *layer, uint32_t total_mem_size)
{
  Task *self = task_now();

  Console cons = {
    .layer = layer,
    .cursor_x = 8,
    .cursor_y  = 28,
    .cursor_c = COLOR_WHITE,
  };

  uint8_t *fat = (uint8_t *) memman_alloc_4k(memman, NUM_DISK_SECTORS * 4);
  file_readfat(fat, (uint8_t *) (ADR_DISKIMG + 0x000200));

  int32_t fifo_buf[FIFO_BUF_SIZE];
  fifo_init(&self->fifo, FIFO_BUF_SIZE, fifo_buf, self);

  Timer *timer = timer_alloc();
  timer_init(timer, &self->fifo, 1);
  timer_set_timeout(timer, 50);

  char cmdline[CMDLINE_LEN];

  cons_putchar(&cons, '>', TRUE);

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
        cons.cursor_c = COLOR_BLACK;
        break;
      case 1:
        timer_init(timer, &self->fifo, 0);
        cons.cursor_c = COLOR_WHITE;
        break;
      }
      timer_set_timeout(timer, 50);
      if (!layer_is_active(layer)) {
        cons.cursor_c = COLOR_BLACK;
      }
      boxfill8(layer->buf, layer->bxsize, cons.cursor_c, cons.cursor_x, cons.cursor_y, cons.cursor_x + 7, cons.cursor_y + 15);
      layer_refresh(layer, cons.cursor_x, cons.cursor_y, cons.cursor_x + 8, cons.cursor_y + 16);
    }

    // Keyboard input event
    if (256 <= event && event <= 511) {
      char c = event - 256;
      switch (c) {
      case 0: // Null character
        break;
      case 0x0e: // Backspace
        if (cons.cursor_x > 16) {
          cons_putchar(&cons, ' ', FALSE);
          cons.cursor_x -= 8;
        }
        break;
      case 0x1c: // Enter
        // Erase cursor
        cons_putchar(&cons, ' ', FALSE);
        cmdline[cons.cursor_x / 8 - 2] = '\0';
        cons_newline(&cons);

        // Run command
        cons_run_cmd(&cons, cmdline, total_mem_size, fat);

        // Draw a prompt
        cons.cursor_x = 8;
        cons_putchar(&cons, '>', TRUE);
        break;
      default:
        if (cons.cursor_x < CONSOLE_WIDTH) {
          cmdline[cons.cursor_x / 8 - 2] = c;
          cons_putchar(&cons, c, TRUE);
        }
        break;
      }

      // Draw cursor
      if (!layer_is_active(layer)) {
        cons.cursor_c = COLOR_BLACK;
      }
      boxfill8(layer->buf, layer->bxsize, cons.cursor_c, cons.cursor_x, cons.cursor_y, cons.cursor_x + 7, 43);
      layer_refresh(layer, cons.cursor_x, cons.cursor_y, cons.cursor_x + 8, cons.cursor_y + 16);
    }
  }
}
