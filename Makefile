# VoxyOS Makefile

.PHONY: all clean run

all: disk.img

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel_entry.o: kernel_entry.asm
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o

kernel.o: kernel.c
	gcc -m32 -ffreestanding -c kernel.c -o kernel.o -nostdlib -fno-pie -O2

kernel.bin: kernel_entry.o kernel.o
	ld -m elf_i386 -Ttext 0x1000 --oformat binary -e _start -o kernel.bin kernel_entry.o kernel.o

disk.img: boot.bin kernel.bin
	dd if=/dev/zero of=disk.img bs=512 count=2048 2>/dev/null
	dd if=boot.bin of=disk.img bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=kernel.bin of=disk.img bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "VoxyOS disk image created successfully"

run: disk.img
	qemu-system-x86_64 -drive format=raw,file=disk.img

clean:
	rm -f *.o *.bin disk.img

debug: disk.img
	qemu-system-x86_64 -drive format=raw,file=disk.img -monitor stdio
