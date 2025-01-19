#pragma once

#include "layer.h"
#include "types.h"

// Console width must be a multiple of 8
// Console height must be a multiple of 16
/* #define CONSOLE_WIDTH 240 */
/* #define CONSOLE_HEIGHT 128 */
#define CONSOLE_WIDTH 632
#define CONSOLE_HEIGHT 320

void console_task_main(Layer *layer, uint32_t total_mem_size);
