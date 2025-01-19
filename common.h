#pragma once

#include "types.h"

#define  PORT_KEYDAT           0x0060
#define  PORT_KEYSTA           0x0064
#define  PORT_KEYCMD           0x0064
#define  KEYSTA_SEND_NOTREADY  0x02
#define  KEYCMD_WRITE_MODE     0x60
#define  KEYCMD_SENDTO_MOUSE   0xd4
#define  MOUSECMD_ENABLE       0xf4
#define  KBC_MODE              0x47
#define  FIFO_BUF_SIZE         128
#define  MEMMAN_ADDR           0x003c0000
