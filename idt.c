// idt.c - Interrupt Descriptor Table implementation

#include "idt.h"

// IDT array and pointer
struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

// External assembly function that loads IDT
extern void idt_load(unsigned int);

// Set an IDT gate
void idt_set_gate(unsigned char num, unsigned int base, unsigned short selector, unsigned char flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

// Initialize IDT
void idt_init(void) {
    // Set up IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (unsigned int)&idt;
    
    // Clear IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Load IDT
    idt_load((unsigned int)&idtp);
}
