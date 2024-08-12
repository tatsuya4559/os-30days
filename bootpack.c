#include "common.h"
#include "iolib.h"
#include "graphic.h"

extern void _io_hlt(void);
extern void _load_gdtr(int limit, int addr);
extern void _load_idtr(int limit, int addr);

typedef struct {
    short limit_low, base_low;
    Byte base_mid, access_right;
    Byte limit_high, base_high;
} SegmentDescriptor;

typedef struct {
    short offset_low, selector;
    Byte dw_count, access_right;
    short offset_high;
} GateDescriptor;

void
set_segmdesc(SegmentDescriptor *sd, unsigned int limit, int base, int ar)
{
    if (limit > 0xfffff) {
        ar |= 0x8000;
        limit /= 0x1000;
    }
    sd->limit_low = limit & 0xffff;
    sd->base_low = base & 0xffff;
    sd->base_mid = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high = (base >> 24) & 0xff;
}

void
set_gatedesc(GateDescriptor *gd, int offset, int selector, int ar)
{
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high = (offset >> 16) & 0xffff;
}

void
init_gdtidt(void)
{
    SegmentDescriptor *gdt = (SegmentDescriptor *) 0x00270000;
    GateDescriptor *idt = (GateDescriptor *) 0x0026f800;

    // init GDT
    for (int i=0; i<8192; i++) {
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
    set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
    _load_gdtr(0xffff, 0x00270000);

    // init IDT
    for (int i=0; i< 256; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    _load_idtr(0x7ff, 0x0026f800);
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
