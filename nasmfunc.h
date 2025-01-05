#pragma once

#include "types.h"

extern void _io_hlt(void);
extern void _io_stihlt(void);

extern int32_t _io_in8(int32_t port);
extern void _io_out8(int32_t port, int32_t data);
extern void _io_cli(void);
extern void _io_sti(void);
extern int32_t _io_load_eflags(void);
extern void _io_store_eflags(int32_t eflags);

extern void _load_gdtr(int32_t limit, int32_t addr);
extern void _load_idtr(int32_t limit, int32_t addr);

extern int32_t _load_cr0(void);
extern void _store_cr0(int32_t cr0);

extern void _asm_inthandler20(void);
extern void _asm_inthandler21(void);
extern void _asm_inthandler2c(void);
