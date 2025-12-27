// ui.c - UI drawing functions with box-drawing characters

extern void print_char(char c);
extern void print_string(const char* str);
extern void set_color(unsigned char fg, unsigned char bg);
extern void reset_color(void);
extern unsigned short* vga;
extern int cursor_x;
extern int cursor_y;

#define VGA_WIDTH 80

// Box drawing characters (CP437 encoding)
#define BOX_TL 218  // ┌ top-left
#define BOX_TR 191  // ┐ top-right
#define BOX_BL 192  // └ bottom-left
#define BOX_BR 217  // ┘ bottom-right
#define BOX_H  196  // ─ horizontal
#define BOX_V  179  // │ vertical
#define BOX_VR 195  // ├ vertical-right
#define BOX_VL 180  // ┤ vertical-left
#define BLOCK_FULL 219  // █
#define BLOCK_SHADE 176 // ░

// Draw a box at position (x, y) with given width and height
void draw_box(int x, int y, int width, int height, unsigned char fg, unsigned char bg) {
    unsigned char color = (bg << 4) | fg;
    
    // Top border
    vga[y * VGA_WIDTH + x] = (color << 8) | BOX_TL;
    for (int i = 1; i < width - 1; i++) {
        vga[y * VGA_WIDTH + x + i] = (color << 8) | BOX_H;
    }
    vga[y * VGA_WIDTH + x + width - 1] = (color << 8) | BOX_TR;
    
    // Sides
    for (int row = 1; row < height - 1; row++) {
        vga[(y + row) * VGA_WIDTH + x] = (color << 8) | BOX_V;
        // Fill interior
        for (int col = 1; col < width - 1; col++) {
            vga[(y + row) * VGA_WIDTH + x + col] = (color << 8) | ' ';
        }
        vga[(y + row) * VGA_WIDTH + x + width - 1] = (color << 8) | BOX_V;
    }
    
    // Bottom border
    vga[(y + height - 1) * VGA_WIDTH + x] = (color << 8) | BOX_BL;
    for (int i = 1; i < width - 1; i++) {
        vga[(y + height - 1) * VGA_WIDTH + x + i] = (color << 8) | BOX_H;
    }
    vga[(y + height - 1) * VGA_WIDTH + x + width - 1] = (color << 8) | BOX_BR;
}

// Draw text inside a box
void draw_text_in_box(int x, int y, const char* text, unsigned char fg, unsigned char bg) {
    unsigned char color = (bg << 4) | fg;
    int i = 0;
    while (text[i] != '\0') {
        vga[y * VGA_WIDTH + x + i] = (color << 8) | text[i];
        i++;
    }
}

// Draw a title bar
void draw_title_bar(int x, int y, int width, const char* title, unsigned char fg, unsigned char bg) {
    unsigned char color = (bg << 4) | fg;
    
    // Top border with title
    vga[y * VGA_WIDTH + x] = (color << 8) | BOX_TL;
    
    int title_len = 0;
    while (title[title_len]) title_len++;
    
    int padding = (width - title_len - 4) / 2;
    
    for (int i = 1; i < padding; i++) {
        vga[y * VGA_WIDTH + x + i] = (color << 8) | BOX_H;
    }
    
    vga[y * VGA_WIDTH + x + padding] = (color << 8) | ' ';
    for (int i = 0; i < title_len; i++) {
        vga[y * VGA_WIDTH + x + padding + 1 + i] = (color << 8) | title[i];
    }
    vga[y * VGA_WIDTH + x + padding + title_len + 1] = (color << 8) | ' ';
    
    for (int i = padding + title_len + 2; i < width - 1; i++) {
        vga[y * VGA_WIDTH + x + i] = (color << 8) | BOX_H;
    }
    
    vga[y * VGA_WIDTH + x + width - 1] = (color << 8) | BOX_TR;
}

// Draw a centered message box
void show_message_box(const char* title, const char* message, unsigned char fg, unsigned char bg) {
    int box_width = 50;
    int box_height = 7;
    int x = (VGA_WIDTH - box_width) / 2;
    int y = 8;
    
    // Draw shadow (optional, looks cool)
    for (int row = 1; row < box_height; row++) {
        for (int col = 2; col < box_width + 2; col++) {
            vga[(y + row) * VGA_WIDTH + x + col] = (0x00 << 8) | ' ';
        }
    }
    
    draw_box(x, y, box_width, box_height, fg, bg);
    draw_title_bar(x, y, box_width, title, 0xF, bg);
    
    // Center the message
    int msg_len = 0;
    while (message[msg_len]) msg_len++;
    int msg_x = x + (box_width - msg_len) / 2;
    
    draw_text_in_box(msg_x, y + 3, message, 0xF, bg);
    draw_text_in_box(x + (box_width - 22) / 2, y + 5, "Press any key to close", 0x8, bg);
}
