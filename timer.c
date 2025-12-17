// timer.c - Timer interrupt handler (IRQ0)

unsigned int tick_count = 0;
static unsigned int seconds = 0;
static unsigned int minutes = 0;
static unsigned int hours = 0;

int status_bar_needs_update = 0;  // Flag for main loop

#define TICKS_PER_SECOND 18

void timer_handler(void) {
    tick_count++;
    
    if (tick_count >= TICKS_PER_SECOND) {
        tick_count = 0;
        seconds++;
        status_bar_needs_update = 1;  // Signal update needed
        
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            
            if (minutes >= 60) {
                minutes = 0;
                hours++;
            }
        }
    }
}

unsigned int get_uptime_seconds(void) {
    return hours * 3600 + minutes * 60 + seconds;
}

void get_time_string(char* buffer) {
    buffer[0] = '0' + (hours / 10);
    buffer[1] = '0' + (hours % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (minutes / 10);
    buffer[4] = '0' + (minutes % 10);
    buffer[5] = ':';
    buffer[6] = '0' + (seconds / 10);
    buffer[7] = '0' + (seconds % 10);
    buffer[8] = '\0';
}
