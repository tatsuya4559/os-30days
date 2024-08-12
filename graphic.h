#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

#include "common.h"

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

void init_palette(void);
void putfonts8_asc(Byte *vram, int xsize, int x, int y, Byte c, char *string);
void init_mouse_cursor8(Byte *mouse, Byte background_color);
void putblock8_8(Byte *vram, int vxsize, int pxsize, int pysize, int px0, int py0, Byte *buf, int bxsize);
void boxfill8(Byte *vram, int xsize, Byte c, int x0, int y0, int x1, int y1);

#endif /* _GRAPHIC_H_ */
