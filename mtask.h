#pragma once

#include "timer.h"

extern Timer *mt_timer;
void mt_init(void);
void mt_taskswitch(void);
