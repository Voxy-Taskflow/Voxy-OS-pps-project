// bootanim.c - Boot animation sequences

extern void print_string(const char* str);
extern void print_char(char c);
extern void set_color(unsigned char fg, unsigned char bg);
extern void reset_color(void);
extern void beep(unsigned int freq, unsigned int dur);
extern unsigned int tick_count;

static void delay_ticks(unsigned int ticks) {
    unsigned int start = tick_count;
    while ((tick_count - start) < ticks) {
        __asm__ volatile("hlt");
    }
}
// Simple delay
static void delay(unsigned int count) {
    for (unsigned int i = 0; i < count * 10000000; i++) {  // 10x longer
        __asm__ volatile("nop");
    }
}

// Progress bar
void draw_progress_bar(int percent) {
    set_color(0xB, 0x0);  // Light cyan
    print_string("[");
    
    int filled = percent / 5;  // 20 chars total = 100%
    
    for (int i = 0; i < 20; i++) {
        if (i < filled) {
            set_color(0xA, 0x0);  // Light green
            print_char(219);  // Full block █
        } else {
            set_color(0x8, 0x0);  // Dark gray
            print_char(176);  // Light shade ░
        }
    }
    
    set_color(0xB, 0x0);
    print_string("] ");
    
    set_color(0xE, 0x0);  // Yellow
    if (percent < 10) print_char(' ');
    if (percent < 100) print_char(' ');
    
    // Print percentage
    if (percent >= 100) {
        print_string("100");
    } else if (percent >= 10) {
        print_char('0' + (percent / 10));
        print_char('0' + (percent % 10));
    } else {
        print_char('0' + percent);
    }
    print_char('%');
    reset_color();
}

// Animated boot sequence
void animate_boot(void) {
    set_color(0xB, 0x0);  // Light cyan
    print_string("\n  [BOOT] VoxyOS Bootloader v0.1\n\n");
    reset_color();
    
    // Step 1: Hardware detection
    set_color(0x7, 0x0);
    print_string("  Detecting hardware...      ");
    reset_color();
    delay_ticks(20);
    set_color(0xA, 0x0);
    print_string("[OK]\n");
    reset_color();
    beep(800, 50);
    
    // Step 2: Loading kernel
    print_string("  Loading kernel image...    ");
    delay_ticks(15);
    draw_progress_bar(0);
    
    for (int i = 0; i <= 100; i += 10) {
        delay_ticks(3);
        // Move cursor back
        for (int j = 0; j < 28; j++) {
            print_char('\b');
        }
        draw_progress_bar(i);
        if (i % 20 == 0) {
            beep(600 + i * 2, 30);
        }
    }
    print_char('\n');
    
    // Step 3: Initializing subsystems
    print_string("  Initializing interrupts... ");
    delay_ticks(10);
    set_color(0xA, 0x0);
    print_string("[OK]\n");
    reset_color();
    beep(900, 50);
    
    print_string("  Mounting filesystem...     ");
    delay_ticks(10);
    set_color(0xA, 0x0);
    print_string("[OK]\n");
    reset_color();
    beep(1000, 50);
    
    // Step 4: Ready
    print_char('\n');
    set_color(0xE, 0x0);  // Yellow
    print_string("  System ready. Welcome to VoxyOS!\n\n");
    reset_color();
    
    // Success chime
    beep(523, 80);
    delay_ticks(5);
    beep(659, 80);
    delay_ticks(10);
    beep(784, 120);
    
    delay(15);
}
