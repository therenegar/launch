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
