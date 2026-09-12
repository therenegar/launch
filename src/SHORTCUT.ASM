.intel_syntax noprefix
.code16
.section .text
.global _start

/* SHORTCUT.COM 2.7 - tiny prompt-aware Launch! hotkey TSR.
 * Build on a modern GNU binutils host:
 *   as --32 SHORTCUT.ASM -o SHORTCUT.O
 *   ld -m elf_i386 -Ttext 0x100 --oformat binary --entry=_start \
 *      -o SHORTCUT.COM SHORTCUT.O
 */

_start:
    jmp installer

/* -------------------------- resident portion -------------------------- */

old09:          .word 0,0
old15:          .word 0,0
old16:          .word 0,0
old28:          .word 0,0
old2f:          .word 0,0
shell_psp:      .word 0
hot_scan:       .byte 0x34             /* period */
hot_mods:       .byte 0x06             /* Ctrl + Alt */
hot_extended:   .byte 0                /* key uses an E0 prefix */
raw_mods:       .byte 0
raw_e0:         .byte 0
hot_down:       .byte 0
pending:        .byte 0
macro_active:   .byte 0
idle_enabled:   .byte 0
idle_timeout:   .long 0
idle_last:      .long 0
idle_macro:     .byte 27,'!',' ','/','N','O','W',13
hotkey_len:     .word 0
launch_macro:   .space 132,0           /* Esc + ! command/path + Enter */
queue_pos:      .word 0
queue_len:      .word 0
macro_queue:    .space 384,0

int09_handler:
    push ax
    push es
    mov ax,0x40
    mov es,ax
    mov ax,word ptr es:[0x6c]
    mov word ptr cs:[idle_last],ax
    mov ax,word ptr es:[0x6e]
    mov word ptr cs:[idle_last+2],ax
    pop es
    pop ax
.ifdef DOSBOX_BUILD
    /* Inspect the raw Set-1 byte before the DOSBox-X BIOS consumes the IRQ.
     * Every non-hotkey byte is still passed to the original handler. */
    push ax
    push bx
    in al,0x60
    cmp al,0xe0
    jne int09_db_not_prefix
    mov byte ptr cs:[raw_e0],1
    jmp int09_db_chain
int09_db_not_prefix:
    mov bl,al
    and bl,0x7f
    cmp bl,0x1d
    je int09_db_ctrl
    cmp bl,0x38
    je int09_db_alt
    cmp bl,0x2a
    je int09_db_shift
    cmp bl,0x36
    je int09_db_shift
    cmp bl,byte ptr cs:[hot_scan]
    jne int09_db_other_key
    mov bl,byte ptr cs:[raw_e0]
    cmp bl,byte ptr cs:[hot_extended]
    jne int09_db_other_key
    mov byte ptr cs:[raw_e0],0
    test al,0x80
    jnz int09_db_hot_release
    cmp byte ptr cs:[hot_down],0
    jne int09_db_discard
    mov bl,byte ptr cs:[raw_mods]
    cmp bl,byte ptr cs:[hot_mods]
    jne int09_db_chain
    mov byte ptr cs:[hot_down],1
    mov byte ptr cs:[pending],1
    jmp int09_db_discard
int09_db_hot_release:
    cmp byte ptr cs:[hot_down],0
    je int09_db_chain
    mov byte ptr cs:[hot_down],0
    jmp int09_db_discard
int09_db_ctrl:
    mov bl,0x02
    jmp int09_db_modifier
int09_db_alt:
    mov bl,0x04
    jmp int09_db_modifier
int09_db_shift:
    mov bl,0x01
int09_db_modifier:
    mov byte ptr cs:[raw_e0],0
    test al,0x80
    jnz int09_db_modifier_up
    or byte ptr cs:[raw_mods],bl
    jmp int09_db_chain
int09_db_modifier_up:
    not bl
    and byte ptr cs:[raw_mods],bl
    jmp int09_db_chain
int09_db_other_key:
    mov byte ptr cs:[raw_e0],0
int09_db_chain:
    pop bx
    pop ax
    jmp far ptr cs:[old09]
int09_db_discard:
    /* The BIOS is bypassed for the matched hotkey, so acknowledge the
     * keyboard interface and master PIC here. */
    in al,0x61
    mov ah,al
    or al,0x80
    out 0x61,al
    mov al,ah
    out 0x61,al
    mov al,0x20
    out 0x20,al
    call queue_launch_hotkey
    pop bx
    pop ax
    iret
.else
    pushf
    call far ptr cs:[old09]
    push ax
    push bx
    push es
    mov ax,0x40
    mov es,ax
    mov ah,byte ptr es:[0x17]
    xor al,al
    test ah,0x03                       /* either Shift */
    jz 1f
    or al,0x01
1:  test ah,0x04                       /* Ctrl */
    jz 2f
    or al,0x02
2:  test ah,0x08                       /* Alt */
    jz 3f
    or al,0x04
3:  cmp al,byte ptr cs:[hot_mods]
    jne int09_done
    mov bx,word ptr es:[0x1c]          /* keyboard-buffer tail */
    cmp bx,word ptr es:[0x1a]
    je int09_done
    sub bx,2
    cmp bx,0x1e
    jae 4f
    mov bx,0x3c
