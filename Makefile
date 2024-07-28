ASM := ipl.asm
IMG := ipl.bin
LST := ipl.lst


all: run

$(IMG): $(ASM)
	nasm $< -o $@ -l $(LST)

.PHONY: run
run: $(IMG)
	qemu-system-i386 $<

.PHONY: clean
clean:
	rm $(IMG)
