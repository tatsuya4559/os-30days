#include "fifo.h"
#include "mtask.h"

void
fifo_init(FIFO *fifo, int32_t cap, int32_t *buf, void *metadata)
{
  fifo->buf = buf;
  fifo->reading_next = 0;
  fifo->writing_next = 0;
  fifo->cap = cap;
  fifo->len = 0;
  fifo->flags = 0;
  fifo->metadata = metadata;
}

int32_t
fifo_enqueue(FIFO *fifo, int32_t b)
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
  if (fifo->metadata != NULL) {
    Task *task = (Task *) fifo->metadata;
    if (task->status != TASK_RUNNING) {
      task_run(task, -1, 0);
    }
  }
  return 0;
}

int32_t
fifo_dequeue(FIFO *fifo)
{
  int32_t b = fifo->buf[fifo->reading_next];
  fifo->len--;
  fifo->reading_next++;
  if (fifo->reading_next == fifo->cap) {
    fifo->reading_next = 0;
  }
  return b;
}
