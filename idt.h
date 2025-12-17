// idt.h - Interrupt Descriptor Table definitions

#ifndef IDT_H
#define IDT_H

// IDT entry structure (8 bytes)
struct idt_entry {
    unsigned short offset_low;   // Handler address bits 0-15
    unsigned short selector;     // Code segment selector (0x08)
    unsigned char zero;          // Always 0
    unsigned char flags;         // Type and attributes
    unsigned short offset_high;  // Handler address bits 16-31
} __attribute__((packed));

// IDT pointer structure (6 bytes) for LIDT instruction
struct idt_ptr {
    unsigned short limit;        // Size of IDT - 1
    unsigned int base;           // Address of IDT
} __attribute__((packed));

// IDT with 256 entries
#define IDT_ENTRIES 256

// Initialize IDT
void idt_init(void);

// Set an IDT gate
void idt_set_gate(unsigned char num, unsigned int base, unsigned short selector, unsigned char flags);

#endif