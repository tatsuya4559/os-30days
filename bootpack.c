#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

extern void _io_hlt(void);
extern void _io_cli(void);
extern void _io_out8(int port, int data);
extern int _io_load_eflags(void);
extern void _io_store_eflags(int eflags);

void
set_palette(int start, int end, unsigned char *rgb)
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
    static unsigned char table_rgb[16 * 3] = {
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
boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            vram[x + y * xsize] = c;
        }
    }
}

void
hari_main(void)
{
    init_palette();

    unsigned char *vram = (unsigned char *) 0xa0000;

    boxfill8(vram, 320, COL8_FF0000, 20, 20, 120, 120);
    boxfill8(vram, 320, COL8_00FF00, 70, 50, 170, 150);
    boxfill8(vram, 320, COL8_0000FF, 120, 80, 220, 180);

    for (;;) {
        _io_hlt();
    }
}
