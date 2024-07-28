all: run

haribote.sys: haribote.asm
	nasm $< -o $@

ipl.bin: ipl.asm
	nasm $< -o $@ -l ipl.lst

haribote.img: ipl.bin haribote.sys
	mformat -f 1440 -C -B ipl.bin -i $@
	mcopy -i $@ haribote.sys ::

.PHONY: run
run: haribote.img
	qemu-system-i386 -fda $<

.PHONY: clean
clean:
	rm ipl.bin haribote.sys haribote.img
