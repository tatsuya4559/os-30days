#ifndef _FIFO_H_
#define _FIFO_H_

#include "common.h"

#define FIFO_OVERRUN 0x0001

typedef struct {
    Byte *buf;
    int reading_next;
    int writing_next;
    int cap;
    int len;
    int flags;
} FIFO;

void fifo_init(FIFO *fifo, int cap, Byte *buf);
int fifo_enqueue(FIFO *fifo, Byte b);
Byte fifo_dequeue(FIFO *fifo);

#endif /* _FIFO_H_ */
