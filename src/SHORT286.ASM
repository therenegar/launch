/* SHORT286.COM - 80286-safe Launch! hotkey TSR wrapper.
 *
 * GNU as otherwise relaxes long conditional branches in .code16 input to
 * 386-only 0F 8x near jumps.  Selecting i286 makes it expand those branches
 * into 286-safe short-condition/near-jump pairs.
 *
 *   as --32 SHORT286.ASM -o SHORT286.O
 *   ld -m elf_i386 -Ttext 0x100 --oformat binary --entry=_start \
 *      -o SHORT286.COM SHORT286.O
 */
.intel_syntax noprefix
.code16
.arch i286
.set CPU286_BUILD,1
.include "SHORTCUT.ASM"
