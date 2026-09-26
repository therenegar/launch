/*
    __                           __    __
   / /   ____ ___  ______  _____/ /_  / /
  / /   / __ `/ / / / __ \/ ___/ __ \/ / 
 / /___/ /_/ / /_/ / / / /__/ / / /_/  
/_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
Launch! for DOS ---------------------
*/
/*
 * MAINTAINER NOTES - Launch! 3.73
 * File: MDFONTS.H
 * Role: !MKDOWN graphical font interface
 * Build/ownership: Consumed by MKDOWN.C.
 * Maintainer contract: Keep font IDs/mappings synchronized with MDFONTS.C and packaged resources.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
#ifndef MDFONTS_H
#define MDFONTS_H

/* Private !MKDOWN Show fonts. These are embedded into !MKDOWN.EXE and are
   deliberately not added to FONT.DAT or the Launch! font selector. */
extern const unsigned char far md_body_font[4096];
extern const unsigned char far md_h1_font[6144];
extern const unsigned char far md_h2_font[4864];
extern const unsigned char far md_h3_font[4864];
extern const unsigned char far md_h4_font[4864];
extern const unsigned char far md_h5_font[4096];

#endif
