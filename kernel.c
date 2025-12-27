// VoxyOS v0.1 - Final polished version
#include "idt.h"
extern void isr_keyboard(void);
extern void isr_keyboard(void);
extern void pic_init(void);
extern unsigned int interrupt_count;
extern void isr_timer(void);
extern void get_time_string(char* buffer);
extern unsigned int get_uptime_seconds(void);
extern int status_bar_needs_update;
extern void play_startup_sound(void);
extern void play_error_sound(void);
extern void play_success_sound(void);
extern void beep(unsigned int freq, unsigned int dur);  
extern int ata_read_sector(unsigned int lba, unsigned char* buffer);
extern int ata_write_sector(unsigned int lba, unsigned char* buffer);
extern int diskfs_init(void);                                          
extern int diskfs_save_file(const char* filename, const char* data, int size);  
extern int diskfs_load_file(const char* filename, char* buffer, int max_size);  
extern int diskfs_delete_file(const char* filename);                   
extern int diskfs_count_files(void);                                   
extern int diskfs_get_file_info(int index, char* name, int* size); 

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define CMD_BUFFER_SIZE 256
#define EDITOR_BUFFER_SIZE 2048

#define MAX_FILES 16
#define FILENAME_LEN 16
#define MAX_FILE_SIZE 1024

#define COLOR_BLACK 0x0
#define COLOR_BLUE 0x1
#define COLOR_GREEN 0x2
#define COLOR_CYAN 0x3
#define COLOR_RED 0x4
#define COLOR_MAGENTA 0x5
#define COLOR_BROWN 0x6
#define COLOR_LIGHT_GRAY 0x7
#define COLOR_DARK_GRAY 0x8
#define COLOR_LIGHT_BLUE 0x9
#define COLOR_LIGHT_GREEN 0xA
#define COLOR_LIGHT_CYAN 0xB
#define COLOR_LIGHT_RED 0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW 0xE
#define COLOR_WHITE 0xF



