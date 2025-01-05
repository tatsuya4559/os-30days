#pragma once

#include "fifo.h"

void wait_KBC_sendready(void);
void init_keyboard(FIFO *fifo, int32_t data0);
