#ifndef _NASMFUNC_H_
#define _NASMFUNC_H_

extern void _io_hlt(void);

extern int _io_in8(int port);
extern void _io_out8(int port, int data);
extern void _io_cli(void);
extern void _io_sti(void);
extern int _io_load_eflags(void);
extern void _io_store_eflags(int eflags);

extern void _load_gdtr(int limit, int addr);
extern void _load_idtr(int limit, int addr);

extern void _asm_inthandler21(void);
extern void _asm_inthandler2c(void);

#endif /* _NASMFUNC_H_ */
