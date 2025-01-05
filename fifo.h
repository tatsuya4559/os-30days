#pragma once

#include "common.h"

#define FIFO_OVERRUN 0x0001

typedef struct {
  uint8_t *buf;
  int32_t reading_next;
  int32_t writing_next;
  int32_t cap;
  int32_t len;
  int32_t flags;
} FIFO;

void fifo_init(FIFO *fifo, int32_t cap, uint8_t *buf);
int32_t fifo_enqueue(FIFO *fifo, uint8_t b);
uint8_t fifo_dequeue(FIFO *fifo);
