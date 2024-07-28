ASM := helloos.asm
IMG := helloos.img

all: run
build: $(IMG)

.PHONY: run
run: $(IMG)
	# qemu-system-x86_64 -hda $(IMG)
	qemu-system-i386 $(IMG)

$(IMG): $(ASM)
	nasm $(ASM) -o $(IMG)

.PHONY: clean
clean:
	rm $(IMG)
