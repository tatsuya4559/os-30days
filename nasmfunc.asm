; vim: tabstop=8 noexpandtab

section .text
	GLOBAL	io_hlt

; void io_hlt(void)
io_hlt:
	HLT
	RET
