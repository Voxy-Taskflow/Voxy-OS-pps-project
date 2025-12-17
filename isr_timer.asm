; isr_timer.asm - Timer interrupt handler wrapper

BITS 32

extern timer_handler

global isr_timer

isr_timer:
    pusha
    
    call timer_handler
    
    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al
    
    popa
    iret
