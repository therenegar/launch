#ifndef LAUNCH_ACCLIB_H
#define LAUNCH_ACCLIB_H

#include <stdio.h>

#define ACC_PATH 128
#define ACC_MOUSE_MOVED 0x8000
#define ACC_MOUSE_OUTSIDE 0x4000
#define ACC_MAXIMIZE_L 248
#define ACC_MAXIMIZE_R 249
#define ACC_ATTR(bg,fg) (((bg)<<4)|(fg))

typedef struct {
  unsigned char background,border,titlebar_fg,titlebar_bg,main_title,titles,folders,launchers;
  unsigned char selected_fg,selected_bg,controls_fg,controls_bg,labels;
  unsigned char menu_top,show_collections,show_explore,show_power,show_time;
  unsigned char screensaver,saver_color,saver_delay,hour_12;
  unsigned char font_id,font_persist,mouse_cursor,prompt_inactivity;
} ACC_APPEARANCE;

extern ACC_APPEARANCE acc_appearance;
extern int acc_cols,acc_rows,acc_mouse_present;
extern char acc_directory[ACC_PATH];

#define ACC_BG ACC_ATTR(acc_appearance.background,acc_appearance.background)
#define ACC_BORDER ACC_ATTR(acc_appearance.background,acc_appearance.border)
#define ACC_TITLE ACC_ATTR(acc_appearance.background,acc_appearance.main_title)
#define ACC_HEADING ACC_ATTR(acc_appearance.background,acc_appearance.titles)
#define ACC_TEXT ACC_ATTR(acc_appearance.background,acc_appearance.launchers)
#define ACC_LABEL ACC_ATTR(acc_appearance.background,acc_appearance.labels)
#define ACC_SELECT ACC_ATTR(acc_appearance.selected_bg,acc_appearance.selected_fg)
#define ACC_CONTROL ACC_ATTR(acc_appearance.controls_bg,acc_appearance.controls_fg)

int acc_begin(const char *argv0,const char *title,int graphics);
void acc_end_screen(void);
void acc_end(void);
void acc_restore_screen(void);
void acc_restore_text_screen(void);
void acc_clear(int attr);
void acc_put(int x,int y,int ch,int attr);
void acc_text(int x,int y,const char *text,int attr,int width);
void acc_fill(int x,int y,int width,int height,int ch,int attr);
void acc_box(int x,int y,int width,int height,const char *title);
void acc_subbox(int x,int y,int width,int height,const char *title,int toolbar);
void acc_set_close_glyphs(int left,int right);
void acc_button(int x,int y,const char *text,int selected);
void acc_button_disabled(int x,int y,const char *text);
void acc_press_button(int x,int y,const char *text);
void acc_scrollbar(int x,int top,int height,int position,int total,int page);
void acc_shadow(int x,int y,int width,int height);
void acc_glyph_read(int code,unsigned char *glyph);
void acc_glyph_write(int code,const unsigned char *glyph);
void acc_glyph_library(int logical_id,int code);
void acc_game_glyphs_suspend(void);
void acc_game_glyphs_resume(void);
int acc_font_height(void);
int acc_glyph_is_custom(int code);
int acc_key(void);
int acc_key_ready(void);
int acc_mouse(int *x,int *y,int *buttons);
void acc_wait(int *key,int *x,int *y,unsigned *buttons);
void acc_set_idle_hook(void (*fn)(void));
void acc_tooltip_region(int x,int y,int width,const char *text,int active);
void acc_tooltip_clear_regions(void);
void acc_modal_begin(void);
void acc_modal_end(void);
void acc_mouse_display(int show);
void acc_canvas_cursor_region(int x,int y,int width,int height,int colour);
void acc_mouse_reapply_cursor(void);
void acc_caret_set(int x,int y);
void acc_caret_hide(void);
void acc_path(char *out,const char *subdir,const char *name);
int acc_make_dir(const char *path);
int acc_exists(const char *path);
void acc_notice(const char *title,const char *message);
unsigned long acc_ticks(void);
int acc_help(int argc,char **argv,const char *name,const char *description);

#endif
