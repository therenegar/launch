;     __                           __    __
;    / /   ____ ___  ______  _____/ /_  / /
;   / /   / __ `/ / / / __ \/ ___/ __ \/ /
;  / /___/ /_/ / /_/ / / / /__/ / / /_/  
; /_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
; Launch! for DOS ---------------------
/*
 * Launch! transient command injector (LH12).
 *
 * The helper exists only when !KEY/!TKEY is unavailable and a command will
 * not fit completely in the BIOS type-ahead buffer. It streams the remaining
 * text through INT 16h. Once all text is in the BIOS buffer it restores its
 * vectors and arranges for the original INT 16h IRET to return to a tiny
 * cleanup trampoline copied onto the caller's stack. The trampoline therefore
 * runs back in normal caller context, frees the helper MCB with DOS AH=49h,
 * restores the BIOS-return registers/flags, and jumps to the original caller.
 */
.intel_syntax noprefix
.code16
.section .text
.global _start

_start:
int16_handler:
    call fill_keyboard_buffer
    cmp byte ptr cs:[helper_state],2
    je detach_for_cleanup
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
5:  mov bx,word ptr cs:[macro_pos]
    cmp bx,word ptr cs:[macro_len]
    jb 6f
    cmp word ptr cs:[macro_len],0
    je 6f
    mov byte ptr cs:[helper_state],2
6:  pop es
    pop di
    pop cx
    pop bx
    pop ax
    popf
    ret

/* No DOS call is made here. We are still servicing INT 16h. Restore our
 * vectors, move the interrupt return frame down below a stack-resident cleanup
 * trampoline, then chain to the original BIOS handler. Its IRET enters the
 * trampoline in normal caller context. */
detach_for_cleanup:
    push bp
    mov bp,sp
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push ds
    push es
    cli

    xor ax,ax
    mov es,ax
    mov ax,word ptr cs:[old16]
    mov word ptr es:[0x58],ax
    mov ax,word ptr cs:[old16+2]
    mov word ptr es:[0x5a],ax
    mov ax,word ptr cs:[old2f]
    mov word ptr es:[0xbc],ax
    mov ax,word ptr cs:[old2f+2]
    mov word ptr es:[0xbe],ax

    /* Original frame begins at SS:[BP+2]: IP, CS, FLAGS. */
    mov ax,bp
    add ax,2
    sub ax,71
    mov word ptr cs:[cleanup_frame_sp],ax
    mov di,ax
    mov dx,ax
    add dx,6

    /* New IRET frame: return to SS:DX (the copied trampoline), preserving the
       original caller flags so the old BIOS handler can update them normally. */
    mov word ptr ss:[di],dx
    mov ax,ss
    mov word ptr ss:[di+2],ax
    mov ax,word ptr ss:[bp+6]
    mov word ptr ss:[di+4],ax

    /* Copy and patch the post-IRET trampoline. */
    mov di,dx
    push cs
    pop ds
    mov ax,ss
    mov es,ax
    mov si,offset cleanup_trampoline
    mov cx,65
    cld
    rep movsb
    mov bx,dx
    mov ax,cs
    mov word ptr ss:[bx+TRAMP_HELPER_SEG],ax
    mov ax,bp
    add ax,8                  /* original SP after a normal IRET: (BP+2)+6 */
    mov word ptr ss:[bx+TRAMP_RETURN_SP],ax
    mov ax,word ptr ss:[bp+2]
    mov word ptr ss:[bx+TRAMP_RETURN_IP],ax
    mov ax,word ptr ss:[bp+4]
    mov word ptr ss:[bx+TRAMP_RETURN_CS],ax

    pop es
    pop ds
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    pop bp
    mov sp,word ptr cs:[cleanup_frame_sp]
    jmp far ptr cs:[old16]

int2f_handler:
    cmp ax,0xd5a0
    je installed_query
    cmp ax,0xd5a1
    je queue_command
    cmp ax,0xd5a2
    je status_query
    jmp far ptr cs:[old2f]
installed_query:
    mov bx,0x4c48
    iret
status_query:
    mov bx,0x4c48
    xor ax,ax
    mov al,byte ptr cs:[helper_state]
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
    mov byte ptr cs:[helper_state],1
    cmp cx,384
    jbe 7f
    mov cx,384
7:  mov word ptr cs:[macro_len],cx
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

/* Runs only after the original INT 16h handler has IRET'd back to the caller.
 * Save the BIOS result, free LH12 using ordinary DOS process context, restore
 * the BIOS result and return to the instruction that originally invoked INT16. */
cleanup_trampoline:
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
    sti
    mov ah,0x51
    int 0x21
    .byte 0xb8
cleanup_helper_seg:
    .word 0
    dec ax
    mov es,ax
    mov word ptr es:[1],bx
    inc ax
    mov es,ax
    mov ah,0x49
    int 0x21
    jnc cleanup_free_ok
    mov ax,es
    dec ax
    mov es,ax
    mov word ptr es:[1],8
cleanup_free_ok:
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    popf
    .byte 0xbc
cleanup_return_sp:
    .word 0
    .byte 0xea
cleanup_return_ip:
    .word 0
cleanup_return_cs:
    .word 0
cleanup_trampoline_end:

.set TRAMP_BYTES, cleanup_trampoline_end-cleanup_trampoline
.set TRAMP_HELPER_SEG, cleanup_helper_seg-cleanup_trampoline
.set TRAMP_RETURN_SP, cleanup_return_sp-cleanup_trampoline
.set TRAMP_RETURN_IP, cleanup_return_ip-cleanup_trampoline
.set TRAMP_RETURN_CS, cleanup_return_cs-cleanup_trampoline

signature:        .ascii "LH12"
old16:            .word 0,0
old2f:            .word 0,0
helper_state:     .byte 0
                  .byte 0
macro_pos:        .word 0
macro_len:        .word 0
cleanup_frame_sp: .word 0
macro_queue:      .space 384,0
