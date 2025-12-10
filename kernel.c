// VoxyOS v0.1 - Final polished version

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

struct file {
    char filename[FILENAME_LEN];
    char data[MAX_FILE_SIZE];
    int size;
    int used;
};

static struct file ramdisk[MAX_FILES];
static unsigned short* vga = (unsigned short*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static char cmd_buffer[CMD_BUFFER_SIZE];
static int cmd_len = 0;
static unsigned char current_color = (COLOR_BLACK << 4) | COLOR_LIGHT_GRAY;

static char editor_buffer[EDITOR_BUFFER_SIZE];
static int editor_len = 0;
static int editor_active = 0;
static char current_filename[FILENAME_LEN];
static unsigned int rand_seed = 12345;

unsigned int rand(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768;
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
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
    int file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (ramdisk[i].used) file_count++;
    }
    
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
    
    const char* suffix = " | Type 'help' ";
    while (*suffix && x < VGA_WIDTH) {
        vga[bar_y * VGA_WIDTH + x] = (bar_color << 8) | *suffix;
        suffix++;
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

int fs_find_file(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (ramdisk[i].used && strcmp(ramdisk[i].filename, filename) == 0) {
            return i;
        }
    }
    return -1;
}

int fs_save_file(const char* filename, const char* data, int size) {
    if (size > MAX_FILE_SIZE) return -1;
    
    int idx = fs_find_file(filename);
    
    if (idx == -1) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (!ramdisk[i].used) {
                idx = i;
                break;
            }
        }
    }
    
    if (idx == -1) return -2;
    
    strcpy(ramdisk[idx].filename, filename);
    memcpy(ramdisk[idx].data, data, size);
    ramdisk[idx].size = size;
    ramdisk[idx].used = 1;
    
    return 0;
}

int fs_load_file(const char* filename, char* buffer, int max_size) {
    int idx = fs_find_file(filename);
    if (idx == -1) return -1;
    
    int copy_size = ramdisk[idx].size;
    if (copy_size > max_size) copy_size = max_size;
    
    memcpy(buffer, ramdisk[idx].data, copy_size);
    return copy_size;
}

int fs_delete_file(const char* filename) {
    int idx = fs_find_file(filename);
    if (idx == -1) return -1;
    
    ramdisk[idx].used = 0;
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
        
        int loaded = fs_load_file(current_filename, editor_buffer, EDITOR_BUFFER_SIZE);
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
                        int result = fs_save_file(current_filename, editor_buffer, editor_len);
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
}

void cmd_clear(void) {
    clear_screen();
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
    int count = 0;
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print_string("Files in RAM disk:\n");
    reset_color();
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (ramdisk[i].used) {
            set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
            print_string("  * ");
            reset_color();
            print_string(ramdisk[i].filename);
            set_color(COLOR_DARK_GRAY, COLOR_BLACK);
            print_string("  (");
            print_number(ramdisk[i].size);
            print_string(" bytes)\n");
            reset_color();
            count++;
        }
    }
    
    if (count == 0) {
        set_color(COLOR_DARK_GRAY, COLOR_BLACK);
        print_string("  (empty)\n");
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
    int size = fs_load_file(filename, buffer, MAX_FILE_SIZE);
    
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
    
    int result = fs_delete_file(filename);
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
    } else {
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
    for (int i = 0; i < MAX_FILES; i++) {
        ramdisk[i].used = 0;
    }
    
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
    
    draw_status_bar();
    
    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    print_string("> ");
    reset_color();
    
    cmd_len = 0;
    current_filename[0] = '\0';
    
    while (1) {
        unsigned char status = inb(KEYBOARD_STATUS_PORT);
        if (status & 0x01) {
            unsigned char scancode = inb(KEYBOARD_DATA_PORT);
            
            if (!(scancode & 0x80)) {
                char c = scancode_to_ascii(scancode);
                
                if (c == '\n') {
                    print_char('\n');
                    execute_command();
                    draw_status_bar();
                    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
                    print_string("> ");
                    reset_color();
                    cmd_len = 0;
                } else if (c == '\b') {
                    if (cmd_len > 0) {
                        cmd_len--;
                        print_char('\b');
                    }
                } else if (c && cmd_len < CMD_BUFFER_SIZE - 1) {
                    cmd_buffer[cmd_len++] = c;
                    print_char(c);
                }
            }
        }
    }
}
