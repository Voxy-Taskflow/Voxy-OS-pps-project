BITS 16
ORG 0x7C00

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov si, msg_load
    call print_string

    ; Load 41 sectors
    mov ah, 0x02
    mov al, 57          ; Load 57 sectors
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    mov bx, 0x1000
    int 0x13
    jc .disk_error

    jmp 0x0000:0x1000

.disk_error:
    mov si, msg_error
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

msg_load: db 'Loading VoxyOS...', 0x0D, 0x0A, 0
msg_error: db 'Boot error!', 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55