4:  mov ah,byte ptr es:[bx+1]
    cmp ah,byte ptr cs:[hot_scan]
    jne int09_done
    mov word ptr es:[0x1c],bx          /* remove hotkey keystroke */
    mov byte ptr cs:[pending],1
int09_done:
    pop es
    pop bx
    pop ax
    iret
.endif

/* DOSBox-X does not reliably issue the DOS INT 28h idle callback used by the
 * real-hardware build. Queue the already-built command without making DOS
 * calls; the command interpreter consumes it after this IRQ returns. */
queue_launch_hotkey:
.ifdef DOSBOX_BUILD
    cmp byte ptr cs:[pending],0
    je queue_hotkey_done
    push cx
    push si
    push di
    push ds
    push es
    push cs
    pop ds
    push cs
    pop es
    mov si,offset launch_macro
    mov di,offset macro_queue
    mov cx,word ptr cs:[hotkey_len]
    mov word ptr cs:[queue_len],cx
    cld
    rep movsb
    mov word ptr cs:[queue_pos],0
    mov byte ptr cs:[macro_active],1
    mov byte ptr cs:[pending],0
    call fill_keyboard_buffer
    pop es
    pop ds
    pop di
    pop si
    pop cx
queue_hotkey_done:
.endif
    ret

/* Raw BIOS keyboard intercept.  DOSBox-X may not place Ctrl+Alt combinations
 * in the BIOS ring buffer, so catch Set-1 make/break codes before translation.
 * CF clear tells the BIOS to discard the matched hotkey keystroke. */
int15_handler:
    pushf
    cmp ah,0x4f
    jne int15_chain
    push bx
    cmp al,0xe0
    jne int15_not_prefix
    mov byte ptr cs:[raw_e0],1
    jmp int15_chain_pop
int15_not_prefix:
    mov bl,al
    and bl,0x7f
    cmp bl,0x1d
    je int15_ctrl
    cmp bl,0x38
    je int15_alt
    cmp bl,0x2a
    je int15_shift
    cmp bl,0x36
    je int15_shift
    cmp bl,byte ptr cs:[hot_scan]
    jne int15_other_key
    mov bl,byte ptr cs:[raw_e0]
    cmp bl,byte ptr cs:[hot_extended]
    jne int15_other_key
    mov byte ptr cs:[raw_e0],0
    test al,0x80
    jnz int15_hot_release
    cmp byte ptr cs:[hot_down],0
    jne int15_discard
    mov bl,byte ptr cs:[raw_mods]
    cmp bl,byte ptr cs:[hot_mods]
    jne int15_chain_pop
    mov byte ptr cs:[hot_down],1
    mov byte ptr cs:[pending],1
    jmp int15_discard
int15_hot_release:
    cmp byte ptr cs:[hot_down],0
    je int15_chain_pop
    mov byte ptr cs:[hot_down],0
int15_discard:
    pop bx
    popf
    clc
    retf 2
int15_ctrl:
    mov bl,0x02
    jmp int15_modifier
int15_alt:
    mov bl,0x04
    jmp int15_modifier
int15_shift:
    mov bl,0x01
int15_modifier:
    mov byte ptr cs:[raw_e0],0
    test al,0x80
    jnz int15_modifier_up
    or byte ptr cs:[raw_mods],bl
    jmp int15_chain_pop
int15_modifier_up:
    not bl
    and byte ptr cs:[raw_mods],bl
    jmp int15_chain_pop
int15_other_key:
    mov byte ptr cs:[raw_e0],0
int15_chain_pop:
    pop bx
int15_chain:
    popf
    jmp far ptr cs:[old15]

int16_handler:
    call fill_keyboard_buffer
    jmp far ptr cs:[old16]

/* Fill every free slot in the standard BIOS keyboard ring buffer. */
fill_keyboard_buffer:
    pushf
    cmp byte ptr cs:[macro_active],0
    jne 1f
    popf
    ret
1:
    cli
    push ax
    push bx
    push cx
    push di
    push es
    mov ax,0x40
    mov es,ax
2:  mov bx,word ptr cs:[queue_pos]
    cmp bx,word ptr cs:[queue_len]
    jae 6f
    mov di,word ptr es:[0x1c]          /* current tail */
    mov cx,di
    add cx,2
    cmp cx,0x3e
    jb 3f
    mov cx,0x1e
3:  cmp cx,word ptr es:[0x1a]          /* next tail == head means full */
    je 7f
    mov al,byte ptr cs:[macro_queue+bx]
    xor ah,ah
    cmp al,0x1b
    jne 4f
    mov ah,0x01
4:  cmp al,0x0d
    jne 5f
    mov ah,0x1c
5:  mov word ptr es:[di],ax
    mov word ptr es:[0x1c],cx
    inc bx
    mov word ptr cs:[queue_pos],bx
    jmp 2b
6:  mov byte ptr cs:[macro_active],0
7:  pop es
    pop di
    pop cx
    pop bx
    pop ax
    popf
    ret

