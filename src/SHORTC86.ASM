.intel_syntax noprefix
.code16
.section .text
.global _start

/* SHORTC86.COM - Launch! 3.86 POC hotkey TSR.
 * 8088/8086-safe: no 186+ instructions, no E0/101-key dependency.
 * Default hotkey: Ctrl+Alt+period. Queues "!86" at the DOS prompt.
 * Build: as --32 SHORTC86.ASM -o SHORTC86.O
 *        ld -m elf_i386 -Ttext 0x100 --oformat binary --entry=_start -o SHORTC86.COM SHORTC86.O
 */
_start: jmp installer
old09: .word 0,0
old28: .word 0,0
hot_scan: .byte 0x34
pending: .byte 0
cmd: .byte 27,'!','8','6',13

int09_handler:
 pushf
 call far ptr cs:[old09]
 push ax
 push bx
 push es
 mov ax,0x40
 mov es,ax
 mov al,byte ptr es:[0x17]
 and al,0x0c
 cmp al,0x0c
 jne int09_done
 mov bx,word ptr es:[0x1c]
 cmp bx,word ptr es:[0x1a]
 je int09_done
 sub bx,2
 cmp bx,0x1e
 jae int09_nowrap
 mov bx,0x3c
int09_nowrap:
 mov ah,byte ptr es:[bx+1]
 cmp ah,byte ptr cs:[hot_scan]
 jne int09_done
 mov word ptr es:[0x1c],bx
 mov byte ptr cs:[pending],1
int09_done:
 pop es
 pop bx
 pop ax
 iret

int28_handler:
 pushf
 call far ptr cs:[old28]
 cmp byte ptr cs:[pending],0
 je int28_done
 push ax
 push bx
 push cx
 push si
 push di
 push ds
 push es
 mov ax,0x40
 mov es,ax
 mov bx,word ptr es:[0x1a]
 cmp bx,word ptr es:[0x1c]
 jne int28_restore
 push cs
 pop ds
 mov si,offset cmd
 mov cx,5
int28_put:
 mov di,word ptr es:[0x1c]
 mov bx,di
 add bx,2
 cmp bx,0x3e
 jb int28_space
 mov bx,0x1e
int28_space:
 cmp bx,word ptr es:[0x1a]
 je int28_restore
 lodsb
 xor ah,ah
 cmp al,27
 jne int28_notesc
 mov ah,1
int28_notesc:
 cmp al,13
 jne int28_notenter
 mov ah,0x1c
int28_notenter:
 mov word ptr es:[di],ax
 mov word ptr es:[0x1c],bx
 loop int28_put
 mov byte ptr cs:[pending],0
int28_restore:
 pop es
 pop ds
 pop di
 pop si
 pop cx
 pop bx
 pop ax
int28_done:
 iret

installer:
 push cs
 pop ds
 mov ax,0x3509
 int 0x21
 mov word ptr [old09],bx
 mov word ptr [old09+2],es
 mov ax,0x3528
 int 0x21
 mov word ptr [old28],bx
 mov word ptr [old28+2],es
 mov dx,offset int09_handler
 mov ax,0x2509
 int 0x21
 mov dx,offset int28_handler
 mov ax,0x2528
 int 0x21
 mov dx,offset msg
 mov ah,9
 int 0x21
 mov dx,offset installer
 add dx,15
 mov cl,4
 shr dx,cl
 mov ax,0x3100
 int 0x21
msg: .ascii "Launch! 8086 shortcut installed - CTRL+ALT+. opens !86.$\r\n"
