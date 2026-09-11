/* Launch! 2.2 resident VGA font keeper.
 * Hooks only INT 10h; previews and selection use the VGA BIOS directly.
 */
.code16
.arch i8086
.intel_syntax noprefix
.section .text
.global _start

_start:
old_int10:      .word 0, 0
active_id:      .byte 0
resident_mark:  .ascii "LAFONT22"
.balign 2
active_font:
    .space 4096, 0

resident_load_custom:
    push ax
    push bx
    push cx
    push dx
    push bp
    push es
    push cs
    pop es
    mov bp, offset active_font
    mov ax, 0x1100
    mov bx, 0x1000
    mov cx, 256
    xor dx, dx
    pushf
    call far ptr cs:[old_int10]
    pop es
    pop bp
    pop dx
    pop cx
    pop bx
    pop ax
    ret

resident_mode_is_80x25:
    push ax
    push ds
    mov ax, 0x0040
    mov ds, ax
    mov al, byte ptr [0x49]
    cmp al, 2
    je 1f
    cmp al, 3
    je 1f
    cmp al, 7
    jne 2f
1:  cmp word ptr [0x4a], 80
    jne 2f
    cmp byte ptr [0x84], 24
    jne 2f
    cmp word ptr [0x85], 16
    jne 2f
    pop ds
    pop ax
    clc
    ret
2:  pop ds
    pop ax
    stc
    ret

int10_handler:
    cmp ah, 0x00
    je int10_mode_set
    jmp far ptr cs:[old_int10]

int10_mode_set:
    pushf
    call far ptr cs:[old_int10]
    pushf
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    cmp byte ptr cs:[active_id], 0
    je 1f
    call resident_mode_is_80x25
    jc 1f
    call resident_load_custom
1:  pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    popf
    iret

resident_end:
