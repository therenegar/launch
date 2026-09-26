;     __                           __    __
;    / /   ____ ___  ______  _____/ /_  / /
;   / /   / __ `/ / / / __ \/ ___/ __ \/ / 
;  / /___/ /_/ / /_/ / / / /__/ / / /_/  
; /_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
; Launch! for DOS ---------------------
/*
 * MAINTAINER NOTES - Launch! 3.73
 * File: !KEY286.ASM
 * Role: 286-compatible !KEY TSR variant
 * Build/ownership: Installed where the 286-specific variant is required.
 * Maintainer contract: Keep external behavior aligned with !KEY.COM while respecting CPU constraints.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* !KEY286.COM - 80286-safe Launch! hotkey TSR wrapper.
 *
 * GNU as otherwise relaxes long conditional branches in .code16 input to
 * 386-only 0F 8x near jumps.  Selecting i286 makes it expand those branches
 * into 286-safe short-condition/near-jump pairs.
 *
 *   as --32 !KEY286.ASM -o !KEY286.O
 *   ld -m elf_i386 -Ttext 0x100 --oformat binary --entry=_start \
 *      -o !KEY286.COM !KEY286.O
 */
.intel_syntax noprefix
.code16
.arch i286
.set CPU286_BUILD,1
.include "!KEY.ASM"
