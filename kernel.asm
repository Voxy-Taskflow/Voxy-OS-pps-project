BITS 16
ORG 0x1000

start:
    mov si, msg
    call print_string
    
    ; Wait for key
    xor ah, ah
    int 0x16
    
    ; Print the key
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    
    ; Newline
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
    
    mov si, msg_done
    call print_string
    
    cli
    hlt
    jmp $-1

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp print_string
.done:
    ret

msg: db 'Kernel running! Press a key: ', 0
msg_done: db 0x0D, 0x0A, 'Key received. Halting.', 0x0D, 0x0A, 0
