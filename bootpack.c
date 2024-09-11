#include "nasmfunc.h"
#include "common.h"
#include "iolib.h"
#include "graphic.h"
#include "dsctbl.h"
#include "int.h"

#define  PORT_KEYDAT           0x0060
#define  PORT_KEYSTA           0x0064
#define  PORT_KEYCMD           0x0064
#define  KEYSTA_SEND_NOTREADY  0x02
#define  KEYCMD_WRITE_MODE     0x60
#define  KEYCMD_SENDTO_MOUSE   0xd4
#define  MOUSECMD_ENABLE       0xf4
#define  KBC_MODE              0x47

static
void
wait_KBC_sendready(void)
{
    for (;;) {
        if ((_io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            return;
        }
    }
}

static
void
init_keyboard(void)
{
    wait_KBC_sendready();
    _io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    _io_out8(PORT_KEYDAT, KBC_MODE);
}

typedef enum {
    MOUSE_DEC_PHASE_WAITING_0XFA,
    MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE,
    MOUSE_DEC_PHASE_RECEIVED_2ND_BYTE,
    MOUSE_DEC_PHASE_RECEIVED_3RD_BYTE,
} MouseDecPhase;

typedef struct {
    Byte buf[3];
    MouseDecPhase phase;
    int x, y, btn;
} MouseDec;

static
void
enable_mouse(MouseDec *mdec)
{
    wait_KBC_sendready();
    _io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    _io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = MOUSE_DEC_PHASE_WAITING_0XFA;
}

static
int
mouse_decode(MouseDec *mdec, Byte data)
{
    switch (mdec->phase) {
        case MOUSE_DEC_PHASE_WAITING_0XFA:
            if (data == 0xfa) {
                mdec->phase = MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE;
            }
            return 0;
        case MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE:
            if ((data & 0xc8) == 0x08) {
                mdec->buf[0] = data;
                mdec->phase = MOUSE_DEC_PHASE_RECEIVED_2ND_BYTE;
            }
            return 0;
        case MOUSE_DEC_PHASE_RECEIVED_2ND_BYTE:
            mdec->buf[1] = data;
            mdec->phase = MOUSE_DEC_PHASE_RECEIVED_3RD_BYTE;
            return 0;
        case MOUSE_DEC_PHASE_RECEIVED_3RD_BYTE:
            mdec->buf[2] = data;
            mdec->phase = MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE;
            mdec->btn = mdec->buf[0] & 0x07; // extract lower 3 bits
            mdec->x = mdec->buf[1];
            mdec->y = mdec->buf[2];
            if ((mdec->buf[0] & 0x10) != 0) {
                mdec->x |= 0xffffff00;
            }
            if ((mdec->buf[0] & 0x20) != 0) {
                mdec->y |= 0xffffff00;
            }
            mdec->y = -mdec->y; // マウスではy方向の符号が画面と逆
            return 1;
        default:
            return -1; // unreachable
    }
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

static
void
show_message(BootInfo *binfo)
{
    Byte *vram = binfo->vram;

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
    show_message(binfo);

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
            sprintf(s, "%x", keycode);
            boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COLOR_WHITE, s);
        } else if (mousefifo.len != 0) {
            keycode = fifo_dequeue(&mousefifo);
            _io_sti();
            if (mouse_decode(&mdec, keycode) != 0) {
                sprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
                if ((mdec.btn & 0x01) != 0) {
                    s[1] = 'L';
                }
                if ((mdec.btn & 0x02) != 0) {
                    s[3] = 'R';
                }
                if ((mdec.btn & 0x04) != 0) {
                    s[2] = 'C';
                }
                boxfill8(binfo->vram, binfo->scrnx, COLOR_DARK_CYAN, 32, 16, 32 + 15*8 - 1, 31);
                putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COLOR_WHITE, s);
            }
        }
    }
}
