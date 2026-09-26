;     __                           __    __
;    / /   ____ ___  ______  _____/ /_  / /
;   / /   / __ `/ / / / __ \/ ___/ __ \/ / 
;  / /___/ /_/ / /_/ / / / /__/ / / /_/  
; /_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
; Launch! for DOS ---------------------
/*
 * MAINTAINER NOTES - Launch! 3.73
 * File: !KEYDB.ASM
 * Role: Debug/alternate !KEY TSR build
 * Build/ownership: Diagnostic TSR variant.
 * Maintainer contract: Do not let debug behavior leak into normal !KEY builds.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* !KEYDB.COM - DOSBox-X build wrapper.
 * Build on a modern GNU binutils host from this directory:
 *   as --32 !KEYDB.ASM -o !KEYDB.O
 *   ld -m elf_i386 -Ttext 0x100 --oformat binary --entry=_start \
 *      -o !KEYDB.COM !KEYDB.O
 * !KEY.ASM remains the shared implementation.
 */
.set DOSBOX_BUILD,1
.include "!KEY.ASM"