static unsigned short* vga = (unsigned short*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
char cmd_buffer[CMD_BUFFER_SIZE];
int cmd_len = 0;
static unsigned char current_color = (COLOR_BLACK << 4) | COLOR_LIGHT_GRAY;

static char editor_buffer[EDITOR_BUFFER_SIZE];
static int editor_len = 0;
static int editor_active = 0;
static char current_filename[FILENAME_LEN];
static unsigned int rand_seed = 0;


unsigned int rand(void) {
    // Seed with timer tick if not initialized
    if (rand_seed == 0) {
        extern unsigned int tick_count;
        rand_seed = tick_count + 12345;  // Add offset to avoid 0
    }
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768;
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outb(unsigned short port, unsigned char data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

void set_color(unsigned char fg, unsigned char bg) {
    current_color = (bg << 4) | fg;
}

void reset_color(void) {
    current_color = (COLOR_BLACK << 4) | COLOR_LIGHT_GRAY;
}

void clear_screen(void) {
    unsigned char bg_color = (COLOR_BLACK << 4) | COLOR_LIGHT_GRAY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (bg_color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

void draw_status_bar(void) {
    int file_count = diskfs_count_files();
    
    int bar_y = VGA_HEIGHT - 1;
    unsigned char bar_color = (COLOR_CYAN << 4) | COLOR_BLACK;
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga[bar_y * VGA_WIDTH + x] = (bar_color << 8) | ' ';
    }
    
    const char* prefix = " VoxyOS v0.1 | Files: ";
    int x = 0;
    
    while (*prefix && x < VGA_WIDTH) {
        vga[bar_y * VGA_WIDTH + x] = (bar_color << 8) | *prefix;
        prefix++;
        x++;
    }
    
    char num[8];
    int i = 0;
    int n = file_count;
    if (n == 0) {
        num[i++] = '0';
    } else {
        while (n > 0) {
            num[i++] = '0' + (n % 10);
            n /= 10;
        }
    }
    while (i > 0 && x < VGA_WIDTH) {
        vga[bar_y * VGA_WIDTH + x] = (bar_color << 8) | num[--i];
        x++;
    }
    
    const char* suffix = " | ";
    while (*suffix && x < VGA_WIDTH) {
        vga[bar_y * VGA_WIDTH + x] = (bar_color << 8) | *suffix;
        suffix++;
        x++;
    }
    
    // Add clock
    char time_str[16];
    get_time_string(time_str);
    int j = 0;
    while (time_str[j] && x < VGA_WIDTH) {
        vga[bar_y * VGA_WIDTH + x] = (bar_color << 8) | time_str[j];
        j++;
        x++;
    }
    
    const char* suffix2 = " | Type 'help' ";
    while (*suffix2 && x < VGA_WIDTH) {
        vga[bar_y * VGA_WIDTH + x] = (bar_color << 8) | *suffix2;
        suffix2++;
        x++;
    }
}
void scroll(void) {
    for (int i = 0; i < (VGA_HEIGHT - 2) * VGA_WIDTH; i++) {
        vga[i] = vga[i + VGA_WIDTH];
    }
    unsigned char clear_color = (COLOR_BLACK << 4) | COLOR_LIGHT_GRAY;
    for (int i = (VGA_HEIGHT - 2) * VGA_WIDTH; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga[i] = (clear_color << 8) | ' ';
    }
    cursor_y = VGA_HEIGHT - 2;
}

void print_char(char c) {
    if (cursor_y >= VGA_HEIGHT - 1) {
        cursor_y = VGA_HEIGHT - 2;
    }
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga[cursor_y * VGA_WIDTH + cursor_x] = (current_color << 8) | ' ';
        }
        return;
    } else {
        vga[cursor_y * VGA_WIDTH + cursor_x] = (current_color << 8) | c;
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT - 1) {
        scroll();
    }
}

void print_string(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

void print_number(int num) {
    char buffer[12];
    int i = 0;
    
    if (num == 0) {
        print_char('0');
        return;
    }
    
    if (num < 0) {
        print_char('-');
        num = -num;
    }
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        print_char(buffer[--i]);
    }
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

void memcpy(void* dest, const void* src, int n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (int i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

int atoi(const char* str) {
    int result = 0;
    
    while (*str == ' ') str++;
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result;
}

char scancode_to_ascii(unsigned char scancode) {
    static const char sc_ascii[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };
    
    if (scancode < sizeof(sc_ascii)) {
        return sc_ascii[scancode];
    }
    return 0;
}

void game_run(void) {
    clear_screen();
    set_color(COLOR_YELLOW, COLOR_BLACK);
    print_string("===============================================\n");
    print_string("       GUESS THE NUMBER GAME\n");
    print_string("===============================================\n");
    reset_color();
    print_string("I'm thinking of a number between 1-100\n");
    print_string("Press ESC to quit\n\n");
    
    int secret = (rand() % 100) + 1;
    int attempts = 0;
    char guess_buffer[16];
    int guess_len = 0;
    
    while (1) {
        set_color(COLOR_CYAN, COLOR_BLACK);
        print_string("Your guess: ");
        reset_color();
        guess_len = 0;
        
        while (1) {
            unsigned char status = inb(KEYBOARD_STATUS_PORT);
            if (status & 0x01) {
                unsigned char scancode = inb(KEYBOARD_DATA_PORT);
                
                if (!(scancode & 0x80)) {
                    if (scancode == 0x01) {
                        set_color(COLOR_RED, COLOR_BLACK);
                        print_string("\n\nGame quit!\n\n");
                        reset_color();
                        draw_status_bar();
                        return;
                    }
                    
                    char c = scancode_to_ascii(scancode);
                    
                    if (c == '\n') {
                        guess_buffer[guess_len] = '\0';
                        print_char('\n');
                        break;
                    } else if (c == '\b') {
                        if (guess_len > 0) {
                            guess_len--;
                            print_char('\b');
                        }
                    } else if (c >= '0' && c <= '9' && guess_len < 15) {
                        guess_buffer[guess_len++] = c;
                        print_char(c);
                    }
                }
            }
        }
        
        if (guess_len == 0) continue;
        
        int guess = atoi(guess_buffer);
        attempts++;
        
        if (guess == secret) {
            play_success_sound();
            set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
            print_string("*** CORRECT! *** You won in ");
            print_number(attempts);
            print_string(" attempts!\n");
            reset_color();
            print_string("Press any key to continue...\n");
            
            while (1) {
                unsigned char status = inb(KEYBOARD_STATUS_PORT);
                if (status & 0x01) {
                    inb(KEYBOARD_DATA_PORT);
                    break;
                }
            }
            draw_status_bar();
            return;
        } else if (guess < secret) {
            set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
            print_string("Too low! Try higher.\n");
            reset_color();
        } else {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            print_string("Too high! Try lower.\n");
            reset_color();
        }
    }
}

void editor_run(void) {
    clear_screen();
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print_string("===============================================\n");
    print_string("         VoxyOS TEXT EDITOR\n");
    print_string("===============================================\n");
    reset_color();
    
    if (strlen(current_filename) > 0) {
        set_color(COLOR_YELLOW, COLOR_BLACK);
        print_string("File: ");
        print_string(current_filename);
        print_char('\n');
        reset_color();
        
        int loaded = diskfs_load_file(current_filename, editor_buffer, EDITOR_BUFFER_SIZE);
        if (loaded > 0) {
            editor_len = loaded;
            set_color(COLOR_GREEN, COLOR_BLACK);
            print_string("Loaded ");
            print_number(loaded);
            print_string(" bytes\n");
            reset_color();
        }
    }
    
    print_string("Press ESC to save and exit\n");
    print_string("-----------------------------------------------\n");
    
    for (int i = 0; i < editor_len; i++) {
        print_char(editor_buffer[i]);
    }
    
    editor_active = 1;
    
    while (editor_active) {
        unsigned char status = inb(KEYBOARD_STATUS_PORT);
        if (status & 0x01) {
            unsigned char scancode = inb(KEYBOARD_DATA_PORT);
            
            if (!(scancode & 0x80)) {
                if (scancode == 0x01) {
                    editor_active = 0;
                    
                    if (strlen(current_filename) > 0) {
                        int result = diskfs_save_file(current_filename, editor_buffer, editor_len);
                        clear_screen();
                        if (result == 0) {
                            set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
                            print_string("[OK] Saved: ");
                            print_string(current_filename);
                            print_string(" (");
                            print_number(editor_len);
                            print_string(" bytes)\n");
                            reset_color();
                        } else {
                            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
                            print_string("[ERROR] Failed to save\n");
                            reset_color();
                        }
                    } else {
                        clear_screen();
                        set_color(COLOR_YELLOW, COLOR_BLACK);
                        print_string("[WARNING] No filename - not saved\n");
                        reset_color();
                    }
                    print_char('\n');
                    draw_status_bar();
                    break;
                }
                
                char c = scancode_to_ascii(scancode);
                
                if (c == '\n') {
                    if (editor_len < EDITOR_BUFFER_SIZE - 1) {
                        editor_buffer[editor_len++] = '\n';
                        print_char('\n');
                    }
                } else if (c == '\b') {
                    if (editor_len > 0) {
                        editor_len--;
                        print_char('\b');
                    }
                } else if (c && editor_len < EDITOR_BUFFER_SIZE - 1) {
                    editor_buffer[editor_len++] = c;
                    print_char(c);
                }
            }
        }
    }
}

void cmd_help(void) {
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print_string("=== VoxyOS Commands ===\n");
    reset_color();
    print_string("  help         Show this help\n");
    print_string("  clear        Clear screen\n");
    print_string("  about        About VoxyOS\n");
    print_string("  edit <file>  Text editor\n");
    print_string("  cat <file>   Display file\n");
    print_string("  rm <file>    Delete file\n");
    print_string("  ls           List files\n");
    print_string("  game         Number guessing game\n");
    print_string("  stats        Show interrupt statistics\n");
    print_string("  disktest     Test ATA disk read\n");
    print_string("  diskwrite    Test ATA disk write\n");  
}

void cmd_clear(void) {
    clear_screen();
    beep(1000, 50);
    draw_status_bar();
}

void cmd_about(void) {
    set_color(COLOR_LIGHT_MAGENTA, COLOR_BLACK);
    print_string("===============================================\n");
    print_string("              VoxyOS v0.1\n");
    print_string("===============================================\n");
    reset_color();
    print_string("32-bit Protected Mode Operating System\n");
    print_string("Built from scratch for college project\n\n");
    set_color(COLOR_YELLOW, COLOR_BLACK);
    print_string("Features:\n");
    reset_color();
    print_string("  * RAM disk filesystem\n");
    print_string("  * Text editor with save/load\n");
    print_string("  * Interactive game\n");
    print_string("  * Command shell with colors\n");
}

void cmd_ls(void) {
    int count = diskfs_count_files();
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print_string("Files on disk:\n");
    reset_color();
    
    if (count == 0) {
        set_color(COLOR_DARK_GRAY, COLOR_BLACK);
        print_string("  (empty)\n");
        reset_color();
        return;
    }
    
    char filename[FILENAME_LEN];
    int size;
    
    for (int i = 0; i < count; i++) {
        if (diskfs_get_file_info(i, filename, &size) == 0) {
            set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
            print_string("  * ");
            reset_color();
            print_string(filename);
            set_color(COLOR_DARK_GRAY, COLOR_BLACK);
            print_string("  (");
            print_number(size);
            print_string(" bytes)\n");
            reset_color();
        }
    }
}
void cmd_stats(void) {
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print_string("=== System Statistics ===\n");
    reset_color();
    print_string("Keyboard interrupts: ");
    print_number(interrupt_count);
    print_char('\n');
    print_string("Uptime: ");
    print_number(get_uptime_seconds());
    print_string(" seconds\n");
}
void cmd_diskwrite(void) {
    set_color(COLOR_YELLOW, COLOR_BLACK);
    print_string("Testing ATA disk write...\n");
    reset_color();
    
    unsigned char write_buffer[512];
    unsigned char read_buffer[512];
    
    // Fill buffer with test pattern
    for (int i = 0; i < 512; i++) {
        write_buffer[i] = i & 0xFF;  // 0-255 repeating pattern
    }
    
    // Write to sector 100 (safe test location)
    print_string("Writing test pattern to sector 100...\n");
    int result = ata_write_sector(100, write_buffer);
    
    if (result != 0) {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        print_string("[ERROR] Write failed!\n");
        reset_color();
        return;
    }
    
    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    print_string("[OK] Write successful!\n");
    reset_color();
    
    // Read it back to verify
    print_string("Reading back sector 100...\n");
    result = ata_read_sector(100, read_buffer);
    
    if (result != 0) {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        print_string("[ERROR] Read failed!\n");
        reset_color();
        return;
    }
    
    // Verify data matches
    int mismatches = 0;
    for (int i = 0; i < 512; i++) {
        if (write_buffer[i] != read_buffer[i]) {
            mismatches++;
        }
    }
    
    if (mismatches == 0) {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        print_string("[OK] Verification passed! Data persists!\n");
        play_success_sound();
        reset_color();
        
        // Show first 64 bytes
        print_string("First 64 bytes:\n");
        for (int i = 0; i < 64; i++) {
            if (i % 16 == 0) print_char('\n');
            char hex[] = "0123456789ABCDEF";
            print_char(hex[(read_buffer[i] >> 4) & 0x0F]);
            print_char(hex[read_buffer[i] & 0x0F]);
            print_char(' ');
        }
        print_char('\n');
    } else {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        print_string("[ERROR] Verification failed! Mismatches: ");
        print_number(mismatches);
        print_char('\n');
        play_error_sound();
        reset_color();
    }
}
void cmd_disktest(void) {
    set_color(COLOR_YELLOW, COLOR_BLACK);
    print_string("Testing ATA disk read...\n");
    reset_color();
    
    unsigned char buffer[512];
    
    // Try to read sector 0 (boot sector)
    print_string("Reading boot sector (LBA 0)...\n");
    int result = ata_read_sector(100, buffer);
    
    if (result == 0) {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        print_string("[OK] Read successful!\n");
        reset_color();
        
        // Show first 64 bytes
        print_string("First 64 bytes:\n");
        for (int i = 0; i < 64; i++) {
            if (i % 16 == 0) print_char('\n');
            
            // Print hex byte
            char hex[] = "0123456789ABCDEF";
            print_char(hex[(buffer[i] >> 4) & 0x0F]);
            print_char(hex[buffer[i] & 0x0F]);
            print_char(' ');
        }
        print_char('\n');
    } else {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        print_string("[ERROR] Disk read failed!\n");
        reset_color();
    }
}

void cmd_cat(const char* filename) {
    if (strlen(filename) == 0) {
        set_color(COLOR_YELLOW, COLOR_BLACK);
        print_string("Usage: cat <filename>\n");
        reset_color();
        return;
    }
    
    char buffer[MAX_FILE_SIZE];
    int size = diskfs_load_file(filename, buffer, MAX_FILE_SIZE);
    
    if (size < 0) {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        print_string("[ERROR] File not found: ");
        print_string(filename);
        print_char('\n');
        reset_color();
        return;
    }
    
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print_string("--- ");
    print_string(filename);
    print_string(" ---\n");
    reset_color();
    
    for (int i = 0; i < size; i++) {
        print_char(buffer[i]);
    }
    print_char('\n');
}

void cmd_rm(const char* filename) {
    if (strlen(filename) == 0) {
        set_color(COLOR_YELLOW, COLOR_BLACK);
        print_string("Usage: rm <filename>\n");
        reset_color();
        return;
    }
    
    int result = diskfs_delete_file(filename);
    if (result == 0) {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        print_string("[OK] Deleted: ");
        print_string(filename);
        print_char('\n');
        reset_color();
        draw_status_bar();
    } else {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        print_string("[ERROR] File not found: ");
        print_string(filename);
        print_char('\n');
        reset_color();
    }
}

void execute_command(void) {
    cmd_buffer[cmd_len] = '\0';
    
    if (cmd_len == 0) return;
    
    char* arg = cmd_buffer;
    while (*arg && *arg != ' ') arg++;
    if (*arg == ' ') {
        *arg = '\0';
        arg++;
        while (*arg == ' ') arg++;
    } else {
        arg = "";
    }
    
    if (strcmp(cmd_buffer, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd_buffer, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(cmd_buffer, "about") == 0) {
        cmd_about();
    } else if (strcmp(cmd_buffer, "edit") == 0) {
        if (strlen(arg) > 0) {
            strcpy(current_filename, arg);
        } else {
            current_filename[0] = '\0';
        }
        editor_len = 0;
        editor_run();
    } else if (strcmp(cmd_buffer, "cat") == 0) {
        cmd_cat(arg);
    } else if (strcmp(cmd_buffer, "rm") == 0) {
        cmd_rm(arg);
    } else if (strcmp(cmd_buffer, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd_buffer, "game") == 0) {
        game_run();
    } else if (strcmp(cmd_buffer, "stats") == 0) {
        cmd_stats();
    }
    else if (strcmp(cmd_buffer, "disktest") == 0)
    {
        cmd_disktest();
    }
    else if (strcmp(cmd_buffer, "diskwrite") == 0) {  
        cmd_diskwrite();
    }
    else {
        play_error_sound(); 
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        print_string("[ERROR] Unknown command: ");
        print_string(cmd_buffer);
        print_char('\n');
        reset_color();
        set_color(COLOR_DARK_GRAY, COLOR_BLACK);
        print_string("Type 'help' for available commands\n");
        reset_color();
    }
}

void kernel_main(void) {
    // Initialize IDT and interrupts
    idt_init();
    pic_init();
    // Initialize disk filesystem
    set_color(COLOR_DARK_GRAY, COLOR_BLACK);
    print_string("[BOOT] Initializing disk filesystem...\n");
    reset_color();
    if (diskfs_init() != 0) {
        set_color(COLOR_YELLOW, COLOR_BLACK);
        print_string("[WARN] Filesystem not found, creating new...\n");
        reset_color();
    }
    idt_set_gate(32, (unsigned int)isr_timer, 0x08, 0x8E);      // IRQ0 = INT 32 (Timer)
    idt_set_gate(33, (unsigned int)isr_keyboard, 0x08, 0x8E);  // IRQ1 = INT 33 (Keyboard)
    __asm__ volatile("sti");
    
    clear_screen();
    
    // Simple, clean ASCII logo
    print_char('\n');
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print_string("        ##     ##  #######  ##     ## ##    ##  #######   ######\n");
    print_string("        ##     ## ##     ##  ##   ##   ##  ##  ##     ## ##    ##\n");
    print_string("        ##     ## ##     ##   ## ##     ####   ##     ## ##\n");
    print_string("        ##     ## ##     ##    ###       ##    ##     ##  ######\n");
    print_string("         ##   ##  ##     ##   ## ##      ##    ##     ##       ##\n");
    print_string("          ## ##   ##     ##  ##   ##     ##    ##     ## ##    ##\n");
    print_string("           ###     #######  ##     ##    ##     #######   ######\n");
    reset_color();
    print_char('\n');
    
    set_color(COLOR_YELLOW, COLOR_BLACK);
    print_string("                   32-bit Protected Mode OS v0.1\n");
    reset_color();
    set_color(COLOR_DARK_GRAY, COLOR_BLACK);
    print_string("                Type 'help' for available commands\n\n");
    reset_color();
    
    play_startup_sound();

    draw_status_bar();
    
    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    print_string("> ");
    reset_color();
    
    cmd_len = 0;
    current_filename[0] = '\0';
    
    while (1) {
        __asm__ volatile("hlt");  // Wait for keyboard interrupt
        
        // Update status bar if timer signals
        if (status_bar_needs_update) {
            status_bar_needs_update = 0;
            draw_status_bar();
        }
        
        // Echo any new characters in buffer
        static int last_cmd_len = 0;
        if (cmd_len > last_cmd_len) {
            // New character added - echo it
            char c = cmd_buffer[cmd_len - 1];
            print_char(c);
            last_cmd_len = cmd_len;
        } else if (cmd_len < last_cmd_len) {
            // Backspace - erase character
            print_char('\b');
            last_cmd_len = cmd_len;
        }
        
        // Check if Enter was pressed
        if (cmd_len > 0 && cmd_buffer[cmd_len - 1] == '\n') {
            cmd_buffer[cmd_len - 1] = '\0';  // Remove newline
            execute_command();
            draw_status_bar();
            set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
            print_string("> ");
            reset_color();
            cmd_len = 0;
            last_cmd_len = 0;
        }
    }
}