int28_handler:
    push ax
    push bx
    push cx
    push si
    push di
    push ds
    push es
    pushf
    call far ptr cs:[old28]
    cmp byte ptr cs:[pending],0
    jne int28_have_pending
    cmp byte ptr cs:[idle_enabled],0
    je int28_done
    mov ax,0x40
    mov es,ax
    mov ax,word ptr es:[0x6c]
    mov dx,word ptr es:[0x6e]
    sub ax,word ptr cs:[idle_last]
    sbb dx,word ptr cs:[idle_last+2]
    cmp dx,word ptr cs:[idle_timeout+2]
    jb int28_done
    ja int28_idle_due
    cmp ax,word ptr cs:[idle_timeout]
    jb int28_done
int28_idle_due:
    mov byte ptr cs:[pending],2
int28_have_pending:
    cmp byte ptr cs:[pending],0
    je int28_done
    mov ah,0x51                       /* current PSP */
    int 0x21
    cmp bx,word ptr cs:[shell_psp]
    jne int28_done
    mov ax,0x40
    mov es,ax
    mov bx,word ptr es:[0x1a]
    cmp bx,word ptr es:[0x1c]         /* wait for an empty BIOS buffer */
    jne int28_done
    push cs
    pop ds
    push cs
    pop es
    cmp byte ptr cs:[pending],2
    jne int28_hotkey_macro
    mov si,offset idle_macro
    mov cx,8
    jmp int28_copy_macro
int28_hotkey_macro:
    mov si,offset launch_macro
    mov cx,word ptr cs:[hotkey_len]
int28_copy_macro:
    mov di,offset macro_queue
    mov word ptr cs:[queue_len],cx
    cld
    rep movsb
    mov word ptr cs:[queue_pos],0
    mov byte ptr cs:[macro_active],1
    mov byte ptr cs:[pending],0
    mov ax,0x40
    mov es,ax
    mov ax,word ptr es:[0x6c]
    mov word ptr cs:[idle_last],ax
    mov ax,word ptr es:[0x6e]
    mov word ptr cs:[idle_last+2],ax
    call fill_keyboard_buffer
int28_done:
    pop es
    pop ds
    pop di
    pop si
    pop cx
    pop bx
    pop ax
    iret

int2f_handler:
    cmp ax,0xd5b0
    jne 6f
    mov bx,0x5343                     /* "SC" installation signature */
    mov cx,0x0207                     /* version 2.7 */
    push cs
    pop es
    iret
6:  cmp ax,0xd5b1
    jne 7f
    mov bx,0x554e                     /* "UN" unload-query signature */
    mov cx,0x0207
    push cs
    pop es
    iret
7:  cmp ax,0xd5b3
    jne 8f
    mov bx,0x4d51                     /* "MQ": shared macro queue present */
    mov cx,384
    push cs
    pop es
    iret
8:  cmp ax,0xd5b2
    jne 22f
    push ax
    push cx
    push si
    push di
    push es
    push cs
    pop es
    mov di,offset macro_queue
    cmp cx,384
    jbe 9f
    mov cx,384
9:  mov word ptr cs:[queue_len],cx
    mov word ptr cs:[queue_pos],0
    cld
    rep movsb
    mov byte ptr cs:[pending],0
    mov byte ptr cs:[macro_active],1
    pop es
    pop di
    pop si
    pop cx
    pop ax
    call fill_keyboard_buffer
    xor al,al
    iret
22: cmp ax,0xd5b4
    jne 10f
    mov byte ptr cs:[idle_enabled],bl
    mov word ptr cs:[idle_timeout],cx
    mov word ptr cs:[idle_timeout+2],dx
    mov ax,0x40
    mov es,ax
    mov ax,word ptr es:[0x6c]
    mov word ptr cs:[idle_last],ax
    mov ax,word ptr es:[0x6e]
    mov word ptr cs:[idle_last+2],ax
    xor al,al
    iret
10: jmp far ptr cs:[old2f]

resident_end:

/* ------------------------- transient installer ------------------------ */

installer:
    push cs
    pop ds
    call parse_command
    jc syntax_error
    cmp al,1
    je show_help
    cmp al,2
    je unload_shortcut

    mov ax,0xd5b0
    xor bx,bx
    int 0x2f
    cmp bx,0x5343
    je already_installed

    call build_launch_path
    jc path_error

    mov ax,word ptr cs:[0x16]         /* parent command processor PSP */
    mov word ptr [shell_psp],ax

    mov ax,0x3509
    int 0x21
    mov word ptr [old09],bx
    mov word ptr [old09+2],es
    mov ax,0x3515
    int 0x21
    mov word ptr [old15],bx
    mov word ptr [old15+2],es
    mov ax,0x3516
    int 0x21
    mov word ptr [old16],bx
    mov word ptr [old16+2],es
    mov ax,0x3528
    int 0x21
    mov word ptr [old28],bx
    mov word ptr [old28+2],es
    mov ax,0x352f
    int 0x21
    mov word ptr [old2f],bx
    mov word ptr [old2f+2],es

    push ds
    push cs
    pop ds
    mov dx,offset int09_handler
    mov ax,0x2509
    int 0x21
    mov dx,offset int15_handler
    mov ax,0x2515
    int 0x21
    mov dx,offset int16_handler
    mov ax,0x2516
    int 0x21
    mov dx,offset int28_handler
    mov ax,0x2528
    int 0x21
    mov dx,offset int2f_handler
    mov ax,0x252f
    int 0x21
    pop ds

    call show_installed_message

    mov es,word ptr cs:[0x2c]         /* release environment block */
    mov ah,0x49
    int 0x21

    mov dx,offset resident_end+15
    shr dx,4
    mov ax,0x3100
    int 0x21

