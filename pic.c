// pic.c - PIC (8259) initialization and management

extern void outb(unsigned short port, unsigned char data);
extern unsigned char inb(unsigned short port);

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

// Remap PIC to avoid conflicts with CPU exceptions
void pic_remap(void) {
    // Save masks
    unsigned char mask1 = inb(PIC1_DATA);
    unsigned char mask2 = inb(PIC2_DATA);
    
    // Start initialization
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    // Set vector offsets
    outb(PIC1_DATA, 0x20);  // Master PIC starts at INT 32
    outb(PIC2_DATA, 0x28);  // Slave PIC starts at INT 40
    
    // Tell PICs about cascade
    outb(PIC1_DATA, 4);     // Slave on IRQ2
    outb(PIC2_DATA, 2);
    
    // Set mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    // Unmask IRQ0 (timer) and IRQ1 (keyboard)
    outb(PIC1_DATA, 0xFC);  // 11111100 = unmask IRQ0 and IRQ1
    outb(PIC2_DATA, 0xFF);  // Mask all slave IRQs
}

// Initialize PIC
void pic_init(void) {
    pic_remap();
}
