; isr_keyboard.asm - Keyboard interrupt handler wrapper

BITS 32

extern keyboard_handler

global isr_keyboard

isr_keyboard:
    pusha                       ; Save all registers
    
    call keyboard_handler       ; Call C handler
    
    ; Send EOI (End of Interrupt) to PIC
    mov al, 0x20
    out 0x20, al
    
    popa                        ; Restore all registers
    iret                        ; Return from interrupt