already_installed:
    mov dx,offset msg_already
    jmp print_exit
syntax_error:
    mov dx,offset msg_syntax
    jmp print_exit_error
path_error:
    mov dx,offset msg_path
    jmp print_exit_error
show_help:
    mov dx,offset msg_help
print_exit:
    mov ah,9
    int 0x21
    mov ax,0x4c00
    int 0x21
print_exit_error:
    mov ah,9
    int 0x21
    mov ax,0x4c01
    int 0x21

/* Build Esc + sibling !.EXE path + Enter in resident storage. */
build_launch_path:
    mov ax,word ptr cs:[0x2c]
    or ax,ax
    jz build_path_bad
    mov es,ax
    xor bx,bx
    mov cx,0x7fff
8:  cmp byte ptr es:[bx],0
    jne 9f
    cmp byte ptr es:[bx+1],0
    je 10f
9:  inc bx
    loop 8b
    jmp build_path_bad
10: add bx,4                         /* double NUL and following count word */
    mov byte ptr [launch_macro],0x1b
    mov di,offset launch_macro+1
    xor bp,bp
    mov cx,127
11: mov al,byte ptr es:[bx]
    or al,al
    jz 13f
    cmp di,offset launch_macro+121
    jae build_path_bad
    mov byte ptr [di],al
    inc di
    inc bx
    cmp al,'\\'
    je 12f
    cmp al,'/'
    je 12f
    cmp al,':'
    jne 11b
12: mov bp,di                        /* first byte after final separator */
    jmp 11b
13: or bp,bp
    jz build_path_bad
    call sibling_on_path
    jnc build_short_path
    mov di,bp
    mov si,offset launch_name
    mov cx,5
    push ds                           /* REP MOVSB writes through ES:DI */
    pop es
    rep movsb
    mov byte ptr [di],0x0d
    inc di
    sub di,offset launch_macro
    mov word ptr [hotkey_len],di
    clc
    ret
build_short_path:
    mov di,offset launch_macro+1
    mov si,offset launch_short_name
    mov cx,1
    push ds
    pop es
    rep movsb
    mov byte ptr [di],0x0d
    inc di
    sub di,offset launch_macro
    mov word ptr [hotkey_len],di
    clc
    ret
build_path_bad:
    stc
    ret

/* CF clear when SHORTCUT.COM's directory is one of the PATH components.
 * ES still addresses the installing process's environment; BP is the first
 * byte after the final separator in launch_macro.  Surrounding quotes/spaces
 * and a trailing slash are ignored during comparison. */
sibling_on_path:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    mov bx,bp
    sub bx,offset launch_macro+1
14: cmp bx,0
    je path_not_found
    mov di,offset launch_macro
    add di,bx
    mov al,byte ptr [di]
    cmp al,'\\'
    je 15f
    cmp al,'/'
    jne 16f
15: dec bx
    jmp 14b

16: xor si,si
find_path_variable:
    cmp byte ptr es:[si],0
    je path_not_found
    mov al,byte ptr es:[si]
    and al,0xdf
    cmp al,'P'
    jne skip_environment_string
    mov al,byte ptr es:[si+1]
    and al,0xdf
    cmp al,'A'
    jne skip_environment_string
    mov al,byte ptr es:[si+2]
    and al,0xdf
    cmp al,'T'
    jne skip_environment_string
    mov al,byte ptr es:[si+3]
    and al,0xdf
    cmp al,'H'
    jne skip_environment_string
    cmp byte ptr es:[si+4],'='
    je path_component_start
skip_environment_string:
    cmp byte ptr es:[si],0
    je 17f
    inc si
    jmp skip_environment_string
17: inc si
    jmp find_path_variable

path_component_start:
    add si,5
next_path_component:
    cmp byte ptr es:[si],' '
    jne 18f
    inc si
    jmp next_path_component
18: cmp byte ptr es:[si],'"'
    jne 19f
    inc si
19: mov di,si
scan_path_component:
    mov al,byte ptr es:[si]
    cmp al,';'
    je path_component_end
    or al,al
    je path_component_end
    inc si
    jmp scan_path_component
path_component_end:
    mov bp,si                         /* delimiter for the next component */
    mov dx,si                         /* exclusive, trimmed component end */
trim_path_component:
    cmp dx,di
    jbe compare_path_component
    mov si,dx
    dec si
    mov al,byte ptr es:[si]
    cmp al,' '
    je trim_one_path_byte
    cmp al,'"'
    je trim_one_path_byte
    cmp al,'\\'
    je trim_one_path_byte
    cmp al,'/'
    jne compare_path_component
trim_one_path_byte:
    dec dx
    jmp trim_path_component
compare_path_component:
    mov cx,dx
    sub cx,di
    cmp cx,bx
    jne advance_path_component
    mov si,di
    mov di,offset launch_macro+1
compare_path_bytes:
    jcxz path_found
    mov al,byte ptr es:[si]
    cmp al,'a'
    jb 20f
    cmp al,'z'
    ja 20f
    sub al,32
