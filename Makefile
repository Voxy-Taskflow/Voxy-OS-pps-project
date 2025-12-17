# VoxyOS Makefile with Interrupts + Timer

.PHONY: all clean run

all: disk.img

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel_entry.o: kernel_entry.asm
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o

idt_load.o: idt_load.asm
	nasm -f elf32 idt_load.asm -o idt_load.o

isr_keyboard.o: isr_keyboard.asm
	nasm -f elf32 isr_keyboard.asm -o isr_keyboard.o

isr_timer.o: isr_timer.asm
	nasm -f elf32 isr_timer.asm -o isr_timer.o

idt.o: idt.c idt.h
	gcc -m32 -ffreestanding -c idt.c -o idt.o -nostdlib -fno-pie -O2

pic.o: pic.c
	gcc -m32 -ffreestanding -c pic.c -o pic.o -nostdlib -fno-pie -O2

keyboard.o: keyboard.c
	gcc -m32 -ffreestanding -c keyboard.c -o keyboard.o -nostdlib -fno-pie -O2

timer.o: timer.c
	gcc -m32 -ffreestanding -c timer.c -o timer.o -nostdlib -fno-pie -O2

kernel.o: kernel.c idt.h
	gcc -m32 -ffreestanding -c kernel.c -o kernel.o -nostdlib -fno-pie -O2

speaker.o: speaker.c
	gcc -m32 -ffreestanding -c speaker.c -o speaker.o -nostdlib -fno-pie -O2

ata.o: ata.c ata.h
	gcc -m32 -ffreestanding -c ata.c -o ata.o -nostdlib -fno-pie -O2

kernel.bin: kernel_entry.o kernel.o idt.o idt_load.o isr_keyboard.o isr_timer.o keyboard.o timer.o pic.o speaker.o ata.o
	ld -m elf_i386 -Ttext 0x1000 --oformat binary -e _start -o kernel.bin kernel_entry.o kernel.o idt.o idt_load.o isr_keyboard.o isr_timer.o keyboard.o timer.o pic.o speaker.o ata.o

disk.img: boot.bin kernel.bin
	dd if=/dev/zero of=disk.img bs=512 count=2048 2>/dev/null
	dd if=boot.bin of=disk.img bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=kernel.bin of=disk.img bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "VoxyOS built successfully"
	@echo "Sectors: $$((($$(stat -c%s kernel.bin) + 511) / 512))"

run: disk.img
	qemu-system-x86_64 -drive format=raw,file=disk.img

clean:
	rm -f *.o *.bin disk.img
