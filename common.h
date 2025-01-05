#pragma once

#include "types.h"

typedef struct {
  uint8_t cyls, leds, vmode, reserve;
  short scrnx, scrny;
  uint8_t *vram;
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