20: mov ah,byte ptr [di]
    cmp ah,'a'
    jb 21f
    cmp ah,'z'
    ja 21f
    sub ah,32
21: cmp al,ah
    jne advance_path_component
    inc si
    inc di
    dec cx
    jmp compare_path_bytes
advance_path_component:
    mov si,bp
    cmp byte ptr es:[si],0
    je path_not_found
    inc si
    jmp next_path_component
path_found:
    clc
    jmp path_compare_done
path_not_found:
    stc
path_compare_done:
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

unload_shortcut:
    mov ax,0xd5b1
    xor bx,bx
    int 0x2f
    cmp bx,0x554e
    jne not_installed
    cmp cx,0x0207
    jne unload_unsafe
    mov word ptr [resident_seg],es

    mov ax,0x3509
    int 0x21
    call check_09
    jc unload_unsafe
    mov ax,0x3515
    int 0x21
    call check_15
    jc unload_unsafe
    mov ax,0x3516
    int 0x21
    call check_16
    jc unload_unsafe
    mov ax,0x3528
    int 0x21
    call check_28
    jc unload_unsafe
    mov ax,0x352f
    int 0x21
    call check_2f
    jc unload_unsafe

    mov al,0x09
    mov bx,offset old09
    call restore_vector
    mov al,0x15
    mov bx,offset old15
    call restore_vector
    mov al,0x16
    mov bx,offset old16
    call restore_vector
    mov al,0x28
    mov bx,offset old28
    call restore_vector
    mov al,0x2f
    mov bx,offset old2f
    call restore_vector
    mov es,word ptr [resident_seg]
    mov ah,0x49
    int 0x21
    jc unload_unsafe
    mov dx,offset msg_unloaded
    jmp print_exit

check_09:
    mov dx,offset int09_handler
    jmp check_vector
check_15:
    mov dx,offset int15_handler
    jmp check_vector
check_16:
    mov dx,offset int16_handler
    jmp check_vector
check_28:
    mov dx,offset int28_handler
    jmp check_vector
check_2f:
    mov dx,offset int2f_handler
check_vector:
    mov ax,es
    cmp ax,word ptr [resident_seg]
    jne check_bad
    cmp bx,dx
    jne check_bad
    clc
    ret
check_bad:
    stc
    ret

/* AL=vector, BX=resident far-pointer field. */
restore_vector:
    push ax
    push cx
    push dx
    push ds
    mov cl,al
    mov es,word ptr [resident_seg]
    mov dx,word ptr es:[bx]
    mov ax,word ptr es:[bx+2]
    mov ds,ax
    mov al,cl
    mov ah,0x25
    int 0x21
    pop ds
    pop dx
    pop cx
    pop ax
    ret

not_installed:
    mov dx,offset msg_not_installed
    jmp print_exit_error
unload_unsafe:
    mov dx,offset msg_unload_unsafe
    jmp print_exit_error

show_installed_message:
    mov dx,offset msg_press
    mov ah,9
    int 0x21
    test byte ptr [hot_mods],0x02
    jz 1f
    mov si,offset display_ctrl
    call print_green
1:  test byte ptr [hot_mods],0x04
    jz 2f
    mov si,offset display_alt
    call print_green
2:  test byte ptr [hot_mods],0x01
    jz 3f
    mov si,offset display_shift
    call print_green
3:  cmp byte ptr [hot_extended],0
    je display_normal_key
    mov al,byte ptr [hot_scan]
    cmp al,0x5b
    jne display_check_rwin
    mov si,offset display_lwin
    call print_green
    jmp 7f
display_check_rwin:
    cmp al,0x5c
    jne display_check_menu
    mov si,offset display_rwin
    call print_green
    jmp 7f
display_check_menu:
    mov si,offset display_menu
    call print_green
    jmp 7f
display_normal_key:
    mov al,byte ptr [hot_scan]
    cmp al,0x3b
    jb display_not_function
    cmp al,0x44
    jbe display_function_1_10
    cmp al,0x57
    je display_function_11
    cmp al,0x58
    je display_function_12
    jmp display_not_function
display_function_1_10:
    sub al,0x3a
    jmp display_function_number
display_function_11:
    mov al,11
    jmp display_function_number
display_function_12:
    mov al,12
display_function_number:
    mov byte ptr [display_function+2],0
    cmp al,10
    jae display_function_two_digits
    add al,'0'
    mov byte ptr [display_function+1],al
    jmp display_function_ready
display_function_two_digits:
    mov byte ptr [display_function+1],'1'
    sub al,10
    add al,'0'
    mov byte ptr [display_function+2],al
    mov byte ptr [display_function+3],0
display_function_ready:
    mov si,offset display_function
    call print_green
    jmp 7f
display_not_function:
    cmp al,0x39
    jne 4f
    mov si,offset display_space
    call print_green
    jmp 7f
4:  mov bx,offset character_table
5:  cmp byte ptr [bx],0
    je 6f
    cmp al,byte ptr [bx+1]
    je display_character_key
    add bx,2
    jmp 5b
display_character_key:
    mov al,byte ptr [bx]
    mov byte ptr [display_key],al
    mov si,offset display_key
    call print_green
    jmp 7f
