CC := gcc
# -march=i486 生成するコードをi486（Intel 80486）アーキテクチャ向けに最適化します。このオプションにより、特定のCPU命令セットを利用し、パフォーマンスを向上させることができます。
# -m32 32ビットコードを生成します。これにより、32ビットアーキテクチャ用のバイナリが生成されます。通常、x86プロセッサ向けの32ビットコードを生成するために使用されます。
# -fno-pic Position Independent Code（PIC）を生成しないように指示します。PICは共有ライブラリの再配置をサポートするために使用されますが、このオプションにより通常の（位置依存の）コードが生成されます。
# -nostdlib  標準ライブラリをリンクしないように指示します。カーネルやブートローダなどの特殊なプログラムを作成する際に使用されます。このオプションを使うと、標準ライブラリ関数（例：printfなど）が利用できなくなります。
CFLAGS := -march=i486 -m32 -fno-pic -nostdlib

C_SRC := iolib.c graphic.c dsctbl.c int.c fifo.c keyboard.c mouse.c memory.c
C_OBJ := $(patsubst %.c,%.o,$(C_SRC))

all: run

asmhead.bin: asmhead.asm
	nasm $< -o $@ -l asmhead.lst

font.h: makefont.py hankaku.txt
	python makefont.py

nasmfunc.o: nasmfunc.asm
	nasm -g -f elf $< -o $@ -l nasmfunc.lst

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bootpack.hrb: bootpack.c font.h os.ld nasmfunc.o $(C_OBJ)
	# -T os.ld `os.ldというリンクスクリプトを使用してリンクします。リンクスクリプトは、生成されるバイナリのレイアウトやメモリマップを指定するために使用されます。
	$(CC) $(CFLAGS) -T os.ld $(filter %.o,$^) bootpack.c -o $@

haribote.sys: asmhead.bin bootpack.hrb
	cat $^ > $@

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
	rm -f *.lst *.bin *.sys *.img *.hrb *.o
