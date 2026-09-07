/* SHORTCDB.COM - DOSBox-X build wrapper.
 * Build on a modern GNU binutils host from this directory:
 *   as --32 SHORTCDB.ASM -o SHORTCDB.O
 *   ld -m elf_i386 -Ttext 0x100 --oformat binary --entry=_start \
 *      -o SHORTCDB.COM SHORTCDB.O
 * SHORTCUT.ASM remains the shared implementation.
 */
.set DOSBOX_BUILD,1
.include "SHORTCUT.ASM"