6:  mov byte ptr [display_scan],'S'
    mov byte ptr [display_scan+1],'C'
    mov al,byte ptr [hot_scan]
    mov dl,al
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    and al,0x0f
    mov bx,offset hex_digits
    xlat
    mov byte ptr [display_scan+2],al
    mov al,dl
    and al,0x0f
    xlat
    mov byte ptr [display_scan+3],al
    mov si,offset display_scan
    call print_green
7:  mov dx,offset msg_open_menu
    mov ah,9
    int 0x21
    ret

/* Print DS:SI ASCIIZ in bright green while retaining each cell's background. */
print_green:
    push ax
    push bx
    push cx
    push dx
    push si
8:  lodsb
    or al,al
    jz 9f
    push ax
    mov ah,0x03
    xor bh,bh
    int 0x10
    push dx
    mov ah,0x08
    xor bh,bh
    int 0x10
    mov bl,ah
    and bl,0xf0
    or bl,0x0a
    pop dx
    pop ax
    mov ah,0x09
    xor bh,bh
    mov cx,1
    int 0x10
    inc dl
    mov ah,0x02
    xor bh,bh
    int 0x10
    jmp 8b
9:  pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

/* AL=0 install, AL=1 help, AL=2 unload, CF=1 error. */
parse_command:
    mov cl,byte ptr cs:[0x80]
    xor ch,ch
    mov si,0x81
    call skip_spaces
    or cx,cx
    jnz parse_have_command
    jmp parse_default
parse_have_command:
    mov al,byte ptr [si]
    cmp al,'/'
    je 8f
    cmp al,'-'
    jne parse_bad
8:  inc si
    dec cx
    jcxz parse_bad
    cmp byte ptr [si],'?'
    jne parse_key_option
    inc si
    dec cx
    call skip_spaces
    jcxz parse_help
    jmp parse_bad

parse_key_option:
    push cx
    push si
    mov di,offset word_unload
    call token_equal
    pop si
    pop cx
    jnc parse_unload
    call match_key_prefix
    jc parse_bad
    mov byte ptr [hot_mods],0
    mov byte ptr [hot_scan],0
    mov byte ptr [hot_extended],0
parse_token:
    call skip_spaces
    jcxz parse_finish
    mov di,si
    xor bx,bx                         /* token length */
9:  jcxz 10f
    mov al,byte ptr [si]
    cmp al,'+'
    je 10f
    cmp al,' '
    je 10f
    inc si
    dec cx
    inc bx
    jmp 9b
10: cmp bx,0
    je parse_bad
    push cx
    push si
    mov si,di
    mov cx,bx
    call parse_token_value
    pop si
    pop cx
    jc parse_bad
    call skip_spaces
    jcxz parse_finish
    cmp byte ptr [si],'+'
    jne parse_bad
    inc si
    dec cx
    jcxz parse_bad
    jmp parse_token

parse_finish:
    cmp byte ptr [hot_scan],0
    je parse_bad
parse_default:
    xor al,al
    clc
    ret
parse_help:
    mov al,1
    clc
    ret
parse_unload:
    mov al,2
    clc
    ret
parse_bad:
    stc
    ret

skip_spaces:
    jcxz 12f
11: cmp byte ptr [si],' '
    jne 12f
    inc si
    dec cx
    jnz 11b
12: ret

/* Consume case-insensitive "KEY=" from DS:SI/CX. */
match_key_prefix:
    cmp cx,4
    jb 13f
    mov al,byte ptr [si]
    and al,0xdf
    cmp al,'K'
    jne 13f
    mov al,byte ptr [si+1]
    and al,0xdf
    cmp al,'E'
    jne 13f
    mov al,byte ptr [si+2]
    and al,0xdf
    cmp al,'Y'
    jne 13f
    cmp byte ptr [si+3],'='
    jne 13f
    add si,4
    sub cx,4
    clc
    ret
13: stc
    ret

/* Parse one /KEY token in DS:SI/CX. */
parse_token_value:
    push bx
    push dx
    cmp cx,2
    jne parse_named_four
    cmp byte ptr [si],'F'
    je parse_function_short
    cmp byte ptr [si],'f'
    jne parse_named_four
parse_function_short:
    mov al,byte ptr [si+1]
    cmp al,'1'
    jb token_bad
    cmp al,'9'
    ja token_bad
    sub al,'1'
    add al,0x3b
    jmp set_key
parse_named_four:
    cmp cx,4
    jne 14f
    mov di,offset word_ctrl
    call token_equal
    jnc set_ctrl
    mov di,offset word_alt
    call token_equal
    jnc set_alt
    mov di,offset word_lwin
    call token_equal
    jnc key_lwin
    mov di,offset word_rwin
    call token_equal
    jnc key_rwin
    mov di,offset word_menu
    call token_equal
    jnc key_menu
14: cmp cx,5
    jne 15f
    mov di,offset word_shift
    call token_equal
    jnc set_shift
    mov di,offset word_space
    call token_equal
    jnc key_space
