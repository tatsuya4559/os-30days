#include "fifo.h"

void
fifo_init(FIFO *fifo, int cap, Byte *buf)
{
  fifo->buf = buf;
  fifo->reading_next = 0;
  fifo->writing_next = 0;
  fifo->cap = cap;
  fifo->len = 0;
  fifo->flags = 0;
}

int
fifo_enqueue(FIFO *fifo, Byte b)
{
  if (fifo->len >= fifo->cap) {
    fifo->flags |= FIFO_OVERRUN;
    return -1;
  }
  fifo->buf[fifo->writing_next] = b;
  fifo->len++;
  fifo->writing_next++;
  if (fifo->writing_next == fifo->cap) {
    fifo->writing_next = 0;
  }
  return 0;
}

Byte
fifo_dequeue(FIFO *fifo)
{
  Byte b = fifo->buf[fifo->reading_next];
  fifo->len--;
  fifo->reading_next++;
  if (fifo->reading_next == fifo->cap) {
    fifo->reading_next = 0;
  }
  return b;
}
