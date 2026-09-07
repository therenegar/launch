.intel_syntax noprefix
.code16
.section .text
.global _start

/* Embedded Launch! command injector.  It fills the BIOS type-ahead buffer in
 * bursts and refills it whenever INT 16h is called.  The final binary is
 * converted to the macro_blob[] initializer in LAUNCH.C. */

_start:
int16_handler:
    call fill_keyboard_buffer
    jmp far ptr cs:[old16]

fill_keyboard_buffer:
    pushf
    cli
    push ax
    push bx
    push cx
    push di
    push es
    mov ax,0x40
    mov es,ax
1:  mov bx,word ptr cs:[macro_pos]
    cmp bx,word ptr cs:[macro_len]
    jae 5f
    mov di,word ptr es:[0x1c]
    mov cx,di
    add cx,2
    cmp cx,0x3e
    jb 2f
    mov cx,0x1e
2:  cmp cx,word ptr es:[0x1a]
    je 5f
    mov al,byte ptr cs:[macro_queue+bx]
    xor ah,ah
    cmp al,0x1b
    jne 3f
    mov ah,0x01
3:  cmp al,0x0d
    jne 4f
    mov ah,0x1c
4:  mov word ptr es:[di],ax
    mov word ptr es:[0x1c],cx
    inc bx
    mov word ptr cs:[macro_pos],bx
    jmp 1b
5:  pop es
    pop di
    pop cx
    pop bx
    pop ax
    popf
    ret

int2f_handler:
    cmp ax,0xd5a0
    je installed_query
    cmp ax,0xd5a1
    je queue_command
    jmp far ptr cs:[old2f]
installed_query:
    mov bx,0x4c48
    iret
queue_command:
    push ax
    push cx
    push si
    push di
    push es
    push cs
    pop es
    xor di,di
    mov word ptr cs:[macro_pos],di
    cmp cx,384
    jbe 6f
    mov cx,384
6:  mov word ptr cs:[macro_len],cx
    mov di,offset macro_queue
    cld
    rep movsb
    pop es
    pop di
    pop si
    pop cx
    pop ax
    call fill_keyboard_buffer
    xor al,al
    iret

signature:      .ascii "LH10"
old16:          .word 0,0
old2f:          .word 0,0
macro_pos:      .word 0
macro_len:      .word 0
macro_queue:    .space 384,0
