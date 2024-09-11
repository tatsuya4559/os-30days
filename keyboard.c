#include "common.h"
#include "nasmfunc.h"

void
wait_KBC_sendready(void)
{
    for (;;) {
        if ((_io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            return;
        }
    }
}

void
init_keyboard(void)
{
    wait_KBC_sendready();
    _io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    _io_out8(PORT_KEYDAT, KBC_MODE);
}
