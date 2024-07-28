; hello-os
; vim: tabstop=8 noexpandtab

; address to where program loaded
ORG	0x7c00

JMP	entry
DB	0x90

; FORMAT
DB     "HELLOIPL"
DW     512
DB     1
DW     1
DB     2
DW     224
DW     2880
DB     0xf0
DW     9
DW     18
DW     2
DD     0
DD     2880
DB     0,0,0x29
DD     0xffffffff
DB     "HELLO-OS   "
DB     "FAT12   "
TIMES  18      DB      0


; BODY
entry:
	MOV	AX,0
	MOV	SS,AX
	MOV	SP,0x7c00
	MOV	DS,AX

	; Load disk
	MOV	AX,0x0820
	MOV	ES,AX
	MOV	DH,0	; at head 0
	MOV	CH,0	; at cylinder 0
	MOV	CL,2	; at sector 2

readloop:
	MOV	SI,0	; retry counter

retry:
	MOV	AH,0x02	; load disk
	MOV	AL,1	; for 1 sector
	MOV	BX,0	; into this address (ES*16 + BX)
	MOV	DL,0x00	; from drive 0
	INT	0x13	; invoke BIOS function: load disk
	JNC	next
	ADD	SI,1
	CMP	SI,5
	JAE	error
	; Reset drive
	MOV	AH,0x00
	MOV	DL,0x00
	INT	0x13
	JMP	retry

next:
	MOV	AX,ES
	ADD	AX,0x020	; 0x20 = 512byte(=1 sector)/16
	MOV	ES,AX
	ADD	CL,1
	CMP	CL,18
	JBE	readloop

putloop:
	MOV	AL,[SI]
	ADD	SI,1
	CMP	AL,0
	JE	fin
	MOV	AH,0x0e	; print a char function
	MOV	BX,15	; color code
	INT	0x10	; invoke BIOS function
	JMP	putloop

fin:
	HLT
	JMP	fin

error:
	MOV	SI,msg

msg:
	DB	0x0a,0x0a
	DB	"hello, world"
	DB	0x0a
	DB	0

RESB	0x7dfe-0x7c00-($-$$)
DB	0x55,0xaa
