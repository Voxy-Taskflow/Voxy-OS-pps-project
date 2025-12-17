// keyboard.c - Keyboard interrupt handler

extern unsigned char inb(unsigned short port);
extern char scancode_to_ascii(unsigned char scancode);

extern char cmd_buffer[];
extern int cmd_len;

#define CMD_BUFFER_SIZE 256
#define KEYBOARD_STATUS_PORT 0x64

unsigned int interrupt_count = 0;

void keyboard_handler(void) {
    interrupt_count++;
    
    // Wait for keyboard controller to be ready
    while (inb(KEYBOARD_STATUS_PORT) & 0x02);
    
    unsigned char scancode = inb(0x60);
    
    if (!(scancode & 0x80)) {
        char c = scancode_to_ascii(scancode);
        
        // Only add to buffer, don't print here
        if (c == '\b') {
            if (cmd_len > 0) {
                cmd_len--;
            }
        } else if (c && cmd_len < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_len++] = c;
        }
    }
}