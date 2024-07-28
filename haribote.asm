; vim: tabstop=8 noexpandtab

; BOOT INFO
CYLS	EQU	0x0ff0	; set by boot sector
LEDS	EQU	0x0ff1
VMODE	EQU	0x0ff2	; bit color
SCRNX	EQU	0x0ff4	; screen x
SCRNY	EQU	0x0ff6	; screen y
VRAM	EQU	0x0ff8	; base address of graphic buffer

ORG	0xc200

MOV	AL,0x13	; VGA graphics
MOV	AH,0x00
INT	0x10
MOV	BYTE [VMODE],8
MOV	WORD [SCRNX],320
MOV	WORD [SCRNY],200
MOV	DWORD [VRAM],0x000a0000

; state of keyboard
MOV	AH,0x02
INT	0x16	; keyboard BIOS
MOV	[LEDS],AL

fin:
	HLT
	JMP fin
