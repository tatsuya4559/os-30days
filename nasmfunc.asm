; vim: tabstop=8 noexpandtab

section .text
	GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
	GLOBAL	_io_in8, _io_in16, _io_in32
	GLOBAL	_io_out8, _io_out16, _io_out32
	GLOBAL	_io_load_eflags, _io_store_eflags
	GLOBAL	_load_gdtr, _load_idtr
	GLOBAL	_load_tr, _farjmp
	GLOBAL	_load_cr0, _store_cr0
	GLOBAL	_asm_inthandler20, _asm_inthandler21, _asm_inthandler2c
	EXTERN	inthandler20, inthandler21, inthandler2c

_io_hlt: ; void _io_hlt(void);
	HLT
	RET

_io_cli: ; void _io_cli(void);
	CLI
	RET

_io_sti: ; void _io_sti(void);
	STI
	RET

_io_stihlt: ; void _io_stihlt(void);
	STI
	HLT
	RET

_io_in8: ; int32_t _io_in8(int32_t port);
	MOV	EDX,[ESP+4]	; port
	MOV	EAX,0
	IN	AL,DX
	RET

_io_in16: ; int32_t _io_in16(int32_t port);
	MOV	EDX,[ESP+4]	; port
	MOV	EAX,0
	IN	AX,DX
	RET

_io_in32: ; int32_t _io_in32(int32_t port);
	MOV	EDX,[ESP+4]	; port
	IN	EAX,DX
	RET

_io_out8: ; void _io_out8(int32_t port, int32_t data);
	MOV	EDX,[ESP+4]	; port
	MOV	AL,[ESP+8]	; data
	OUT	DX,AL
	RET

_io_out16: ; void _io_out16(int32_t port, int32_t data);
	MOV	EDX,[ESP+4]	; port
	MOV	EAX,[ESP+8]	; data
	OUT	DX,AX
	RET

_io_out32: ; void _io_out32(int32_t port, int32_t data);
	MOV	EDX,[ESP+4]	; port
	MOV	EAX,[ESP+8]	; data
	OUT	DX,EAX
	RET

_io_load_eflags: ; int32_t _io_load_eflags(void);
	PUSHFD
	POP	EAX	; returnしたときにEAXに入っている値が戻り値
	RET

_io_store_eflags: ; void _io_store_eflags(int32_t eflags);
	MOV	EAX,[ESP+4]
	PUSH	EAX
	POPFD
	RET

_load_gdtr: ; void _load_gdtr(int32_t limit, int32_t addr);
	MOV	AX,[ESP+4]
	MOV	[ESP+6],AX
	LGDT	[ESP+6]
	RET

_load_idtr: ; void _load_idtr(int32_t limit, int32_t addr);
	MOV	AX,[ESP+4]
	MOV	[ESP+6],AX
	LIDT	[ESP+6]
	RET

_load_tr: ; void _load_tr(int32_t tr);
	LTR	[ESP+4]
	RET

_farjmp: ; void _farjmp(int32_t eip, int32_t cs);
	JMP	FAR [ESP+4]
	RET

_load_cr0: ; int32_t _load_cr0(void);
	MOV	EAX,CR0
	RET

_store_cr0: ; void _store_cr0(int32_t cr0);
	MOV	EAX,[ESP+4]
	MOV	CR0,EAX
	RET

_asm_inthandler20:
	PUSH	ES
	PUSH	DS
	PUSHAD
	MOV	EAX,ESP
	PUSH	EAX
	MOV	AX,SS
	MOV	DS,AX
	MOV	ES,AX
	CALL	inthandler20
	POP	EAX
	POPAD
	POP	DS
	POP	ES
	IRETD

_asm_inthandler21:
    PUSH	ES
    PUSH	DS
    PUSHAD
    MOV		EAX,ESP
    PUSH	EAX
    MOV		AX,SS
    MOV		DS,AX
    MOV		ES,AX
    CALL	inthandler21
    POP		EAX
    POPAD
    POP		DS
    POP		ES
    IRETD

_asm_inthandler2c:
    PUSH	ES
    PUSH	DS
    PUSHAD
    MOV		EAX,ESP
    PUSH	EAX
    MOV		AX,SS
    MOV		DS,AX
    MOV		ES,AX
    CALL	inthandler2c
    POP		EAX
    POPAD
    POP		DS
    POP		ES
    IRETD
