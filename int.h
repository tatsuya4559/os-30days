#ifndef _INT_H_
#define _INT_H_

#include "common.h"

#define  PIC0_ICW1  0x0020
#define  PIC0_OCW2  0x0020
#define  PIC0_IMR   0x0021
#define  PIC0_ICW2  0x0021
#define  PIC0_ICW3  0x0021
#define  PIC0_ICW4  0x0021
#define  PIC1_ICW1  0x00a0
#define  PIC1_OCW2  0x00a0
#define  PIC1_IMR   0x00a1
#define  PIC1_ICW2  0x00a1
#define  PIC1_ICW3  0x00a1
#define  PIC1_ICW4  0x00a1

void init_pic(void);

#define KEYBUF_QUEUE_SIZE 32

typedef struct {
    Byte data[KEYBUF_QUEUE_SIZE];
    int reading_next;
    int writing_next;
    int len;
} KeyBuf;

void keybuf_enqueue(KeyBuf *keybuf, Byte keycode);
Byte keybuf_dequeue(KeyBuf *keybuf);

extern KeyBuf keybuf;

#endif /* _INT_H_ */
