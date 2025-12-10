# VoxyOS v0.1

A minimal 32-bit protected mode operating system built from scratch in C and x86 assembly.

![VoxyOS Boot Screen](screenshot.png)

## Features

- **Bootloader**: Custom BIOS bootloader that loads kernel from disk
- **Protected Mode**: Full 32-bit protected mode with GDT setup
- **Filesystem**: RAM disk with file create/read/delete operations
- **Text Editor**: Full-featured text editor with save/load capability
- **Game**: Interactive number guessing game
- **Shell**: Command-line interface with colored output
- **VGA Output**: Direct VGA text mode manipulation with 16-color support

## Commands

- `help` - Show available commands
- `clear` - Clear screen
- `about` - About VoxyOS
- `edit <filename>` - Open text editor
- `cat <filename>` - Display file contents
- `rm <filename>` - Delete file
- `ls` - List files in RAM disk
- `game` - Play number guessing game

## Building from Source

### Prerequisites
- WSL2 or Linux environment
- gcc (with 32-bit support)
- nasm
- QEMU

### Build Instructions
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install -y gcc nasm qemu-system-x86 binutils

# Build bootloader
nasm -f bin boot.asm -o boot.bin

# Build kernel entry
nasm -f elf32 kernel_entry.asm -o kernel_entry.o

# Compile kernel
gcc -m32 -ffreestanding -c kernel.c -o kernel.o -nostdlib -fno-pie -O2

# Link kernel
ld -m elf_i386 -Ttext 0x1000 --oformat binary -e _start -o kernel.bin kernel_entry.o kernel.o

# Create disk image
dd if=/dev/zero of=disk.img bs=512 count=2048
dd if=boot.bin of=disk.img bs=512 count=1 conv=notrunc
dd if=kernel.bin of=disk.img bs=512 seek=1 conv=notrunc

# Run in QEMU
qemu-system-x86_64 -drive format=raw,file=disk.img
```

Or use the provided Makefile:
```bash
make clean
make
make run
```

## Project Structure
```
voxyos/
├── boot.asm          # BIOS bootloader (stage 1)
├── kernel_entry.asm  # Kernel entry point (16→32 bit transition)
├── kernel.c          # Main kernel code
├── disk.img          # Bootable disk image
├── Makefile          # Build automation
└── README.md         # This file
```

## Technical Details

### Boot Process
1. BIOS loads 512-byte boot sector at `0x7C00`
2. Bootloader loads kernel (33 sectors) from disk to `0x1000`
3. Kernel entry sets up GDT and switches to protected mode
4. Jumps to C kernel at `kernel_main()`

### Memory Layout
- `0x0000-0x7BFF`: Available
- `0x7C00-0x7DFF`: Boot sector
- `0x1000-0x????`: Kernel code
- `0x90000`: Stack
- `0xB8000`: VGA text buffer

### Filesystem
- RAM-based (non-persistent)
- 16 file slots
- 1KB max file size
- Simple flat structure

## Known Limitations

- Files stored in RAM only (lost on reboot)
- No interrupt handling (polled I/O only)
- No multitasking
- Limited to VGA text mode (80x25)
- No disk persistence (would need ATA driver)

## Future Enhancements

- [ ] Real disk I/O with ATA PIO driver
- [ ] Interrupt handling (IDT, IRQs)
- [ ] Virtual memory / paging
- [ ] More games and applications
- [ ] Graphics mode support
- [ ] Network stack

## License

MIT License - See LICENSE file for details

## Author

Built as a college project to learn OS development fundamentals.

## Acknowledgments

- OSDev Wiki
- Writing a Simple Operating System from Scratch by Nick Blundell
- The Little OS Book
