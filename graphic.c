#include "nasmfunc.h"
#include "font.h"
#include "graphic.h"

static
void
set_palette(int start, int end, Byte *rgb)
{
    int i, eflags;
    eflags = _io_load_eflags(); // load interrupt flag
    _io_cli(); // set interrupt flag to 0; forbid interrupting
    _io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        _io_out8(0x03c9, rgb[0] / 4);
        _io_out8(0x03c9, rgb[1] / 4);
        _io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    _io_store_eflags(eflags); // recover interrupt flag
    return;
}

void
init_palette(void)
{
    // 普通に初期値を書くと要素数だけ代入文にコンパイルされる
    // 非効率なのでstatic宣言することで一発で初期化できる
    static Byte table_rgb[16 * 3] = {
        0x00, 0x00, 0x00, // 0: black
        0xff, 0x00, 0x00, // 1: light red
        0x00, 0xff, 0x00, // 2: light green
        0xff, 0xff, 0x00, // 3: light yellow
        0x00, 0x00, 0xff, // 4: light blue
        0xff, 0x00, 0xff, // 5: light purple
        0x00, 0xff, 0xff, // 6: light cyan
        0xff, 0xff, 0xff, // 7: white
        0xc6, 0xc6, 0xc6, // 8: light gray
        0x84, 0x00, 0x00, // 9: dark red
        0x00, 0x84, 0x00, // 10: dark green
        0x84, 0x84, 0x00, // 11: dark yellow
        0x00, 0x00, 0x84, // 12: dark blue
        0x84, 0x00, 0x84, // 13: dark purple
        0x00, 0x84, 0x84, // 14: dark cyan
        0x84, 0x84, 0x84, // 15: dark gray
    };
    set_palette(0, 15, table_rgb);
    return;
}

void
init_mouse_cursor8(Byte *mouse, Byte background_color)
{
    static char cursor[16][16] = {
        "**************..",
        "*ooooooooooo*...",
        "*oooooooooo*....",
        "*ooooooooo*.....",
        "*oooooooo*......",
        "*ooooooo*.......",
        "*ooooooo*.......",
        "*oooooooo*......",
        "*oooo**ooo*.....",
        "*ooo*..*ooo*....",
        "*oo*....*ooo*...",
        "*o*......*ooo*..",
        "**........*ooo*.",
        "*..........*ooo*",
        "............*oo*",
        ".............***",
    };

    int x, y;
    for (y=0; y<16; y++) {
        for (x=0; x<16; x++) {
            switch (cursor[y][x]) {
            case '*':
                mouse[y*16 + x] = COLOR_BLACK;
                break;
            case 'o':
                mouse[y*16 + x] = COLOR_WHITE;
                break;
            case '.':
                mouse[y*16 + x] = background_color;
                break;
            }
        }
    }
}

static
void
putfont8(Byte *vram, int xsize, int x, int y, Byte c, Byte *font)
{
    Byte *p, d;
    for (int i = 0; i < 16; i++) {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0) { p[0] = c; }
        if ((d & 0x40) != 0) { p[1] = c; }
        if ((d & 0x20) != 0) { p[2] = c; }
        if ((d & 0x10) != 0) { p[3] = c; }
        if ((d & 0x08) != 0) { p[4] = c; }
        if ((d & 0x04) != 0) { p[5] = c; }
        if ((d & 0x02) != 0) { p[6] = c; }
        if ((d & 0x01) != 0) { p[7] = c; }
    }
}

void
putfonts8_asc(Byte *vram, int xsize, int x, int y, Byte c, char *string)
{
    for (char *ch = string; *ch != '\0'; ch++) {
        putfont8(vram, xsize, x, y, c, fonts[*ch]);
        x += 8;
    }
}

void
putblock8_8(Byte *vram, int vxsize, int pxsize,
            int pysize, int px0, int py0, Byte *buf, int bxsize)
{
    int x, y;
    for (y=0; y<pysize; y++) {
        for (x=0; x<pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
}

void
boxfill8(Byte *vram, int xsize, Byte c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            vram[x + y * xsize] = c;
        }
    }
}

void
init_screen8(Byte *vram, int x, int y)
{
    boxfill8(vram, x, COLOR_DARK_CYAN, 0, 0, x - 1, y - 29);
    boxfill8(vram, x, COLOR_LIGHT_GRAY, 0, y - 28, x - 1, y - 28);
    boxfill8(vram, x, COLOR_WHITE, 0, y - 27, x - 1, y - 27);
    boxfill8(vram, x, COLOR_LIGHT_GRAY, 0, y - 26, x - 1, y - 1);

    boxfill8(vram, x, COLOR_WHITE, 3, y - 24, 59, y - 24);
    boxfill8(vram, x, COLOR_WHITE, 2, y - 24, 2, y - 4);
    boxfill8(vram, x, COLOR_DARK_GRAY, 3, y - 4, 59, y - 4);
    boxfill8(vram, x, COLOR_DARK_GRAY, 59, y - 23, 59, y - 5);
    boxfill8(vram, x, COLOR_BLACK, 2, y - 3, 59, y - 3);
    boxfill8(vram, x, COLOR_BLACK, 60, y - 24, 60, y - 3);

    boxfill8(vram, x, COLOR_DARK_GRAY, x - 47, y - 24, x - 4, y - 24);
    boxfill8(vram, x, COLOR_DARK_GRAY, x - 47, y - 23, x - 47, y - 4);
    boxfill8(vram, x, COLOR_WHITE, x - 47, y - 3, x - 4, y - 3);
    boxfill8(vram, x, COLOR_WHITE, x - 3, y - 24, x - 3, y - 3);
}
