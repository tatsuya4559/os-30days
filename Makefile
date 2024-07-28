all: run

ipl.bin: ipl.asm
	nasm $< -o $@ -l ipl.lst

.PHONY: run
run: ipl.bin
	qemu-system-i386 $<

.PHONY: clean
clean:
	rm $(IMG)
