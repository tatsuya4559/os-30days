#ifndef _COMMON_H_
#define _COMMON_H_

typedef unsigned char Byte;

typedef struct {
  Byte cyls, leds, vmode, reserve;
  short scrnx, scrny;
  Byte *vram;
} BootInfo;

#define ADR_BOOTINFO 0x00000ff0

#define  PORT_KEYDAT           0x0060
#define  PORT_KEYSTA           0x0064
#define  PORT_KEYCMD           0x0064
#define  KEYSTA_SEND_NOTREADY  0x02
#define  KEYCMD_WRITE_MODE     0x60
#define  KEYCMD_SENDTO_MOUSE   0xd4
#define  MOUSECMD_ENABLE       0xf4
#define  KBC_MODE              0x47

#endif /* _COMMON_H_ */
