; vim: tabstop=8 noexpandtab

section .text
	GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
	GLOBAL	_io_in8, _io_in16, _io_in32
	GLOBAL	_io_out8, _io_out16, _io_out32
	GLOBAL	_io_load_eflags, _io_store_eflags
	GLOBAL	_write_mem8

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

_io_in8: ; int _io_in8(int port);
	MOV	EDX,[ESP+4]	; port
	MOV	EAX,0
	IN	AL,DX
	RET

_io_in16: ; int _io_in16(int port);
	MOV	EDX,[ESP+4]	; port
	MOV	EAX,0
	IN	AX,DX
	RET

_io_in32: ; int _io_in32(int port);
	MOV	EDX,[ESP+4]	; port
	IN	EAX,DX
	RET

_io_out8: ; void _io_out8(int port, int data);
	MOV	EDX,[ESP+4]	; port
	MOV	AL,[ESP+8]	; data
	OUT	DX,AL
	RET

_io_out16: ; void _io_out16(int port, int data);
	MOV	EDX,[ESP+4]	; port
	MOV	EAX,[ESP+8]	; data
	OUT	DX,AX
	RET

_io_out32: ; void _io_out32(int port, int data);
	MOV	EDX,[ESP+4]	; port
	MOV	EAX,[ESP+8]	; data
	OUT	DX,EAX
	RET

_io_load_eflags: ; int _io_load_eflags(void);
	PUSHFD
	POP	EAX	; returnしたときにEAXに入っている値が戻り値
	RET

_io_store_eflags: ; void _io_store_eflags(int eflags);
	MOV	EAX,[ESP+4]
	PUSH	EAX
	POPFD
	RET

_write_mem8: ; void _write_mem8(int addr, int data);
	MOV	ECX,[ESP+4]	; addr
	MOV	AL,[ESP+8]	; data
	MOV	[ECX],AL
	RET
