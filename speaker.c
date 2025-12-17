// speaker.c - PC Speaker driver

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);

#define PIT_CHANNEL_2 0x42
#define PIT_COMMAND   0x43
#define SPEAKER_PORT  0x61

// Play a tone at given frequency
void speaker_play(unsigned int frequency) {
    if (frequency == 0) return;
    
    // Calculate PIT divisor (1193180 Hz base frequency)
    unsigned int divisor = 1193180 / frequency;
    
    // Set PIT to square wave mode
    outb(PIT_COMMAND, 0xB6);
    
    // Send frequency divisor
    outb(PIT_CHANNEL_2, (unsigned char)(divisor & 0xFF));
    outb(PIT_CHANNEL_2, (unsigned char)((divisor >> 8) & 0xFF));
    
    // Enable speaker
    unsigned char tmp = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, tmp | 0x03);
}

// Stop speaker
void speaker_stop(void) {
    unsigned char tmp = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, tmp & 0xFC);
}

// Simple delay (busy wait)
void speaker_delay(unsigned int ms) {
    // Rough delay - adjust based on CPU speed
    for (unsigned int i = 0; i < ms * 1000; i++) {
        __asm__ volatile("nop");
    }
}

// Play a beep for duration in milliseconds
void beep(unsigned int frequency, unsigned int duration_ms) {
    speaker_play(frequency);
    speaker_delay(duration_ms);
    speaker_stop();
}

// Startup chime
void play_startup_sound(void) {
    beep(523, 100);  // C5
    speaker_delay(50);
    beep(659, 100);  // E5
    speaker_delay(50);
    beep(784, 150);  // G5
}

// Error beep
void play_error_sound(void) {
    beep(200, 100);
    speaker_delay(50);
    beep(150, 100);
}

// Success beep
void play_success_sound(void) {
    beep(800, 80);
    speaker_delay(30);
    beep(1000, 120);
}
