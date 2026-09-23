/* !KEYDB.COM - DOSBox-X build wrapper.
 * Build on a modern GNU binutils host from this directory:
 *   as --32 !KEYDB.ASM -o !KEYDB.O
 *   ld -m elf_i386 -Ttext 0x100 --oformat binary --entry=_start \
 *      -o !KEYDB.COM !KEYDB.O
 * !KEY.ASM remains the shared implementation.
 */
.set DOSBOX_BUILD,1
.include "!KEY.ASM"
