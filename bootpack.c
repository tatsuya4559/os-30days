#include "font.h"
#include "iolib.h"

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

typedef unsigned char Byte;

extern void _io_hlt(void);
extern void _io_cli(void);
extern void _io_out8(int port, int data);
extern int _io_load_eflags(void);
extern void _io_store_eflags(int eflags);

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
            if (cursor[y][x] == '*') {
                mouse[y*16 + x] = COLOR_BLACK;
            }
            if (cursor[y][x] == 'o') {
                mouse[y*16 + x] = COLOR_WHITE;
            }
            if (cursor[y][x] == '.') {
                mouse[y*16 + x] = background_color;
            }
        }
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

typedef struct {
    Byte cyls, leds, vmode, reserve;
    short scrnx, scrny;
    Byte *vram;
} BootInfo;

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
hari_main(void)
{
    init_palette();

    BootInfo *binfo = (BootInfo *) 0x0ff0;
    Byte *vram = binfo->vram;

    boxfill8(vram, binfo->scrnx, COLOR_DARK_CYAN, 0, 0, binfo->scrnx - 1, binfo->scrny - 29);
    boxfill8(vram, binfo->scrnx, COLOR_LIGHT_GRAY, 0, binfo->scrny - 28, binfo->scrnx - 1, binfo->scrny - 28);
    boxfill8(vram, binfo->scrnx, COLOR_WHITE, 0, binfo->scrny - 27, binfo->scrnx - 1, binfo->scrny - 27);
    boxfill8(vram, binfo->scrnx, COLOR_LIGHT_GRAY, 0, binfo->scrny - 26, binfo->scrnx - 1, binfo->scrny - 1);

    boxfill8(vram, binfo->scrnx, COLOR_WHITE, 3, binfo->scrny - 24, 59, binfo->scrny - 24);
    boxfill8(vram, binfo->scrnx, COLOR_WHITE, 2, binfo->scrny - 24, 2, binfo->scrny - 4);
    boxfill8(vram, binfo->scrnx, COLOR_DARK_GRAY, 3, binfo->scrny - 4, 59, binfo->scrny - 4);
    boxfill8(vram, binfo->scrnx, COLOR_DARK_GRAY, 59, binfo->scrny - 23, 59, binfo->scrny - 5);
    boxfill8(vram, binfo->scrnx, COLOR_BLACK, 2, binfo->scrny - 3, 59, binfo->scrny - 3);
    boxfill8(vram, binfo->scrnx, COLOR_BLACK, 60, binfo->scrny - 24, 60, binfo->scrny - 3);

    boxfill8(vram, binfo->scrnx, COLOR_DARK_GRAY, binfo->scrnx - 47, binfo->scrny - 24, binfo->scrnx - 4, binfo->scrny - 24);
    boxfill8(vram, binfo->scrnx, COLOR_DARK_GRAY, binfo->scrnx - 47, binfo->scrny - 23, binfo->scrnx - 47, binfo->scrny - 4);
    boxfill8(vram, binfo->scrnx, COLOR_WHITE, binfo->scrnx - 47, binfo->scrny - 3, binfo->scrnx - 4, binfo->scrny - 3);
    boxfill8(vram, binfo->scrnx, COLOR_WHITE, binfo->scrnx - 3, binfo->scrny - 24, binfo->scrnx - 3, binfo->scrny - 3);

    putfonts8_asc(vram, binfo->scrnx, 40, 40, COLOR_WHITE, "Hello World!");

    char s[20];
    sprintf(s, "scrnx = %d", binfo->scrnx);
    putfonts8_asc(vram, binfo->scrnx, 40, 80, COLOR_WHITE, s);

    int mx, my;
    Byte mcursor[256];
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COLOR_DARK_CYAN);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COLOR_WHITE, s);

    for (;;) {
        _io_hlt();
    }
}