15: cmp cx,3
    jne 16f
    mov di,offset word_alt
    call token_equal
    jnc set_alt
    mov di,offset word_dot
    call token_equal
    jnc key_period
    mov di,offset word_win
    call token_equal
    jnc key_lwin
    mov di,offset word_f10
    call token_equal
    jnc key_f10
    mov di,offset word_f11
    call token_equal
    jnc key_f11
    mov di,offset word_f12
    call token_equal
    jnc key_f12
16: cmp cx,6
    jne 17f
    mov di,offset word_period
    call token_equal
    jnc key_period
17: cmp cx,1
    jne parse_hex_token
    mov al,byte ptr [si]
    call character_scan
    jc token_bad
    jmp set_key

parse_hex_token:
    call hex_scan
    jc token_bad
    cmp al,0x1d
    je set_ctrl
    cmp al,0x38
    je set_alt
    cmp al,0x2a
    je set_shift
    cmp al,0x36
    je set_shift
    jmp set_key
set_ctrl:
    or byte ptr [hot_mods],0x02
    jmp token_ok
set_alt:
    or byte ptr [hot_mods],0x04
    jmp token_ok
set_shift:
    or byte ptr [hot_mods],0x01
    jmp token_ok
key_space:
    mov al,0x39
    jmp set_key
key_period:
    mov al,0x34
    jmp set_key
key_f10:
    mov al,0x44
    jmp set_key
key_f11:
    mov al,0x57
    jmp set_key
key_f12:
    mov al,0x58
    jmp set_key
key_lwin:
    mov al,0x5b
    jmp set_extended_key
key_rwin:
    mov al,0x5c
    jmp set_extended_key
key_menu:
    mov al,0x5d
set_extended_key:
    mov byte ptr [hot_extended],1
set_key:
    cmp byte ptr [hot_scan],0
    jne token_bad
    mov byte ptr [hot_scan],al
token_ok:
    pop dx
    pop bx
    clc
    ret
token_bad:
    pop dx
    pop bx
    stc
    ret

/* Compare DS:SI/CX with ASCIIZ DS:DI, folding input to uppercase. */
token_equal:
    push ax
    push bx
    push cx
    push si
    push di
18: jcxz 19f
    mov al,byte ptr [si]
    cmp al,'a'
    jb 20f
    cmp al,'z'
    ja 20f
    sub al,32
20: cmp al,byte ptr [di]
    jne 21f
    inc si
    inc di
    dec cx
    jmp 18b
19: cmp byte ptr [di],0
    jne 21f
    clc
    jmp 22f
21: stc
22: pop di
    pop si
    pop cx
    pop bx
    pop ax
    ret

/* Hex token: 34, 0x34, or 34h. Returns AL. */
hex_scan:
    push bx
    push dx
    push di
    xor bx,bx
    cmp cx,2
    jb hex_bad
    cmp cx,2
    je 23f
    mov al,byte ptr [si]
    cmp al,'0'
    jne 24f
    mov al,byte ptr [si+1]
    and al,0xdf
    cmp al,'X'
    jne 24f
    add si,2
    sub cx,2
24: mov di,si
    add di,cx
    dec di
    mov al,byte ptr [di]
    and al,0xdf
    cmp al,'H'
    jne 23f
    dec cx
23: cmp cx,1
    jb hex_bad
    cmp cx,2
    ja hex_bad
25: mov al,byte ptr [si]
    call hex_digit
    jc hex_bad
    shl bl,4
    or bl,al
    inc si
    loop 25b
    cmp bl,1
    jb hex_bad
    cmp bl,0x7f
    ja hex_bad
    mov al,bl
    pop di
    pop dx
    pop bx
    clc
    ret
hex_bad:
    pop di
    pop dx
    pop bx
    stc
    ret

hex_digit:
    cmp al,'0'
    jb 26f
    cmp al,'9'
    jbe 27f
    and al,0xdf
    cmp al,'A'
    jb 26f
    cmp al,'F'
    ja 26f
    sub al,'A'-10
    clc
    ret
27: sub al,'0'
    clc
    ret
26: stc
    ret

/* Common US Set-1 alphanumeric and punctuation keys. */
character_scan:
    cmp al,'a'
    jb 28f
    cmp al,'z'
    ja 28f
    sub al,32
28: push bx
    mov bx,offset character_table
29: cmp byte ptr [bx],0
    je 30f
    cmp al,byte ptr [bx]
    je 31f
    add bx,2
    jmp 29b
31: mov al,byte ptr [bx+1]
    pop bx
    clc
    ret
30: pop bx
    stc
    ret

word_ctrl:      .asciz "CTRL"
word_alt:       .asciz "ALT"
word_shift:     .asciz "SHIFT"
word_space:     .asciz "SPACE"
word_dot:       .asciz "DOT"
word_period:    .asciz "PERIOD"
word_lwin:      .asciz "LWIN"
word_rwin:      .asciz "RWIN"
word_win:       .asciz "WIN"
word_menu:      .asciz "MENU"
word_f10:       .asciz "F10"
word_f11:       .asciz "F11"
word_f12:       .asciz "F12"
word_unload:    .asciz "UNLOAD"
launch_name:    .ascii "!.EXE"
launch_short_name: .ascii "!"
resident_seg:   .word 0

