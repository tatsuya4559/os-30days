#ifndef _NASMFUNC_H_
#define _NASMFUNC_H_

extern void _io_hlt(void);

extern void _io_out8(int port, int data);
extern void _io_cli(void);
extern int _io_load_eflags(void);
extern void _io_store_eflags(int eflags);

extern void _load_gdtr(int limit, int addr);
extern void _load_idtr(int limit, int addr);

#endif /* _NASMFUNC_H_ */
