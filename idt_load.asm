; idt_load.asm - Load IDT into CPU

BITS 32

global idt_load

idt_load:
    mov eax, [esp + 4]    ; Get IDT pointer address from stack
    lidt [eax]            ; Load IDT
    ret