character_table:
    .byte 'A',0x1e,'B',0x30,'C',0x2e,'D',0x20,'E',0x12,'F',0x21
    .byte 'G',0x22,'H',0x23,'I',0x17,'J',0x24,'K',0x25,'L',0x26
    .byte 'M',0x32,'N',0x31,'O',0x18,'P',0x19,'Q',0x10,'R',0x13
    .byte 'S',0x1f,'T',0x14,'U',0x16,'V',0x2f,'W',0x11,'X',0x2d
    .byte 'Y',0x15,'Z',0x2c
    .byte '1',0x02,'2',0x03,'3',0x04,'4',0x05,'5',0x06
    .byte '6',0x07,'7',0x08,'8',0x09,'9',0x0a,'0',0x0b
    .byte '.',0x34,',',0x33,'/',0x35,';',0x27,'-',0x0c,'=',0x0d
    .byte '[',0x1a,']',0x1b,'\\',0x2b,'`',0x29,0,0

msg_already:
.ifdef DOSBOX_BUILD
    .ascii "SHORTCDB is already installed.\r\n$"
.else
    .ascii "SHORTCUT is already installed.\r\n$"
.endif
msg_press:
    .ascii "Press $"
msg_open_menu:
    .ascii " to open the LAUNCH! menu.\r\n$"
msg_syntax:
.ifdef DOSBOX_BUILD
    .ascii "SHORTCDB: invalid option. Use SHORTCDB /?\r\n$"
.else
    .ascii "SHORTCUT: invalid option. Use SHORTCUT /?\r\n$"
.endif
msg_path:
.ifdef DOSBOX_BUILD
    .ascii "SHORTCDB: cannot determine its executable directory.\r\n$"
.else
    .ascii "SHORTCUT: cannot determine its executable directory.\r\n$"
.endif
msg_unloaded:
.ifdef DOSBOX_BUILD
    .ascii "SHORTCDB unloaded.\r\n$"
.else
    .ascii "SHORTCUT unloaded.\r\n$"
.endif
msg_not_installed:
.ifdef DOSBOX_BUILD
    .ascii "SHORTCDB is not installed.\r\n$"
.else
    .ascii "SHORTCUT is not installed.\r\n$"
.endif
msg_unload_unsafe:
.ifdef DOSBOX_BUILD
    .ascii "SHORTCDB cannot unload: another TSR is above it.\r\n"
.else
    .ascii "SHORTCUT cannot unload: another TSR is above it.\r\n"
.endif
    .ascii "Unload that TSR first, or reboot.\r\n$"
msg_help:
.ifdef DOSBOX_BUILD
    .ascii "SHORTCDB 2.7 - DOSBox-X hotkey for Launch!\r\n\r\n"
    .ascii "Usage: SHORTCDB [/KEY=keys]\r\n"
    .ascii "       SHORTCDB /UNLOAD\r\n"
    .ascii "       SHORTCDB /?\r\n\r\n"
.else
    .ascii "SHORTCUT 2.7 - prompt-aware hotkey for Launch!\r\n\r\n"
    .ascii "Usage: SHORTCUT [/KEY=keys]\r\n"
    .ascii "       SHORTCUT /UNLOAD\r\n"
    .ascii "       SHORTCUT /?\r\n\r\n"
.endif
    .ascii "Default: CTRL+ALT+.\r\n"
    .ascii "/KEY accepts CTRL, ALT, SHIFT, F1-F12, WIN/LWIN/RWIN/MENU,\r\n"
    .ascii "a character, or hexadecimal\r\n"
    .ascii "Set-1 make codes separated by +. Modifier codes are 1D (Ctrl),\r\n"
    .ascii "38 (Alt), and 2A or 36 (Shift).\r\n\r\n"
    .ascii "Examples:\r\n"
.ifdef DOSBOX_BUILD
    .ascii "  SHORTCDB /KEY=CTRL+ALT+L\r\n"
    .ascii "  SHORTCDB /KEY=1D+38+34\r\n\r\n"
    .ascii "If SHORTCDB's directory is on PATH, the hotkey types !.\r\n"
.else
    .ascii "  SHORTCUT /KEY=CTRL+ALT+L\r\n"
    .ascii "  SHORTCUT /KEY=1D+38+34\r\n\r\n"
    .ascii "If SHORTCUT's directory is on PATH, the hotkey types !.\r\n"
.endif
    .ascii "Otherwise it falls back to the full sibling !.EXE path.\r\n"
    .ascii "Install at the DOS/4DOS prompt or from AUTOEXEC.BAT.\r\n"
.ifdef DOSBOX_BUILD
    .ascii "Use LOADHIGH SHORTCDB to place the resident portion in a UMB.\r\n$"
.else
    .ascii "Use LOADHIGH SHORTCUT to place the resident portion in a UMB.\r\n$"
.endif

display_ctrl:   .asciz "CTRL+"
display_alt:    .asciz "ALT+"
display_shift: .asciz "SHIFT+"
display_lwin:  .asciz "LWIN"
display_rwin:  .asciz "RWIN"
display_menu:  .asciz "MENU"
display_function: .byte 'F',0,0,0
display_space:  .asciz "SPACE"
display_key:    .byte 0,0
display_scan:   .byte 0,0,0,0,0
hex_digits:     .ascii "0123456789ABCDEF"
