#ifndef _COMMON_H_
#define _COMMON_H_

typedef unsigned char Byte;

typedef struct {
    Byte cyls, leds, vmode, reserve;
    short scrnx, scrny;
    Byte *vram;
} BootInfo;

#define ADR_BOOTINFO 0x00000ff0


#endif /* _COMMON_H_ */
