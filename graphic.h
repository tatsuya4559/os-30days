#pragma once

#include "common.h"
#include "layer.h"

#define COLOR_BLACK        0
#define COLOR_LIGHT_RED    1
#define COLOR_LIGHT_GREEN  2
#define COLOR_LIGHT_YELLOW 3
#define COLOR_LIGHT_BLUE   4
#define COLOR_LIGHT_PURPLE 5
#define COLOR_LIGHT_CYAN   6
#define COLOR_WHITE        7
#define COLOR_LIGHT_GRAY   8
#define COLOR_DARK_RED     9
#define COLOR_DARK_GREEN   10
#define COLOR_DARK_YELLOW  11
#define COLOR_DARK_BLUE    12
#define COLOR_DARK_PURPLE  13
#define COLOR_DARK_CYAN    14
#define COLOR_DARK_GRAY    15
#define COLOR_TRANSPARENT  99

void init_palette(void);
void init_mouse_cursor8(uint8_t *mouse, uint8_t background_color);
void boxfill8(uint8_t *vram, int32_t xsize, uint8_t c, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
void init_screen8(uint8_t *vram, int32_t x, int32_t y);
void print_on_layer(Layer *layer, int32_t x, int32_t y, uint8_t bg_color, uint8_t fg_color, char *s);
void make_window_title(uint8_t *buf, int32_t xsize, char *title, bool_t active);
void make_window8(uint8_t *buf, int32_t xsize, int32_t ysize, char *title, bool_t active);
void make_textbox8(Layer *layer, int32_t x0, int32_t y0, int32_t width, int32_t height, int32_t color);
