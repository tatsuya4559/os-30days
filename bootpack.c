#include <stdbool.h>
#include "nasmfunc.h"
#include "common.h"
#include "iolib.h"
#include "graphic.h"
#include "dsctbl.h"
#include "int.h"
#include "keyboard.h"
#include "mouse.h"

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int
memtest_sub(unsigned int start, unsigned int end)
{
    // optimizaion may break this function
    unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
    for (i = start; i <= end; i += 0x1000) {
        p = (unsigned int *) (i + 0xffc); // check last 4 bytes
        old = *p;
        *p = pat0; // try writing
        *p ^= 0xffffffff; // invert
        if (*p != pat1) {
not_memory:
            *p = old;
            break;
        }
        *p ^= 0xffffffff; // re-invert
        if (*p != pat0) {
            goto not_memory;
        }
        *p = old;
    }
    return i;
}

/* test how much memory can be used */
static
unsigned int
memtest(unsigned int start, unsigned int end)
{
    bool is486 = false;
    unsigned int eflg, cr0, i;

    // check if CPU is 386 or 486
    eflg = _io_load_eflags();
    eflg |= EFLAGS_AC_BIT; // AC-bit = 1
    _io_store_eflags(eflg);
    eflg = _io_load_eflags();
    if ((eflg & EFLAGS_AC_BIT) != 0) { // 386ではAC=1にしても自動で0に戻ってしまう
        is486 = true;
    }
    eflg &= ~EFLAGS_AC_BIT; // AC-bit = 0
    _io_store_eflags(eflg);

    if (is486) {
        cr0 = _load_cr0();
        cr0 |= CR0_CACHE_DISABLE; // disable cache
        _store_cr0(cr0);
    }

    i = memtest_sub(start, end);

    if (is486) {
        cr0 = _load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE; // enable cache
        _store_cr0(cr0);
    }

    return i;
}

#define MEMMAN_ADDR 0x003c0000
#define MEMMAN_FREES 4090

typedef struct {
    unsigned int addr, size;
} FreeInfo;

typedef struct {
    int frees, maxfrees, lostsize, losts;
    FreeInfo free[MEMMAN_FREES];
} MemoryManager;

void
memman_init(MemoryManager *man)
{
    man->frees = 0;
    man->maxfrees = 0;
    man->lostsize = 0;
    man->losts = 0;
}

unsigned int
memman_total(MemoryManager *man)
{
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++) {
        t += man->free[i].size;
    }
    return t;
}

unsigned int
memman_alloc(MemoryManager *man, unsigned int size)
{
    unsigned int i, a;
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].size >= size) {
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0) {
                // close the hole
                man->frees--;
                for (; i < man->frees; i++) {
                    man->free[i] = man->free[i + 1];
                }
            }
            return a;
        }
    }
    return 0; // no enough memory
}

int
memman_free(MemoryManager *man, unsigned int addr, unsigned int size)
{
    int i, j;
    // find the hole to insert
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }
    // free[i - 1].addr < addr < free[i].addr
    if (i > 0) {
        if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
            // merge with the hole
            man->free[i - 1].size += size;
            if (i < man->frees) {
                if (addr + size == man->free[i].addr) {
                    // merge with the next hole
                    man->free[i - 1].size += man->free[i].size;
                    // remove the hole
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
            return 0; // success
        }
    }
    if (i < man->frees) {
        if (addr + size == man->free[i].addr) {
            // merge with the next hole
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0; // success
        }
    }
    if (man->frees < MEMMAN_FREES) {
        // shift right
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees;
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0; // success
    }
    man->losts++;
    man->lostsize += size;
    return -1; // fail
}

static
void
set_screen(BootInfo *binfo)
{
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
}

void
hari_main(void)
{
    init_gdtidt();
    init_pic();
    _io_sti();

    Byte keybuf[32], mousebuf[128];
    fifo_init(&keyfifo, 32, keybuf);
    fifo_init(&mousefifo, 128, mousebuf);
    MouseDec mdec;

    _io_out8(PIC0_IMR, 0xf9); // PIC1とキーボードを許可
    _io_out8(PIC1_IMR, 0xef); // マウスを許可

    init_keyboard();

    init_palette();
    BootInfo *binfo = (BootInfo *) ADR_BOOTINFO;
    set_screen(binfo);

    Byte *vram = binfo->vram;

    char s0[20];
    sprintf(s0, "scrnx = %d", binfo->scrnx);
    putfonts8_asc(vram, binfo->scrnx, 40, 80, COLOR_WHITE, s0);

    int mx, my;
    Byte mcursor[256];
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COLOR_DARK_CYAN);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s0, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COLOR_WHITE, s0);

    MemoryManager *memman = (MemoryManager *) MEMMAN_ADDR;

    unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    sprintf(s0, "memory %dMB   free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COLOR_WHITE, s0);

    enable_mouse(&mdec);

    Byte keycode;
    char s[4];
    for (;;) {
        _io_cli(); // 割り込み禁止
        if (keyfifo.len == 0 && mousefifo.len == 0) {
            _io_stihlt();
        } else if (keyfifo.len != 0) {
            keycode = fifo_dequeue(&keyfifo);
            _io_sti(); // 割り込み禁止解除
            sprintf(s0, "%x", keycode);
            boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COLOR_WHITE, s0);
        } else if (mousefifo.len != 0) {
            keycode = fifo_dequeue(&mousefifo);
            _io_sti();
            if (mouse_decode(&mdec, keycode) != 0) {
                sprintf(s0, "[lcr %d %d]", mdec.x, mdec.y);
                if ((mdec.btn & 0x01) != 0) {
                    s0[1] = 'L';
                }
                if ((mdec.btn & 0x02) != 0) {
                    s0[3] = 'R';
                }
                if ((mdec.btn & 0x04) != 0) {
                    s0[2] = 'C';
                }
                boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, 32, 16, 32 + 15*8 - 1, 31);
                putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COLOR_WHITE, s0);

                // move mouse cursor
                boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, mx, my, mx + 15, my + 15); // hide mouse cursor
                mx += mdec.x;
                my += mdec.y;
                if (mx < 0) {
                    mx = 0;
                }
                if (my < 0) {
                    my = 0;
                }
                if (mx > binfo->scrnx - 16) {
                    mx = binfo->scrnx - 16;
                }
                if (my > binfo->scrny - 16) {
                    my = binfo->scrny - 16;
                }
                sprintf(s, "(%d, %d)", mx, my);
                boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, 0, 0, 79, 15); // hide coordinates
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COLOR_WHITE, s); // show coordinates
                putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); // show mouse cursor
            }
        }
    }
}
