#include "graphic.h"
#include "mtask.h"
#include "file.h"
#include "nasmfunc.h"
#include "iolib.h"
#include "strutil.h"
#include "console.h"

#define ADR_DISKIMG  0x00100000
#define MAX_FILES 224

typedef struct {
  FileInfo *finfo;
  int32_t index;
} FileInfoIterator;

static
FileInfo *
next_file(FileInfoIterator *iter)
{
  if (iter->index >= MAX_FILES) {
    return NULL;
  }
  for (;;) {
    FileInfo *next = &iter->finfo[iter->index++];
    if (next->name[0] == 0x00) {
      // If the first byte of the name is 0x00, there are no more files.
      return NULL;
    }
    if (next->name[0] == 0xe5) {
      // If the first byte of the name is 0xe5, the file is deleted.
      continue;
    }
    return next;
  }
  return NULL;
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
FileInfo *
search_file(FileInfo *finfo, const char *filename)
{
  char name[13];
  FileInfoIterator iter = {
    .finfo = finfo,
    .index = 0,
  };
  FileInfo *file;
  while ((file = next_file(&iter)) != NULL) {
    file_normalized_name(file, name);
    if (str_equal(name, filename)) {
      return file;
    }
  }
  return NULL;
}

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
          FileInfoIterator iter = {
            .finfo = finfo,
            .index = 0,
          };
          FileInfo *file;
          while ((file = next_file(&iter)) != NULL) {
            if (file->type == 0x00 || file->type == 0x20) {
              char filename[13];
              file_normalized_name(file, filename);
              sprintf(s, "%s %d", filename, file->size);
              print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, s);
              cursor_y = console_newline(cursor_y, layer);
            }
          }
        } else if (str_has_prefix(cmdline, "type ")) {
          char *filename = str_to_upper(str_trim_prefix(cmdline, "type "));
          FileInfo *target_file = search_file(finfo, filename);

          if (target_file != NULL) {
            print_on_layer(layer, 8, cursor_y, COLOR_BLACK, COLOR_WHITE, filename);
            cursor_y = console_newline(cursor_y, layer);

            uint8_t *buf = (uint8_t *) memman_alloc_4k(memman, target_file->size);
            file_load(target_file->clustno, target_file->size, buf, fat, (uint8_t *) (ADR_DISKIMG + 0x003e00));

            // Print the file content
            cursor_x = 8;
            for (int32_t p = 0; p < target_file->size; p++) {
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
            memman_free_4k(memman, (uint32_t) buf, target_file->size);
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
