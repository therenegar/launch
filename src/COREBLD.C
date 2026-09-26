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
 * File: COREBLD.C
 * Role: Generated/build copy of Launch! core
 * Build/ownership: Derived from LAUNCH.C; normal BUILD compiles this file. Keep behavioral edits in LAUNCH.C and synchronize/regenerate.
 * Maintainer contract: Must remain functionally identical to the canonical core except for deliberate build-time transforms.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* Launch! 3.73 - modal command menu for DOS
 * Microsoft C/C++ 7.0, medium model (.EXE), 286/EGA or later.
 */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <io.h>
#include <malloc.h>
#include <process.h>
#include "CLOCKDAT.H"
#include "LOGODAT.H"
#include "FONTRES.H"
#define LAUNCH_GLYPH_FAR
#include "GLYPHDAT.H"

#define MAX_NODES 96
#define MAX_TITLE 24
#define MAX_CMD 128
#define MAX_MACRO 384
#define MAX_DEPTH 4
#define MAX_CHILD 22
#define MENU_CAPACITY 20
#define MENU_WIDTH 20
#define MAX_EXPLORE_ENTRIES 256
#define EXPLORE_ROWS 11
#define EXPLORE_COLS 4
#define MAKE_FP(seg,off) ((void far *)((((unsigned long)(seg))<<16) | \
                                      (unsigned short)(off)))

typedef struct {
  unsigned char background,border,titlebar_fg,titlebar_bg,main_title,titles,folders,launchers;
  unsigned char selected_fg,selected_bg,controls_fg,controls_bg,labels;
  unsigned char menu_top,show_collections,show_explore,show_power,show_time,show_sysbar;
  unsigned char screensaver,saver_color,saver_delay,hour_12;
  unsigned char font_id,font_persist,mouse_cursor,prompt_inactivity;
} APPEARANCE;

static const APPEARANCE default_appearance={1,11,15,7,12,14,15,10,15,3,0,7,7,0,1,1,1,1,0,1,10,0,1,1,0,0,0};
static APPEARANCE appearance={1,11,15,7,12,14,15,10,15,3,0,7,7,0,1,1,1,1,0,1,10,0,1,1,0,0,0};

static void (interrupt far *setkey_old_int09)();
static volatile unsigned char setkey_scan,setkey_e0,setkey_mods;
static volatile unsigned char setkey_key,setkey_key_mods;

#ifndef __GNUC__
static void interrupt far setkey_int09()
{
  unsigned char scan,base,mask;
  _asm { in al, 60h }
  _asm { mov setkey_scan, al }
  scan=setkey_scan;
  if(scan==0xE0)setkey_e0=1;
  else {
    base=(unsigned char)(scan&0x7F);mask=0;
    if(base==0x1D)mask=4;
    else if(base==0x38)mask=8;
    else if(base==0x2A || base==0x36)mask=1;
    if(mask){
      if(scan&0x80)setkey_mods&=(unsigned char)~mask;
      else setkey_mods|=mask;
    } else if(!(scan&0x80) && !setkey_key){
      setkey_key=base;setkey_key_mods=setkey_mods;
    }
    setkey_e0=0;
  }
  _chain_intr(setkey_old_int09);
}
#else
static void setkey_int09(void) {}
#endif

/* VGA attribute byte: high nibble = background, low nibble = foreground. */
#define ATTR(bg,fg)       (((bg)<<4)|(fg))
#define C_MENU_BACKGROUND ATTR(appearance.background,appearance.background)
#define C_BORDER          ATTR(appearance.background,appearance.border)
#define C_TITLE           ATTR(appearance.background,appearance.titles)
#define C_ROOT_TITLE      ATTR(appearance.background,appearance.main_title)
#define C_FOLDER          ATTR(appearance.background,appearance.folders)
#define C_ITEM            ATTR(appearance.background,appearance.launchers)
#define C_EMPTY           ATTR(appearance.background,appearance.border)
#define C_SELECTED        ATTR(appearance.selected_bg,appearance.selected_fg)
#define C_INPUT_FIELD     ATTR(appearance.controls_bg,appearance.controls_fg)
#define C_BUTTON          ATTR(appearance.controls_bg,appearance.controls_fg)
#define C_INPUT_LABEL     ATTR(appearance.background,appearance.labels)
#define C_CONSOLE_PATH    0x0F  /* fixed bright white on black */
#define C_FAUX_SHADOW     0x08  /* dark grey on black */
#define C_BLOCK_SHADOW_FG 0x00  /* black half-block foreground */

typedef struct {
  char title[MAX_TITLE];
  char command[MAX_CMD];
  int parent;
  unsigned char folder;
  unsigned char separator;
  unsigned char active;
  unsigned char order;
  unsigned char press_enter;
  unsigned char change_dir;
  unsigned char prompt_params;
  unsigned char add_path;
} NODE;

/* Preassembled 8086 burst-injection helper.  The final 384 bytes are its queue. */
#define MACRO_BLOB_SIZE 563
#define MACRO_INT16_OFF 0x0000
#define MACRO_INT2F_OFF 0x0063
#define MACRO_SIGNATURE_OFF 0x00A3
#define MACRO_OLD16_OFF 0x00A7
#define MACRO_OLD2F_OFF 0x00AB
static unsigned char macro_blob[MACRO_BLOB_SIZE]={
232,5,0,46,255,46,167,0,156,250,80,83,81,87,6,184,
64,0,142,192,46,139,30,175,0,46,59,30,177,0,115,60,
38,139,62,28,0,137,249,131,193,2,131,249,62,114,3,185,
30,0,38,59,14,26,0,116,35,46,138,135,179,0,48,228,
60,27,117,2,180,1,60,13,117,2,180,28,38,137,5,38,
137,14,28,0,67,46,137,30,175,0,235,184,7,95,89,91,
88,157,195,61,160,213,116,10,61,161,213,116,9,46,255,46,
171,0,187,72,76,207,80,81,86,87,6,14,7,49,255,46,
137,62,175,0,129,249,128,1,118,3,185,128,1,46,137,14,
177,0,191,179,0,252,243,164,7,95,94,89,88,232,104,255,
48,192,207,76,72,49,49
};

static NODE nodes[MAX_NODES];
static int node_count;
static int screen_cols, screen_rows;
static unsigned short far *video;
static unsigned short far *saved;
static unsigned short far *render_buffer;
static unsigned short far *render_real;
static int render_active;
static unsigned char cursor_start,cursor_end,cursor_x,cursor_y;
static int run_node;
static char run_command[MAX_CMD];
static int prompt_macro_pending=0;
static int time_macro_pending=0;
static char config_file[MAX_CMD];
static char backup_file[MAX_CMD];
static char temp_file[MAX_CMD];
static char backup_temp_file[MAX_CMD];
static char bad_file[MAX_CMD];
static char appearance_file[MAX_CMD];
static char appearance_temp_file[MAX_CMD];
static char logos_file[MAX_CMD];
static char font_file[MAX_CMD];
static char collections_file[MAX_CMD];
static char program_dir[MAX_CMD];
static int font_is_vga(void);
static int font_preview(unsigned char id);
static int font_commit(unsigned char id);
static void font_restore(void);
static void launchui_install(void);
static void launchui_rebase(void);
static void launchui_restore(void);
static void shortcut_refresh(void);
static int shortcut_set_dialog(void);
static int shortcut_loadhigh_supported(void);
static int shortcut_unload(void);
static int shortcut_activate(void);
static void shortcut_idle_sync(void);
static int children(int parent,int *list);
static void delete_tree(int node);
static char write_path[MAX_CMD];
static unsigned char copy_buffer[512];
static unsigned char key_shift;
static unsigned char key_scan;
static int mouse_present;
static int mouse_visible;
static int mouse_glyph_saved;
static unsigned char mouse_old_glyph[32];
static int mouse_target_saved;
static unsigned char mouse_old_target[32];
static int dialog_close_x=-1,dialog_close_y=-1;
static char remove_item_name[MAX_TITLE];
static int remove_item_colour=0;
static unsigned mouse_last_buttons;
static unsigned mouse_raw_x,mouse_raw_y;
static int added_visible_node;
static int config_shortcut_active,config_shortcut_changed;
static char config_shortcut_combination[64];

#define BUILTIN_EXPLORE (-3)
#define BUILTIN_COLLECTIONS (-5)
#define BUILTIN_POWER (-2)
#define BUILTIN_CONFIG (-4)
#define MINUTE_TICKS 1092UL
#define MOUSE_MOVED 0x8000U
#define CONFIG_TAB_COUNT 6
#define CONFIG_CONTROL_BASE 6

typedef struct {
  char name[13];
  unsigned char directory;
} EXPLORE_ENTRY;

static EXPLORE_ENTRY explore_entries[MAX_EXPLORE_ENTRIES];
static int explore_count;
static unsigned char explore_drive_symbols[26];
static int explore_drive_positions[26];

static void cursor_restore(void);
static void mouse_stop(void);
static void mouse_pointer_restore(void);
static unsigned mouse_poll(int *column,int *row);
void wait_vertical_retrace(void);

static void video_init(void)
{
  unsigned char far *m = (unsigned char far *)MAKE_FP(0x40,0x49);
  unsigned short far *c = (unsigned short far *)MAKE_FP(0x40,0x4A);
  unsigned char far *r = (unsigned char far *)MAKE_FP(0x40,0x84);
  screen_cols = *c;
  if (screen_cols <= 0 || screen_cols > 80) screen_cols = 80;
  screen_rows = (*r >= 24 && *r < 60) ? *r + 1 : 25;
  video = (unsigned short far *)MAKE_FP((*m == 7) ? 0xB000 : 0xB800,0);
  launchui_install();
}

static int save_screen(void)
{
  unsigned i,n=(unsigned)(screen_cols*screen_rows);
  saved=(unsigned short far *)_fmalloc(n*2U);
  if(!saved)return 0;
  for (i=0; i<n; ++i) saved[i] = video[i];
  render_buffer=(unsigned short far *)_fmalloc(n*2U);render_real=0;render_active=0;
  return 1;
}

static void render_begin(void)
{
  unsigned i,n;if(render_active||!render_buffer)return;n=(unsigned)(screen_cols*screen_rows);
  render_real=video;for(i=0;i<n;i++)render_buffer[i]=render_real[i];video=render_buffer;render_active=1;
}
static void render_end(void)
{
  unsigned i,n;
  if(!render_active)return;
  /* Keep the screen pointer far throughout.  Under Microsoft C 7.0 medium
     model, the previous comma-declared automatic pointer was converted to a
     near pointer (C4759), discarding B800h/B000h and causing R6001. */
  video=render_real;
  render_active=0;
  n=(unsigned)(screen_cols*screen_rows);
  wait_vertical_retrace();
  for(i=0;i<n;i++)if(video[i]!=render_buffer[i])video[i]=render_buffer[i];
  render_real=0;
}

static void restore_screen(void)
{
  int i, n = screen_cols * screen_rows;if(render_active)render_end();
  if(!saved)return;
  for (i=0; i<n; ++i) video[i] = saved[i];
}

static void close_menu(void)
{
  mouse_stop();mouse_pointer_restore();
  launchui_restore();
  restore_screen();
  cursor_restore();
  if(render_buffer){_ffree(render_buffer);render_buffer=0;}
  _ffree(saved);saved=0;
}

static void cell(int x,int y,int ch,int at)
{
  unsigned short value;
  if (x<0 || x>=screen_cols || y<0 || y>=screen_rows) return;
  /* Match the Accessories/Games renderer: repeated dialog paints are common,
     but unchanged text cells must not be written back to video memory. */
  value=(unsigned short)(((unsigned short)(at&255)<<8)|(unsigned char)ch);
  if(video[y*screen_cols+x]!=value)video[y*screen_cols+x]=value;
}

void wait_vertical_retrace(void)
{
  unsigned port=(*(unsigned far *)MAKE_FP(0x40,0x63))+6;
  unsigned count=0xFFFF;
  while((_inp(port)&8) && --count);
  count=0xFFFF;while(!(_inp(port)&8) && --count);
}

static void row_attribute(int x,int y,int width,int at)
{
  int i;unsigned short value;
  if(y<0 || y>=screen_rows)return;
  for(i=0;i<width && x+i<screen_cols;i++)if(x+i>=0){
    value=video[y*screen_cols+x+i];
    if((unsigned char)(value>>8)!=(unsigned char)at)
      video[y*screen_cols+x+i]=(unsigned short)((value&255)|((unsigned short)(at&255)<<8));
  }
}

static void textout(int x,int y,const char *s,int at,int width)
{
  int i,ended=0;
  for (i=0;i<width;i++) {
    if (!ended && !s[i]) ended=1;
    cell(x+i,y,ended ? ' ' : s[i],at);
  }
}

static void cursor_hide(void)
{
  union REGS r;
  r.h.ah=3; r.h.bh=0; int86(0x10,&r,&r);
  cursor_start=r.h.ch; cursor_end=r.h.cl;cursor_x=r.h.dl;cursor_y=r.h.dh;
  r.h.ah=1; r.h.ch=0x20; r.h.cl=0; int86(0x10,&r,&r);
}

static void cursor_restore(void)
{
  union REGS r;
  r.h.ah=1; r.h.ch=cursor_start; r.h.cl=cursor_end;
  int86(0x10,&r,&r);
  r.h.ah=2;r.h.bh=0;r.h.dh=cursor_y;r.h.dl=cursor_x;int86(0x10,&r,&r);
}

static void edit_caret_hide(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.h.ah=1;r.h.ch=0x20;r.h.cl=0;int86(0x10,&r,&r);
}
static void edit_caret_set(int x,int y)
{
  union REGS r;if(x<0||x>=screen_cols||y<0||y>=screen_rows){edit_caret_hide();return;}
  memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=0;r.h.dh=(unsigned char)y;r.h.dl=(unsigned char)x;int86(0x10,&r,&r);
  r.h.ah=1;r.h.ch=0x0D;r.h.cl=0x0F;int86(0x10,&r,&r);
}

static void box(int x,int y,int w,int h,const char *title,int title_attr,int title_bg,int title_border)
{
  int i,j,len,top_border=ATTR(title_bg,title_border),top_title=ATTR(title_bg,title_attr&15);
  for(j=0;j<h;j++)for(i=0;i<w;i++)cell(x+i,y+j,' ',C_MENU_BACKGROUND);
  for(i=0;i<w;i++)cell(x+i,y,211,top_border);
  cell(x,y+h-1,192,C_BORDER);cell(x+w-1,y+h-1,217,C_BORDER);for(i=1;i<w-1;i++)cell(x+i,y+h-1,196,C_BORDER);for(j=1;j<h-1;j++){cell(x,y+j,179,C_BORDER);cell(x+w-1,y+j,179,C_BORDER);}
  if(title&&*title){len=strlen(title);if(len>w-6)len=w-6;cell(x+2,y,' ',top_title);textout(x+3,y,title,top_title,len);cell(x+3+len,y,' ',top_title);}
}

static void subdialog_shadow(int x,int y,int w,int h)
{
  int i,j; unsigned short v;
  /* Classic Launch! faux drop shadow: two cells on the right and one row
     below.  Preserve the underlying character and darken its attribute. */
  for(j=1;j<=h;j++) for(i=0;i<2;i++)
    if(x+w+i<screen_cols && y+j<screen_rows){
      v=video[(y+j)*screen_cols+x+w+i];
      cell(x+w+i,y+j,v&255,C_FAUX_SHADOW);
    }
  if(y+h<screen_rows) for(i=1;i<=w+1;i++)
    if(x+i<screen_cols){
      v=video[(y+h)*screen_cols+x+i];
      cell(x+i,y+h,v&255,C_FAUX_SHADOW);
    }
}

static void toolbar_divider(int x,int y,int width);

static void subdialog_box(int x,int y,int w,int h,const char *title)
{
  /* Sub-dialogs use the same titlebar treatment as all other Launch! dialogs. */
  box(x,y,w,h,title,C_TITLE,appearance.titlebar_bg,appearance.titlebar_fg);
  subdialog_shadow(x,y,w,h);
  if(h>=7)toolbar_divider(x,y+h-4,w);
  cell(x+w-5,y,200,ATTR(appearance.titlebar_bg,appearance.main_title));
  cell(x+w-4,y,201,ATTR(appearance.titlebar_bg,appearance.main_title));
  dialog_close_x=x+w-5;dialog_close_y=y;
}

static void dialog_box(int x,int y,int w,int h,const char *title)
{
  int i; unsigned short v;
  box(x,y,w,h,title,C_TITLE,appearance.titlebar_bg,appearance.titlebar_fg);
  if(h>=7)toolbar_divider(x,y+h-4,w);
  cell(x+w-5,y,200,ATTR(appearance.titlebar_bg,appearance.main_title));cell(x+w-4,y,201,ATTR(appearance.titlebar_bg,appearance.main_title));dialog_close_x=x+w-5;dialog_close_y=y;
  for(i=1;i<=h;i++){
    v=video[(y+i)*screen_cols+x+w];
    cell(x+w,y+i,v&255,C_FAUX_SHADOW);
  }
  for(i=1;i<=w;i++){
    v=video[(y+h)*screen_cols+x+i];
    cell(x+i,y+h,v&255,C_FAUX_SHADOW);
  }
}

static void menu_box(int x,int y,int w,int h,const char *title,int title_attr,int root)
{
  int i; unsigned short v;
  box(x,y,w,h,title,title_attr,root?appearance.titlebar_bg:appearance.background,
      root?appearance.titlebar_fg:appearance.border);
  if(!root){cell(x,y,218,C_BORDER);cell(x+w-1,y,191,C_BORDER);for(i=1;i<w-1;i++)cell(x+i,y,196,C_BORDER);}
  for(i=1;i<=h && x+w<screen_cols;i++) if(y+i<screen_rows){
    v=video[(y+i)*screen_cols+x+w];cell(x+w,y+i,v&255,C_FAUX_SHADOW);
  }
  for(i=1;i<=w;i++) if(y+h<screen_rows){
    v=video[(y+h)*screen_cols+x+i];cell(x+i,y+h,v&255,C_FAUX_SHADOW);
  }
}

static char *trim(char *s)
{
  char *e;
  while(*s && isspace((unsigned char)*s)) ++s;
  e=s+strlen(s); while(e>s && isspace((unsigned char)e[-1])) --e; *e=0;
  return s;
}

static int file_exists(const char *name)
{
  FILE *f=fopen(name,"rb");
  if(!f)return 0;
  fclose(f);return 1;
}

static void config_path(const char *program)
{
  const char *p,*q,*path; char exe[MAX_CMD],candidate[MAX_CMD];
  int n,len,has_extension,found;
  strncpy(exe,program,MAX_CMD-1);exe[MAX_CMD-1]=0;
  if(!strchr(exe,'\\') && !strchr(exe,'/') && !strchr(exe,':')){
    path=getenv("PATH");
    while(path && *path){
      p=strchr(path,';');len=p?(int)(p-path):(int)strlen(path);
      if(len>0 && len<MAX_CMD-14){
        strncpy(candidate,path,len);candidate[len]=0;
        if(candidate[len-1]!='\\' && candidate[len-1]!='/')strcat(candidate,"\\");
        strcat(candidate,exe);
        has_extension=strrchr(exe,'.')!=0;
        found=file_exists(candidate);
        if(!found && !has_extension){strcat(candidate,".EXE");found=file_exists(candidate);}
        if(found){
          strcpy(exe,candidate);break;
        }
      }
      if(!p)break;
      path=p+1;
    }
  }
  p=strrchr(exe,'\\');q=strrchr(exe,'/');
  if(!p || (q && q>p))p=q;
  if(p)n=(int)(p-exe)+1;
  else if(exe[0] && exe[1]==':')n=2;
  else n=0;
  if(n>MAX_CMD-11)n=0;
  strncpy(program_dir,exe,n);program_dir[n]=0;
  strncpy(config_file,exe,n);config_file[n]=0;strcat(config_file,"LAUNCH.MNU");
  strncpy(backup_file,exe,n);backup_file[n]=0;strcat(backup_file,"LAUNCH.BAK");
  strncpy(temp_file,exe,n);temp_file[n]=0;strcat(temp_file,"LAUNCH.$$$");
  strncpy(backup_temp_file,exe,n);backup_temp_file[n]=0;strcat(backup_temp_file,"LAUNCH.BK$");
  strncpy(bad_file,exe,n);bad_file[n]=0;strcat(bad_file,"LAUNCH.BAD");
  strncpy(appearance_file,exe,n);appearance_file[n]=0;strcat(appearance_file,"LAUNCH.CFG");
  strncpy(appearance_temp_file,exe,n);appearance_temp_file[n]=0;strcat(appearance_temp_file,"LAUNCH.CF$");
  strncpy(logos_file,exe,n);logos_file[n]=0;strcat(logos_file,"PWROFF.BMP");
  strncpy(font_file,exe,n);font_file[n]=0;strcat(font_file,"FONT.DAT");
  strncpy(collections_file,exe,n);collections_file[n]=0;strcat(collections_file,"ASSOC.CFG");
}

static int select_menu_file(const char *name)
{
  char base[MAX_CMD],*slash,*dot;int n;
  if(!name||!*name)return 0;
  if(strchr(name,'\\')||strchr(name,'/')||strchr(name,':')){
    if(strlen(name)>=MAX_CMD)return 0;
    strcpy(config_file,name);
  } else {
    if(strlen(program_dir)+strlen(name)>=MAX_CMD)return 0;
    strcpy(config_file,program_dir);strcat(config_file,name);
  }
  strcpy(base,config_file);slash=strrchr(base,'\\');dot=strrchr(base,'.');
  if(dot&&(!slash||dot>slash))*dot=0;
  n=strlen(base);if(n>MAX_CMD-6)return 0;
  strcpy(backup_file,base);strcat(backup_file,".BAK");
  strcpy(temp_file,base);strcat(temp_file,".$$$");
  strcpy(backup_temp_file,base);strcat(backup_temp_file,".BK$");
  strcpy(bad_file,base);strcat(bad_file,".BAD");return 1;
}

static int appearance_value(APPEARANCE *a,const char *key,int value)
{
  unsigned char *field=0;int limit=15;
  if(!stricmp(key,"BACKGROUND")){field=&a->background;limit=7;}
  else if(!stricmp(key,"BORDER"))field=&a->border;
  else if(!stricmp(key,"TITLEBAR_FG"))field=&a->titlebar_fg;
  else if(!stricmp(key,"TITLEBAR_BG")){field=&a->titlebar_bg;limit=7;}
  else if(!stricmp(key,"MAIN_TITLE"))field=&a->main_title;
  else if(!stricmp(key,"TITLES"))field=&a->titles;
  else if(!stricmp(key,"FOLDERS"))field=&a->folders;
  else if(!stricmp(key,"LAUNCHERS"))field=&a->launchers;
  else if(!stricmp(key,"SELECTED_FG"))field=&a->selected_fg;
  else if(!stricmp(key,"SELECTED_BG")){field=&a->selected_bg;limit=7;}
  else if(!stricmp(key,"CONTROLS_FG"))field=&a->controls_fg;
  else if(!stricmp(key,"CONTROLS_BG")){field=&a->controls_bg;limit=7;}
  else if(!stricmp(key,"LABELS"))field=&a->labels;
  else if(!stricmp(key,"MENU_TOP")){field=&a->menu_top;limit=1;}
  else if(!stricmp(key,"SHOW_COLLECTIONS")){field=&a->show_collections;limit=1;}
  else if(!stricmp(key,"SHOW_EXPLORE")){field=&a->show_explore;limit=1;}
  else if(!stricmp(key,"SHOW_POWER")){field=&a->show_power;limit=1;}
  else if(!stricmp(key,"SHOW_TIME")){field=&a->show_time;limit=1;}
  else if(!stricmp(key,"SHOW_SYSBAR")){field=&a->show_sysbar;limit=1;}
  else if(!stricmp(key,"SCREENSAVER")){
    field=&a->screensaver;limit=14;
  }
  else if(!stricmp(key,"SAVER_COLOR"))field=&a->saver_color;
  else if(!stricmp(key,"SAVER_DELAY")){field=&a->saver_delay;limit=3;}
  else if(!stricmp(key,"HOUR_12")){field=&a->hour_12;limit=1;}
  else if(!stricmp(key,"FONT_ID")){field=&a->font_id;limit=63;}
  else if(!stricmp(key,"FONT_PERSIST")){field=&a->font_persist;limit=1;}
  else if(!stricmp(key,"MOUSE_CURSOR")){field=&a->mouse_cursor;limit=2;}
  else if(!stricmp(key,"PROMPT_INACTIVITY")){field=&a->prompt_inactivity;limit=1;}
  if(!field || value<0 || value>limit)return 0;
  *field=(unsigned char)value;return 1;
}

static int load_appearance(void)
{
  FILE *f;static char line[80];char *p,*q,*end;long value;APPEARANCE loaded;int title_fg_set=0,title_bg_set=0;
  appearance=default_appearance;
  f=fopen(appearance_file,"rt");if(!f)return 1;
  loaded=default_appearance;
  while(fgets(line,sizeof(line),f)){
    p=trim(line);if(!*p || *p==';' || *p=='#')continue;
    q=strchr(p,'=');if(!q){fclose(f);return 0;}
    *q++=0;q=trim(q);p=trim(p);
    value=strtol(q,&end,10);end=trim(end);
    if(!*q || *end || !appearance_value(&loaded,p,(int)value)){fclose(f);return 0;}
    if(!stricmp(p,"TITLEBAR_FG"))title_fg_set=1;
    if(!stricmp(p,"TITLEBAR_BG"))title_bg_set=1;
  }
  if(ferror(f)){fclose(f);return 0;}
  fclose(f);if(!title_fg_set)loaded.titlebar_fg=loaded.border;if(!title_bg_set)loaded.titlebar_bg=loaded.controls_bg;appearance=loaded;return 1;
}

static int find_folder(const char *title,int parent)
{
  int i;
  for(i=0;i<node_count;i++) if(nodes[i].active && nodes[i].folder && nodes[i].parent==parent && !stricmp(nodes[i].title,title)) return i;
  return -1;
}

static int next_order(int parent)
{
  int i,n=0;
  for(i=0;i<node_count;i++) if(nodes[i].active && nodes[i].parent==parent && (int)nodes[i].order>=n) n=(int)nodes[i].order+1;
  return n;
}

static int add_node(const char *title,const char *cmd,int parent,int folder)
{
  int n,ord;
  if(!*title) return -1;
  ord=next_order(parent);
  for(n=0;n<node_count;n++) if(!nodes[n].active) break;
  if(n==node_count){if(node_count>=MAX_NODES)return -1;node_count++;}
  strncpy(nodes[n].title,title,MAX_TITLE-1); nodes[n].title[MAX_TITLE-1]=0;
  strncpy(nodes[n].command,cmd ? cmd : "",MAX_CMD-1); nodes[n].command[MAX_CMD-1]=0;
  nodes[n].parent=parent; nodes[n].folder=(unsigned char)folder;
  nodes[n].separator=0;
  nodes[n].active=1; nodes[n].order=(unsigned char)ord;
  nodes[n].press_enter=1;nodes[n].change_dir=0;nodes[n].prompt_params=0;nodes[n].add_path=0;
  return n;
}

#pragma code_seg("DEFAULT_TEXT")
/* Default menu construction is deliberately generated at runtime.  No
   LAUNCH.MNU template is shipped: this prevents a developer's personal menu
   from ever becoming an installation default and lets DOS-family-specific
   entries be selected on the machine where Launch! is installed. */
#define DOS_FAMILY_OTHER 0
#define DOS_FAMILY_MS    1
#define DOS_FAMILY_PC    2
#define DOS_FAMILY_DR    3
#define DOS_FAMILY_FREE  4

typedef struct {
  int family;
  int major;
  int minor;
} DOS_INFO;

static int contains_text_ci(const char *text,const char *wanted)
{
  char copy[MAX_CMD];int i;
  if(!text)return 0;
  strncpy(copy,text,MAX_CMD-1);copy[MAX_CMD-1]=0;
  for(i=0;copy[i];i++)copy[i]=(char)toupper((unsigned char)copy[i]);
  return strstr(copy,wanted)!=0;
}

static int dr_dos_version_id(void)
{
  unsigned result=0,found=0;
#ifndef __GNUC__
  /* DR DOS requires CF set before AX=4452h so an unsupported DOS cannot
     accidentally look like a successful detection. */
  _asm {
    mov ax,4452h
    stc
    int 21h
    jc dr_version_done
    mov result,ax
    mov found,1
  dr_version_done:
  }
#else
  (void)result;
#endif
  return found?(int)(result&0x00FFU):-1;
}

static void detect_dos_info(DOS_INFO *info)
{
  union REGS inregs,outregs;char *os,*comspec;
  int dr_id,is_dr,is_free,is_pc,oem;
  memset(&inregs,0,sizeof(inregs));inregs.h.ah=0x30;inregs.h.al=0;
  intdos(&inregs,&outregs);
  info->major=outregs.h.al;info->minor=outregs.h.ah;oem=outregs.h.bh;
  os=getenv("OS");comspec=getenv("COMSPEC");
  is_free=(oem==0xFD)||contains_text_ci(os,"FREEDOS")||
          contains_text_ci(comspec,"FREECOM")||getenv("FREEDOS")!=0;
  dr_id=dr_dos_version_id();
  is_dr=(dr_id>=0||oem==0xEE||oem==0xEF);
  is_pc=(oem==0);
  if(!is_dr && info->major>=5){
    memset(&inregs,0,sizeof(inregs));inregs.x.ax=0x3306;intdos(&inregs,&outregs);
    if(outregs.h.bl>=5 && outregs.h.bl<100){info->major=outregs.h.bl;info->minor=outregs.h.bh;}
  }
  if(is_dr && dr_id>=0){
    if(dr_id==0x65){info->major=5;info->minor=0;}
    else if((dr_id>=0x66&&dr_id<=0x71)){info->major=6;info->minor=0;}
    else if(dr_id>=0x72){info->major=7;info->minor=0;}
    else {info->major=3;info->minor=0;}
  }
  if(is_free)info->family=DOS_FAMILY_FREE;
  else if(is_dr)info->family=DOS_FAMILY_DR;
  else if(is_pc)info->family=DOS_FAMILY_PC;
  else info->family=DOS_FAMILY_MS;
}

static int command_file_at(const char *directory,const char *command,char *resolved)
{
  static const char *exts[]={".COM",".EXE",".BAT",0};char candidate[MAX_CMD];int i,n;
  for(i=0;exts[i];i++){
    candidate[0]=0;
    if(directory&&*directory){
      n=(int)strlen(directory);
      if(n+(int)strlen(command)+6>=MAX_CMD)continue;
      strcpy(candidate,directory);
      if(candidate[n-1]!='\\'&&candidate[n-1]!='/')strcat(candidate,"\\");
    }
    strcat(candidate,command);strcat(candidate,exts[i]);
    if(file_exists(candidate)){
      if(resolved){strncpy(resolved,candidate,MAX_CMD-1);resolved[MAX_CMD-1]=0;}
      return 1;
    }
  }
  return 0;
}

static int find_dos_command(const char *command,char *resolved)
{
  char candidate[MAX_CMD],dir[MAX_CMD],*comspec,*slash;const char *path,*end;int n;
  if(command_file_at("",command,0)){
    if(resolved){strncpy(resolved,command,MAX_CMD-1);resolved[MAX_CMD-1]=0;}
    return 1;
  }
  path=getenv("PATH");
  while(path&&*path){
    end=strchr(path,';');n=end?(int)(end-path):(int)strlen(path);
    if(n>0&&n<MAX_CMD){
      strncpy(dir,path,n);dir[n]=0;
      while(*dir==' ')memmove(dir,dir+1,strlen(dir));
      n=(int)strlen(dir);while(n>0&&dir[n-1]==' ')dir[--n]=0;
      if(command_file_at(dir,command,0)){
        if(resolved){strncpy(resolved,command,MAX_CMD-1);resolved[MAX_CMD-1]=0;}
        return 1;
      }
    }
    if(!end)break;path=end+1;
  }
  comspec=getenv("COMSPEC");
  if(comspec&&*comspec){
    strncpy(candidate,comspec,MAX_CMD-1);candidate[MAX_CMD-1]=0;
    slash=strrchr(candidate,'\\');if(!slash)slash=strrchr(candidate,'/');
    if(slash){*slash=0;if(command_file_at(candidate,command,resolved))return 1;}
  }
  return 0;
}

static int sibling_exists(const char *name)
{
  char path[MAX_CMD];
  if(strlen(program_dir)+strlen(name)>=MAX_CMD)return 0;
  strcpy(path,program_dir);strcat(path,name);return file_exists(path);
}

static int launcher_exists_in(int parent,const char *title,const char *command)
{
  int i;
  for(i=0;i<node_count;i++)if(nodes[i].active&&!nodes[i].folder&&!nodes[i].separator&&nodes[i].parent==parent){
    if(title&&*title&&!stricmp(nodes[i].title,title))return 1;
    if(command&&*command&&!stricmp(nodes[i].command,command))return 1;
  }
  return 0;
}

static int ensure_launcher(int parent,const char *title,const char *command,int press_enter,int *changed)
{
  int node;
  if(launcher_exists_in(parent,title,command))return 1;
  node=add_node(title,command,parent,0);if(node<0)return 0;
  nodes[node].press_enter=(unsigned char)press_enter;
  nodes[node].change_dir=0;nodes[node].prompt_params=0;nodes[node].add_path=0;
  *changed=1;return 1;
}

static int ensure_dos_launcher(int parent,const char *title,const char *command,int press_enter,int *changed)
{
  char resolved[MAX_CMD];
  if(!find_dos_command(command,resolved))return 1;
  return ensure_launcher(parent,title,resolved,press_enter,changed);
}

static int ensure_default_folder(const char *title,int *changed)
{
  int folder=find_folder(title,-1);
  if(folder>=0)return folder;
  folder=add_node(title,"",-1,1);if(folder>=0)*changed=1;return folder;
}

static void sort_default_folder_alpha(int parent,int *changed)
{
  int list[MAX_CHILD],n,i,j,t,moved=0;
  n=children(parent,list);
  for(i=0;i<n-1;i++)for(j=i+1;j<n;j++)
    if(stricmp(nodes[list[i]].title,nodes[list[j]].title)>0){
      t=list[i];list[i]=list[j];list[j]=t;moved=1;
    }
  for(i=0;i<n;i++){
    if(nodes[list[i]].order!=(unsigned char)i)moved=1;
    nodes[list[i]].order=(unsigned char)i;
  }
  if(moved)*changed=1;
}

static int merge_default_menu_nodes(int *changed)
{
  DOS_INFO dos;int folder;
  *changed=0;

  /* 3.73: SysBar is built into Core again.  Remove the short-lived
     external !SYSBAR launcher from menus upgraded from early 3.73 tests. */
  {
    int i;
    for(i=0;i<node_count;i++){
      if(nodes[i].active&&!nodes[i].folder&&!nodes[i].separator&&
         (!stricmp(nodes[i].title,"SysBar")||!strnicmp(nodes[i].command,"!SYSBAR",7))){
        delete_tree(i);
        *changed=1;
      }
    }
  }

  /* 3.65: migrate the former System Info launcher to DOS Fetch. */
  if(sibling_exists("!DFETCH.EXE")){
    int i;for(i=0;i<node_count;i++)if(nodes[i].active&&!nodes[i].folder&&!nodes[i].separator&&
      (!stricmp(nodes[i].command,"!SYSINFO")||!stricmp(nodes[i].title,"System Info"))){
      strcpy(nodes[i].title,"DOS Fetch");strcpy(nodes[i].command,"!DFETCH");*changed=1;
    }
  }

  if(sibling_exists("!CAL.EXE")||sibling_exists("!CALC.EXE")||sibling_exists("!DRAW.EXE")||sibling_exists("!MKDOWN.EXE")||
     sibling_exists("!JOURNAL.EXE")||sibling_exists("!NOTE.EXE")||sibling_exists("!STACK.EXE")||
     sibling_exists("!DFETCH.EXE")||sibling_exists("!TODOS.EXE")){
    folder=ensure_default_folder("Accessories",changed);if(folder<0)return 0;
    if(sibling_exists("!CALC.EXE")&&!ensure_launcher(folder,"Calculator","!CALC",1,changed))return 0;
    if(sibling_exists("!CAL.EXE")&&!ensure_launcher(folder,"Calendar","!CAL",1,changed))return 0;
    if(sibling_exists("!STACK.EXE")&&!ensure_launcher(folder,"Card Stack","!STACK",1,changed))return 0;
    if(sibling_exists("!JOURNAL.EXE")&&!ensure_launcher(folder,"Journal","!JOURNAL",1,changed))return 0;
    if(sibling_exists("!MKDOWN.EXE")&&!ensure_launcher(folder,"Markdown","!MKDOWN",1,changed))return 0;
    if(sibling_exists("!NOTE.EXE")&&!ensure_launcher(folder,"Note","!NOTE",1,changed))return 0;
    if(sibling_exists("!DRAW.EXE")&&!ensure_launcher(folder,"Pixel Draw","!DRAW",1,changed))return 0;
    if(sibling_exists("!DFETCH.EXE")&&!ensure_launcher(folder,"DOS Fetch","!DFETCH",1,changed))return 0;
    if(sibling_exists("!TODOS.EXE")&&!ensure_launcher(folder,"To-Dos","!TODOS",1,changed))return 0;
  }

  if(sibling_exists("!BOXES.EXE")||sibling_exists("!FCELL.EXE")||sibling_exists("!POP.EXE")||
     sibling_exists("!SNAKE.EXE")||sibling_exists("!SOL.EXE")||sibling_exists("!PLUMB.EXE")||
     sibling_exists("!WORDZ.EXE")||sibling_exists("!TYPO.EXE")){
    folder=ensure_default_folder("Games",changed);if(folder<0)return 0;
    if(sibling_exists("!BOXES.EXE")&&!ensure_launcher(folder,"Boxes","!BOXES",1,changed))return 0;
    if(sibling_exists("!FCELL.EXE")&&!ensure_launcher(folder,"FreeCell","!FCELL",1,changed))return 0;
    if(sibling_exists("!POP.EXE")&&!ensure_launcher(folder,"Pop","!POP",1,changed))return 0;
    if(sibling_exists("!SNAKE.EXE")&&!ensure_launcher(folder,"Snake","!SNAKE",1,changed))return 0;
    if(sibling_exists("!SOL.EXE")&&!ensure_launcher(folder,"Solitaire","!SOL",1,changed))return 0;
    if(sibling_exists("!PLUMB.EXE")&&!ensure_launcher(folder,"Plumb","!PLUMB",1,changed))return 0;
    if(sibling_exists("!WORDZ.EXE")&&!ensure_launcher(folder,"Wordz","!WORDZ",1,changed))return 0;
    if(sibling_exists("!TYPO.EXE")&&!ensure_launcher(folder,"Typo","!TYPO",1,changed))return 0;
  }

  folder=ensure_default_folder("DOS Commands",changed);if(folder<0)return 0;
  detect_dos_info(&dos);
  if(!ensure_dos_launcher(folder,"Check Disk","CHKDSK",1,changed))return 0;
  if(!ensure_dos_launcher(folder,"Format Disk","FORMAT",0,changed))return 0;
  if(!ensure_dos_launcher(folder,"Partition Disk","FDISK",1,changed))return 0;
  if(!ensure_dos_launcher(folder,"System Disk","SYS",0,changed))return 0;
  if(dos.major>=4||dos.family==DOS_FAMILY_FREE)
    if(!ensure_dos_launcher(folder,"Memory Information","MEM",1,changed))return 0;
  if(!ensure_dos_launcher(folder,"File Attributes","ATTRIB",0,changed))return 0;
  if(!ensure_dos_launcher(folder,"Find Text","FIND",0,changed))return 0;
  if(!ensure_dos_launcher(folder,"Sort Text","SORT",0,changed))return 0;
  if(!ensure_dos_launcher(folder,"XCopy","XCOPY",0,changed))return 0;
  if(!ensure_dos_launcher(folder,"Delete Tree","DELTREE",0,changed))return 0;
  if(!ensure_dos_launcher(folder,"Undelete Files","UNDELETE",1,changed))return 0;
  if(!ensure_dos_launcher(folder,"Scan Disk","SCANDISK",1,changed))return 0;

  if(dos.family==DOS_FAMILY_DR){
    if(!ensure_dos_launcher(folder,"DR DOS Editor","EDITOR",1,changed))return 0;
    if((dos.major==5||dos.major==6)&&!ensure_dos_launcher(folder,"ViewMAX","VIEWMAX",1,changed))return 0;
    if(dos.major>=6&&!ensure_dos_launcher(folder,"TaskMAX","TASKMAX",1,changed))return 0;
    if(!ensure_dos_launcher(folder,"Disk Optimizer","DISKOPT",1,changed))return 0;
    if(!ensure_dos_launcher(folder,"DOSBook Help","DOSBOOK",1,changed))return 0;
  } else {
    if(!ensure_dos_launcher(folder,"Defragment Disk","DEFRAG",1,changed))return 0;
    if(dos.family==DOS_FAMILY_PC){
      if(!ensure_dos_launcher(folder,"E Editor","E",1,changed))return 0;
      if(dos.major>=7&&!ensure_dos_launcher(folder,"PC DOS Viewer","VIEW",1,changed))return 0;
      if(!ensure_dos_launcher(folder,"DOS Shell","DOSSHELL",1,changed))return 0;
      if(!ensure_dos_launcher(folder,"Help","HELP",1,changed))return 0;
    } else if(dos.family==DOS_FAMILY_MS){
      if(!ensure_dos_launcher(folder,"MS-DOS Editor","EDIT",1,changed))return 0;
      if(!ensure_dos_launcher(folder,"DOS Shell","DOSSHELL",1,changed))return 0;
      if(!ensure_dos_launcher(folder,"Help","HELP",1,changed))return 0;
    } else if(dos.family==DOS_FAMILY_FREE){
      if(!ensure_dos_launcher(folder,"FreeDOS Edit","EDIT",1,changed))return 0;
      if(!ensure_dos_launcher(folder,"FDIMPLES Packages","FDIMPLES",1,changed))return 0;
      if(!ensure_dos_launcher(folder,"Help","HELP",1,changed))return 0;
    }
  }
  /* Keep the generated/merged DOS Commands collection deterministic and easy
     to scan regardless of DOS family or which optional utilities are present. */
  sort_default_folder_alpha(folder,changed);
  return 1;
}

#pragma code_seg()

static int section_parent(char *path)
{
  char *p,*q; int parent=-1,n;
  p=path;
  if(!strnicmp(p,"Launcher",8) && (p[8]==0 || p[8]=='\\')){
    p+=8;
    if(*p=='\\') ++p;
    if(!*p) return -1;
  }
  while(*p){
    q=strchr(p,'\\'); if(q) *q=0;
    n=find_folder(trim(p),parent); if(n<0) n=add_node(trim(p),"",parent,1);
    parent=n; if(!q) break; p=q+1;
  }
  return parent;
}

static int load_config(const char *name)
{
  FILE *f; static char line[256];char *p,*q; int parent=-1;
  int valid=1,root_seen=0,in_section=0,len;
  node_count=0;
  f=fopen(name,"rt"); if(!f) return 0;
  while(fgets(line,sizeof(line),f)){
    len=strlen(line);
    if(len==(int)sizeof(line)-1 && line[len-1]!='\n' && !feof(f)){valid=0;break;}
    p=trim(line); if(!*p || *p==';' || *p=='#') continue;
    if(*p=='[' && (q=strchr(p,']'))!=0 && !trim(q+1)[0]){
      *q=0;p=trim(p+1);
      if(strnicmp(p,"Launcher",8) || (p[8] && p[8]!='\\')){valid=0;break;}
      if(!stricmp(p,"Launcher")){parent=-1;root_seen=1;}
      else {parent=section_parent(p);if(parent<0){valid=0;break;}}
      in_section=1;continue;
    }
    if(!in_section){valid=0;break;}
    if(!strnicmp(p,"ITEM=",5)){
      char *r;int node,enter=1,cd=0,prompt=0,addpath=0,flags[4],flag_count=0;
      p=trim(p+5); q=strchr(p,'|');
      if(q && *trim(p)){
        *q++=0;
        while(flag_count<4 && (r=strrchr(q,'|'))!=0 &&
              (r[1]=='0' || r[1]=='1') && r[2]==0){
          flags[flag_count++]=r[1]-'0';*r=0;
        }
        if(flag_count>=1)cd=flags[0];
        if(flag_count>=2){enter=flags[1];cd=flags[0];}
        if(flag_count>=3){enter=flags[2];cd=flags[1];prompt=flags[0];}
        if(flag_count>=4){enter=flags[3];cd=flags[2];prompt=flags[1];addpath=flags[0];}
        p=trim(p);q=trim(q);
        if(!*p || !*q){valid=0;break;}
        node=add_node(p,q,parent,0);
        if(node<0){valid=0;break;}
        nodes[node].press_enter=(unsigned char)enter;nodes[node].change_dir=(unsigned char)cd;
        nodes[node].prompt_params=(unsigned char)prompt;nodes[node].add_path=(unsigned char)addpath;
      } else {valid=0;break;}
    }
    else if(!strnicmp(p,"FOLDER=",7)){
      p=trim(p+7);
      if(!*p){valid=0;break;}
      if(find_folder(p,parent)<0 && add_node(p,"",parent,1)<0){valid=0;break;}
    }
    else if(!stricmp(p,"SEPARATOR") || !stricmp(p,"SEPARATOR=")){
      int node=add_node("-","",parent,0);
      if(node<0){valid=0;break;}
      nodes[node].separator=1;
    }
    else {valid=0;break;}
  }
  if(ferror(f))valid=0;
  fclose(f);
  if(!valid || !root_seen){node_count=0;return 0;}
  return 1;
}

static int children(int parent,int *list)
{
  int i,j,t,n=0;
  for(i=0;i<node_count && n<MAX_CHILD;i++) if(nodes[i].active && nodes[i].parent==parent) list[n++]=i;
  for(i=0;i<n-1;i++) for(j=i+1;j<n;j++)
    if(nodes[list[j]].order<nodes[list[i]].order){t=list[i];list[i]=list[j];list[j]=t;}
  return n;
}

static int menu_children(int parent,int *list)
{
  int n=children(parent,list);
  if(parent==-1 && appearance.show_collections && n<MAX_CHILD)list[n++]=BUILTIN_COLLECTIONS;
  if(parent==-1 && appearance.show_explore && n<MAX_CHILD)list[n++]=BUILTIN_EXPLORE;
  if(parent==-1 && appearance.show_power && n<MAX_CHILD)list[n++]=BUILTIN_POWER;
  return n;
}

static int keyread(void)
{
  unsigned short far *head=(unsigned short far *)MAKE_FP(0x40,0x1A);
  unsigned short far *tail=(unsigned short far *)MAKE_FP(0x40,0x1C);
  unsigned short pos,next,word;
  while(*head==*tail) ;
  _disable();
  pos=*head; word=*(unsigned short far *)MAKE_FP(0x40,pos);
  next=pos+2;if(next>=0x3E)next=0x1E;*head=next;
  key_shift=*(unsigned char far *)MAKE_FP(0x40,0x17);
  _enable();
  key_scan=(unsigned char)(word>>8);
  if((word&255)==0 || (word&255)==0xE0)return key_scan<<8;
  return word&255;
}

static int key_waiting(void)
{
  unsigned short far *head=(unsigned short far *)MAKE_FP(0x40,0x1A);
  unsigned short far *tail=(unsigned short far *)MAKE_FP(0x40,0x1C);
  return *head!=*tail;
}

static unsigned char draw_clock(int y,unsigned char last_second)
{
  static unsigned long last_tick=0xFFFFFFFFUL;
  unsigned long tick=*(unsigned long far *)MAKE_FP(0x40,0x6C);
  union REGS r;char value[9],separator;int x,len,i,hour;
  if(last_second!=255 && tick==last_tick)return last_second;
  last_tick=tick;
  r.h.ah=0x2C;int86(0x21,&r,&r);
  if(r.h.dh!=last_second){
    separator=(r.h.dh&1)?' ':':';hour=r.h.ch;
    if(appearance.hour_12){
      const char *period=hour>=12?"PM":"AM";hour%=12;if(!hour)hour=12;
      sprintf(value,"%u%c%02u %s",hour,separator,r.h.cl,period);
    } else sprintf(value,"%02u%c%02u",hour,separator,r.h.cl);
    for(i=8;i<=18;i++)cell(i,y,196,C_BORDER);
    len=strlen(value);x=18-len;
    cell(x-1,y,180,C_BORDER);textout(x,y,value,C_TITLE,len);cell(x+len,y,195,C_BORDER);
  }
  return r.h.dh;
}

unsigned long bios_ticks(void)
{
  return *(unsigned long far *)MAKE_FP(0x40,0x6C);
}

static unsigned long elapsed_ticks(unsigned long start,unsigned long now)
{
  if(now>=start)return now-start;
  return (0x1800B0UL-start)+now;
}

static unsigned long saver_delay_ticks(void)
{
  static const unsigned char minutes[4]={1,5,15,30};
  return (unsigned long)minutes[appearance.saver_delay&3]*MINUTE_TICKS;
}

/* Exact 640x350 raster spans generated from the supplied VFD SVG artwork. */
static const unsigned char segment_mask[10]={
  0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F
};

static unsigned char far *ega_memory=(unsigned char far *)MAKE_FP(0xA000,0);

typedef struct { int x,y,z,px,py,qx,qy; } WARP_STAR;
typedef struct { int x,y,direction;unsigned char colour; } PIPE_HEAD;
typedef struct { int x,y,dx,dy,mass; } HALFTONE_MASS;
typedef struct {
  int x[4],y[4],dx[4],dy[4];
  int old_x[12][4],old_y[12][4];
  unsigned char old_colour[12],used[12],slot,colour;
} MYSTIFY_SHAPE;
typedef struct {
  unsigned short x,y;
  unsigned short life;
  unsigned char colour,base;
} NIGHT_PIXEL;
typedef struct {
  int x,y;
  unsigned short wait;
  unsigned char on,active;
} NIGHT_BEACON;
#define NIGHT_PIXEL_COUNT 1152
#define NIGHT_BEACON_COUNT 24
#define HALFTONE_COLS 40
#define HALFTONE_ROWS 25
#define HALFTONE_MASSES 6
static WARP_STAR warp_stars[56];
static PIPE_HEAD pipe_heads[5];
static MYSTIFY_SHAPE mystify_shapes[2];
static unsigned short skyline_top[640];
static unsigned char skyline_colour[640];
static NIGHT_PIXEL night_pixels[NIGHT_PIXEL_COUNT];
static NIGHT_BEACON night_beacons[NIGHT_BEACON_COUNT];
static HALFTONE_MASS halftone_masses[HALFTONE_MASSES];
static unsigned char halftone_previous[HALFTONE_COLS*HALFTONE_ROWS];
static unsigned long saver_random_state=1;

void ega_span(int y,int left,int right,unsigned char colour);
void ega_rectangle(int x,int y,int width,int height,unsigned char colour);
void ega_line(int x0,int y0,int x1,int y1,unsigned char colour);

unsigned saver_random(unsigned limit)
{
  saver_random_state=saver_random_state*1103515245UL+12345UL;
  return limit?(unsigned)((saver_random_state>>16)%limit):0;
}

static void night_pixel_draw(const NIGHT_PIXEL *pixel,int erase)
{
  unsigned char colour=erase?pixel->base:pixel->colour;
  if(pixel->base==0)ega_span(pixel->y,pixel->x,pixel->x,colour);
  else ega_rectangle(pixel->x,pixel->y,2,3,colour);
}

static void night_pixel_add(int x,int y,unsigned char colour,unsigned char base)
{
  int i;
  for(i=0;i<NIGHT_PIXEL_COUNT;i++)if(night_pixels[i].x==0xFFFF){
    night_pixels[i].x=(unsigned short)x;night_pixels[i].y=(unsigned short)y;
    night_pixels[i].life=(unsigned short)MINUTE_TICKS;
    night_pixels[i].colour=colour;night_pixels[i].base=base;
    night_pixel_draw(&night_pixels[i],0);return;
  }
}

static void night_pixels_decay(void)
{
  int i;NIGHT_PIXEL *pixel;
  for(i=0;i<NIGHT_PIXEL_COUNT;i++){
    pixel=&night_pixels[i];
    if(pixel->x!=0xFFFF){
      if(pixel->life){pixel->life--;continue;}
      if(saver_random(80)==0){
        night_pixel_draw(pixel,1);pixel->x=0xFFFF;
      }
    }
  }
}

static void draw_beacon(const NIGHT_BEACON *beacon,unsigned char colour)
{
  ega_span(beacon->y,beacon->x+1,beacon->x+2,colour);
  ega_span(beacon->y+1,beacon->x,beacon->x+3,colour);
  ega_span(beacon->y+2,beacon->x,beacon->x+3,colour);
  ega_span(beacon->y+3,beacon->x+1,beacon->x+2,colour);
}

static void add_beacon(int x,int y)
{
  int i;
  for(i=0;i<NIGHT_BEACON_COUNT;i++)if(!night_beacons[i].active){
    night_beacons[i].x=x;night_beacons[i].y=y;
    night_beacons[i].wait=(unsigned short)(182+saver_random(183));
    night_beacons[i].on=0;night_beacons[i].active=1;return;
  }
}

static void update_beacons(void)
{
  int i;NIGHT_BEACON *beacon;
  for(i=0;i<NIGHT_BEACON_COUNT;i++){
    beacon=&night_beacons[i];if(!beacon->active)continue;
    if(beacon->wait){beacon->wait--;continue;}
    if(beacon->on){
      draw_beacon(beacon,0);
      ega_span(beacon->y+2,beacon->x+2,beacon->x+2,8);
      beacon->on=0;
      beacon->wait=(unsigned short)(182+saver_random(183));
    } else {
      draw_beacon(beacon,12);beacon->on=1;beacon->wait=36;
    }
  }
}

void ega_span(int y,int left,int right,unsigned char colour)
{
  int first,last,b;unsigned char mask;unsigned offset;
  volatile unsigned char latch;
  if(y<0 || y>=350 || right<0 || left>=640)return;
  if(left<0)left=0;
  if(right>639)right=639;
  if(left>right)return;
  first=left>>3;last=right>>3;offset=(unsigned)(y*80+first);
  if(first==last){
    mask=(unsigned char)((0xFFu>>(left&7))&(0xFFu<<(7-(right&7))));
    _outpw(0x3CE,(unsigned)((mask<<8)|8));latch=ega_memory[offset];
    ega_memory[offset]=colour;(void)latch;return;
  }
  mask=(unsigned char)(0xFFu>>(left&7));
  _outpw(0x3CE,(unsigned)((mask<<8)|8));latch=ega_memory[offset];
  ega_memory[offset++]=colour;
  _outpw(0x3CE,0xFF08);
  for(b=first+1;b<last;b++){latch=ega_memory[offset];ega_memory[offset++]=colour;}
  mask=(unsigned char)(0xFFu<<(7-(right&7)));
  _outpw(0x3CE,(unsigned)((mask<<8)|8));latch=ega_memory[offset];
  ega_memory[offset]=colour;(void)latch;
}

void ega_rectangle(int x,int y,int width,int height,unsigned char colour)
{
  int row;for(row=0;row<height;row++)ega_span(y+row,x,x+width-1,colour);
}

void ega_line(int x0,int y0,int x1,int y1,unsigned char colour)
{
  int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1;
  int error=dx+dy,twice;
  for(;;){
    ega_span(y0,x0,x0,colour);if(x0==x1 && y0==y1)break;
    twice=error*2;
    if(twice>=dy){error+=dy;x0+=sx;}
    if(twice<=dx){error+=dx;y0+=sy;}
  }
}

static unsigned char ega_pixel(int x,int y)
{
  unsigned offset,mask,plane;unsigned char colour=0,value;
  if(x<0 || x>=640 || y<0 || y>=350)return 0;
  offset=(unsigned)(y*80+(x>>3));mask=0x80u>>(x&7);
  for(plane=0;plane<4;plane++){
    _outpw(0x3CE,(unsigned)((plane<<8)|4));value=ega_memory[offset];
    if(value&mask)colour|=(unsigned char)(1u<<plane);
  }
  return colour;
}

static void draw_clock_shape(int x,int y,const CLOCK_SHAPE *shape,
                             unsigned char colour)
{
  int row;const CLOCK_SPAN *span=shape->rows;
  for(row=0;row<(int)shape->count;row++,span++)if(span->left!=255)
    ega_span(y+shape->top+row,x+span->left,x+span->right,colour);
}

static void draw_segment_digit(int x,int y,int digit,unsigned char colour)
{
  int segment;ega_rectangle(x,y,109,114,0);
  for(segment=0;segment<7;segment++)draw_clock_shape(x,y,&clock_shapes[segment],8);
  if(digit>=0)for(segment=0;segment<7;segment++)
    if(segment_mask[digit]&(1<<segment))
      draw_clock_shape(x,y,&clock_shapes[segment],colour);
}

static void draw_clock_dot(int cx,int cy,unsigned char colour)
{
  static const unsigned char extents[9]={11,10,10,10,9,8,7,5,3};
  int dy,extent;
  for(dy=-8;dy<=8;dy++){
    extent=extents[dy<0?-dy:dy];
    ega_span(cy+dy,cx-extent,cx+extent,colour);
  }
}

static void draw_clock_bitmap(int x,int y,const unsigned char *bits,
                              int width,int height,int stride,
                              unsigned char colour)
{
  int row,column,start;
  for(row=0;row<height;row++){
    column=0;
    while(column<width){
      while(column<width && !(bits[row*stride+(column>>3)]&(0x80>>(column&7))))column++;
      start=column;
      while(column<width && (bits[row*stride+(column>>3)]&(0x80>>(column&7))))column++;
      if(start<column)ega_span(y+row,x+start,x+column-1,colour);
    }
  }
}

static void set_ega_saver_mode(int clock_palette)
{
  union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x0010;int86(0x10,&r,&r);

  /* A VGA can give colour 8 a subtler VFD-like inactive glow.  Its DAC
     components are six-bit values, so 10/63 is approximately RGB #282828.
     A genuine EGA has only its
     fixed 64-colour palette and therefore retains normal dark grey. */
  memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);
  if(clock_palette && r.h.al==0x1A){
    memset(&r,0,sizeof(r));r.x.ax=0x1000;r.h.bl=8;r.h.bh=8;
    int86(0x10,&r,&r);
    memset(&r,0,sizeof(r));r.x.ax=0x1010;r.x.bx=8;
    r.h.dh=10;r.h.ch=10;r.h.cl=10;int86(0x10,&r,&r);
  }
  _outpw(0x3C4,0x0F02); /* all four planes */
  _outpw(0x3CE,0x0003); /* replace, no rotate */
  _outpw(0x3CE,0x0205); /* write mode 2 */
}

static unsigned char draw_graphics_time(unsigned char previous_second,
                                        signed char previous_digits[4])
{
  static const int positions[4]={86,206,386,506};
  union REGS r;signed char digits[4];unsigned char colour,colon_colour;
  int i,hour,is_pm,shift;
  memset(&r,0,sizeof(r));r.h.ah=0x2C;int86(0x21,&r,&r);
  if(r.h.dh==previous_second)return previous_second;
  hour=r.h.ch;is_pm=hour>=12;
  if(appearance.hour_12){hour%=12;if(!hour)hour=12;digits[0]=(signed char)(hour>=10?hour/10:-1);}
  else digits[0]=(signed char)(hour/10);
  digits[1]=(signed char)(hour%10);digits[2]=(signed char)(r.h.cl/10);
  digits[3]=(signed char)(r.h.cl%10);
  shift=appearance.hour_12?0:-31;
  colour=(unsigned char)(appearance.saver_color&15);
  wait_vertical_retrace();
  for(i=0;i<4;i++)if(previous_digits[i]!=digits[i]){
    draw_segment_digit(positions[i]+shift,118,digits[i],colour);previous_digits[i]=digits[i];
  }
  colon_colour=(r.h.dh&1)?8:colour;
  draw_clock_dot(355+shift,150,colon_colour);
  draw_clock_dot(343+shift,201,colon_colour);
  if(appearance.hour_12){
    draw_clock_bitmap(21,143,clock_am_bits,CLOCK_AM_WIDTH,CLOCK_AM_HEIGHT,
                      CLOCK_AM_STRIDE,(unsigned char)(!is_pm?colour:8));
    draw_clock_bitmap(16,191,clock_pm_bits,CLOCK_PM_WIDTH,CLOCK_PM_HEIGHT,
                      CLOCK_PM_STRIDE,(unsigned char)(is_pm?colour:8));
  }
  return r.h.dh;
}

typedef struct {
  unsigned char seq2,seq4,gc4,gc5,gc6;
} FONT_REGS;

#pragma code_seg("FONT_TEXT")

static unsigned char indexed_read(unsigned port,unsigned char index)
{
  _outp(port,index);return (unsigned char)_inp(port+1);
}

static void indexed_write(unsigned port,unsigned char index,unsigned char value)
{
  _outp(port,index);_outp(port+1,value);
}

static void font_plane_open(FONT_REGS *old)
{
  old->seq2=indexed_read(0x3C4,2);old->seq4=indexed_read(0x3C4,4);
  old->gc4=indexed_read(0x3CE,4);old->gc5=indexed_read(0x3CE,5);
  old->gc6=indexed_read(0x3CE,6);
  indexed_write(0x3C4,2,4);indexed_write(0x3C4,4,7);
  indexed_write(0x3CE,4,2);indexed_write(0x3CE,5,0);
  indexed_write(0x3CE,6,0);
}

static void font_plane_close(const FONT_REGS *old)
{
  indexed_write(0x3C4,2,old->seq2);indexed_write(0x3C4,4,old->seq4);
  indexed_write(0x3CE,4,old->gc4);indexed_write(0x3CE,5,old->gc5);
  indexed_write(0x3CE,6,old->gc6);
}

static const unsigned char launchui_codes[41]={16,17,30,31,169,170,173,174,175,181,182,183,184,185,186,187,188,189,190,198,225,200,201,202,224,204,205,234,229,208,209,210,211,212,213,235,215,255,220,244,245};
/* Release 3.72 icon runtime slots.  Left halves marked with * in the source
   glyph map stay in C0h-DFh so VGA supplies the ninth-column extension. */
#define UI_OPEN_L    199
#define UI_OPEN_R    230
#define UI_SEARCH_L  206
#define UI_SEARCH_R  231
#define UI_PARENT_L  207
#define UI_PARENT_R  232
#define UI_EXIT_L    214
#define UI_EXIT_R    233
static const unsigned char launchui_extra_codes[8]={UI_OPEN_L,UI_OPEN_R,UI_SEARCH_L,UI_SEARCH_R,UI_PARENT_L,UI_PARENT_R,UI_EXIT_L,UI_EXIT_R};
static const unsigned char launchui_extra_logical[8]={89,90,91,92,61,62,93,94};

static int launchui_ega14(void)
{
  unsigned char far *h=(unsigned char far *)MAKE_FP(0x40,0x85);
  return *h==14;
}

static void ega14_glyph_write(int code,const unsigned char far *glyph)
{
 unsigned fseg=FP_SEG(glyph),foff=FP_OFF(glyph);
 _asm {
  push bp
  push es
  mov ax,1100h
  mov bh,14
  mov bl,0
  mov cx,1
  mov dx,code
  mov ax,fseg
  mov es,ax
  mov bp,foff
  mov ax,1100h
  int 10h
  pop es
  pop bp
 }
}
static void ega14_rom_read(int code,unsigned char far *glyph)
{
 unsigned fseg,foff;unsigned char far *p;int j;
 _asm {
  push bp
  push es
  mov ax,1130h
  mov bh,2
  int 10h
  mov ax,es
  mov fseg,ax
  mov foff,bp
  pop es
  pop bp
 }
 p=(unsigned char far *)MAKE_FP(fseg,foff);p+=(unsigned)code*14U;for(j=0;j<14;j++)glyph[j]=p[j];for(;j<32;j++)glyph[j]=0;
}
static unsigned char far launchui_old[41][32];
/* Browser icons use safe CP437 line-drawing slots. C0-DF positions permit VGA
   ninth-column extension for the left halves exactly as the source artwork expects. */
/* Browser icon runtime slots.  Left halves must live in C0-DF so VGA's
   ninth-column extension joins the two-cell artwork.  193/194/197 are unused
   single-line/double-line positions in Launch!'s UI.  The right halves use
   otherwise-unused 183/184/186 slots.  In particular, never use 217/218
   (box corners) or 219 (solid block used by message icons). */
/* 194 is the single-line down-T used by Configuration tabs and must never
   be substituted.  Use 221 for the executable icon's extending left half. */
static const unsigned char launchui_browser_codes[6]={193,183,221,184,197,186};
static const unsigned char far legacy_browser_file_l16[32]={0x00,0x07,0x0C,0x1C,0x3D,0x20,0x2F,0x20,0x2F,0x20,0x20,0x20,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char far legacy_browser_file_l14[32]={0x07,0x0C,0x1C,0x3D,0x20,0x2F,0x20,0x2F,0x20,0x20,0x20,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static unsigned char far launchui_browser_old[6][32];
static unsigned char far launchui_extra_old[8][32];
static unsigned char far launchui_divider_old[32];
static int launchui_active=0;
/* EGA 8x14 font handling.
   Load the adapter's complete ROM 8x14 font into character block 0, then
   explicitly select block 0 before overlaying Launch!'s custom glyphs. */
static void ega14_rom_reset(void)
{
  union REGS r;
  memset(&r,0,sizeof(r));
  r.x.ax=0x1101;       /* Load ROM 8x14 text font */
  r.x.bx=0;            /* Character block 0 */
  int86(0x10,&r,&r);
  memset(&r,0,sizeof(r));
  r.x.ax=0x1103;       /* Select character block 0 for both maps */
  r.x.bx=0;
  int86(0x10,&r,&r);
}
/* Modify the active EGA/VGA character-generator RAM in place.  On genuine
   EGA, loading one character through INT 10h/AH=11 can switch to a user font
   block whose untouched slots are undefined; that is what allowed custom
   icons to appear in ordinary CP437 borders. */
static void launchui_install(void)
{
  FONT_REGS old;unsigned char far *font;int i,j,ega=launchui_ega14();
  if(launchui_active)return;
  if(ega){
    ega14_rom_reset();
    for(i=0;i<(int)sizeof(launchui_codes);i++)
      ega14_glyph_write(launchui_codes[i],launch_glyph14[i]);
    for(i=0;i<6;i++)
      ega14_glyph_write(launchui_browser_codes[i],i?launch_glyph14[49+i]:legacy_browser_file_l14);
    for(i=0;i<8;i++)
      ega14_glyph_write(launchui_extra_codes[i],launch_glyph14[launchui_extra_logical[i]-1]);
    ega14_glyph_write(216,launch_glyph14[55]);
    launchui_active=1;
    return;
  }
  font_plane_open(&old);
  for(i=0;i<(int)sizeof(launchui_codes);i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_codes[i]*32);
    for(j=0;j<32;j++){launchui_old[i][j]=font[j];font[j]=launch_glyph16[i][j];}
  }
  for(i=0;i<6;i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_browser_codes[i]*32);
    for(j=0;j<32;j++){launchui_browser_old[i][j]=font[j];font[j]=i?launch_glyph16[49+i][j]:legacy_browser_file_l16[j];}
  }
  for(i=0;i<8;i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_extra_codes[i]*32);
    for(j=0;j<32;j++){launchui_extra_old[i][j]=font[j];font[j]=launch_glyph16[launchui_extra_logical[i]-1][j];}
  }
  font=(unsigned char far *)MAKE_FP(0xA000,216*32);
  for(j=0;j<32;j++){launchui_divider_old[j]=font[j];font[j]=launch_glyph16[55][j];}
  font_plane_close(&old);
  launchui_active=1;
}

static void launchui_rebase(void)
{
  FONT_REGS old;unsigned char far *font;int i,j,ega=launchui_ega14();
  if(!launchui_active)return;
  if(ega){
    ega14_rom_reset();
    for(i=0;i<(int)sizeof(launchui_codes);i++)
      ega14_glyph_write(launchui_codes[i],launch_glyph14[i]);
    for(i=0;i<6;i++)
      ega14_glyph_write(launchui_browser_codes[i],i?launch_glyph14[49+i]:legacy_browser_file_l14);
    for(i=0;i<8;i++)
      ega14_glyph_write(launchui_extra_codes[i],launch_glyph14[launchui_extra_logical[i]-1]);
    ega14_glyph_write(216,launch_glyph14[55]);
    return;
  }
  font_plane_open(&old);
  for(i=0;i<(int)sizeof(launchui_codes);i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_codes[i]*32);
    for(j=0;j<32;j++){launchui_old[i][j]=font[j];font[j]=launch_glyph16[i][j];}
  }
  for(i=0;i<6;i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_browser_codes[i]*32);
    for(j=0;j<32;j++){launchui_browser_old[i][j]=font[j];font[j]=i?launch_glyph16[49+i][j]:legacy_browser_file_l16[j];}
  }
  for(i=0;i<8;i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_extra_codes[i]*32);
    for(j=0;j<32;j++){launchui_extra_old[i][j]=font[j];font[j]=launch_glyph16[launchui_extra_logical[i]-1][j];}
  }
  font=(unsigned char far *)MAKE_FP(0xA000,216*32);
  for(j=0;j<32;j++)font[j]=launch_glyph16[55][j];
  font_plane_close(&old);
}

static void launchui_restore(void)
{
  FONT_REGS old;unsigned char far *font;int i,j;
  if(!launchui_active)return;
  if(launchui_ega14()){
    ega14_rom_reset();
    launchui_active=0;
    return;
  }
  font_plane_open(&old);
  font=(unsigned char far *)MAKE_FP(0xA000,216*32);
  for(j=0;j<32;j++)font[j]=launchui_divider_old[j];
  for(i=0;i<8;i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_extra_codes[i]*32);
    for(j=0;j<32;j++)font[j]=launchui_extra_old[i][j];
  }
  for(i=0;i<6;i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_browser_codes[i]*32);
    for(j=0;j<32;j++)font[j]=launchui_browser_old[i][j];
  }
  for(i=0;i<(int)sizeof(launchui_codes);i++){
    font=(unsigned char far *)MAKE_FP(0xA000,launchui_codes[i]*32);
    for(j=0;j<32;j++)font[j]=launchui_old[i][j];
  }
  font_plane_close(&old);
  launchui_active=0;
}

static void mouse_glyph_write(const unsigned char *glyph)
{
  FONT_REGS old;unsigned char far *font;int i;
  if(launchui_ega14()){ega14_glyph_write(127,glyph);return;}font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,127*32);for(i=0;i<32;i++)font[i]=glyph[i];font_plane_close(&old);
}

static void mouse_target_write(const unsigned char *glyph)
{
  FONT_REGS old;unsigned char far *font;int i;
  if(launchui_ega14()){ega14_glyph_write(8,glyph);return;}font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,8*32);
  for(i=0;i<32;i++)font[i]=glyph[i];
  font_plane_close(&old);
}

static void mouse_pointer_install(void)
{
  static const unsigned char arrow16[16]={
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,
    0x18,0x1C,0x1E,0x1F,0x1E,0x12,0x03,0x01
  };
  FONT_REGS old;unsigned char far *font;
  unsigned char arrow[32];unsigned char far *height_ptr;
  union REGS r;int i,height,source;
  if(appearance.mouse_cursor){
    if(mouse_glyph_saved||mouse_target_saved)mouse_pointer_restore();
    memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;
    if(appearance.mouse_cursor==1){r.x.cx=0xFFFF;r.x.dx=0x7700;}
    else {r.x.cx=0xF000;r.x.dx=0x0FB8;}
    int86(0x33,&r,&r);return;
  }
  if(!mouse_glyph_saved){
    if(launchui_ega14())ega14_rom_read(127,mouse_old_glyph);else{font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,127*32);for(i=0;i<32;i++)mouse_old_glyph[i]=font[i];font_plane_close(&old);}
    mouse_glyph_saved=1;
  }
  memset(arrow,0,sizeof(arrow));height_ptr=(unsigned char far *)MAKE_FP(0x40,0x85);height=*height_ptr;if(height<8||height>32)height=16;
  for(i=0;i<height;i++){source=i*16/height;if(source>15)source=15;arrow[i]=arrow16[source];}
  mouse_glyph_write(arrow);
  memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;r.x.cx=0xF000;r.x.dx=0x0F7F;int86(0x33,&r,&r);
}

static void mouse_pointer_restore(void)
{
  union REGS r;if(!mouse_glyph_saved&&!mouse_target_saved)return;
  if(mouse_glyph_saved)mouse_glyph_write(mouse_old_glyph);
  if(mouse_target_saved)mouse_target_write(mouse_old_target);
  memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;
  r.x.cx=0xFFFF;r.x.dx=0x7700;int86(0x33,&r,&r);
  mouse_glyph_saved=0;
  mouse_target_saved=0;
}

static void mouse_show(void)
{
  union REGS r;if(!mouse_present || mouse_visible)return;
  memset(&r,0,sizeof(r));r.x.ax=1;int86(0x33,&r,&r);
  mouse_visible=1;
}

int saver_input(unsigned start_x,unsigned start_y)
{
  int mx=0,my=0;unsigned buttons;
  if(key_waiting()){keyread();return 1;}
  buttons=mouse_poll(&mx,&my);
  return buttons || mouse_raw_x!=start_x || mouse_raw_y!=start_y;
}

#pragma code_seg()

static void clock_saver_loop(unsigned start_x,unsigned start_y)
{
  unsigned char second=255;signed char previous_digits[4]={-2,-2,-2,-2};
  while(!saver_input(start_x,start_y))
    second=draw_graphics_time(second,previous_digits);
}

static void starry_saver_loop(unsigned start_x,unsigned start_y)
{
  unsigned long last_tick=bios_ticks();int x=0,i,bx,bw,bh,top,wx,wy,phase=0;
  unsigned char building;
  memset(night_pixels,0xFF,sizeof(night_pixels));
  memset(night_beacons,0,sizeof(night_beacons));
  for(i=0;i<640;i++){skyline_top[i]=350;skyline_colour[i]=0;}
  while(!saver_input(start_x,start_y)){
    unsigned long tick=bios_ticks();
    wait_vertical_retrace();
    if(tick==last_tick)continue;
    last_tick=tick;
    night_pixels_decay();
    if(x<640){
      bw=22+(int)saver_random(43);if(x+bw>640)bw=640-x;
      bh=42+(int)saver_random(130);top=350-bh;
      building=(unsigned char)(saver_random(2)?1:8);
      ega_rectangle(x,top,bw,bh,building);
      for(i=0;i<NIGHT_PIXEL_COUNT;i++)
        if(night_pixels[i].x!=0xFFFF && night_pixels[i].base==0 &&
           night_pixels[i].x>=(unsigned)x && night_pixels[i].x<(unsigned)(x+bw) &&
           night_pixels[i].y>=(unsigned)top)night_pixels[i].x=0xFFFF;
      for(i=x;i<x+bw;i++){
        skyline_top[i]=(unsigned short)top;skyline_colour[i]=building;
      }
      if(saver_random(3)==0){
        int antenna=x+bw/2,antenna_top=top-(8+(int)saver_random(18));
        ega_line(antenna,top,antenna,antenna_top,8);
        add_beacon(antenna-2,antenna_top-2);
      }
      x+=bw;
    }
    if((phase++&1)==0){
      /* Stars fill the screen behind, rather than stopping above the skyline. */
      bx=(int)saver_random(640);wy=(int)saver_random(350);
      if(wy<(int)skyline_top[bx])
        night_pixel_add(bx,wy,(unsigned char)(saver_random(5)?7:15),0);
    } else {
      /* Windows may appear from the roof line to the foot of a building. */
      wx=(int)saver_random(638);top=(int)skyline_top[wx];
      if(top<346){
        wy=top+3+(int)saver_random((unsigned)(346-top));
        night_pixel_add(wx,wy,(unsigned char)(saver_random(5)?14:7),
                        skyline_colour[wx]);
      }
    }
    update_beacons();
  }
}

static void reset_warp_star(WARP_STAR *star,int varied_depth)
{
  star->x=(int)saver_random(641)-320;star->y=(int)saver_random(351)-175;
  if(star->x>-18 && star->x<18)star->x+=star->x<0?-28:28;
  if(star->y>-12 && star->y<12)star->y+=star->y<0?-20:20;
  star->z=varied_depth?18+(int)saver_random(78):95;
  star->px=star->qx=320;star->py=star->qy=175;
}

static void warp_saver_loop(unsigned start_x,unsigned start_y)
{
  int i,sx,sy,tx,ty;unsigned char colour;WARP_STAR *star;
  for(i=0;i<56;i++)reset_warp_star(&warp_stars[i],1);
  while(!saver_input(start_x,start_y)){
    wait_vertical_retrace();
    for(i=0;i<56;i++){
      star=&warp_stars[i];ega_line(star->px,star->py,star->qx,star->qy,0);
      star->z-=2;
      if(star->z<6){reset_warp_star(star,0);continue;}
      sx=320+star->x*72/star->z;sy=175+star->y*72/star->z;
      tx=320+star->x*66/star->z;ty=175+star->y*66/star->z;
      if(sx<0 || sx>=640 || sy<0 || sy>=350){reset_warp_star(star,0);continue;}
      star->px=tx;star->py=ty;star->qx=sx;star->qy=sy;
      colour=(unsigned char)(star->z<28?15:(star->z<58?7:8));
      ega_line(tx,ty,sx,sy,colour);
      if(star->z<20)ega_span(sy+1,sx,sx,colour);
    }
  }
}

static int pipe_area_used(int x,int y,int horizontal)
{
  int d;
  for(d=-6;d<=6;d++)
    if(ega_pixel(horizontal?x:x+d,horizontal?y+d:y))return 1;
  return 0;
}

static void pipe_segment(int x0,int y0,int x1,int y1,
                         unsigned char colour,int under)
{
  int horizontal=y0==y1,step,i=0,x=x0,y=y0,used;
  if(!under){
    if(horizontal){
      ega_rectangle(x0<x1?x0:x1,y0-5,abs(x1-x0)+1,10,0);
      ega_rectangle(x0<x1?x0:x1,y0-3,abs(x1-x0)+1,6,colour);
    } else {
      ega_rectangle(x0-5,y0<y1?y0:y1,10,abs(y1-y0)+1,0);
      ega_rectangle(x0-3,y0<y1?y0:y1,6,abs(y1-y0)+1,colour);
    }
    return;
  }
  step=horizontal?(x1>=x0?1:-1):(y1>=y0?1:-1);
  for(;;){
    used=i>4&&pipe_area_used(x,y,horizontal);
    if(!used){
      if(horizontal){ega_rectangle(x,y-5,1,10,0);ega_rectangle(x,y-3,1,6,colour);}
      else {ega_rectangle(x-5,y,10,1,0);ega_rectangle(x-3,y,6,1,colour);}
    }
    if(x==x1&&y==y1)break;
    if(horizontal)x+=step;else y+=step;i++;
  }
}

static void pipe_reset(void)
{
  static const unsigned char colours[7]={9,10,11,12,13,14,15};
  int i;ega_rectangle(0,0,640,350,0);
  for(i=0;i<5;i++){
    pipe_heads[i].x=20+(int)saver_random(600);
    pipe_heads[i].y=20+(int)saver_random(310);
    pipe_heads[i].direction=(int)saver_random(4);
    pipe_heads[i].colour=colours[saver_random(7)];
    ega_rectangle(pipe_heads[i].x-5,pipe_heads[i].y-5,10,10,0);
    ega_rectangle(pipe_heads[i].x-3,pipe_heads[i].y-3,6,6,
                  pipe_heads[i].colour);
  }
}

static void pipes_saver_loop(unsigned start_x,unsigned start_y)
{
  static const int vx[4]={1,0,-1,0},vy[4]={0,1,0,-1};
  unsigned long last_tick=bios_ticks(),tick;int index=0,segments=0;
  PIPE_HEAD *head;int length,nx,ny,turn,under;
  pipe_reset();
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    head=&pipe_heads[index];length=12+(int)saver_random(33);
    nx=head->x+vx[head->direction]*length;
    ny=head->y+vy[head->direction]*length;
    if(nx<7)nx=7;
    if(nx>632)nx=632;
    if(ny<7)ny=7;
    if(ny>342)ny=342;
    under=(int)(saver_random(3)==0);
    wait_vertical_retrace();
    pipe_segment(head->x,head->y,nx,ny,head->colour,under);
    head->x=nx;head->y=ny;
    turn=saver_random(2)?1:-1;
    if(nx<=7||nx>=632||ny<=7||ny>=342||saver_random(4)!=0)
      head->direction=(head->direction+turn+4)%4;
    index=(index+1)%5;
    if(++segments>=500){pipe_reset();segments=0;}
  }
}

int boing_isqrt(int value)
{
  int root=0,bit=0x4000,trial;
  while(bit>value)bit>>=2;
  while(bit){
    trial=root+bit;root>>=1;
    if(value>=trial){value-=trial;root+=bit;}
    bit>>=2;
  }
  return root;
}

static unsigned char boing_background_colour(int y)
{
  int stripe=y&7;
  return (unsigned char)(stripe==0?9:(stripe==1?1:0));
}

static void boing_restore_area(int left,int top,int right,int bottom)
{
  int y;if(left<0)left=0;if(right>639)right=639;
  if(top<0)top=0;if(bottom>349)bottom=349;
  for(y=top;y<=bottom;y++)ega_span(y,left,right,boing_background_colour(y));
}

static void boing_background(void)
{
  boing_restore_area(0,0,639,349);
}

static int boing_half_width(int dy,int x_radius,int y_radius)
{
  if(dy< -y_radius || dy>y_radius)return -1;
  return x_radius*boing_isqrt(y_radius*y_radius-dy*dy)/y_radius;
}

static void boing_shadow(int cx,int cy,int x_radius,int y_radius)
{
  int row,half,ball_half,screen_y,left,right;
  for(row=-y_radius;row<=y_radius;row++){
    screen_y=cy+5+row;
    if(boing_background_colour(screen_y)){
      half=boing_half_width(row,x_radius,y_radius);
      left=cx+6-half;right=cx+6+half;
      ball_half=boing_half_width(screen_y-cy,x_radius,y_radius);
      if(ball_half<0)ega_span(screen_y,left,right,5);
      else {
        if(left<cx-ball_half)ega_span(screen_y,left,cx-ball_half-1,5);
        if(right>cx+ball_half)ega_span(screen_y,cx+ball_half+1,right,5);
      }
    }
  }
}

static void boing_ball(int cx,int cy,int x_radius,int y_radius,int phase)
{
  int dy,dx,half,inner,left,right,start,longitude,latitude,cell;
  unsigned char colour,last;
  for(dy=-y_radius;dy<=y_radius;dy++){
    half=boing_half_width(dy,x_radius,y_radius);
    left=cx-half;right=cx+half;
    inner=half-2;
    if(inner<=0){ega_span(cy+dy,left,right,0);continue;}
    start=-inner;last=255;
    for(dx=-inner;dx<=inner;dx++){
      longitude=((dx+inner)*128)/(inner*2+1)+phase;
      latitude=((dy+y_radius)*6)/(y_radius*2+1);
      cell=((longitude/16)+latitude)&1;
      colour=(unsigned char)(cell?12:15);
      if(last==255){last=colour;start=dx;}
      else if(colour!=last){ega_span(cy+dy,cx+start,cx+dx-1,last);start=dx;last=colour;}
    }
    ega_span(cy+dy,cx+start,cx+inner,last);
    ega_span(cy+dy,left,left+1,0);ega_span(cy+dy,right-1,right,0);
  }
}

static int boing_silhouette_bounds(int screen_y,int cx,int cy,
                                   int x_radius,int y_radius,
                                   int *left,int *right)
{
  int half,valid=0;
  half=boing_half_width(screen_y-cy,x_radius,y_radius);
  if(half>=0){*left=cx-half;*right=cx+half;valid=1;}
  if(boing_background_colour(screen_y)){
    half=boing_half_width(screen_y-(cy+5),x_radius,y_radius);
    if(half>=0){
      if(!valid){*left=cx+6-half;*right=cx+6+half;valid=1;}
      else {if(cx+6-half<*left)*left=cx+6-half;if(cx+6+half>*right)*right=cx+6+half;}
    }
  }
  return valid;
}

static void boing_restore_exposed(int old_x,int old_y,int new_x,int new_y,
                                  int x_radius,int y_radius)
{
  int row,old_left,old_right,new_left,new_right,old_used,new_used;
  for(row=old_y-y_radius;row<=old_y+y_radius+5;row++){
    old_used=boing_silhouette_bounds(row,old_x,old_y,x_radius,y_radius,
                                     &old_left,&old_right);
    if(!old_used)continue;
    new_used=boing_silhouette_bounds(row,new_x,new_y,x_radius,y_radius,
                                     &new_left,&new_right);
    if(!new_used || new_right<old_left || new_left>old_right)
      ega_span(row,old_left,old_right,boing_background_colour(row));
    else {
      if(old_left<new_left)
        ega_span(row,old_left,new_left-1,boing_background_colour(row));
      if(old_right>new_right)
        ega_span(row,new_right+1,old_right,boing_background_colour(row));
    }
  }
}

static void boing_saver_loop(unsigned start_x,unsigned start_y)
{
  const int x_radius=80,y_radius=58,floor_y=334;
  int x=90,y=70,old_x=x,old_y=y,phase=0;
  int x_fixed=x*16,y_fixed=y*16,dx_fixed=40,dy_fixed=8,phase_fixed=0;
  unsigned long last_tick=bios_ticks(),tick;
  boing_background();
  boing_shadow(x,y,x_radius,y_radius);
  boing_ball(x,y,x_radius,y_radius,phase);
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    old_x=x;old_y=y;
    x_fixed+=dx_fixed;y_fixed+=dy_fixed;dy_fixed+=2;
    phase_fixed=(phase_fixed+24)&511;
    x=x_fixed/16;y=y_fixed/16;phase=phase_fixed/16;
    if(x-x_radius<10){x=x_radius+10;x_fixed=x*16;dx_fixed=abs(dx_fixed);}
    else if(x+x_radius>629){x=629-x_radius;x_fixed=x*16;dx_fixed=-abs(dx_fixed);}
    if(y+y_radius>=floor_y){y=floor_y-y_radius;y_fixed=y*16;dy_fixed=-112;}
    if(y-y_radius<8){y=y_radius+8;y_fixed=y*16;dy_fixed=abs(dy_fixed);}
    wait_vertical_retrace();
    boing_shadow(x,y,x_radius,y_radius);
    boing_ball(x,y,x_radius,y_radius,phase);
    boing_restore_exposed(old_x,old_y,x,y,x_radius,y_radius);
  }
}

static void halftone_dot(int column,int row,int radius,
                         unsigned char background,unsigned char foreground)
{
  int x=column*16,y=row*14,dy,half,x_radius;
  ega_rectangle(x,y,16,14,background);
  if(!radius)return;
  x+=8;y+=7;x_radius=(radius*4+2)/3;
  for(dy=-radius;dy<=radius;dy++){
    half=x_radius*boing_isqrt(radius*radius-dy*dy)/radius;
    ega_span(y+dy,x-half,x+half,foreground);
  }
}

static void halftone_reset(unsigned char *background,unsigned char *foreground)
{
  static const unsigned char dark[7]={1,2,3,4,5,6,8};
  static const unsigned char bright[7]={9,10,11,12,13,14,15};
  int i;HALFTONE_MASS *mass;
  *background=dark[saver_random(7)];*foreground=bright[saver_random(7)];
  ega_rectangle(0,0,640,350,*background);
  memset(halftone_previous,0xFF,sizeof(halftone_previous));
  for(i=0;i<HALFTONE_MASSES;i++){
    mass=&halftone_masses[i];
    mass->x=20+(int)saver_random(600);mass->y=20+(int)saver_random(310);
    mass->dx=1+(int)saver_random(3);mass->dy=1+(int)saver_random(2);
    if(saver_random(2))mass->dx=-mass->dx;
    if(saver_random(2))mass->dy=-mass->dy;
    mass->mass=3600+(int)saver_random(5200);
  }
}

static void halftone_move_masses(void)
{
  int i;HALFTONE_MASS *mass;
  for(i=0;i<HALFTONE_MASSES;i++){
    mass=&halftone_masses[i];mass->x+=mass->dx;mass->y+=mass->dy;
    if(mass->x<0){mass->x=0;mass->dx=abs(mass->dx);}
    else if(mass->x>639){mass->x=639;mass->dx=-abs(mass->dx);}
    if(mass->y<0){mass->y=0;mass->dy=abs(mass->dy);}
    else if(mass->y>349){mass->y=349;mass->dy=-abs(mass->dy);}
  }
}

static void halftone_frame(unsigned char background,unsigned char foreground)
{
  int column,row,index,mass_index,px,py,dx,dy,strength,radius;
  long scaled_dx,scaled_dy;unsigned long distance;
  HALFTONE_MASS *mass;
  for(row=0;row<HALFTONE_ROWS;row++)for(column=0;column<HALFTONE_COLS;column++){
    px=column*16+8;py=row*14+7;strength=0;
    for(mass_index=0;mass_index<HALFTONE_MASSES;mass_index++){
      mass=&halftone_masses[mass_index];
      dx=(px-mass->x)*3/4;dy=py-mass->y;
      scaled_dx=dx;scaled_dy=dy;
      distance=(unsigned long)(scaled_dx*scaled_dx+scaled_dy*scaled_dy+80L);
      strength+=(int)((unsigned long)mass->mass/distance);
    }
    radius=strength;if(radius>6)radius=6;
    index=row*HALFTONE_COLS+column;
    if(halftone_previous[index]!=(unsigned char)radius){
      halftone_dot(column,row,radius,background,foreground);
      halftone_previous[index]=(unsigned char)radius;
    }
  }
}

static void halftone_saver_loop(unsigned start_x,unsigned start_y)
{
  unsigned char background,foreground;
  unsigned long last_tick=bios_ticks(),tick;unsigned frames=0;
  halftone_reset(&background,&foreground);halftone_frame(background,foreground);
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    halftone_move_masses();wait_vertical_retrace();
    halftone_frame(background,foreground);
    if(++frames>=900){halftone_reset(&background,&foreground);frames=0;}
  }
}

static void mystify_draw(const int *x,const int *y,unsigned char colour)
{
  int i;for(i=0;i<4;i++)ega_line(x[i],y[i],x[(i+1)&3],y[(i+1)&3],colour);
}

static void mystify_reset(void)
{
  int shape,point;MYSTIFY_SHAPE *m;
  ega_rectangle(0,0,640,350,0);
  memset(mystify_shapes,0,sizeof(mystify_shapes));
  for(shape=0;shape<2;shape++){
    m=&mystify_shapes[shape];m->colour=(unsigned char)(9+saver_random(7));
    for(point=0;point<4;point++){
      m->x[point]=20+(int)saver_random(600);
      m->y[point]=20+(int)saver_random(310);
      m->dx[point]=(int)saver_random(7)-3;if(!m->dx[point])m->dx[point]=1;
      m->dy[point]=(int)saver_random(5)-2;if(!m->dy[point])m->dy[point]=-1;
    }
  }
}

static void mystify_saver_loop(unsigned start_x,unsigned start_y)
{
  unsigned long last_tick=bios_ticks(),tick;int shape,point,slot;
  MYSTIFY_SHAPE *m;mystify_reset();
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    wait_vertical_retrace();
    for(shape=0;shape<2;shape++){
      m=&mystify_shapes[shape];slot=m->slot;
      if(m->used[slot])mystify_draw(m->old_x[slot],m->old_y[slot],0);
      for(point=0;point<4;point++){
        m->x[point]+=m->dx[point];m->y[point]+=m->dy[point];
        if(m->x[point]<=2){m->x[point]=2;m->dx[point]=abs(m->dx[point]);}
        else if(m->x[point]>=637){m->x[point]=637;m->dx[point]=-abs(m->dx[point]);}
        if(m->y[point]<=2){m->y[point]=2;m->dy[point]=abs(m->dy[point]);}
        else if(m->y[point]>=347){m->y[point]=347;m->dy[point]=-abs(m->dy[point]);}
        m->old_x[slot][point]=m->x[point];m->old_y[slot][point]=m->y[point];
      }
      m->colour=(unsigned char)(9+((m->colour-8)%7));
      m->old_colour[slot]=m->colour;m->used[slot]=1;
      mystify_draw(m->x,m->y,m->colour);m->slot=(unsigned char)((slot+1)%12);
    }
  }
}

static void draw_logo(int x,int y)
{
  unsigned i;const LOGO_SPAN *span;
  for(i=0;i<(unsigned)LOGO_SPAN_COUNT;i++){
    span=&logo_spans[i];
    ega_span(y+span->y,x+span->left,x+span->right,span->colour);
  }
}

static int logo_pixel_set(int x,int y)
{
  unsigned i;
  if(x<0 || x>=LOGO_WIDTH || y<0 || y>=LOGO_HEIGHT)return 0;
  for(i=logo_row_first[y];i<logo_row_first[y+1];i++)
    if(x>=(int)logo_spans[i].left && x<=(int)logo_spans[i].right)return 1;
  return 0;
}

static void erase_old_logo_edges(int old_x,int old_y,int new_x,int new_y)
{
  unsigned i;int x,start,local_y;const LOGO_SPAN *span;
  for(i=0;i<(unsigned)LOGO_SPAN_COUNT;i++){
    span=&logo_spans[i];x=span->left;local_y=old_y+span->y-new_y;
    while(x<=(int)span->right){
      while(x<=(int)span->right &&
            logo_pixel_set(old_x+x-new_x,local_y))x++;
      start=x;
      while(x<=(int)span->right &&
            !logo_pixel_set(old_x+x-new_x,local_y))x++;
      if(start<x)ega_span(old_y+span->y,old_x+start,old_x+x-1,0);
    }
  }
}

static void logo_saver_loop(unsigned start_x,unsigned start_y)
{
  const int floor_y=334;
  int x=(int)saver_random(620-LOGO_WIDTH+1)+10;
  int y=8+(int)saver_random(floor_y-LOGO_HEIGHT-7);
  int x_fixed=x*16,y_fixed=y*16;
  int dx_fixed=saver_random(2)?40:-40,dy_fixed=8;
  unsigned long last_tick=bios_ticks(),tick;
  draw_logo(x,y);
  while(!saver_input(start_x,start_y)){
    int old_x,old_y;
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    old_x=x;old_y=y;
    x_fixed+=dx_fixed;y_fixed+=dy_fixed;dy_fixed+=2;
    x=x_fixed/16;y=y_fixed/16;
    if(x<10){x=10;x_fixed=x*16;dx_fixed=abs(dx_fixed);}
    else if(x+LOGO_WIDTH>630){x=630-LOGO_WIDTH;x_fixed=x*16;dx_fixed=-abs(dx_fixed);}
    if(y+LOGO_HEIGHT>=floor_y){
      y=floor_y-LOGO_HEIGHT;y_fixed=y*16;dy_fixed=-112;
    }
    if(y<8){y=8;y_fixed=y*16;dy_fixed=abs(dy_fixed);}
    wait_vertical_retrace();draw_logo(x,y);
    erase_old_logo_edges(old_x,old_y,x,y);
  }
}

#include "SAVERS.H"

static void run_screensaver(void)
{
  union REGS r;int mx=0,my=0,old_mode,old_rows=screen_rows;
  unsigned start_x,start_y;
  if(!appearance.screensaver)return;
  mouse_stop();
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);old_mode=r.h.al;
  set_ega_saver_mode(appearance.screensaver==1);
  saver_random_state=bios_ticks()^0xA5A55A5AUL;
  /* A mode change can rescale or reset the mouse driver's coordinates. */
  (void)mouse_poll(&mx,&my);start_x=mouse_raw_x;start_y=mouse_raw_y;
  if(appearance.screensaver==1)clock_saver_loop(start_x,start_y);
  else if(appearance.screensaver==2)boing_saver_loop(start_x,start_y);
  else if(appearance.screensaver==3)logo_saver_loop(start_x,start_y);
  else if(appearance.screensaver==4)abstractile_saver_loop(start_x,start_y);
  else if(appearance.screensaver==5)mystify_saver_loop(start_x,start_y);
  else if(appearance.screensaver==6)splotchy_saver_loop(start_x,start_y);
  else if(appearance.screensaver==7)halftone_saver_loop(start_x,start_y);
  else if(appearance.screensaver==8)pipes_saver_loop(start_x,start_y);
  else if(appearance.screensaver==9)scooter_saver_loop(start_x,start_y);
  else if(appearance.screensaver==10)rocks_saver_loop(start_x,start_y);
  else if(appearance.screensaver==11)blaster_saver_loop(start_x,start_y);
  else if(appearance.screensaver==12)squiral_saver_loop(start_x,start_y);
  else if(appearance.screensaver==13)starry_saver_loop(start_x,start_y);
  else warp_saver_loop(start_x,start_y);
  memset(&r,0,sizeof(r));r.h.al=(unsigned char)old_mode;int86(0x10,&r,&r);
  memset(&r,0,sizeof(r));r.x.ax=0x1003;r.x.bx=0;int86(0x10,&r,&r);
  if(old_rows>25){memset(&r,0,sizeof(r));r.x.ax=0x1112;r.h.bl=0;int86(0x10,&r,&r);}
  video_init();
  /* Restoring the text mode also restores the adapter's character set.
     Reapply the selected VGA font, or the EGA LaunchUI overlay, before
     rebuilding the mouse pointer glyph. */
  if(font_is_vga())font_restore();
  else launchui_rebase();
  memset(&r,0,sizeof(r));r.h.ah=1;r.h.ch=0x20;r.h.cl=0;int86(0x10,&r,&r);
  if(mouse_present)mouse_pointer_install();
}

static int mouse_start(void)
{
  union REGS r;
  mouse_visible=0;
  r.x.ax=0;int86(0x33,&r,&r);if(r.x.ax==0)return 0;
  r.x.ax=4;r.x.cx=1;r.x.dx=1;int86(0x33,&r,&r);
  mouse_pointer_install();
  r.x.ax=1;int86(0x33,&r,&r);
  mouse_visible=1;
  r.x.ax=3;int86(0x33,&r,&r);mouse_last_buttons=r.x.bx;
  mouse_raw_x=r.x.cx;mouse_raw_y=r.x.dx;
  return 1;
}

static void mouse_stop(void)
{
  union REGS r;if(!mouse_present || !mouse_visible)return;
  r.x.ax=2;int86(0x33,&r,&r);mouse_visible=0;
}

static unsigned mouse_poll(int *column,int *row)
{
  union REGS r;unsigned pressed;
  if(!mouse_present)return 0;
  r.x.ax=3;int86(0x33,&r,&r);
  mouse_raw_x=r.x.cx;mouse_raw_y=r.x.dx;
  *column=r.x.cx/8;*row=r.x.dx/8;
  pressed=r.x.bx&~mouse_last_buttons;mouse_last_buttons=r.x.bx;
  return pressed;
}

static void wait_input(int *key,int *column,int *row,unsigned *buttons)
{
  int start_column,start_row;
  *key=0;*buttons=0;
  start_column=(int)(mouse_raw_x/8);start_row=(int)(mouse_raw_y/8);
  mouse_show();
  do {
    if(key_waiting()){*key=keyread();edit_caret_hide();mouse_stop();return;}
    *buttons=mouse_poll(column,row);
    if(!*buttons && (*column!=start_column || *row!=start_row)){
      *buttons=MOUSE_MOVED;edit_caret_hide();mouse_stop();return;
    }
  } while(!*buttons);
  mouse_stop();
  if((*buttons&1) && (*column==dialog_close_x||*column==dialog_close_x+1) && *row==dialog_close_y){
    *buttons=0;*key=27;
  }
}

static int child_count(int parent)
{
  int i,n=0;
  for(i=0;i<node_count;i++)if(nodes[i].active && nodes[i].parent==parent)n++;
  return n;
}

static int ordered_child(int parent,int position)
{
  int i,pass,best=-1,best_order,last_order=-1,last_index=-1;
  for(pass=0;pass<=position;pass++){
    best=-1;best_order=256;
    for(i=0;i<node_count;i++)if(nodes[i].active && nodes[i].parent==parent &&
       ((int)nodes[i].order>last_order ||
        ((int)nodes[i].order==last_order && i>last_index)) &&
       ((int)nodes[i].order<best_order ||
        ((int)nodes[i].order==best_order && i<best))){
      best=i;best_order=nodes[i].order;
    }
    if(best<0)return -1;
    last_order=best_order;last_index=best;
  }
  return best;
}

static int write_section(FILE *f,int parent,int path_len)
{
  int n,i,node,old_len;
  n=child_count(parent);
  if(fprintf(f,"[%s]\n",write_path)<0)return 0;
  for(i=0;i<n;i++){
    node=ordered_child(parent,i);if(node<0)return 0;
    if(nodes[node].separator){if(fputs("SEPARATOR=\n",f)==EOF)return 0;}
    else if(nodes[node].folder){if(fprintf(f,"FOLDER=%s\n",nodes[node].title)<0)return 0;}
    else if(fprintf(f,"ITEM=%s|%s|%u|%u|%u|%u\n",nodes[node].title,nodes[node].command,
                    nodes[node].press_enter,nodes[node].change_dir,
                    nodes[node].prompt_params,nodes[node].add_path)<0)return 0;
  }
  if(fputc('\n',f)==EOF)return 0;
  for(i=0;i<n;i++){
    node=ordered_child(parent,i);
    if(node>=0 && nodes[node].folder){
      old_len=path_len;
      if(old_len+1+(int)strlen(nodes[node].title)>=MAX_CMD)return 0;
      write_path[old_len]='\\';strcpy(write_path+old_len+1,nodes[node].title);
      if(!write_section(f,node,old_len+1+strlen(nodes[node].title)))return 0;
      write_path[old_len]=0;
    }
  }
  return 1;
}

static int write_current_config(const char *name)
{
  FILE *f=fopen(name,"wt");int ok;
  if(!f)return 0;
  ok=fputs("; Launch! 3.5 menu definition\n; ITEM=title|command and parameters|press Enter|change directory|prompt|add to PATH (0/1)\n; SEPARATOR= adds a movable horizontal separator\n\n",f)!=EOF;
  strcpy(write_path,"Launcher");
  if(ok)ok=write_section(f,-1,8);
  if(fclose(f)!=0)ok=0;
  return ok;
}

static int copy_file(const char *source,const char *destination)
{
  FILE *in,*out;size_t got;int ok=1;
  in=fopen(source,"rb");if(!in)return 0;
  out=fopen(destination,"wb");if(!out){fclose(in);return 0;}
  while((got=fread(copy_buffer,1,sizeof(copy_buffer),in))!=0)
    if(fwrite(copy_buffer,1,got,out)!=got){ok=0;break;}
  if(ferror(in))ok=0;
  if(fclose(out)!=0)ok=0;
  fclose(in);
  if(!ok)remove(destination);
  return ok;
}

static int save_appearance(void)
{
  FILE *f;int ok=1;
  remove(appearance_temp_file);
  f=fopen(appearance_temp_file,"wt");if(!f)return 0;
  if(fputs("; Launch! 3.5 appearance settings\n",f)==EOF)ok=0;
  if(ok && fprintf(f,"BACKGROUND=%u\nBORDER=%u\nTITLEBAR_FG=%u\nTITLEBAR_BG=%u\nMAIN_TITLE=%u\nTITLES=%u\n"
      "FOLDERS=%u\nLAUNCHERS=%u\nSELECTED_FG=%u\nSELECTED_BG=%u\n"
      "CONTROLS_FG=%u\nCONTROLS_BG=%u\nLABELS=%u\nMENU_TOP=%u\n"
      "SCREENSAVER=%u\nSAVER_COLOR=%u\nSAVER_DELAY=%u\nHOUR_12=%u\n"
      "SHOW_COLLECTIONS=%u\nSHOW_EXPLORE=%u\n"
      "SHOW_POWER=%u\nSHOW_TIME=%u\nSHOW_SYSBAR=%u\nFONT_ID=%u\nFONT_PERSIST=%u\n"
      "MOUSE_CURSOR=%u\n",
      appearance.background,appearance.border,appearance.titlebar_fg,appearance.titlebar_bg,appearance.main_title,appearance.titles,
      appearance.folders,appearance.launchers,appearance.selected_fg,appearance.selected_bg,
      appearance.controls_fg,appearance.controls_bg,appearance.labels,
      appearance.menu_top,appearance.screensaver,appearance.saver_color,
      appearance.saver_delay,appearance.hour_12,
      appearance.show_collections,appearance.show_explore,appearance.show_power,
      appearance.show_time,appearance.show_sysbar,appearance.font_id,appearance.font_persist,
      appearance.mouse_cursor)<0)ok=0;
  if(fclose(f)!=0)ok=0;
  if(!ok){remove(appearance_temp_file);return 0;}
  remove(appearance_file);
  if(rename(appearance_temp_file,appearance_file)!=0)return 0;
  shortcut_idle_sync();
  return 1;
}

static int update_backup(void);
static int save_config(void);

/* force_recreate discards the current menu and rebuilds only the standard
   installed-component/DOS entries.  With force_recreate false, the same
   entries are merged into an existing valid menu without duplicating folders
   or launchers. */
static int default_menu_file(int force_recreate)
{
  int changed=0,had_menu=file_exists(config_file);
  if(force_recreate){
    node_count=0;remove(temp_file);remove(backup_temp_file);
    if(!had_menu)remove(backup_file);
  } else if(had_menu){
    if(!load_config(config_file))return 0;
  } else node_count=0;
  if(!merge_default_menu_nodes(&changed))return 0;
  if(force_recreate||!had_menu||changed){
    if(!save_config())return 0;
    if(!file_exists(backup_file)&&!update_backup())return 0;
  }
  return 1;
}

/* 1=normal, 2=restored backup, 3=rebuilt defaults, 0=failure. */
static int prepare_config(void)
{
  int primary_exists=file_exists(config_file);
  int backup_exists=file_exists(backup_file);
  if(primary_exists && load_config(config_file))return 1;
  /* A genuinely missing LAUNCH.MNU means "rebuild the installed defaults".
     Do not resurrect an old personal LAUNCH.BAK in that case. */
  if(!primary_exists){
    if(default_menu_file(1))return 3;
    return 0;
  }
  if(backup_exists && load_config(backup_file)){
    remove(bad_file);rename(config_file,bad_file);
    if(copy_file(backup_file,config_file))return 2;
    return 0;
  }
  remove(bad_file);
  if(rename(config_file,bad_file)!=0)return 0;
  if(!default_menu_file(1))return 0;
  return 3;
}

static int update_backup(void)
{
  remove(backup_temp_file);
  if(!copy_file(config_file,backup_temp_file))return 0;
  remove(backup_file);
  if(rename(backup_temp_file,backup_file)!=0){remove(backup_temp_file);return 0;}
  return 1;
}

static int save_config(void)
{
  remove(temp_file);
  if(!write_current_config(temp_file)){remove(temp_file);return 0;}
  if(file_exists(config_file) && !update_backup()){
    remove(temp_file);load_config(config_file);return 0;
  }
  if(remove(config_file)!=0 && file_exists(config_file)){
    remove(temp_file);load_config(config_file);return 0;
  }
  if(rename(temp_file,config_file)!=0){
    if(file_exists(backup_file))copy_file(backup_file,config_file);
    remove(temp_file);load_config(config_file);return 0;
  }
  return 1;
}

static void delete_tree(int node)
{
  int i;
  nodes[node].active=0;
  for(i=0;i<node_count;i++) if(nodes[i].active && nodes[i].parent==node) delete_tree(i);
}

static void normalize_order(int parent)
{
  int list[MAX_CHILD],n,i;
  n=children(parent,list); for(i=0;i<n;i++) nodes[list[i]].order=(unsigned char)i;
}

static int move_item(int parent,int position,int direction)
{
  int list[MAX_CHILD],n,a,b,t;
  n=children(parent,list); b=position+direction;
  if(position<0 || position>=n || b<0 || b>=n) return position;
  a=list[position];
  if(!stricmp(nodes[a].title,"More") || !stricmp(nodes[list[b]].title,"More"))return position;
  t=nodes[a].order; nodes[a].order=nodes[list[b]].order; nodes[list[b]].order=(unsigned char)t;
  return b;
}
static void sort_menu(int parent)
{
  int list[MAX_CHILD],n,i,j,t;
  n=children(parent,list);
  for(i=0;i<n-1;i++) for(j=i+1;j<n;j++)
    if(!nodes[list[i]].separator && !nodes[list[j]].separator &&
       (!stricmp(nodes[list[i]].title,"More") ||
       (stricmp(nodes[list[j]].title,"More") && stricmp(nodes[list[i]].title,nodes[list[j]].title)>0)))
      {t=list[i];list[i]=list[j];list[j]=t;}
  for(i=0;i<n;i++) nodes[list[i]].order=(unsigned char)i;
}

static void toolbar_divider(int x,int y,int width){int i;cell(x,y,179,C_BORDER);for(i=1;i<width-1;i++)cell(x+i,y,216,C_BORDER);cell(x+width-1,y,179,C_BORDER);}

static int button_icon(const char *label,int *a,int *b)
{
  /* Preview is deliberately a text button. */
  if(strstr(label,"Preview"))return 0;
  if(!strcmp(label,"  ?  ")){*a=174;*b=-1;return 1;}
  if(strstr(label,"Prev")){*a=17;*b=-1;return 1;}
  if(strstr(label,"Next")){*a=16;*b=-1;return 1;}
  if(strstr(label,"Open")){*a=UI_OPEN_L;*b=UI_OPEN_R;return 3;}
  if(strstr(label,"Search")){*a=UI_SEARCH_L;*b=UI_SEARCH_R;return 1;}
  if(strstr(label,"Save")||strstr(label,"Export")){*a=204;*b=181;return 1;}
  if(strstr(label,"Yes")||strstr(label," OK ")){*a=198;*b=225;return 1;}
  if(strstr(label,"Cancel")||strstr(label,"No")||strstr(label,"Close")){*a=200;*b=201;return 1;}
  if(strstr(label,"Exit")){*a=UI_EXIT_L;*b=UI_EXIT_R;return 1;}
  if(strstr(label,"Run")&&!strstr(label,"Preview")){*a=202;*b=224;return 2;}
  if(strstr(label,"Print")){*a=234;*b=229;return 1;}
  if(strstr(label,"Add")||strstr(label,"New")){*a=208;*b=187;return 1;}
  if(strstr(label,"Edit")){*a=210;*b=182;return 1;}
  if(strstr(label,"Delete")||strstr(label,"Remove")){*a=209;*b=188;return 1;}
  if(strstr(label,"Retry")||strstr(label,"Refresh")){*a=235;*b=255;return 1;}
  return 0;
}
static void draw_button_state(int x,int y,const char *label,int width,int focused,int enabled)
{
  int i,a=0,b=0,icon=button_icon(label,&a,&b),left;unsigned short v;if(icon==2)width=9;else if(icon==3)width=9;else if(icon)width=(b<0)?5:6;/* Clear only the actual rendered button plus its one-cell shadow.  The old code cleared the caller's legacy text width after shrinking an icon button, which erased the two-cell right margin and dialog border. */for(i=0;i<=width;i++){cell(x+i,y,' ',C_MENU_BACKGROUND);cell(x+i,y+1,' ',C_MENU_BACKGROUND);}
  for(i=1;i<=width;i++){v=video[(y+1)*screen_cols+x+i];cell(x+i,y+1,220,((v>>8)&0xF0)|C_BLOCK_SHADOW_FG);}
  v=video[y*screen_cols+x+width];cell(x+width,y,245,((v>>8)&0xF0)|C_BLOCK_SHADOW_FG);v=video[(y+1)*screen_cols+x+width];cell(x+width,y+1,244,((v>>8)&0xF0)|C_BLOCK_SHADOW_FG);
  {int ba=enabled?C_BUTTON:ATTR(appearance.controls_bg,(appearance.controls_fg&7)|8);textout(x,y,"",ba,width);if(icon==2){left=x+2;cell(left,y,a,ba);cell(left+1,y,b,ba);textout(left+2,y,"Run",ba,3);}else if(icon==3){left=x+1;cell(left,y,a,ba);cell(left+1,y,b,ba);textout(left+3,y,"Open",ba,4);}else if(icon){left=x+(b<0?2:(width-2)/2);cell(left,y,a,ba);if(b>=0)cell(left+1,y,b,ba);}else textout(x,y,label,ba,width);}if(focused&&enabled){int fa=ATTR(appearance.controls_bg,appearance.main_title);cell(x,y,169,fa);cell(x+width-1,y,170,fa);}
}
static void draw_button(int x,int y,const char *label,int width,int focused){draw_button_state(x,y,label,width,focused,1);}
static void draw_button_disabled(int x,int y,const char *label,int width){draw_button_state(x,y,label,width,0,0);}
static void press_button(int x,int y,const char *label,int width)
{
  int i,mx=0,my=0,a=0,b=0,rw=width,icon=button_icon(label,&a,&b),left;
  if(icon==2)rw=9;else if(icon==3)rw=9;else if(icon)rw=(b<0)?5:6;
  /* Pressed state: keep the focused end glyphs, but remove the complete
     one-cell drop shadow for the duration of the press. */
  for(i=0;i<=rw;i++){cell(x+i,y,' ',C_MENU_BACKGROUND);cell(x+i,y+1,' ',C_MENU_BACKGROUND);}
  textout(x,y,"",C_BUTTON,rw);
  if(icon==2){left=x+2;cell(left,y,a,C_BUTTON);cell(left+1,y,b,C_BUTTON);textout(left+2,y,"Run",C_BUTTON,3);}
  else if(icon==3){left=x+1;cell(left,y,a,C_BUTTON);cell(left+1,y,b,C_BUTTON);textout(left+3,y,"Open",C_BUTTON,4);}
  else if(icon){left=x+(b<0?2:(rw-2)/2);cell(left,y,a,C_BUTTON);if(b>=0)cell(left+1,y,b,C_BUTTON);}
  else textout(x,y,label,C_BUTTON,rw);
  {int fa=ATTR(appearance.controls_bg,appearance.main_title);cell(x,y,169,fa);cell(x+rw-1,y,170,fa);}
  mouse_show();do{(void)mouse_poll(&mx,&my);}while(mouse_last_buttons&1);mouse_stop();
}

static void wrap_message(const char *message,char *line1,char *line2,int width)
{
  int len=strlen(message),cut=width;
  line1[0]=line2[0]=0;
  if(len<=width){strcpy(line1,message);return;}
  while(cut>0 && message[cut]!=' ')cut--;
  if(cut==0)cut=width;
  strncpy(line1,message,cut);line1[cut]=0;
  while(message[cut]==' ')cut++;
  strncpy(line2,message+cut,width);line2[width]=0;
}

static void message_icon(int x,int y,int type)
{
 int fg,bg,glyph=(type==2)?174:173;
 if(type==0){fg=4;bg=4;}else if(type==1){fg=6;bg=6;}else{fg=7;bg=7;}
 cell(x,y,219,ATTR(appearance.background,fg));cell(x+1,y,glyph,ATTR(bg,(type==2)?9:15));cell(x+2,y,219,ATTR(appearance.background,fg));
}
static int message_type(const char *title,const char *message)
{
 if(title&&(!stricmp(title,"Remove Item")||strstr(title,"Shutdown")))return 2;
 if((title&&(strstr(title,"Error")||strstr(title,"Full")||strstr(title,"Reserved")||strstr(title,"Duplicate")))||(message&&(strstr(message,"Unable")||strstr(message,"Could not")||strstr(message,"too long"))))return 0;
 if(title&&strstr(title,"Warning"))return 1;return 2;
}

static int confirm_box(const char *title,const char *message)
{
  int h,k,yes=0,mx=0,my=0,x,y,by;
  unsigned mb;
  char line1[43],line2[43];
  wrap_message(message,line1,line2,42);
  h=line2[0]?9:8; /* one clear row between message and toolbar divider */
  x=(screen_cols-48)/2;y=(screen_rows-h)/2;by=y+h-3;
  subdialog_box(x,y,48,h,title);message_icon(x+3,y+2,message_type(title,message));
  textout(x+7,y+2,line1,C_INPUT_LABEL,38);
  if(line2[0])textout(x+7,y+3,line2,C_INPUT_LABEL,38);
  if(!stricmp(title,"Remove Item")&&remove_item_name[0]){
    int nx=x+7+(int)strlen("Remove ");
    textout(x+7,y+2,"                                      ",C_INPUT_LABEL,38);
    textout(x+7,y+2,"Remove ",C_INPUT_LABEL,7);
    textout(nx,y+2,remove_item_name,remove_item_colour,24);
    nx+=(int)strlen(remove_item_name);textout(nx,y+2," from the menu?",C_INPUT_LABEL,15);
  }
  for(;;){
    wait_vertical_retrace();
    {int yx=!stricmp(title,"Remove Item")?x+3:x+14,nx=!stricmp(title,"Remove Item")?x+11:x+28;
    draw_button(yx,by,"  Yes  ",7,yes);
    draw_button(nx,by,"  No  ",6,!yes);
    wait_input(&k,&mx,&my,&mb);
    if((mb&MOUSE_MOVED) && my==by){if(mx>=yx&&mx<yx+7)yes=1;else if(mx>=nx&&mx<nx+6)yes=0;continue;}
    if((mb&1)&&my==by){if(mx>=yx&&mx<yx+7){press_button(yx,by,"  Yes  ",7);return 1;}if(mx>=nx&&mx<nx+6){press_button(nx,by,"  No  ",6);return 0;}}}
    if(k==27)return 0;
    if(k==0x4B00||k==0x4D00||k==9||k==0x0F00)yes=!yes;
    else if(k==13)return yes;else if(k=='y'||k=='Y')return 1;else if(k=='n'||k=='N')return 0;
  }
}

static void notice_box(const char *title,const char *message)
{
  int w=48,h,k,mx=0,my=0,x,y,button_x,by;
  unsigned mb;char line1[49],line2[49];
  wrap_message(message,line1,line2,42);
  if((int)strlen(line1)>38||(int)strlen(line2)>38)w=54;
  h=line2[0]?9:8; /* one clear row between message and toolbar divider */
  x=(screen_cols-w)/2;y=(screen_rows-h)/2;button_x=x+(w-6)/2;by=y+h-3;
  subdialog_box(x,y,w,h,title);message_icon(x+3,y+2,message_type(title,message));
  textout(x+7,y+2,line1,C_INPUT_LABEL,w-10);if(line2[0])textout(x+7,y+3,line2,C_INPUT_LABEL,w-10);
  draw_button(button_x,by,"  OK  ",6,1);
  for(;;){wait_input(&k,&mx,&my,&mb);if(mb&MOUSE_MOVED)continue;if(mb&1){if(my==by&&mx>=button_x&&mx<button_x+6){press_button(button_x,by,"  OK  ",6);return;}continue;}if(k)return;}
}

static unsigned char physical_byte(unsigned long address)
{
  return *(unsigned char far *)MAKE_FP((unsigned)(address>>4),
                                        (unsigned)(address&15));
}

static unsigned physical_word(unsigned long address)
{
  return (unsigned)physical_byte(address)|
         ((unsigned)physical_byte(address+1)<<8);
}

static unsigned long physical_dword(unsigned long address)
{
  return (unsigned long)physical_word(address)|
         ((unsigned long)physical_word(address+2)<<16);
}

static int physical_match(unsigned long address,const char *text,int length)
{
  int i;for(i=0;i<length;i++)if(physical_byte(address+i)!=(unsigned char)text[i])return 0;
  return 1;
}

static int physical_checksum(unsigned long address,unsigned long length)
{
  unsigned char sum=0;unsigned long i;
  for(i=0;i<length;i++)sum=(unsigned char)(sum+physical_byte(address+i));
  return sum==0;
}

static unsigned long find_rsdp_range(unsigned long first,unsigned long last)
{
  unsigned long p;
  for(p=first;p+20<=last;p+=16)
    if(physical_match(p,"RSD PTR ",8) && physical_checksum(p,20))return p;
  return 0;
}

static unsigned long find_rsdp(void)
{
  unsigned long ebda=((unsigned long)physical_word(0x40E))<<4;
  unsigned long found=0;
  if(ebda>=0x80000UL && ebda<0xA0000UL)found=find_rsdp_range(ebda,ebda+1024);
  return found?found:find_rsdp_range(0xE0000UL,0x100000UL);
}

static int aml_integer(unsigned long p,unsigned *value)
{
  unsigned char op=physical_byte(p);
  if(op==0){*value=0;return 1;}
  if(op==1){*value=1;return 1;}
  if(op==0x0A){*value=physical_byte(p+1);return 1;}
  if(op==0x0B){*value=physical_word(p+1);return 1;}
  if(op==0x0C){*value=(unsigned)physical_dword(p+1);return 1;}
  return 0;
}

static int find_s5(unsigned long dsdt,unsigned *type_a,unsigned *type_b)
{
  unsigned long length,p,end,q;unsigned char lead,follow;
  if(!dsdt || dsdt>=0x100000UL || !physical_match(dsdt,"DSDT",4))return 0;
  length=physical_dword(dsdt+4);if(length<36 || length>0x10000UL)return 0;
  end=dsdt+length;
  for(p=dsdt+36;p+12<end;p++)if(physical_match(p,"_S5_",4)){
    q=p+4;if(physical_byte(q)!=0x12)continue;q++;
    lead=physical_byte(q++);follow=(unsigned char)(lead>>6);
    q+=follow;if(q>=end)continue;
    q++; /* package element count */
    if(!aml_integer(q,type_a))continue;
    lead=physical_byte(q);
    q+=(lead==0x0A)?2:(lead==0x0B)?3:(lead==0x0C)?5:1;
    if(!aml_integer(q,type_b))*type_b=*type_a;
    return 1;
  }
  return 0;
}

static int acpi_sleep_info(unsigned *pm1a,unsigned *pm1b,unsigned *smi,
                           unsigned char *enable,unsigned *type_a,unsigned *type_b)
{
  unsigned long rsdp,rsdt,length,p,fadt=0,dsdt;
  rsdp=find_rsdp();if(!rsdp)return 0;
  rsdt=physical_dword(rsdp+16);
  if(!rsdt || rsdt>=0x100000UL || !physical_match(rsdt,"RSDT",4))return 0;
  length=physical_dword(rsdt+4);
  if(length<36 || length>0x10000UL || rsdt+length>0x100000UL ||
     !physical_checksum(rsdt,length))return 0;
  for(p=rsdt+36;p+4<=rsdt+length;p+=4){
    fadt=physical_dword(p);
    if(fadt && fadt<0x100000UL && physical_match(fadt,"FACP",4))break;
    fadt=0;
  }
  if(!fadt || physical_dword(fadt+4)<72)return 0;
  dsdt=physical_dword(fadt+40);
  *smi=(unsigned)physical_dword(fadt+48);*enable=physical_byte(fadt+52);
  *pm1a=(unsigned)physical_dword(fadt+64);*pm1b=(unsigned)physical_dword(fadt+68);
  return *pm1a && find_s5(dsdt,type_a,type_b);
}

static unsigned port_in_word(unsigned port)
{
  return (unsigned)_inpw(port);
}

static void port_out_word(unsigned port,unsigned value)
{
  _outpw(port,value);
}

static void port_out_byte(unsigned port,unsigned char value)
{
  _outp(port,value);
}

static void flush_disk_buffers(void)
{
  union REGS r;
  memset(&r,0,sizeof(r));r.h.ah=0x0D;int86(0x21,&r,&r);
  memset(&r,0,sizeof(r));r.x.ax=0x4A10;r.x.bx=0;r.x.cx=0;
  int86(0x2F,&r,&r);
  if(r.x.ax==0xBABE){
    memset(&r,0,sizeof(r));r.x.ax=0x4A10;r.x.bx=1;r.x.cx=0;
    int86(0x2F,&r,&r);
  }
  memset(&r,0,sizeof(r));r.h.ah=0x0D;int86(0x21,&r,&r);
}

static unsigned read_word(FILE *f)
{
  int a=fgetc(f),b=fgetc(f);
  if(a==EOF || b==EOF)return 0;
  return (unsigned)a|((unsigned)b<<8);
}

static unsigned long read_dword(FILE *f)
{
  unsigned long low=read_word(f),high=read_word(f);
  return low|(high<<16);
}

static void wait_for_escape(void)
{
  union REGS r;
  do {memset(&r,0,sizeof(r));int86(0x16,&r,&r);} while(r.h.al!=27);
}

static void text_safe_screen(void)
{
  int i,x,y;const char *first="It's now safe to turn off";
  const char *second="your computer.";
  video_init();if(!save_screen())return;
  cursor_hide();
  for(i=0;i<screen_cols*screen_rows;i++)video[i]=(unsigned short)(0x0C00|' ');
  y=screen_rows/2-1;x=(screen_cols-(int)strlen(first))/2;
  textout(x,y,first,0x0C,(int)strlen(first));
  x=(screen_cols-(int)strlen(second))/2;
  textout(x,y+1,second,0x0C,(int)strlen(second));
  wait_for_escape();restore_screen();cursor_restore();_ffree(saved);saved=0;
}

static int bitmap_header(FILE *f,unsigned long *bits)
{
  unsigned long width,height,compression;
  if(read_word(f)!=0x4D42)return 0;
  (void)read_dword(f);(void)read_word(f);(void)read_word(f);*bits=read_dword(f);
  if(read_dword(f)<40)return 0;
  width=read_dword(f);height=read_dword(f);
  if(read_word(f)!=1 || read_word(f)!=8)return 0;
  compression=read_dword(f);
  return width==320 && height==400 && compression==0 && *bits>=1078;
}

static void set_320x400_mode(void)
{
  union REGS r;unsigned far *clear=(unsigned far *)MAKE_FP(0xA000,0);unsigned i;
  memset(&r,0,sizeof(r));r.h.al=0x13;int86(0x10,&r,&r);
  port_out_word(0x3C4,0x0604); /* disable chain 4 and odd/even */
  port_out_word(0x3CE,0x4005); /* disable Graphics Controller odd/even */
  port_out_word(0x3CE,0x0106); /* disable Graphics Controller chaining */
  port_out_word(0x3C4,0x0F02); /* enable writes to all four planes */
  for(i=0;i<0x8000;i++)clear[i]=0; /* clear both 320x400 pages */
  port_out_word(0x3D4,0x0009); /* one displayed scan line per bitmap row */
  port_out_word(0x3D4,0x2014); /* disable doubleword addressing */
  port_out_word(0x3D4,0xE317); /* enable byte-mode display addressing */
}

static int graphics_safe_screen(void)
{
  FILE *f;union REGS r;unsigned long bits;unsigned old_mode,old_cursor;
  int old_cols,old_rows,i,y,plane,failed=0;static unsigned char row[320];
  unsigned char far *vga=(unsigned char far *)MAKE_FP(0xA000,0);
  f=fopen(logos_file,"rb");if(!f)return 0;
  if(!bitmap_header(f,&bits)){fclose(f);return 0;}
  if(fseek(f,0L,SEEK_END)!=0 || ftell(f)<(long)bits+128000L){fclose(f);return 0;}
  memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);
  if(r.h.al!=0x1A){fclose(f);return 0;} /* 320x400 requires VGA */
  video_init();old_cols=screen_cols;old_rows=screen_rows;
  if(!save_screen()){fclose(f);return 0;}
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);old_mode=r.h.al;
  memset(&r,0,sizeof(r));r.h.ah=3;r.h.bh=0;int86(0x10,&r,&r);old_cursor=r.x.dx;
  cursor_hide();set_320x400_mode();
  fseek(f,54L,SEEK_SET);port_out_byte(0x3C8,0);
  for(i=0;i<256;i++){
    int blue=fgetc(f),green=fgetc(f),red=fgetc(f);(void)fgetc(f);
    if(blue==EOF || green==EOF || red==EOF){failed=1;break;}
    port_out_byte(0x3C9,(unsigned char)(red>>2));
    port_out_byte(0x3C9,(unsigned char)(green>>2));
    port_out_byte(0x3C9,(unsigned char)(blue>>2));
  }
  for(y=0;y<400 && !failed;y++){
    if(fseek(f,(long)bits+(long)(399-y)*320L,SEEK_SET)!=0 ||
       fread(row,1,320,f)!=320){failed=1;break;}
    for(plane=0;plane<4;plane++){
      port_out_word(0x3C4,(unsigned)(((1<<plane)<<8)|2));
      for(i=0;i<80;i++)vga[y*80+i]=row[i*4+plane];
    }
  }
  fclose(f);if(!failed)wait_for_escape();
  memset(&r,0,sizeof(r));r.h.al=(unsigned char)old_mode;int86(0x10,&r,&r);
  if(old_rows>25){memset(&r,0,sizeof(r));r.x.ax=0x1112;r.h.bl=0;int86(0x10,&r,&r);}
  screen_cols=old_cols;screen_rows=old_rows;
  video=(unsigned short far *)MAKE_FP(old_mode==7?0xB000:0xB800,0);
  restore_screen();cursor_restore();
  memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=0;r.x.dx=old_cursor;int86(0x10,&r,&r);
  _ffree(saved);saved=0;return !failed;
}

static void safe_to_turn_off(void)
{
  if(!graphics_safe_screen())text_safe_screen();
}

static int apm_call(unsigned ax,unsigned bx,unsigned cx,union REGS *result)
{
  memset(result,0,sizeof(*result));
  result->x.ax=ax;result->x.bx=bx;result->x.cx=cx;
  int86(0x15,result,result);
  return !result->x.cflag;
}

static int apm_poweroff(void)
{
  union REGS r;

  /* This is the same APM 1.2 sequence used by APMTEST /OFF. */
  if(!apm_call(0x5300,0x0000,0x0000,&r) || r.x.bx!=0x504D)return 0;
  if(!apm_call(0x5301,0x0000,0x0000,&r))return 0;

  if(!apm_call(0x530E,0x0000,0x0102,&r) ||
     !apm_call(0x5308,0x0001,0x0001,&r) ||
     !apm_call(0x530F,0x0001,0x0001,&r)){
    apm_call(0x5304,0x0000,0x0000,&r);
    return 0;
  }

  flush_disk_buffers();
  if(!apm_call(0x5307,0x0001,0x0003,&r)){
    apm_call(0x5304,0x0000,0x0000,&r);
    return 0;
  }

  /* A successful power-off should never return.  If it does, let ACPI try. */
  apm_call(0x5304,0x0000,0x0000,&r);
  return 0;
}

static int acpi_poweroff(void)
{
  unsigned pm1a,pm1b,smi,type_a,type_b,value;unsigned char enable;long wait;
  flush_disk_buffers();
  if(!acpi_sleep_info(&pm1a,&pm1b,&smi,&enable,&type_a,&type_b))return 0;
  if(!(port_in_word(pm1a)&1) && smi && enable){
    port_out_byte(smi,enable);
    for(wait=0;wait<200000L && !(port_in_word(pm1a)&1);wait++);
  }
  value=(unsigned)((port_in_word(pm1a)&0x03FF)|((type_a&7)<<10)|0x2000);
  port_out_word(pm1a,value);
  if(pm1b)port_out_word(pm1b,(unsigned)((port_in_word(pm1b)&0x03FF)|((type_b&7)<<10)|0x2000));
  return 0; /* successful ACPI S5 does not return */
}

static int power_off(void)
{
  if(apm_poweroff())return 1;
  return acpi_poweroff();
}

static void cold_reboot(void)
{
  long wait;
  flush_disk_buffers();
  *(unsigned far *)MAKE_FP(0x40,0x72)=0; /* request a cold, not warm, boot */
  _disable();
  for(wait=0;wait<200000L;wait++){
    unsigned char status=(unsigned char)_inp(0x64);
    if(!(status&2))break;
  }
  port_out_byte(0x64,0xFE);
  for(;;);
}

static int power_dialog(void)
{
  int x=(screen_cols-54)/2,y=(screen_rows-8)/2,k,choice=2,mx=0,my=0;unsigned mb;
  dialog_box(x,y,54,8,"Shutdown...");message_icon(x+3,y+2,2);textout(x+7,y+2,"What do you want to do?",C_INPUT_LABEL,42);
  for(;;){
    wait_vertical_retrace();
    draw_button(x+3,y+5,"  Power Off  ",13,choice==0);
    draw_button(x+18,y+5,"  Reboot  ",10,choice==1);
    draw_button(x+44,y+5,"  Cancel  ",10,choice==2);
    wait_input(&k,&mx,&my,&mb);
    if((mb&MOUSE_MOVED) && my==y+5){
      if(mx>=x+3 && mx<x+16)choice=0;
      else if(mx>=x+18 && mx<x+28)choice=1;
      else if(mx>=x+44 && mx<x+50)choice=2;
      continue;
    }
    if((mb&1) && my==y+5){
      if(mx>=x+3 && mx<x+16){press_button(x+3,y+5,"  Power Off  ",13);return 1;}
      if(mx>=x+18 && mx<x+28){press_button(x+18,y+5,"  Reboot  ",10);return 2;}
      if(mx>=x+44 && mx<x+50){press_button(x+44,y+5,"  Cancel  ",10);return 0;}
    }
    if(k==27)return 0;
    if(k==0x4B00)choice=(choice+2)%3;
    else if(k==0x4D00 || k==9 || k==0x0F00)choice=(k==0x0F00)?(choice+2)%3:(choice+1)%3;
    else if(k==13)return choice==0?1:(choice==1?2:0);
  }
}

static int choose_type(void)
{
  int x=(screen_cols-56)/2,y=(screen_rows-8)/2,k,choice=0,mx=0,my=0;unsigned mb;
  subdialog_box(x,y,56,8,"Add Item");message_icon(x+3,y+2,2);textout(x+7,y+2,"Choose the type of item to add:",C_INPUT_LABEL,44);
  for(;;){
    wait_vertical_retrace();
    draw_button(x+3,y+5,"  Folder  ",10,choice==0);
    draw_button(x+15,y+5,"  Launcher  ",12,choice==1);
    draw_button(x+29,y+5,"  Separator  ",13,choice==2);
    wait_input(&k,&mx,&my,&mb);
    if((mb&MOUSE_MOVED) && my==y+5){
      if(mx>=x+3 && mx<x+13)choice=0;
      else if(mx>=x+15 && mx<x+27)choice=1;
      else if(mx>=x+29 && mx<x+42)choice=2;
      continue;
    }
    if((mb&1) && my==y+5){
      if(mx>=x+3 && mx<x+13){press_button(x+3,y+5,"  Folder  ",10);return 1;}
      if(mx>=x+15 && mx<x+27){press_button(x+15,y+5,"  Launcher  ",12);return 0;}
      if(mx>=x+29 && mx<x+42){press_button(x+29,y+5,"  Separator  ",13);return 2;}
    }
    if(k==27) return -1;
    if(k==0x4B00){choice=(choice+2)%3;}
    else if(k==0x4D00 || k==9){choice=(choice+1)%3;}
    else if(k==13)return choice==0?1:(choice==1?0:2);
  }
}

static void field_line(int x,int y,const char *label,const char *value,
                       int focused,int hovered,int pos,int width)
{
  int len=strlen(label),i,vlen=strlen(value);
  int at=(focused||hovered)?C_SELECTED:C_INPUT_FIELD,scroll=0;
  if(pos>=width)scroll=pos-width+1;
  textout(x,y,label,C_INPUT_LABEL,len); textout(x+13,y,value+scroll,at,width);
  if(focused){i=pos-scroll;edit_caret_set(x+13+i,y);}
}

static void check_line(int x,int y,const char *label,int checked,int focused)
{
  int at=ATTR(appearance.background,appearance.controls_bg);
  cell(x,y,checked?213:212,at);cell(x+1,y,checked?190:189,at);
  textout(x+3,y,label,focused?C_SELECTED:C_INPUT_LABEL,34);
}

static int item_form(int folder,char *name,char *exe,char *params,
                     int *press_enter,int *change_dir,int *prompt_params,int *add_path,int editing)
{
  char *fields[3]; int limits[3],pos[3],count,controls,focus=0,hover=-1;
  int k,x,y,i,len,mx=0,my=0,scroll;
  unsigned mb;
  fields[0]=name;fields[1]=exe;fields[2]=params;
  limits[0]=folder?16:18;limits[1]=MAX_CMD-2;limits[2]=MAX_CMD-1;
  name[limits[0]]=0;
  pos[0]=pos[1]=pos[2]=0; count=folder?1:3;
  controls=folder?count:count+4;
  x=(screen_cols-(folder?52:64))/2;y=(screen_rows-(folder?10:15))/2;
  if(folder)subdialog_box(x,y,52,10,editing?"Edit Folder":"Add Folder");else dialog_box(x,y,64,15,editing?"Edit Launcher":"Add Launcher");
  for(;;){
    wait_vertical_retrace();edit_caret_hide();
    field_line(x+3,y+2,"Name:",name,focus==0,hover==0,pos[0],folder?30:42);
    if(!folder){
      field_line(x+3,y+4,"Command:",exe,focus==1,hover==1,pos[1],42);
      field_line(x+3,y+6,"Parameters:",params,focus==2,hover==2,pos[2],28);
      textout(x+47,y+6,"Prompt?",(focus==3||hover==3)?C_SELECTED:C_INPUT_LABEL,7);
      cell(x+55,y+6,*prompt_params?213:212,ATTR(appearance.background,appearance.controls_bg));
      cell(x+56,y+6,*prompt_params?190:189,ATTR(appearance.background,appearance.controls_bg));
      cell(x+57,y+6,' ',C_MENU_BACKGROUND);
    }
    if(!folder){
      check_line(x+16,y+8,"Provide \021\331 after launcher command",*press_enter,focus==4||hover==4);
      check_line(x+16,y+9,"Change directory first",*change_dir,focus==5||hover==5);
      check_line(x+16,y+10,"Add to PATH for execution",*add_path,focus==6||hover==6);
    }
    draw_button(x+3,y+(folder?7:12),"  OK  ",8,focus==controls||hover==controls);
    draw_button(x+11,y+(folder?7:12),"  Cancel  ",10,focus==controls+1||hover==controls+1);
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      hover=-1;
      if(my==y+2 && mx>=x+16 && mx<x+(folder?46:58))hover=0;
      else if(!folder && my==y+4 && mx>=x+16 && mx<x+58)hover=1;
      else if(!folder && my==y+6 && mx>=x+16 && mx<x+44)hover=2;
      else if(!folder && my==y+6 && mx>=x+47 && mx<x+58)hover=3;
      else if(!folder && my==y+8 && mx>=x+16 && mx<x+54)hover=4;
      else if(!folder && my==y+9 && mx>=x+16 && mx<x+54)hover=5;
      else if(!folder && my==y+10 && mx>=x+16 && mx<x+54)hover=6;
      else if(my==y+(folder?7:12) && mx>=x+3 && mx<x+9)hover=controls;
      else if(my==y+(folder?7:12) && mx>=x+11 && mx<x+17)hover=controls+1;
      continue;
    }
    if(mb&1){
      if(my==y+2 && mx>=x+16 && mx<x+(folder?46:58)){focus=0;i=0;}
      else if(!folder && my==y+4 && mx>=x+16 && mx<x+58){focus=1;i=1;}
      else if(!folder && my==y+6 && mx>=x+16 && mx<x+44){focus=2;i=2;}
      else if(!folder && my==y+6 && mx>=x+47 && mx<x+58){*prompt_params=!*prompt_params;focus=3;continue;}
      else if(!folder && my==y+8 && mx>=x+16 && mx<x+54){*press_enter=!*press_enter;focus=4;continue;}
      else if(!folder && my==y+9 && mx>=x+16 && mx<x+54){*change_dir=!*change_dir;focus=5;continue;}
      else if(!folder && my==y+10 && mx>=x+16 && mx<x+54){*add_path=!*add_path;focus=6;continue;}
      else if(my==y+(folder?7:12) && mx>=x+3 && mx<x+9){
        press_button(x+3,y+(folder?7:12),"  OK  ",8);
        if(*name && (folder || *exe)){edit_caret_hide();return 1;}
        focus=controls;continue;
      }
      else if(my==y+(folder?7:12) && mx>=x+11 && mx<x+17){
        press_button(x+11,y+(folder?7:12),"  Cancel  ",6);edit_caret_hide();return 0;
      }
      else continue;
      len=strlen(fields[i]);scroll=pos[i]>(i==2?27:41)?pos[i]-(i==2?27:41):0;
      pos[i]=scroll+mx-(x+16);if(pos[i]>len)pos[i]=len;
      continue;
    }
    if(k==27){edit_caret_hide();return 0;}
    if(k==9 || k==0x0F00 || k==0x5000){focus=(k==0x0F00)?(focus+controls+1)%(controls+2):(focus+1)%(controls+2);continue;}
    if(k==0x4800){focus=(focus+controls+1)%(controls+2);continue;}
    if(!folder && (focus==3 || focus==4 || focus==5 || focus==6)){
      if(k==' ' || k==13){
        if(focus==3)*prompt_params=!*prompt_params;
        else if(focus==4)*press_enter=!*press_enter;
        else if(focus==5)*change_dir=!*change_dir;
        else *add_path=!*add_path;
      }
      continue;
    }
    if(focus>=controls){
      if(k==0x4B00 || k==0x4D00) focus=(focus==controls)?controls+1:controls;
      else if(k==13){if(focus==controls && *name && (folder || *exe)){edit_caret_hide();return 1;}if(focus==controls+1){edit_caret_hide();return 0;}}
      continue;
    }
    i=focus;len=strlen(fields[i]);
    if(k==0x4700){pos[i]=0;}
    else if(k==0x4F00){pos[i]=len;}
    else if(k==0x4B00){if(pos[i]>0)pos[i]--;}
    else if(k==0x4D00){if(pos[i]<len)pos[i]++;}
    else if(k==8){if(pos[i]>0){memmove(fields[i]+pos[i]-1,fields[i]+pos[i],len-pos[i]+1);pos[i]--;}}
    else if(k==0x5300){if(pos[i]<len)memmove(fields[i]+pos[i],fields[i]+pos[i]+1,len-pos[i]);}
    else if(k==13){focus++;}
    else if(k>=32 && k<127 && len<limits[i] &&
            (i!=0 || (k!='|' && k!='\\' && k!='[' && k!=']'))){
      memmove(fields[i]+pos[i]+1,fields[i]+pos[i],len-pos[i]+1);fields[i][pos[i]++]=(char)k;
    }
  }
}

static const char *colour_names[16]={
  "Black","Blue","Green","Cyan","Red","Magenta","Brown","White",
  "Gray","Bri Blue","Bri Green","Bri Cyan","Bri Red","Bri Magenta",
  "Bri Yellow","Bri White"
};

typedef struct {
  const char *name;
  unsigned char colour[13];
} COLOUR_SCHEME;

#define COLOUR_SCHEME_COUNT 9
#define MAX_CUSTOM_SCHEMES 16
static COLOUR_SCHEME custom_schemes[MAX_CUSTOM_SCHEMES];
static char custom_scheme_names[MAX_CUSTOM_SCHEMES][24];
static int custom_scheme_count=0;
static const COLOUR_SCHEME colour_schemes[COLOUR_SCHEME_COUNT]={
  {"Standard", {1,11,15,7,12,14,15,10,15,3,0,7,7}},
  {"Hot Dog",  {4,0,14,7,14,14,7,15,14,0,4,7,12}},
  {"Mono",     {7,8,8,7,0,0,8,0,15,0,7,0,0}},
  {"Pretty",   {5,12,12,4,14,12,7,12,5,3,5,7,15}},
  {"Pro Style",{7,15,7,1,15,9,8,0,11,3,9,1,8}},
  {"Ranch",    {7,6,12,6,14,15,6,4,15,4,7,6,8}},
  {"Stealth",  {0,9,9,0,14,14,7,15,12,0,9,0,15}},
  {"Swiss",    {7,8,0,7,4,15,4,0,15,0,7,4,8}},
  {"Tranquil", {3,11,10,2,10,0,8,0,10,0,15,2,15}}
};

static int colour_key_index(const char *k)
{
  static const char *keys[13]={"BACKGROUND","BORDER","TITLEBAR_FG","TITLEBAR_BG","MAIN_TITLE","TITLES","FOLDERS","LAUNCHERS","SELECTED_FG","SELECTED_BG","CONTROLS_FG","CONTROLS_BG","LABELS"};
  int i;for(i=0;i<13;i++)if(!stricmp(k,keys[i]))return i;return -1;
}
static void load_custom_schemes(void)
{
  char path[MAX_CMD],line[96],*p,*q;FILE *f;int cur=-1,idx,v,seen[13],i;
  custom_scheme_count=0;strcpy(path,program_dir);strcat(path,"COLORS.CFG");f=fopen(path,"rt");if(!f)return;
  memset(seen,0,sizeof(seen));
  while(fgets(line,sizeof(line),f)){
    p=trim(line);if(!*p||*p==';'||*p=='#')continue;
    if(*p=='['){q=strchr(p,']');if(!q)continue;*q=0;if(custom_scheme_count>=MAX_CUSTOM_SCHEMES){cur=-1;continue;}
      cur=custom_scheme_count++;strncpy(custom_scheme_names[cur],p+1,23);custom_scheme_names[cur][23]=0;custom_schemes[cur].name=custom_scheme_names[cur];
      for(i=0;i<13;i++){custom_schemes[cur].colour[i]=colour_schemes[0].colour[i];seen[i]=0;}continue;}
    if(cur<0)continue;q=strchr(p,'=');if(!q)continue;*q++=0;p=trim(p);q=trim(q);idx=colour_key_index(p);if(idx<0)continue;v=atoi(q);if(v<0||v>15)continue;
    if((idx==0||idx==3||idx==9||idx==11)&&v>7)continue;custom_schemes[cur].colour[idx]=(unsigned char)v;seen[idx]=1;
  }
  fclose(f);
}
static int total_colour_schemes(void){return COLOUR_SCHEME_COUNT+custom_scheme_count;}
static const COLOUR_SCHEME *colour_scheme_at(int i){return i<COLOUR_SCHEME_COUNT?&colour_schemes[i]:&custom_schemes[i-COLOUR_SCHEME_COUNT];}

static int colour_scheme_index(void)
{
  const unsigned char *current=&appearance.background;int i,j;
  for(i=0;i<total_colour_schemes();i++){const COLOUR_SCHEME *sc=colour_scheme_at(i);
    for(j=0;j<13;j++)if(current[j]!=sc->colour[j])break;
    if(j==13)return i;
  }
  return -1;
}

static void apply_colour_scheme(int index)
{
  unsigned char *current=&appearance.background;int i;const COLOUR_SCHEME *sc=colour_scheme_at(index);
  for(i=0;i<13;i++)current[i]=sc->colour[i];
}

static const char *screensaver_names[15]={
  "None","Clock","Boing","Logo","Mosaic","Mystic","Paintball","Particles",
  "Pipes","Scooter","Space Junk","Space Wars","Defrag","Starry Nite","Warp"
};

static const char *saver_delay_names[4]={
  "1 minute","5 minutes","15 minutes","30 minutes"
};
static const char *mouse_cursor_names[3]={"Pointer","Block","Up Arrow"};

static const char *font_names[36]={
  "Standard","Launch!","ISO","Clean","Big","Tall","Bold","Bold Alt",
  "Extra","Max","Chunky","Pixel","Humanist","Elite","Max Elite","Elergon",
  "Neat","Gothic","Hand","Scribble","Script","ProFont","ProFont Bold",
  "Bauhaus '89","Bold Italic","Broadway","Courier","Italic","Modern","Nutso",
  "Super","Times","Tiny","Serif","Poster","News"
};
#define BUILTIN_FONT_COUNT 36
#define MAX_EXTERNAL_FONTS 28
static char external_font_file[MAX_EXTERNAL_FONTS][13],external_font_name[MAX_EXTERNAL_FONTS][13];
static int external_font_count=0;
static void scan_external_fonts(void)
{
  struct find_t ff;char mask[MAX_CMD],tmp[13],*dot;unsigned e;int i,j;
  external_font_count=0;strcpy(mask,program_dir);strcat(mask,"APPDATA\\*.FNT");
  e=_dos_findfirst(mask,_A_NORMAL,&ff);while(!e&&external_font_count<MAX_EXTERNAL_FONTS){
    strncpy(external_font_file[external_font_count],ff.name,12);external_font_file[external_font_count][12]=0;
    strcpy(external_font_name[external_font_count],external_font_file[external_font_count]);dot=strrchr(external_font_name[external_font_count],'.');if(dot)*dot=0;external_font_count++;e=_dos_findnext(&ff);
  }
  for(i=0;i<external_font_count-1;i++)for(j=i+1;j<external_font_count;j++)if(stricmp(external_font_name[i],external_font_name[j])>0){
    strcpy(tmp,external_font_name[i]);strcpy(external_font_name[i],external_font_name[j]);strcpy(external_font_name[j],tmp);
    strcpy(tmp,external_font_file[i]);strcpy(external_font_file[i],external_font_file[j]);strcpy(external_font_file[j],tmp);}
}
static int total_fonts(void){return BUILTIN_FONT_COUNT+external_font_count;}
static const char *font_name_at(int id){return id<BUILTIN_FONT_COUNT?font_names[id]:external_font_name[id-BUILTIN_FONT_COUNT];}


static void cycle_control(int x,int y,const char *value,int focused)
{
  textout(x,y,"               ",focused?C_SELECTED:C_INPUT_FIELD,15);textout(x+1,y,value,focused?C_SELECTED:C_INPUT_FIELD,12);cell(x+13,y,31,focused?C_SELECTED:C_INPUT_FIELD);
}

static void draw_config_tabs(int x,int y,int active,int focus,int hover)
{
  static const int edge[7]={3,10,19,32,41,48,59};
  static const char *name[6]={"Menu","Colors","ScrnSavers","Prompt","Font","Shortcut"};
  int i,j,focused,base,text_attr;

  /* Six compact tabs, one character of breathing room around labels. */
  cell(x+edge[0],y,218,C_BORDER);
  for(i=0;i<6;i++){
    for(j=edge[i]+1;j<edge[i+1];j++)cell(x+j,y,196,C_BORDER);
    cell(x+edge[i+1],y,i==5?191:194,C_BORDER);
  }
  for(j=1;j<65;j++)cell(x+j,y+1,196,C_BORDER);
  for(i=0;i<7;i++)cell(x+edge[i],y+1,179,C_BORDER);
  for(i=0;i<6;i++){
    focused=(focus==i||hover==i);
    base=focused?C_SELECTED:ATTR(appearance.background,appearance.labels);
    text_attr=focused?C_SELECTED:
      (active==i?ATTR(appearance.background,appearance.titles):
                 ATTR(appearance.background,appearance.labels));
    for(j=edge[i]+1;j<edge[i+1];j++)cell(x+j,y+1,' ',base);
    textout(x+edge[i]+2,y+1,name[i],text_attr,(int)strlen(name[i]));
  }
}

/* Keep the font-preview renderer out of the already tight LAUNCH_TEXT
   segment.  Medium model permits separate code segments. */
#pragma code_seg("CONFIG_TEXT")
static void draw_character_preview(int x,int y)
{
  int code,row=0,column=0,i,j;

  /* Font sample only: keep Launch! UI, box-drawing and other custom
     glyph positions out of the preview.  Show the classic low-ASCII
     symbols (smileys, suits, bullets, genders and musical notes), then
     the printable alphanumeric/punctuation range. */
  for(j=0;j<5;j++)for(i=0;i<56;i++)cell(x+i,y+j,' ',C_BORDER);

  for(code=1;code<=15;code++){
    cell(x+column,y+row,code,C_BORDER);
    if(++column==56){column=0;row++;}
  }
  for(code=32;code<=126;code++){
    cell(x+column,y+row,code,C_BORDER);
    if(++column==56){column=0;row++;}
  }
}
#pragma code_seg()

#pragma code_seg("CONFIG_TEXT")
static int config_count(int tab)
{
  if(tab==0)return 8; /* Menu */
  if(tab==1)return 14; /* Colors */
  if(tab==2)return appearance.screensaver==1?4:3; /* Savers */
  if(tab==3)return 2; /* Prompt style + Set */
  if(tab==4)return font_is_vga()?2:0; /* Font */
  if(tab==5)return 2; /* Shortcut */
  return 0;
}

static unsigned char *config_field(int tab,int item,int *limit)
{
  *limit=15;
  if(tab==1)switch(item){
    case 1:*limit=7;return &appearance.background;
    case 2:return &appearance.border;case 3:return &appearance.titlebar_fg;
    case 4:*limit=7;return &appearance.titlebar_bg;
    case 5:return &appearance.main_title;case 6:return &appearance.titles;
    case 7:return &appearance.folders;case 8:return &appearance.launchers;
    case 9:return &appearance.selected_fg;case 10:*limit=7;return &appearance.selected_bg;
    case 11:return &appearance.controls_fg;case 12:*limit=7;return &appearance.controls_bg;
    case 13:return &appearance.labels;
  }
  if(tab==0){
    if(item==0){*limit=1;return &appearance.menu_top;}
    if(item==1){*limit=1;return &appearance.show_collections;}
    if(item==2){*limit=1;return &appearance.show_explore;}
    if(item==3){*limit=1;return &appearance.show_power;}
    if(item==4){*limit=1;return &appearance.show_time;}
    if(item==5){*limit=1;return &appearance.show_sysbar;}
    if(item==6){*limit=1;return &appearance.hour_12;}
    if(item==7){*limit=2;return &appearance.mouse_cursor;}
  }
  if(tab==2){
    if(item==0){*limit=14;return &appearance.screensaver;}
    if(appearance.screensaver==1){
      if(item==1)return &appearance.saver_color;
      if(item==2){*limit=3;return &appearance.saver_delay;}
    } else {
      if(item==1){*limit=3;return &appearance.saver_delay;}
    }
  }
  if(tab==4){
    if(item==0){*limit=63;return &appearance.font_id;}
    if(item==1){*limit=1;return &appearance.font_persist;}
  }
  return 0;
}

static void change_config_value(int tab,int item,int direction)
{
  unsigned char *field;int limit,value;
  if(tab==1 && item==0){
    value=colour_scheme_index();
    if(value<0)value=direction>0?0:COLOUR_SCHEME_COUNT-1;
    else {value+=direction;if(value<0)value=COLOUR_SCHEME_COUNT-1;if(value>=COLOUR_SCHEME_COUNT)value=0;}
    apply_colour_scheme(value);return;
  }
  if(tab==2 && item==0){
    value=(int)appearance.screensaver;
    value+=direction;if(value<0)value=14;if(value>14)value=0;
    appearance.screensaver=(unsigned char)value;return;
  }
  field=config_field(tab,item,&limit);if(!field)return;
  if((tab==0 && item>=1 && item<=3) || (tab==4 && item==1)){
    *field=!*field;return;
  }
  value=(int)*field+direction;if(value<0)value=limit;if(value>limit)value=0;
  if(tab==4 && item==0 && value==16){value+=direction;if(value<0)value=limit;if(value>limit)value=0;}
  *field=(unsigned char)value;
  if(tab==4 && item==0){mouse_pointer_restore();font_preview(appearance.font_id);mouse_pointer_install();}
  if(tab==0 && item==7)mouse_pointer_install();
}


static int prompt_style=0;
static int prompt_ansi=0;
#define MAX_PROMPT_STYLES 24
#define PROMPT_NAME_LEN 24
static char prompt_names[MAX_PROMPT_STYLES][PROMPT_NAME_LEN];
static unsigned char prompt_needs_ansi[MAX_PROMPT_STYLES];
static int prompt_count=0;

static int config_has_ansi_driver(void)
{
  FILE *fp;char path[16],line[256],upper[256],*comspec;int i;
  comspec=getenv("COMSPEC");
  if(comspec && isalpha((unsigned char)comspec[0]) && comspec[1]==':')
    sprintf(path,"%c:\\CONFIG.SYS",toupper((unsigned char)comspec[0]));
  else strcpy(path,"C:\\CONFIG.SYS");
  fp=fopen(path,"rt");if(!fp)return 0;
  while(fgets(line,sizeof(line),fp)){
    for(i=0;line[i]&&i<(int)sizeof(upper)-1;i++)upper[i]=(char)toupper((unsigned char)line[i]);
    upper[i]=0;
    /* ANSI.SYS and the very common faster-compatible NANSI.SYS both
       provide ANSI terminal processing.  Accept DEVICE/DEVICEHIGH paths. */
    if(strstr(upper,"NANSI.SYS")||strstr(upper,"ANSI.SYS")){fclose(fp);return 1;}
  }
  fclose(fp);return 0;
}

static int ansi_installed(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x2F,&r,&r);
  if(r.h.al==0xFF)return 1;
  return config_has_ansi_driver();
}

static void dos_version_text(char *out)
{
  union REGS r;int major,minor,oem;memset(&r,0,sizeof(r));r.h.ah=0x30;int86(0x21,&r,&r);
  major=r.h.al;minor=r.h.ah;oem=r.h.bh;
  if(oem==0)sprintf(out,"PC DOS %d.%02d",major,minor);
  else sprintf(out,"MS-DOS %d.%02d",major,minor);
}

static void prompt_cfg_path(char *out)
{
  strcpy(out,program_dir);strcat(out,"PROMPTS.CFG");
}

static void prompt_expand_value(const char *src,char *out)
{
  char dosver[24];char *q=out;int left=255,n;
  dos_version_text(dosver);
  while(*src&&left>0){
    if(!strncmp(src,"<DOS VER>",9)){
      n=(int)strlen(dosver);if(n>left)n=left;memcpy(q,dosver,n);q+=n;left-=n;src+=9;
    } else {*q++=*src++;left--;}
  }
  *q=0;
}

static void prompt_builtin_value(int style,char *out)
{
  char dosver[24];dos_version_text(dosver);
  switch(style){
    case 0:strcpy(out,"$P$G");break;
    case 1:strcpy(out,"$N$G");break;
    case 2:strcpy(out,"[$P]$G");break;
    case 3:strcpy(out,"$T$H$H$H$H $P$G");break;
    case 4:strcpy(out,"$D $P$G");break;
    case 5:sprintf(out,"$E[s$E[1;1H$E[0;1;33;41m$E[K %s $E[1;57H$D $T$H$H$H$H$E[36;40m$E[u$_$_$P$G$E[0;1;37m",dosver);break;
    case 6:strcpy(out,"$E[1;32m$P$G$E[0;37m");break;
    case 7:strcpy(out,"$E[1;36m[$P]$G$E[0;37m");break;
    case 8:strcpy(out,"$E[7m $P$G $E[0m ");break;
    default:strcpy(out,"$E[1;37m$D $T$H$H$H$H$E[0;36m$_$P$G$E[0;37m");break;
  }
}

static void prompt_load_defaults(void)
{
  static const char *names[10]={"Standard","Drive","Path Box","Time","Date","Red Line","Green","Cyan","Reverse","Bright"};
  int i;prompt_count=10;
  for(i=0;i<10;i++){strncpy(prompt_names[i],names[i],PROMPT_NAME_LEN-1);prompt_names[i][PROMPT_NAME_LEN-1]=0;prompt_needs_ansi[i]=(unsigned char)(i>=5);}
}

static void prompt_load_styles(void)
{
  char path[MAX_CMD],line[384];char *eq,*name,*value,*e;FILE *f;int n=0;
  prompt_cfg_path(path);f=fopen(path,"rt");
  if(!f){prompt_load_defaults();return;}
  while(n<MAX_PROMPT_STYLES&&fgets(line,sizeof(line),f)){
    name=line;while(*name==' '||*name=='\t')name++;
    if(!*name||*name==';'||*name=='#'||*name=='\r'||*name=='\n')continue;
    eq=strchr(name,'=');if(!eq)continue;*eq=0;value=eq+1;
    e=name+strlen(name);while(e>name&&(e[-1]==' '||e[-1]=='\t'))*--e=0;
    e=value+strlen(value);while(e>value&&(e[-1]=='\r'||e[-1]=='\n'))*--e=0;
    if(!*name||!*value)continue;
    strncpy(prompt_names[n],name,PROMPT_NAME_LEN-1);prompt_names[n][PROMPT_NAME_LEN-1]=0;
    prompt_needs_ansi[n]=(unsigned char)(strstr(value,"$E[")!=0 || strstr(value,"$e[")!=0);
    n++;
  }
  fclose(f);if(n)prompt_count=n;else prompt_load_defaults();
  if(prompt_style>=prompt_count)prompt_style=0;
}

static int prompt_next_style(int current,int direction)
{
  int i=current,tries=0;if(prompt_count<1)return 0;
  do{i+=direction;if(i<0)i=prompt_count-1;if(i>=prompt_count)i=0;tries++;}
  while(tries<prompt_count&&!prompt_ansi&&prompt_needs_ansi[i]);
  return i;
}

static void prompt_value(int style,char *out)
{
  char path[MAX_CMD],line[384];char *eq,*name,*value,*e;FILE *f;int n=0;
  if(style<0)style=0;
  prompt_cfg_path(path);f=fopen(path,"rt");
  if(f){
    while(fgets(line,sizeof(line),f)){
      name=line;while(*name==' '||*name=='\t')name++;
      if(!*name||*name==';'||*name=='#'||*name=='\r'||*name=='\n')continue;
      eq=strchr(name,'=');if(!eq)continue;*eq=0;value=eq+1;
      e=name+strlen(name);while(e>name&&(e[-1]==' '||e[-1]=='\t'))*--e=0;
      e=value+strlen(value);while(e>value&&(e[-1]=='\r'||e[-1]=='\n'))*--e=0;
      if(!*name||!*value)continue;
      if(n==style){prompt_expand_value(value,out);fclose(f);return;}n++;
    }
    fclose(f);
  }
  prompt_builtin_value(style,out);
}

static int set_autoexec_prompt(int style)
{
  /* Keep the sizeable file/edit buffers out of the small MSC runtime stack. */
  static char autoexec[20],temp[20],backup[20],line[512],check[512],value[256];
  char *p,*comspec;
  FILE *in,*out;int found=0,ok=1;
  comspec=getenv("COMSPEC");autoexec[0]=(comspec&&comspec[1]==':')?(char)toupper(comspec[0]):'C';
  strcpy(autoexec+1,":\\AUTOEXEC.BAT");strcpy(temp,autoexec);strcpy(strrchr(temp,'.'),".$P$");
  strcpy(backup,autoexec);strcpy(strrchr(backup,'.'),".L!P");
  prompt_value(style,value);
  in=fopen(autoexec,"rt");out=fopen(temp,"wt");if(!out){if(in)fclose(in);return 0;}
  /* Determine whether AUTOEXEC already has SET PROMPT=.  If it does, the
     replacement is written at exactly that line position.  If it does not,
     write the new prompt as line 1 before copying the existing file. */
  if(in){
    while(fgets(line,sizeof(line),in)){
      strcpy(check,line);p=check;while(*p==' '||*p=='\t')p++;
      if(!strnicmp(p,"SET",3)){p+=3;while(*p==' '||*p=='\t')p++;
        if(!strnicmp(p,"PROMPT",6)){p+=6;while(*p==' '||*p=='\t')p++;
          if(*p=='='){found=1;break;}
        }
      }
    }
    rewind(in);
  }
  if(!found){if(fprintf(out,"SET PROMPT=%s\n",value)<0)ok=0;}
  if(in&&ok){
    while(fgets(line,sizeof(line),in)){
      strcpy(check,line);p=check;while(*p==' '||*p=='\t')p++;
      if(!strnicmp(p,"SET",3)){p+=3;while(*p==' '||*p=='\t')p++;if(!strnicmp(p,"PROMPT",6)){p+=6;while(*p==' '||*p=='\t')p++;if(*p=='='){if(fprintf(out,"SET PROMPT=%s\n",value)<0)ok=0;continue;}}}
      if(fputs(line,out)==EOF){ok=0;break;}
    }
    if(ferror(in))ok=0;fclose(in);
  }
  if(fclose(out)!=0)ok=0;if(!ok){remove(temp);return 0;}
  remove(backup);if(in&&rename(autoexec,backup)!=0){remove(temp);return 0;}
  if(rename(temp,autoexec)!=0){if(in)rename(backup,autoexec);remove(temp);return 0;}
  return 1;
}

static int prepare_prompt_macro(int style)
{
  char value[256];
  prompt_value(style,value);
  if((int)strlen(value)+8>=MAX_CMD)return 0;
  strcpy(run_command,"PROMPT ");
  strcat(run_command,value);
  run_node=-1;
  prompt_macro_pending=1;
  return 1;
}

static void draw_prompt_preview(int x,int y,int w,int style)
{
  int i,j,cx=x+1,cy=y+2;char dosver[24],line[64];
  for(j=0;j<7;j++)for(i=0;i<w;i++)cell(x+i,y+j,' ',ATTR(0,7));
  dos_version_text(dosver);
  if(style==5){
    for(i=0;i<w;i++)cell(x+i,y,' ',ATTR(4,7));
    textout(x+1,y,dosver,ATTR(4,14),20);
    textout(x+w-18,y,"Tue 16/09 18:36",ATTR(4,14),17);
    textout(x+1,y+2,"C:\\DOS>",ATTR(0,11),7);cx=x+8;
  } else if(style==6){textout(x+1,y+2,"C:\\DOS>",ATTR(0,10),7);cx=x+8;}
  else if(style==7){textout(x+1,y+2,"[C:\\DOS]>",ATTR(0,11),9);cx=x+10;}
  else if(style==8){textout(x+1,y+2," C:\\DOS> ",ATTR(7,0),9);cx=x+10;}
  else if(style==9){textout(x+1,y+1,"Tue 16/09 18:36",ATTR(0,15),17);textout(x+1,y+3,"C:\\DOS>",ATTR(0,11),7);cx=x+8;cy=y+3;}
  else {
    if(style==0)strcpy(line,"C:\\DOS>");
    else if(style==1)strcpy(line,"C>");
    else if(style==2)strcpy(line,"[C:\\DOS]>");
    else if(style==3)strcpy(line,"18:36:42 C:\\DOS>");
    else strcpy(line,"Tue 16/09 C:\\DOS>");
    textout(x+1,y+2,line,ATTR(0,7),(int)strlen(line));cx=x+1+(int)strlen(line);
  }
  if(cx<x+w){
    unsigned char cursor_fg=7,cursor_bg=0;
    if(style==5)cursor_fg=11;
    else if(style==6)cursor_fg=10;
    else if(style==7)cursor_fg=11;
    else if(style==8){cursor_fg=0;cursor_bg=7;}
    else if(style==9)cursor_fg=11;
    cell(cx,cy,'_',ATTR(cursor_bg,cursor_fg));
  }
}

static void draw_config_page(int x,int y,int tab,int focus,int hover,int full)
{
  int f=focus-CONFIG_CONTROL_BASE,h=hover-CONFIG_CONTROL_BASE,i;
  if(full){dialog_box(x,y,66,20,"Launch! Configuration");textout(x+3,y,"Launch!",ATTR(appearance.titlebar_bg,appearance.main_title),7);}
  draw_config_tabs(x,y+1,tab,focus,hover);
  if(tab==0){
    textout(x+5,y+4,"Menu position:",C_INPUT_LABEL,18);
    cycle_control(x+25,y+4,appearance.menu_top?"Top":"Bottom",f==0||h==0);
    textout(x+5,y+6,"System menu items:",C_INPUT_LABEL,18);
    check_line(x+25,y+6,"Show 'File Open'",appearance.show_collections,f==1||h==1);
    check_line(x+25,y+7,"Show 'Explore & Run'",appearance.show_explore,f==2||h==2);
    check_line(x+25,y+8,"Show 'Shutdown...'",appearance.show_power,f==3||h==3);
    check_line(x+25,y+9,"Show time",appearance.show_time,f==4||h==4);
    textout(x+5,y+10,"Time format:",C_INPUT_LABEL,18);
    cycle_control(x+25,y+10,appearance.hour_12?"12-hour":"24-hour",f==6||h==6);
    textout(x+5,y+12,"Mouse cursor:",C_INPUT_LABEL,18);
    cycle_control(x+25,y+12,mouse_cursor_names[appearance.mouse_cursor],f==7||h==7);
    textout(x+5,y+14,"SysBar:",C_INPUT_LABEL,18);check_line(x+25,y+14,"Show on menu open",appearance.show_sysbar,f==5||h==5);
  } else if(tab==1){
    static const char *labels[10]={"Panels","Border","Titlebar","Accent","Titles","Folders","Launchers","Selected items","Controls","Labels"};
    int scheme=colour_scheme_index();
    textout(x+5,y+4,"Color scheme",C_INPUT_LABEL,18);
    cycle_control(x+25,y+4,scheme<0?"Custom":colour_scheme_at(scheme)->name,f==0||h==0);
    textout(x+25,y+5,"Foreground",C_BORDER,15);textout(x+45,y+5,"Background",C_BORDER,15);
    for(i=0;i<10;i++)textout(x+5,y+6+i,labels[i],C_INPUT_LABEL,18);
    cycle_control(x+45,y+6,colour_names[appearance.background],f==1||h==1);
    cycle_control(x+25,y+7,colour_names[appearance.border],f==2||h==2);
    cycle_control(x+25,y+8,colour_names[appearance.titlebar_fg],f==3||h==3);
    cycle_control(x+45,y+8,colour_names[appearance.titlebar_bg],f==4||h==4);
    cycle_control(x+25,y+9,colour_names[appearance.main_title],f==5||h==5);
    cycle_control(x+25,y+10,colour_names[appearance.titles],f==6||h==6);
    cycle_control(x+25,y+11,colour_names[appearance.folders],f==7||h==7);
    cycle_control(x+25,y+12,colour_names[appearance.launchers],f==8||h==8);
    cycle_control(x+25,y+13,colour_names[appearance.selected_fg],f==9||h==9);
    cycle_control(x+45,y+13,colour_names[appearance.selected_bg],f==10||h==10);
    cycle_control(x+25,y+14,colour_names[appearance.controls_fg],f==11||h==11);
    cycle_control(x+45,y+14,colour_names[appearance.controls_bg],f==12||h==12);
    cycle_control(x+25,y+15,colour_names[appearance.labels],f==13||h==13);
  } else if(tab==2){
    textout(x+5,y+4,"Screensaver",C_INPUT_LABEL,18);cycle_control(x+25,y+4,screensaver_names[appearance.screensaver],f==0||h==0);
    if(appearance.screensaver==1){
      textout(x+5,y+6,"Clock colour",C_INPUT_LABEL,18);cycle_control(x+25,y+6,colour_names[appearance.saver_color],f==1||h==1);
      textout(x+5,y+8,"Inactivity period",C_INPUT_LABEL,18);cycle_control(x+25,y+8,saver_delay_names[appearance.saver_delay],f==2||h==2);
      draw_button(x+25,y+10,"  Preview  ",11,f==3||h==3);textout(x+5,y+12,"Monitoring",C_INPUT_LABEL,18);textout(x+25,y+12,config_shortcut_active?"Command Prompt and Menu":"Menu only",C_ITEM,23);
    } else {
      textout(x+5,y+6,"Inactivity period",C_INPUT_LABEL,18);cycle_control(x+25,y+6,saver_delay_names[appearance.saver_delay],f==1||h==1);
      draw_button(x+25,y+8,"  Preview  ",11,f==2||h==2);textout(x+5,y+10,"Monitoring",C_INPUT_LABEL,18);textout(x+25,y+10,config_shortcut_active?"Command Prompt and Menu":"Menu only",C_ITEM,23);
    }
  } else if(tab==3){
    textout(x+5,y+4,"Prompt style:",C_INPUT_LABEL,18);cycle_control(x+25,y+4,prompt_names[prompt_style],f==0||h==0);
    draw_button(x+43,y+4,"  Set  ",7,f==1||h==1);
    textout(x+5,y+6,prompt_ansi?"ANSI driver detected.":"ANSI driver not detected.",C_ITEM,26);
    draw_prompt_preview(x+5,y+8,56,prompt_style);
  } else if(tab==4&&font_is_vga()){
    textout(x+5,y+4,"VGA display font",C_INPUT_LABEL,18);cycle_control(x+25,y+4,font_name_at((int)appearance.font_id<total_fonts()?(int)appearance.font_id:0),f==0||h==0);
    check_line(x+25,y+6,"Persist",appearance.font_persist,f==1||h==1);draw_character_preview(x+5,y+9);
  } else if(tab==4)textout(x+7,y+9,"Font customization requires a VGA display adapter",C_INPUT_LABEL,52);
  else {
    textout(x+5,y+5,"Keyboard shortcut status:",C_INPUT_LABEL,25);textout(x+31,y+5,config_shortcut_active?"Active":"Inactive",C_ITEM,12);
    if(config_shortcut_active)draw_button(x+45,y+5,"  Unload  ",10,f==0||h==0);else draw_button(x+45,y+5,"  Activate  ",12,f==0||h==0);
    textout(x+5,y+8,"Current combination:",C_INPUT_LABEL,25);textout(x+31,y+8,config_shortcut_combination,C_ITEM,28);
    textout(x+5,y+11,"Set new combination:",C_INPUT_LABEL,25);draw_button(x+31,y+11,"  Choose  ",10,f==1||h==1);
    if(config_shortcut_changed)textout(x+5,y+14,"Shortcut combination changed. Restart to take effect.",C_INPUT_LABEL,54);
  }
  toolbar_divider(x,y+16,66);
  draw_button(x+3,y+17,"  OK  ",8,focus==20||hover==20);draw_button(x+11,y+17,"  Cancel  ",10,focus==21||hover==21);
  draw_button(x+47,y+17,"  Reset  ",9,focus==22||hover==22);draw_button(x+58,y+17,"  ?  ",5,focus==23||hover==23);
}

static int config_hit(int x,int y,int tab,int mx,int my)
{
  static const int left[6]={3,10,19,32,41,48},right[6]={10,19,32,41,48,59};int i;
  if(my==y+1||my==y+2)for(i=0;i<6;i++)if(mx>x+left[i]&&mx<x+right[i])return i;
  if(tab==0){
    if(my==y+4 &&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE;
    if(my==y+6 &&mx>=x+25&&mx<x+63)return CONFIG_CONTROL_BASE+1;
    if(my==y+7 &&mx>=x+25&&mx<x+63)return CONFIG_CONTROL_BASE+2;
    if(my==y+8 &&mx>=x+25&&mx<x+63)return CONFIG_CONTROL_BASE+3;
    if(my==y+9 &&mx>=x+25&&mx<x+63)return CONFIG_CONTROL_BASE+4;
    if(my==y+14&&mx>=x+25&&mx<x+63)return CONFIG_CONTROL_BASE+5;
    if(my==y+10&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE+6;
    if(my==y+12&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE+7;
  } else if(tab==1){
    static const int rows[11]={4,7,8,9,10,11,12,13,14,15,8};
    static const int items[11]={0,2,3,5,6,7,8,9,11,13,4};
    if(mx>=x+25&&mx<x+40)for(i=0;i<11;i++)if(my==y+rows[i])return CONFIG_CONTROL_BASE+items[i];
    if(mx>=x+45&&mx<x+60&&my==y+6)return CONFIG_CONTROL_BASE+1;
    if(mx>=x+45&&mx<x+60&&my==y+8)return CONFIG_CONTROL_BASE+4;
    if(mx>=x+45&&mx<x+60&&my==y+13)return CONFIG_CONTROL_BASE+10;
    if(mx>=x+45&&mx<x+60&&my==y+14)return CONFIG_CONTROL_BASE+12;
  } else if(tab==2){
    if(my==y+4&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE;
    if(my==y+6&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE+1;
    if(appearance.screensaver==1&&my==y+8&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE+2;
    if(appearance.screensaver==1&&my==y+10&&mx>=x+25&&mx<x+36)return CONFIG_CONTROL_BASE+3;
    if(appearance.screensaver!=1&&my==y+8&&mx>=x+25&&mx<x+36)return CONFIG_CONTROL_BASE+2;
  } else if(tab==3){
    if(my==y+4&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE;
    if(my==y+4&&mx>=x+43&&mx<x+50)return CONFIG_CONTROL_BASE+1;
  } else if(tab==4&&font_is_vga()){
    if(my==y+4&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE;
    if(my==y+6&&mx>=x+25&&mx<x+36)return CONFIG_CONTROL_BASE+1;
  } else if(tab==5){
    if(my==y+5&&mx>=x+45&&mx<(config_shortcut_active?x+55:x+57))return CONFIG_CONTROL_BASE;
    if(my==y+11&&mx>=x+31&&mx<x+41)return CONFIG_CONTROL_BASE+1;
  }
  if(my==y+17&&mx>=x+3&&mx<x+9)return 20;if(my==y+17&&mx>=x+11&&mx<x+17)return 21;
  if(my==y+17&&mx>=x+47&&mx<x+56)return 22;if(my==y+17&&mx>=x+58&&mx<x+63)return 23;
  return -1;
}


#pragma code_seg()

#pragma code_seg("CONFIG_TEXT")
static void config_about_box(void)
{
  int w=42,h=11,x=(screen_cols-42)/2,y=(screen_rows-11)/2,k=0,mx=0,my=0,bx,focus=-1;unsigned mb=0;
  bx=x+3;subdialog_box(x,y,w,h,"About Launch!");
  textout(x+3,y+2,"(C)Copyright 2026 Ben Renegar",C_INPUT_LABEL,34);
  textout(x+3,y+3,"www.benrenegar.com",C_INPUT_LABEL,34);
  textout(x+3,y+6,"Version 3.73 - 2026-09-26",C_INPUT_LABEL,34);
  for(;;){
    draw_button(bx,y+h-3,"  OK  ",6,focus==0);
    wait_input(&k,&mx,&my,&mb);
    if((mb&1)&&my==y+h-3&&mx>=bx&&mx<bx+6){focus=0;press_button(bx,y+h-3,"  OK  ",6);return;}
    if(k==9||k==0x0F00){focus=0;continue;}
    if(k==27)return;if(k==13&&focus==0)return;k=0;
  }
}



static int select_popup(int x,int y,const char **items,int count,int current,int width)
{
  int h=count<10?count:10,top=current>=h?current-h+1:0,sel=current,k=0;
  int mx=0,my=0,i,py,row,col,result=current,done=0;
  unsigned mb=0;
  unsigned short far *under=0;
  unsigned under_cells;

  if(width<8)width=8;
  if(width>28)width=28;
  py=y+1;
  if(py+h+2>=screen_rows)py=y-h-2;
  if(py<0)py=0;
  if(x<0)x=0;
  if(x+width>screen_cols)width=screen_cols-x;

  /*
   * A Select popup is allowed to extend beyond its parent dialog.  Saving the
   * cells underneath it is therefore part of the control's contract: the
   * parent only redraws its own rectangle and cannot erase popup pixels that
   * were painted over the surrounding menu/screen.  Restore this backing on
   * every exit path so opening and dismissing a drop-down is visually neutral.
   */
  under_cells=(unsigned)(width*(h+2));
  under=(unsigned short far *)_fmalloc(under_cells*sizeof(unsigned short));
  if(under){
    for(row=0;row<h+2;row++)
      for(col=0;col<width;col++)
        under[(unsigned)(row*width+col)]=video[(py+row)*screen_cols+x+col];
  }

  while(!done){
    cell(x,py,218,C_BORDER);
    for(i=1;i<width-1;i++)cell(x+i,py,196,C_BORDER);
    cell(x+width-1,py,191,C_BORDER);
    for(i=0;i<h;i++){
      int idx=top+i;
      cell(x,py+1+i,179,C_BORDER);
      textout(x+1,py+1+i,idx<count?items[idx]:"",idx==sel?C_SELECTED:C_ITEM,width-2);
      cell(x+width-1,py+1+i,179,C_BORDER);
    }
    cell(x,py+h+1,192,C_BORDER);
    for(i=1;i<width-1;i++)cell(x+i,py+h+1,196,C_BORDER);
    cell(x+width-1,py+h+1,217,C_BORDER);

    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      if(mx>x&&mx<x+width-1&&my>py&&my<py+h+1){
        int n=top+my-py-1;
        if(n<count)sel=n;
      }
      continue;
    }
    if(mb&1){
      if(mx>x&&mx<x+width-1&&my>py&&my<py+h+1){
        int n=top+my-py-1;
        if(n<count)result=n;
      }
      done=1;
      continue;
    }
    if(k==27){done=1;continue;}
    if((k==0x4800||k==256+72)&&sel>0)sel--;
    else if((k==0x5000||k==256+80)&&sel+1<count)sel++;
    else if(k==0x4900||k==256+73){sel-=h;if(sel<0)sel=0;}
    else if(k==0x5100||k==256+81){sel+=h;if(sel>=count)sel=count-1;}
    else if(k==0x4700||k==256+71)sel=0;
    else if(k==0x4F00||k==256+79)sel=count-1;
    else if(k==13||k==' '){result=sel;done=1;continue;}
    else continue;
    if(sel<top)top=sel;
    if(sel>=top+h)top=sel-h+1;
  }

  if(under){
    for(row=0;row<h+2;row++)
      for(col=0;col<width;col++)
        video[(py+row)*screen_cols+x+col]=under[(unsigned)(row*width+col)];
    _ffree(under);
  }
  return result;
}

static int config_is_select(int tab,int item)
{
  if(tab==0)return item==0||item==6||item==7;
  if(tab==1)return 1;
  if(tab==2){if(item==0||item==1)return 1;if(appearance.screensaver==1&&item==2)return 1;return 0;}
  if(tab==3)return item==0;if(tab==4)return item==0;return 0;
}
static void config_select_control(int tab,int item,int x,int y)
{
  int sx=x+25,sy=y+4;
  static const char *posopt[2]={"Bottom","Top"},*timeopt[2]={"24-hour","12-hour"};
  const char *opts[64];unsigned char *field;int n=0,current=0,chosen,i,limit;
  if(tab==0&&item==0){opts[0]=posopt[0];opts[1]=posopt[1];n=2;current=appearance.menu_top;}
  else if(tab==0&&item==6){opts[0]=timeopt[0];opts[1]=timeopt[1];n=2;current=appearance.hour_12;}
  else if(tab==0&&item==7){for(i=0;i<3;i++)opts[i]=mouse_cursor_names[i];n=3;current=appearance.mouse_cursor;}
  else if(tab==1&&item==0){for(i=0;i<total_colour_schemes();i++)opts[i]=colour_scheme_at(i)->name;n=total_colour_schemes();current=colour_scheme_index();if(current<0)current=0;}
  else if(tab==1){field=config_field(tab,item,&limit);for(i=0;i<=limit;i++)opts[i]=colour_names[i];n=limit+1;current=field?*field:0;}
  else if(tab==2&&item==0){for(i=0;i<15;i++)opts[i]=screensaver_names[i];n=15;current=appearance.screensaver;}
  else if(tab==2){field=config_field(tab,item,&limit);if(appearance.screensaver==1&&item==1){for(i=0;i<=limit;i++)opts[i]=colour_names[i];}else{for(i=0;i<=limit;i++)opts[i]=saver_delay_names[i];}n=limit+1;current=field?*field:0;}
  else if(tab==3&&item==0){for(i=0;i<prompt_count;i++)opts[i]=prompt_names[i];n=prompt_count;current=prompt_style;}
  else if(tab==4&&item==0){for(i=0;i<total_fonts();i++)opts[i]=font_name_at(i);n=total_fonts();current=appearance.font_id;if(current>=n)current=0;}
  if(tab==0){if(item==6)sy=y+10;else if(item==7)sy=y+12;}
  else if(tab==1){static const int ry[14]={4,6,7,8,8,9,10,11,12,13,13,14,14,15};static const int rx[14]={25,45,25,25,45,25,25,25,25,25,45,25,45,25};sy=y+ry[item];sx=x+rx[item];}
  else if(tab==2){sy=y+(item==0?4:(item==1?6:8));}
  if(!n)return;chosen=select_popup(sx,sy,opts,n,current,20);if(chosen==current)return;
  if(tab==1&&item==0){apply_colour_scheme(chosen);return;}if(tab==3&&item==0){prompt_style=chosen;return;}field=config_field(tab,item,&limit);if(field)*field=(unsigned char)chosen;
  if(tab==4&&item==0){mouse_pointer_restore();font_preview(appearance.font_id);mouse_pointer_install();}if(tab==0&&item==7)mouse_pointer_install();
}

static int config_reset_box(void)
{
  int w=62,h=8,x=(screen_cols-62)/2,y=(screen_rows-8)/2,by=y+5;
  int focus=-1,k=0,mx=0,my=0,hit=-1;unsigned mb=0;
  subdialog_box(x,y,w,h,"Reset");
  textout(x+3,y+2,"What do you want to reset?",C_INPUT_LABEL,40);
  toolbar_divider(x,y+4,w);
  for(;;){
    draw_button(x+3,by,"  Config  ",10,focus==0);
    draw_button(x+15,by,"  Menu  ",8,focus==1);
    draw_button(x+25,by,"  Everything  ",14,focus==2);
    draw_button(x+52,by,"  Cancel  ",6,focus==3);
    wait_input(&k,&mx,&my,&mb);hit=-1;
    if(my==by){
      if(mx>=x+3&&mx<x+13)hit=0;
      else if(mx>=x+15&&mx<x+23)hit=1;
      else if(mx>=x+25&&mx<x+39)hit=2;
      else if(mx>=x+52&&mx<x+58)hit=3;
    }
    if(mb&MOUSE_MOVED){if(hit>=0)focus=hit;continue;}
    if(mb&1){
      if(hit==0){press_button(x+3,by,"  Config  ",10);return 1;}
      if(hit==1){press_button(x+15,by,"  Menu  ",8);return 2;}
      if(hit==2){press_button(x+25,by,"  Everything  ",14);return 3;}
      if(hit==3){press_button(x+52,by,"  Cancel  ",6);return 0;}
      continue;
    }
    if(k==27)return 0;
    if(k==9||k==0x4D00)focus=focus<0?0:(focus+1)&3;
    else if(k==0x0F00||k==0x4B00)focus=focus<0?3:(focus+3)&3;
    else if(k==13&&focus>=0)return focus==3?0:focus+1;
  }
}

#pragma code_seg()

static int configure_appearance(void)
{
  APPEARANCE original=appearance,before_reset;int x,y,k=0,mx=0,my=0,tab=0,focus=-1,hover=-1;
  int hit,count,item,redraw=2,reset_choice;unsigned mb=0;
  video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 0;}
  cursor_hide();mouse_present=mouse_start();mouse_stop();
  config_shortcut_changed=0;shortcut_refresh();load_custom_schemes();scan_external_fonts();prompt_load_styles();prompt_ansi=ansi_installed();if(!prompt_ansi&&prompt_needs_ansi[prompt_style])prompt_style=prompt_next_style(prompt_style,1);
  x=(screen_cols-66)/2;y=(screen_rows-20)/2;
  for(;;){
    wait_vertical_retrace();if(redraw){render_begin();draw_config_page(x,y,tab,focus,hover,redraw==2);render_end();redraw=0;}
    wait_input(&k,&mx,&my,&mb);hit=config_hit(x,y,tab,mx,my);
    if(mb&MOUSE_MOVED){if(hover!=hit){hover=hit;redraw=1;}continue;}
    if(mb&1){
      if(hit>=0&&hit<CONFIG_TAB_COUNT){tab=hit;focus=hit;hover=-1;redraw=2;continue;}
      if(hit>=CONFIG_CONTROL_BASE&&hit<20){
        focus=hit;item=hit-CONFIG_CONTROL_BASE;
        if(config_is_select(tab,item)){config_select_control(tab,item,x,y);redraw=2;continue;}
        if(tab==5&&item==0){
          if(config_shortcut_active){
            press_button(x+45,y+5,"  Unload  ",10);shortcut_unload();
          } else {
            press_button(x+45,y+5,"  Activate  ",12);shortcut_activate();
          }
          shortcut_refresh();
        } else if(tab==5&&item==1){
          press_button(x+31,y+11,"  Choose  ",10);
          if(shortcut_set_dialog())config_shortcut_changed=1;
          shortcut_refresh();
        } else if(tab==2&&((appearance.screensaver==1&&item==3) ||
                           (appearance.screensaver!=1&&item==2))){
          press_button(x+25,appearance.screensaver==1?y+10:y+8,"  Preview  ",11);
          run_screensaver();
        } else if(tab==3&&item==0){prompt_style=prompt_next_style(prompt_style,1);
        } else if(tab==3&&item==1){
          press_button(x+43,y+4,"  Set  ",7);
          if(!set_autoexec_prompt(prompt_style)){notice_box("Write Error","AUTOEXEC.BAT could not be updated.");redraw=2;continue;}
          if(!prepare_prompt_macro(prompt_style)){notice_box("Prompt Error","The selected prompt is too long.");redraw=2;continue;}
          mouse_pointer_restore();close_menu();return 2;
        } else change_config_value(tab,item,1);
        redraw=2;continue;
      }
      if(hit==20){
        press_button(x+3,y+17,"  OK  ",8);
        mouse_pointer_restore();if(font_commit(appearance.font_id)&&save_appearance()){close_menu();return 1;}mouse_pointer_install();
        notice_box("Write Error","Could not save the configuration.");redraw=2;continue;
      }
      if(hit==21){press_button(x+11,y+17,"  Cancel  ",10);mouse_pointer_restore();appearance=original;font_restore();close_menu();return 0;}
      if(hit==22){
        int reset_ok=1;
        press_button(x+47,y+17,"  Reset  ",9);reset_choice=config_reset_box();
        if(reset_choice==1||reset_choice==3){
          before_reset=appearance;mouse_pointer_restore();appearance=default_appearance;
          if(font_commit(appearance.font_id)&&save_appearance()){original=appearance;mouse_pointer_install();}
          else {appearance=before_reset;font_commit(appearance.font_id);mouse_pointer_install();notice_box("Reset Error","Could not reset the configuration.");reset_ok=0;}
        }
        if((reset_choice==2||reset_choice==3)&&!default_menu_file(1)){notice_box("Reset Error","Could not rebuild LAUNCH.MNU.");reset_ok=0;}
        if(reset_ok&&reset_choice==1)notice_box("Reset","Configuration has been reset to default.");
        else if(reset_ok&&reset_choice==2)notice_box("Reset","Menu has been reset to default.");
        else if(reset_ok&&reset_choice==3)notice_box("Reset","Everything has been reset to default.");
        redraw=2;continue;
      }
      if(hit==23){press_button(x+58,y+17,"  ?  ",5);config_about_box();redraw=2;continue;}
      continue;
    }
    if(k==27){mouse_pointer_restore();appearance=original;font_restore();close_menu();return 0;}
    if(k==0xA500){tab=(tab+1)%CONFIG_TAB_COUNT;focus=tab;redraw=2;continue;}
    count=config_count(tab);
    if(k==9){
      if(focus<0)focus=0;
      else if(focus<CONFIG_TAB_COUNT-1)focus++;
      else if(focus==CONFIG_TAB_COUNT-1)
        focus=count?CONFIG_CONTROL_BASE:20;
      else if(focus>=CONFIG_CONTROL_BASE&&
              focus<CONFIG_CONTROL_BASE+count-1)focus++;
      else if(focus<20)focus=20;else if(focus==20)focus=21;else if(focus==21)focus=22;else if(focus==22)focus=23;else focus=0;
      redraw=1;continue;
    }
    if(k==0x0F00){
      if(focus<0)focus=23;
      else if(focus==0)focus=23;
      else if(focus<CONFIG_TAB_COUNT)focus--;
      else if(focus==20)
        focus=count?CONFIG_CONTROL_BASE+count-1:CONFIG_TAB_COUNT-1;
      else if(focus==23)focus=22;
      else if(focus==22)focus=21;
      else if(focus==21)focus=20;
      else if(focus==CONFIG_CONTROL_BASE)focus=CONFIG_TAB_COUNT-1;
      else focus--;
      redraw=1;continue;
    }
    if(focus>=CONFIG_CONTROL_BASE&&focus<20&&(k==0x4800||k==0x5000)){
      item=focus-CONFIG_CONTROL_BASE;
      if(k==0x4800&&item>0)focus--;
      else if(k==0x5000&&item+1<count)focus++;
      redraw=1;continue;
    }
    if(focus<CONFIG_TAB_COUNT){
      if(k==0x4B00){
        focus=(focus+CONFIG_TAB_COUNT-1)%CONFIG_TAB_COUNT;
        tab=focus;redraw=2;continue;
      } else if(k==0x4D00){
        focus=(focus+1)%CONFIG_TAB_COUNT;tab=focus;redraw=2;continue;
      } else if(k==13||k==' '){
        tab=focus;focus=config_count(tab)?CONFIG_CONTROL_BASE:20;
        redraw=2;continue;
      }
      redraw=1;
      continue;
    }
    if(focus<20){
      item=focus-CONFIG_CONTROL_BASE;
      if(config_is_select(tab,item)&&(k==13||k==' ')){config_select_control(tab,item,x,y);redraw=2;continue;}
      if(tab==5&&item==0){
        if(k==13||k==' '){
          if(config_shortcut_active){
            press_button(x+45,y+5,"  Unload  ",10);shortcut_unload();
          } else {
            press_button(x+45,y+5,"  Activate  ",12);shortcut_activate();
          }
          shortcut_refresh();redraw=2;
        }
      } else if(tab==5&&item==1){
        if(k==13||k==' '){
          press_button(x+31,y+11,"  Choose  ",10);
          if(shortcut_set_dialog())config_shortcut_changed=1;
          shortcut_refresh();redraw=2;
        }
      } else if(tab==2&&((appearance.screensaver==1&&item==3) ||
                         (appearance.screensaver!=1&&item==2))){
        if(k==13||k==' '){run_screensaver();redraw=2;}
      } else if(tab==3&&item==0&&(k==0x4B00||k==0x4D00||k==' ')){prompt_style=prompt_next_style(prompt_style,k==0x4B00?-1:1);redraw=2;
      } else if(tab==3&&item==1&&(k==13||k==' ')){
        if(!set_autoexec_prompt(prompt_style)){notice_box("Write Error","AUTOEXEC.BAT could not be updated.");redraw=2;continue;}
        if(!prepare_prompt_macro(prompt_style)){notice_box("Prompt Error","The selected prompt is too long.");redraw=2;continue;}
        mouse_pointer_restore();close_menu();return 2;
      } else if(!config_is_select(tab,item)&&k==0x4B00){change_config_value(tab,item,-1);redraw=2;}
      else if(!config_is_select(tab,item)&&(k==0x4D00||k==' ')){change_config_value(tab,item,1);redraw=2;}
      else if(k==13){focus=(item+1<count)?focus+1:20;redraw=1;}
      continue;
    }
    if(k==0x4B00||k==0x4D00){
      if(k==0x4B00)focus=focus<=20?23:focus-1;
      else focus=focus>=23?20:focus+1;
      redraw=1;
    } else if((k==13||k==' ')&&focus==20){
      mouse_pointer_restore();if(font_commit(appearance.font_id)&&save_appearance()){close_menu();return 1;}mouse_pointer_install();
      notice_box("Write Error","Could not save the configuration.");redraw=2;
    } else if((k==13||k==' ')&&focus==21){mouse_pointer_restore();appearance=original;font_restore();close_menu();return 0;
    } else if((k==13||k==' ')&&focus==22){
      int reset_ok=1;
      reset_choice=config_reset_box();
      if(reset_choice==1||reset_choice==3){
        before_reset=appearance;mouse_pointer_restore();appearance=default_appearance;
        if(font_commit(appearance.font_id)&&save_appearance()){original=appearance;mouse_pointer_install();}
        else {appearance=before_reset;font_commit(appearance.font_id);mouse_pointer_install();notice_box("Reset Error","Could not reset the configuration.");reset_ok=0;}
      }
      if((reset_choice==2||reset_choice==3)&&!default_menu_file(1)){notice_box("Reset Error","Could not rebuild LAUNCH.MNU.");reset_ok=0;}
      if(reset_ok&&reset_choice==1)notice_box("Reset","Configuration has been reset to default.");
      else if(reset_ok&&reset_choice==2)notice_box("Reset","Menu has been reset to default.");
      else if(reset_ok&&reset_choice==3)notice_box("Reset","Everything has been reset to default.");
      redraw=2;
    } else if((k==13||k==' ')&&focus==23){config_about_box();redraw=2;}
  }
}

static int place_overflow(int parent,int depth,int new_node)
{
  int list[MAX_CHILD],n,more,candidate,result;
  n=children(parent,list);if(n<=MENU_CAPACITY)return new_node;
  if(depth>=MAX_DEPTH-1){delete_tree(new_node);notice_box("Menu Full","No further menu level is available.");return -1;}
  more=find_folder("More",parent);
  if(more<0){
    candidate=list[MENU_CAPACITY-1];
    more=add_node("More","",parent,1);
    if(more<0){delete_tree(new_node);notice_box("Menu Full","The menu database is full.");return -1;}
    nodes[candidate].parent=more;nodes[candidate].order=0;
    nodes[new_node].parent=more;nodes[new_node].order=1;
    nodes[more].order=255;normalize_order(parent);normalize_order(more);
    return more;
  }
  nodes[new_node].parent=more;nodes[new_node].order=(unsigned char)next_order(more);
  nodes[more].order=255;normalize_order(parent);
  result=place_overflow(more,depth+1,new_node);
  return result<0?-1:more;
}

static int add_dialog(int parent,int depth)
{
  int folder,node;static char name[MAX_TITLE],exe[MAX_CMD],params[MAX_CMD],cmd[MAX_CMD];
  int list[MAX_CHILD],press_enter=1,change_dir=0,prompt_params=0,add_path=0;
  name[0]=exe[0]=params[0]=cmd[0]=0;
  if(children(parent,list)>=MENU_CAPACITY && depth>=MAX_DEPTH-1){notice_box("Menu Full","No further menu level is available.");return 0;}
  folder=choose_type(); if(folder<0)return 0;
  if(folder==2){
    node=add_node("-","",parent,0);
    if(node<0){notice_box("Menu Full","The menu database is full.");return 0;}
    nodes[node].separator=1;
    added_visible_node=place_overflow(parent,depth,node);
    return added_visible_node>=0;
  }
  if(!item_form(folder,name,exe,params,&press_enter,&change_dir,&prompt_params,&add_path,0))return 0;
  if(folder && !stricmp(name,"More")){notice_box("Reserved Name","More is reserved for automatic overflow.");return 0;}
  if(folder && find_folder(name,parent)>=0){notice_box("Duplicate Folder","That folder name is already in this menu.");return 0;}
  if(folder) node=add_node(name,"",parent,1);
  else {strcpy(cmd,exe);if(*params){strcat(cmd," ");strncat(cmd,params,MAX_CMD-strlen(cmd)-1);}node=add_node(name,cmd,parent,0);}
  if(node<0){notice_box("Menu Full","The menu database is full.");return 0;}
  nodes[node].press_enter=(unsigned char)press_enter;nodes[node].change_dir=(unsigned char)change_dir;
  nodes[node].prompt_params=(unsigned char)prompt_params;nodes[node].add_path=(unsigned char)add_path;
  added_visible_node=place_overflow(parent,depth,node);
  if(added_visible_node<0)return 0;
  return 1;
}

static void split_command(const char *command,char *exe,char *params)
{
  const char *p=command,*end;
  int n;
  while(*p==' ')p++;
  if(*p=='\"'){
    end=strchr(p+1,'\"'); if(end)end++; else end=p+strlen(p);
  } else {
    end=strchr(p,' '); if(!end)end=p+strlen(p);
  }
  n=(int)(end-p);if(n>=MAX_CMD)n=MAX_CMD-1;
  strncpy(exe,p,n);exe[n]=0;
  while(*end==' ')end++;
  strncpy(params,end,MAX_CMD-1);params[MAX_CMD-1]=0;
}

static int edit_dialog(int node)
{
  int duplicate,press_enter=nodes[node].press_enter,change_dir=nodes[node].change_dir;
  int prompt_params=nodes[node].prompt_params,add_path=nodes[node].add_path;
  static char name[MAX_TITLE],exe[MAX_CMD],params[MAX_CMD],cmd[MAX_CMD];
  exe[0]=params[0]=cmd[0]=0;
  if(nodes[node].separator)return 0;
  strcpy(name,nodes[node].title);
  if(nodes[node].folder && !stricmp(name,"More")){notice_box("Automatic Folder","More is managed automatically.");return 0;}
  if(!nodes[node].folder)split_command(nodes[node].command,exe,params);
  if(!item_form(nodes[node].folder,name,exe,params,&press_enter,&change_dir,&prompt_params,&add_path,1))return 0;
  duplicate=find_folder(name,nodes[node].parent);
  if(nodes[node].folder && !stricmp(name,"More") && stricmp(nodes[node].title,"More")){notice_box("Reserved Name","More is reserved for automatic overflow.");return 0;}
  if(nodes[node].folder && duplicate>=0 && duplicate!=node){notice_box("Duplicate Folder","That folder name is already in this menu.");return 0;}
  strcpy(nodes[node].title,name);
  if(!nodes[node].folder){
    strcpy(cmd,exe);
    if(*params && strlen(cmd)<MAX_CMD-1){strcat(cmd," ");strncat(cmd,params,MAX_CMD-strlen(cmd)-1);}
    strcpy(nodes[node].command,cmd);
    nodes[node].press_enter=(unsigned char)press_enter;
    nodes[node].change_dir=(unsigned char)change_dir;
    nodes[node].prompt_params=(unsigned char)prompt_params;nodes[node].add_path=(unsigned char)add_path;
  }
  return 1;
}











static void parameter_field(int x,int y,const char *value,int position,
                            int focused,int hovered,int width)
{
  int length=strlen(value),scroll=0,cursor;
  if(position>=width)scroll=position-width+1;
  textout(x,y,value+scroll,(focused||hovered)?C_SELECTED:C_INPUT_FIELD,width);
  if(focused){cursor=position-scroll;edit_caret_set(x+cursor,y);}
}

static void command_filename(const char *executable,char *name)
{
  const char *p=executable,*slash,*other;int length;
  if(*p=='\"')p++;
  slash=strrchr(p,'\\');other=strrchr(p,'/');
  if(!slash || (other && other>slash))slash=other;
  if(slash)p=slash+1;
  strncpy(name,p,18);name[18]=0;
  length=strlen(name);if(length && name[length-1]=='\"')name[length-1]=0;
}

static int compose_prompt_command(const char *executable,const char *parameters)
{
  int needed=strlen(executable)+(parameters[0]?1+(int)strlen(parameters):0);
  if(needed>=MAX_CMD)return 0;
  strcpy(run_command,executable);
  if(parameters[0]){strcat(run_command," ");strcat(run_command,parameters);}
  return 1;
}



static int parameter_prompt_command(const char *title,const char *command)
{
  int x=(screen_cols-54)/2,y=(screen_rows-9)/2,k=0,mx=0,my=0;
  int focus=0,hover=-1,position,length,shown;unsigned mb=0;
  static char executable[MAX_CMD],parameters[MAX_CMD];
  char filename[19],display_name[14];
  split_command(command,executable,parameters);position=strlen(parameters);
  command_filename(executable,filename);
  strncpy(display_name,filename,13);display_name[13]=0;strupr(display_name);
  subdialog_box(x,y,54,9,"Run with parameters");
  textout(x+3,y+2,"Run ",C_INPUT_LABEL,4);
  textout(x+7,y+2,display_name,C_ITEM,strlen(display_name));
  textout(x+7+strlen(display_name),y+2," with parameters:",C_INPUT_LABEL,17);
  for(;;){
    wait_vertical_retrace();edit_caret_hide();
    parameter_field(x+3,y+4,parameters,position,focus==0,hover==0,48);
    draw_button(x+3,y+6,"  Run  ",9,focus==1||hover==1);
    draw_button(x+14,y+6,"  Cancel  ",10,focus==2||hover==2);
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      hover=-1;
      if(my==y+4&&mx>=x+3&&mx<x+51)hover=0;
      else if(my==y+6&&mx>=x+3&&mx<x+12)hover=1;
      else if(my==y+6&&mx>=x+14&&mx<x+24)hover=2;
      continue;
    }
    if(mb&1){
      if(my==y+4&&mx>=x+3&&mx<x+51){
        shown=position>=48?position-47:0;focus=0;
        position=shown+mx-(x+3);length=strlen(parameters);
        if(position>length)position=length;continue;
      }
      if(my==y+6&&mx>=x+3&&mx<x+12){
        press_button(x+3,y+6,"  Run  ",9);
        if(compose_prompt_command(executable,parameters))return 1;
        notice_box("Parameters Too Long","Shorten the parameters before running.");continue;
      }
      if(my==y+6&&mx>=x+14&&mx<x+24){
        press_button(x+14,y+6,"  Cancel  ",10);return 0;
      }
      continue;
    }
    if(k==27)return 0;
    if(k==9||k==0x0F00){focus=(k==0x0F00)?(focus+2)%3:(focus+1)%3;continue;}
    if(focus==0){
      length=strlen(parameters);
      if(k==0x4700)position=0;
      else if(k==0x4F00)position=length;
      else if(k==0x4B00){if(position>0)position--;}
      else if(k==0x4D00){if(position<length)position++;}
      else if(k==8){if(position>0){memmove(parameters+position-1,parameters+position,length-position+1);position--;}}
      else if(k==0x5300){if(position<length)memmove(parameters+position,parameters+position+1,length-position);}
      else if(k==13)focus=1;
      else if(k>=32&&k<127&&length<MAX_CMD-1){
        memmove(parameters+position+1,parameters+position,length-position+1);
        parameters[position++]=(char)k;
      }
      continue;
    }
    if(k==0x4B00)focus=focus==1?2:focus-1;
    else if(k==0x4D00)focus=focus==2?1:focus+1;
    else if(k==13){
      if(focus==2)return 0;
      if(compose_prompt_command(executable,parameters))return 1;
      notice_box("Parameters Too Long","Shorten the parameters before running.");
    }
  }
}



static int parameter_prompt(int node)
{
  return parameter_prompt_command(nodes[node].title,nodes[node].command);
}

static int prepare_launcher(int node)
{
  if(nodes[node].prompt_params)return parameter_prompt(node);
  strncpy(run_command,nodes[node].command,MAX_CMD-1);run_command[MAX_CMD-1]=0;
  return 1;
}

static int explore_ram_label(unsigned drive)
{
  struct find_t found;char mask[8];char *name;int a,b;
  mask[0]=(char)('A'+drive);strcpy(mask+1,":\\*.*");
  if(_dos_findfirst(mask,0x08,&found))return 0;
  name=found.name;
  a=toupper((unsigned char)name[0]);b=toupper((unsigned char)name[1]);
  if((a=='R' && (b=='A'||b=='D')) || (a=='X'&&b=='M'))return 1;
  return a=='M' && b=='S' && name[2]=='-' &&
         toupper((unsigned char)name[3])=='R' &&
         toupper((unsigned char)name[4])=='A';
}

/* Same media classification used by DRIVES 1.2. */
static unsigned long explore_cdrom_mask=0UL;
static unsigned char explore_classify_drive(unsigned drive)
{
  union REGS r;struct SREGS s;unsigned char far *dpb;
  static unsigned char driver_name[2];
  unsigned offset,segment,word;
  if(explore_cdrom_mask&(1UL<<drive))return 9;
  if(drive<2){
    int86(0x11,&r,&r);
    if(!(r.x.ax&1) || drive>=(((r.x.ax>>6)&3)+1))return 0;
    return '-';
  }
  r.x.ax=0x1500;r.x.bx=0;int86(0x2F,&r,&r);
  if(r.x.bx){
    r.x.ax=0x150B;r.x.bx=0;r.x.cx=drive;int86(0x2F,&r,&r);
    if(r.x.bx==0xADAD && r.x.ax)return 9;
  }
  r.x.ax=0x4409;r.h.bl=(unsigned char)(drive+1);int86(0x21,&r,&r);
  if(r.x.cflag)return 0;
  if(r.x.dx&0x1000)return 18;
  r.h.ah=0x32;r.h.dl=(unsigned char)(drive+1);segread(&s);
  int86x(0x21,&r,&r,&s);
  if(r.h.al!=0xFF){
    dpb=(unsigned char far *)MAKE_FP(s.ds,r.x.bx);
    offset=*(unsigned short far *)(dpb+0x13);
    segment=*(unsigned short far *)(dpb+0x15);
    offset+=0x0A;
    segread(&s);
    movedata(segment,offset,s.ds,(unsigned)driver_name,2);
    word=(unsigned)toupper(driver_name[0])|
         ((unsigned)toupper(driver_name[1])<<8);
    if(word==('R'|('A'<<8)) || word==('V'|('D'<<8)) ||
       word==('X'|('M'<<8)) || word==('S'|('R'<<8)) ||
       word==('T'|('D'<<8)) || word==('$'|('R'<<8)))return '#';
  }
  if(explore_ram_label(drive))return '#';
  r.x.ax=0x4408;r.h.bl=(unsigned char)(drive+1);int86(0x21,&r,&r);
  if(!r.x.cflag && !r.x.ax)return '-';
  return 240;
}

static void explore_detect_drives(void)
{
  int i;union REGS r;explore_cdrom_mask=0UL;memset(&r,0,sizeof(r));r.x.ax=0x1500;r.x.bx=0;int86(0x2F,&r,&r);if(r.x.bx){unsigned first=r.x.cx,count=r.x.bx;for(i=0;i<(int)count&&first+(unsigned)i<26;i++)explore_cdrom_mask|=(1UL<<(first+i));}
  for(i=0;i<26;i++){
    explore_drive_symbols[i]=explore_classify_drive((unsigned)i);
    explore_drive_positions[i]=-1;
  }
}

static void explore_drive_bar(int x,int y,int focused_drive)
{
  int drive,pos=0;unsigned char symbol;
  int label_attr=ATTR(appearance.background,appearance.labels);
  int drive_attr=ATTR(appearance.background,appearance.launchers);
  int symbol_attr=ATTR(appearance.background,appearance.titles);
  textout(x,y,"",label_attr,70);
  for(drive=0;drive<26;drive++){
    explore_drive_positions[drive]=-1;symbol=explore_drive_symbols[drive];
    if(!symbol)continue;
    if(pos+6>70)break;
    explore_drive_positions[drive]=x+pos;
    cell(x+pos++,y,'[',drive==focused_drive?C_SELECTED:label_attr);
    cell(x+pos++,y,'A'+drive,drive==focused_drive?C_SELECTED:drive_attr);
    cell(x+pos++,y,':',drive==focused_drive?C_SELECTED:drive_attr);
    cell(x+pos++,y,' ',drive==focused_drive?C_SELECTED:label_attr);
    cell(x+pos++,y,symbol,drive==focused_drive?C_SELECTED:symbol_attr);
    cell(x+pos++,y,']',drive==focused_drive?C_SELECTED:label_attr);
    if(pos<70)cell(x+pos++,y,' ',label_attr);
  }
}

static int explore_first_drive(void)
{
  int drive;
  for(drive=0;drive<26;drive++)if(explore_drive_positions[drive]>=0)return drive;
  return -1;
}

static int explore_adjacent_drive(int drive,int direction)
{
  int next=drive+direction;
  while(next>=0 && next<26){
    if(explore_drive_positions[next]>=0)return next;
    next+=direction;
  }
  return drive;
}

static int explore_tab_focus(int focus)
{
  int drive;
  if(focus==0){drive=explore_first_drive();return drive>=0?drive+5:1;}
  if(focus>=5){
    drive=explore_adjacent_drive(focus-5,1);
    return drive==focus-5?1:drive+5;
  }
  if(focus<3)return focus+1;
  return 0;
}

static int explore_shift_tab_focus(int focus)
{
  int drive,last;
  if(focus==0)return 3;
  if(focus==1){
    last=explore_first_drive();
    if(last<0)return 0;
    drive=last;
    while(explore_adjacent_drive(drive,1)!=drive)drive=explore_adjacent_drive(drive,1);
    return drive+5;
  }
  if(focus>=5){
    drive=explore_adjacent_drive(focus-5,-1);
    return drive==focus-5?0:drive+5;
  }
  return focus-1;
}

static int explore_drive_at(int mouse_x)
{
  int drive;
  for(drive=0;drive<26;drive++)
    if(explore_drive_positions[drive]>=0 &&
       mouse_x>=explore_drive_positions[drive] &&
       mouse_x<explore_drive_positions[drive]+6)return drive;
  return -1;
}

static int explore_compare(const void *aa,const void *bb)
{
  const EXPLORE_ENTRY *a=(const EXPLORE_ENTRY *)aa;
  const EXPLORE_ENTRY *b=(const EXPLORE_ENTRY *)bb;
  if(a->directory!=b->directory)return a->directory?-1:1;
  if(!strcmp(a->name,".."))return strcmp(b->name,"..")? -1:0;
  if(!strcmp(b->name,".."))return 1;
  return stricmp(a->name,b->name);
}

static int executable_name(const char *name)
{
  const char *extension=strrchr(name,'.');
  return extension && (!stricmp(extension,".EXE") || !stricmp(extension,".COM") ||
                       !stricmp(extension,".BAT"));
}

static void explore_add(const char *name,int directory)
{
  if(explore_count>=MAX_EXPLORE_ENTRIES)return;
  strncpy(explore_entries[explore_count].name,name,12);
  explore_entries[explore_count].name[12]=0;
  explore_entries[explore_count].directory=(unsigned char)directory;
  explore_count++;
}

static int explore_is_root(const char *path)
{
  return path[0] && path[1]==':' && path[2]=='\\' && path[3]==0;
}

static void explore_parent(char *path)
{
  char *end,*slash;
  if(explore_is_root(path))return;
  end=path+strlen(path);while(end>path+3 && end[-1]=='\\')*--end=0;
  slash=strrchr(path,'\\');
  if(!slash || slash<=path+2)path[3]=0;
  else slash[1]=0;
}

static int explore_load(const char *path)
{
  struct find_t found;char mask[MAX_CMD];unsigned result;
  explore_count=0;
  if(!explore_is_root(path))explore_add("..",1);
  if(strlen(path)>MAX_CMD-4)return 0;
  strcpy(mask,path);strcat(mask,"*.*");
  result=_dos_findfirst(mask,_A_NORMAL|_A_RDONLY|_A_SUBDIR,&found);
  while(!result){
    if((found.attrib&_A_SUBDIR) && strcmp(found.name,".") && strcmp(found.name,".."))
      explore_add(found.name,1);
    else if(!(found.attrib&_A_SUBDIR) && executable_name(found.name))
      explore_add(found.name,0);
    result=_dos_findnext(&found);
  }
  qsort(explore_entries,explore_count,sizeof(explore_entries[0]),explore_compare);
  return 1;
}

#pragma code_seg("BROWSE_TEXT")

static void explore_path_field(int x,int y,const char *path)
{
  int length=strlen(path);
  if(length<=70)textout(x,y,path,C_CONSOLE_PATH,70);
  else {
    char shown[71];strcpy(shown,"...");strcpy(shown+3,path+length-67);
    textout(x,y,shown,C_CONSOLE_PATH,70);
  }
}

static void explore_selection_field(int x,int y,const char *path,int selected)
{
  static char preview[MAX_CMD+16];int needed;
  strcpy(preview,path);
  if(selected>=0 && selected<explore_count){
    if(explore_entries[selected].directory &&
       !strcmp(explore_entries[selected].name,".."))explore_parent(preview);
    else {
      needed=strlen(preview)+strlen(explore_entries[selected].name)+2;
      if(needed<(int)sizeof(preview)){
        strcat(preview,explore_entries[selected].name);
        if(explore_entries[selected].directory)strcat(preview,"\\");
      }
    }
  }
  explore_path_field(x,y,preview);
}

static int explore_activate(char *path,int selected)
{
  int needed;
  if(selected<0 || selected>=explore_count)return 0;
  if(explore_entries[selected].directory){
    if(!strcmp(explore_entries[selected].name,".."))explore_parent(path);
    else {
      needed=strlen(path)+strlen(explore_entries[selected].name)+2;
      if(needed>=MAX_CMD){notice_box("Path Too Long","That directory path is too long.");return 0;}
      strcat(path,explore_entries[selected].name);strcat(path,"\\");
    }
    return 2;
  }
  needed=strlen(path)+strlen(explore_entries[selected].name);
  if(needed>=MAX_CMD){notice_box("Path Too Long","That executable path is too long.");return 0;}
  strcpy(run_command,path);strcat(run_command,explore_entries[selected].name);
  return 1;
}

static int explore_command(char *path,int selected,char *command)
{
  int needed;
  if(selected<0 || selected>=explore_count || explore_entries[selected].directory){notice_box("Parameter Help","Select an executable file first.");return 0;}
  needed=strlen(path)+strlen(explore_entries[selected].name);
  if(needed>=MAX_CMD){notice_box("Path Too Long","That executable path is too long.");return 0;}
  strcpy(command,path);strcat(command,explore_entries[selected].name);
  return 1;
}

static int explore_params(char *path,int selected)
{
  char command[MAX_CMD];
  if(!explore_command(path,selected,command))return 0;
  return parameter_prompt_command(explore_entries[selected].name,command);
}



static void draw_browser_entry(int x,int y,int index,int attr,int width,int executable_icons)
{
  unsigned char left,right;
  char shown[14];
  if(index<0 || index>=explore_count)return;
  if(explore_entries[index].directory&&!strcmp(explore_entries[index].name,"..")){left=UI_PARENT_L;right=UI_PARENT_R;}
  else if(explore_entries[index].directory){left=launchui_browser_codes[4];right=launchui_browser_codes[5];}
  else if(executable_icons){left=launchui_browser_codes[2];right=launchui_browser_codes[3];}
  else {left=launchui_browser_codes[0];right=launchui_browser_codes[1];}
  cell(x,y,left,attr);cell(x+1,y,right,attr);
  strncpy(shown,explore_entries[index].name,width-2);shown[width-2]=0;
  textout(x+2,y,shown,attr,width-2);
}

static int explore_entry_attribute(int index,int selected)
{
  if(selected)return C_SELECTED;
  return explore_entries[index].directory?C_FOLDER:C_ITEM;
}

static void highlight_explore_entry(int x,int y,int top,int index,int selected)
{
  int relative=index-top,row,column;
  if(relative<0 || relative>=EXPLORE_ROWS*EXPLORE_COLS ||
     index<0 || index>=explore_count)return;
  column=relative/EXPLORE_ROWS;row=relative%EXPLORE_ROWS;
  row_attribute(x+2+column*18,y+6+row,16,
                explore_entry_attribute(index,selected));
}

static void select_explore_entry(int x,int y,int next,int *selected,int *top,
                                 int page,int hover,int *redraw,const char *path)
{
  int old=*selected,new_top=(next/page)*page;
  if(next<0 || next>=explore_count || next==old)return;
  if(new_top!=*top){*selected=next;*top=new_top;*redraw=1;return;}
  wait_vertical_retrace();
  highlight_explore_entry(x,y,*top,old,old==hover);*selected=next;
  highlight_explore_entry(x,y,*top,next,1);
  explore_selection_field(x+2,y+2,path,next);
  /* Selection applicability changes the Run/Params enabled state immediately. */
  *redraw=1;
}

static void change_explore_hover(int x,int y,int top,int old_hover,
                                 int new_hover,int selected)
{
  if(old_hover==new_hover)return;
  wait_vertical_retrace();
  if(old_hover>=0 && old_hover!=selected)
    highlight_explore_entry(x,y,top,old_hover,0);
  if(new_hover>=0)highlight_explore_entry(x,y,top,new_hover,1);
}

static void draw_explore_buttons(int x,int y,int focus,int hover,int enabled)
{
  if(enabled){draw_button(x+3,y+18,"  Run  ",9,focus==1||hover==1);draw_button(x+14,y+18,"  Params  ",10,focus==2||hover==2);}
  else {draw_button_disabled(x+3,y+18,"  Run  ",9);draw_button_disabled(x+14,y+18,"  Params  ",10);}
  draw_button(x+66,y+18,"  Exit  ",10,focus==3||hover==3);
}

static int explore_dialog(void)
{
  int x=(screen_cols-76)/2,y=(screen_rows-21)/2,k,mx=0,my=0;
  int selected=0,hover_entry=-1,top=0,page=EXPLORE_ROWS*EXPLORE_COLS;
  int focus=0,hover_control=-1,redraw=1;
  int i,row,column,index,action;unsigned mb;
  int last_click=-1,drive;unsigned long last_click_tick=0,tick;
  static char path[MAX_CMD]="C:\\";
  strcpy(path,"C:\\");
  explore_detect_drives();
  if(!explore_load(path)){notice_box("Explore Error","Unable to read drive C:.");return 0;}
  for(;;){
    if(selected>=explore_count)selected=explore_count?explore_count-1:-1;
    if(selected>=0 && selected<top)top=(selected/page)*page;
    if(selected>=0 && selected>=top+page)top=(selected/page)*page;
    if(redraw){render_begin();dialog_box(x,y,76,21,"Launch! Explore & Run");textout(x+3,y,"Launch!",ATTR(appearance.titlebar_bg,appearance.main_title),7);
    explore_selection_field(x+2,y+2,path,selected);
    cell(x,y+3,195,C_BORDER);cell(x+75,y+3,180,C_BORDER);
    for(i=1;i<75;i++)cell(x+i,y+3,196,C_BORDER);
    explore_drive_bar(x+2,y+4,focus>=5?focus-5:-1);
    cell(x,y+5,195,C_BORDER);cell(x+75,y+5,180,C_BORDER);
    for(i=1;i<75;i++)cell(x+i,y+5,196,C_BORDER);
    toolbar_divider(x,y+17,76);
    for(column=0;column<EXPLORE_COLS;column++)for(row=0;row<EXPLORE_ROWS;row++){
      index=top+column*EXPLORE_ROWS+row;
      if(index<explore_count)draw_browser_entry(x+2+column*18,y+6+row,index,
        explore_entry_attribute(index,index==selected),16,1);
      else textout(x+2+column*18,y+6+row,"",C_MENU_BACKGROUND,16);
    }
    cell(x+73,y+6,top>0?30:' ',C_BUTTON);
    cell(x+73,y+16,top+page<explore_count?31:' ',C_BUTTON);
    if(!explore_count)textout(x+2,y+6,"No executable files or directories",C_EMPTY,38);
    draw_explore_buttons(x,y,focus,hover_control,selected>=0&&selected<explore_count&&!explore_entries[selected].directory);
    render_end();redraw=0;}
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      int next_hover=-1,next_control=-1;
      if(my>=y+6 && my<y+17 && mx>=x+2 && mx<x+72){
        column=(mx-(x+2))/18;row=my-(y+6);
        index=top+column*EXPLORE_ROWS+row;
        if(index<explore_count)next_hover=-1;
      }
      else if(my==y+18 && mx>=x+3 && mx<x+12)next_control=1;
      else if(my==y+18 && mx>=x+14 && mx<x+24)next_control=2;
      else if(my==y+18 && mx>=x+66 && mx<x+72)next_control=3;
      change_explore_hover(x,y,top,hover_entry,next_hover,selected);
      hover_entry=next_hover;
      if(next_control!=hover_control){hover_control=next_control;
        draw_explore_buttons(x,y,focus,hover_control,selected>=0&&selected<explore_count&&!explore_entries[selected].directory);}
      continue;
    }
    if(mb&1){
      if(my==y+4){
        drive=explore_drive_at(mx);
        if(drive>=0 && drive!=toupper((unsigned char)path[0])-'A'){
          path[0]=(char)('A'+drive);strcpy(path+1,":\\");
          if(!explore_load(path))notice_box("Explore Error","Unable to read that drive.");
          selected=-1;top=focus=0;hover_entry=-1;last_click=-1;redraw=1;
        }
        continue;
      }
      if(my>=y+6 && my<y+17 && mx>=x+2 && mx<x+74){
        focus=0;
        if(mx>=x+72){
          if(my<y+12 && top>0){top-=page;selected=top;hover_entry=-1;redraw=1;}
          else if(my>=y+12 && top+page<explore_count){top+=page;selected=top;hover_entry=-1;redraw=1;}
          last_click=-1;
        } else {
          column=(mx-(x+2))/18;row=my-(y+6);index=top+column*EXPLORE_ROWS+row;
          if(index<explore_count){
            tick=*(unsigned long far *)MAKE_FP(0x40,0x6C);
            if(index==last_click && tick>=last_click_tick && tick-last_click_tick<=9UL){
              action=explore_activate(path,index);last_click=-1;
              if(action==1)return 1;
              if(action==2){
                if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");
                selected=-1;top=0;hover_entry=-1;redraw=1;
              }
            } else {
              select_explore_entry(x,y,index,&selected,&top,page,hover_entry,&redraw,path);
              last_click=index;last_click_tick=tick;
            }
          }
        }
        continue;
      }
      if(my==y+18 && mx>=x+3 && mx<x+12){
        focus=1;press_button(x+3,y+18,"  Run  ",9);
        if(selected<0 || selected>=explore_count){notice_box("Run","Select an executable file first.");redraw=1;continue;}
        action=explore_activate(path,selected);
        last_click=-1;
        if(action==1)return 1;
        if(action==2){if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");selected=-1;top=focus=0;hover_entry=-1;redraw=1;}
        continue;
      }
      if(my==y+18 && mx>=x+14 && mx<x+24){
        focus=2;press_button(x+14,y+18,"  Params  ",10);
        if(explore_params(path,selected))return 1;
        redraw=1;continue;
      }
      if(my==y+18 && mx>=x+66 && mx<x+72){
        focus=3;press_button(x+66,y+18,"  Exit  ",10);return 0;
      }
      continue;
    }
    if(k==27)return 0;
    if(k==9||k==0x0F00){
      int old_focus=focus;
      focus=(k==0x0F00)?explore_shift_tab_focus(focus):explore_tab_focus(focus);wait_vertical_retrace();
      if(old_focus>=5 || focus>=5)
        explore_drive_bar(x+2,y+4,focus>=5?focus-5:-1);
      if((old_focus>=1 && old_focus<=3) || (focus>=1 && focus<=3))
        draw_explore_buttons(x,y,focus,hover_control,selected>=0&&selected<explore_count&&!explore_entries[selected].directory);
      continue;
    }
    if(focus>=5){
      int old_drive=focus-5,new_drive=old_drive;
      if(k==0x4B00)new_drive=explore_adjacent_drive(old_drive,-1);
      else if(k==0x4D00)new_drive=explore_adjacent_drive(old_drive,1);
      else if(k==13||k==' '){
        drive=old_drive;
        path[0]=(char)('A'+drive);strcpy(path+1,":\\");
        if(!explore_load(path))notice_box("Explore Error","Unable to read that drive.");
        selected=-1;top=0;hover_entry=-1;last_click=-1;redraw=1;
        continue;
      }
      if(new_drive!=old_drive || focus==0){
        if(focus)focus=new_drive+5;
        wait_vertical_retrace();
        explore_drive_bar(x+2,y+4,focus>=5?focus-5:-1);
      }
      continue;
    }
    if(focus){
      int old_focus=focus;
      if(k==0x4B00)focus=focus==1?3:focus-1;
      else if(k==0x4D00)focus=focus==3?1:focus+1;
      else if(k==0x4800)focus=0;
      else if(k==13||k==' '){
        if(focus==3)return 0;
        if(focus==2){if(explore_params(path,selected))return 1;redraw=1;continue;}
        if(focus==1 && (selected<0 || selected>=explore_count)){notice_box("Run","Select an executable file first.");redraw=1;continue;}
        action=explore_activate(path,selected);
        if(action==1)return 1;
        if(action==2){if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");selected=-1;top=focus=0;hover_entry=-1;redraw=1;}
      }
      if(focus!=old_focus){wait_vertical_retrace();
        draw_explore_buttons(x,y,focus,hover_control,selected>=0&&selected<explore_count&&!explore_entries[selected].directory);}
      continue;
    }
    if(k==0x4800 && selected>0)select_explore_entry(x,y,selected-1,&selected,&top,page,hover_entry,&redraw,path);
    else if(k==0x5000 && selected+1<explore_count)select_explore_entry(x,y,selected+1,&selected,&top,page,hover_entry,&redraw,path);
    else if(k==0x4B00){
      if(selected>=EXPLORE_ROWS)
        select_explore_entry(x,y,selected-EXPLORE_ROWS,&selected,&top,page,
                             hover_entry,&redraw,path);
      else if(!explore_is_root(path)){
        explore_parent(path);explore_load(path);selected=-1;top=0;
        hover_entry=-1;redraw=1;
      }
    }
    else if(k==0x4D00 && selected+EXPLORE_ROWS<explore_count)select_explore_entry(x,y,selected+EXPLORE_ROWS,&selected,&top,page,hover_entry,&redraw,path);
    else if(k==0x4900){top=top>=page?top-page:0;selected=top;hover_entry=-1;redraw=1;}
    else if(k==0x5100 && top+page<explore_count){top+=page;selected=top;hover_entry=-1;redraw=1;}
    else if(k==8 && !explore_is_root(path)){explore_parent(path);explore_load(path);selected=-1;top=0;hover_entry=-1;redraw=1;}
    else if(k==13||k==' '){
      action=explore_activate(path,selected);
      if(action==1)return 1;
      if(action==2){if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");selected=-1;top=0;hover_entry=-1;redraw=1;}
    }
  }
}

/* --- Collections ------------------------------------------------------- */
#define MAX_COLLECTIONS 24
#define MAX_COLLECTION_RESULTS 256
#define COLLECTION_DEPTH 5
#define COLLECTION_PATH 80

typedef struct { char name[17],ext[49]; int launcher; } COLLECTION;
typedef struct { char name[13],date[9],path[COLLECTION_PATH]; } COLLECTION_RESULT;
static COLLECTION collections[MAX_COLLECTIONS];
static COLLECTION_RESULT far *collection_results=0;
static int collection_count,collection_result_count,collection_sort_col,collection_sort_desc;
static int collection_last_changed=-1;
static int collection_macro_direct;
static int collection_run_launcher=-1;
static unsigned char openfile_mark[MAX_EXPLORE_ENTRIES];
/* Search result paths live in the far-heap collection_results table.
   Do not duplicate them in DGROUP: TC17 is already close to the 64K near-data
   ceiling under Microsoft C 7 medium model. */
static int openfile_result_mode=0;

static int core_ctrl_down(void)
{
  unsigned char far *p=(unsigned char far *)MAKE_FP(0x40,0x17);
  return ((*p)&4)!=0;
}

static int collection_name_compare(const void *aa,const void *bb)
{
  const COLLECTION *a=(const COLLECTION *)aa,*b=(const COLLECTION *)bb;
  int n=stricmp(a->name,b->name);
  if(n)return n;
  return stricmp(a->ext,b->ext);
}
static void collections_sort(void)
{
  if(collection_count>1)qsort(collections,collection_count,sizeof(collections[0]),collection_name_compare);
}
static int collection_find_exact(const char *name,const char *ext,int launcher)
{
  int i;for(i=0;i<collection_count;i++)
    if(!stricmp(collections[i].name,name)&&!stricmp(collections[i].ext,ext)&&collections[i].launcher==launcher)return i;
  return 0;
}

static int collections_load(void)
{
  FILE *f;char line[160],*a,*b;collection_count=0;
  f=fopen(collections_file,"rt");if(!f)return 1;
  while(collection_count<MAX_COLLECTIONS && fgets(line,sizeof(line),f)){
    char *t=trim(line);
    if(!*t || *t==';' || *t=='#')continue;
    if(t!=line)memmove(line,t,strlen(t)+1);
    a=strchr(line,'|');if(!a)continue;*a++=0;b=strchr(a,'|');if(!b)continue;*b++=0;
    b[strcspn(b,"\r\n")]=0;
    strncpy(collections[collection_count].name,line,16);collections[collection_count].name[16]=0;
    strncpy(collections[collection_count].ext,a,48);collections[collection_count].ext[48]=0;
    collections[collection_count].launcher=atoi(b);
    if(collections[collection_count].name[0]&&collections[collection_count].ext[0])collection_count++;
  }fclose(f);collections_sort();return 1;
}
static int collections_save(void)
{
  FILE *f=fopen(collections_file,"wt");int i;if(!f)return 0;
  fputs("; Launch! 3.5 File associations - description|extension|launcher node\n",f);
  for(i=0;i<collection_count;i++)fprintf(f,"%s|%s|%d\n",collections[i].name,collections[i].ext,collections[i].launcher);
  return fclose(f)==0;
}
static int collection_launcher_choose_core(const char *ext)
{
  int parent=-1,sel=0,list[MAX_CHILD],n,i,k,x=(screen_cols-MENU_WIDTH)/2,y,node,item_y,h,mx=0,my=0,hover=-1;unsigned mb=0;
  for(;;){
    n=children(parent,list);if(!n){if(parent<0)return -1;parent=nodes[parent].parent;sel=0;continue;}
    if(sel<0)sel=0;if(sel>=n+(parent>=0))sel=n+(parent>=0)-1;
    h=n+(parent<0?5:3);y=(screen_rows-h)/2;if(y<0)y=0;
    menu_box(x,y,MENU_WIDTH,h,0,C_TITLE,0);
    if(parent<0){
      textout(x+1,y+1,"Choose launcher",C_BORDER,MENU_WIDTH-2);
      textout(x+1,y+2,"to open ",C_BORDER,8);textout(x+9,y+2,ext,C_ITEM,MENU_WIDTH-10);
      for(i=1;i<MENU_WIDTH-1;i++)cell(x+i,y+3,196,C_BORDER);
      item_y=y+4;
    } else {
      /* Back is a single menu item: paint its complete interior first so the
         cell between the arrow and label cannot retain the menu background. */
      textout(x+1,y+1,"                  ",sel==0?C_SELECTED:C_FOLDER,MENU_WIDTH-2);
      cell(x+1,y+1,17,sel==0?C_SELECTED:C_FOLDER);
      textout(x+3,y+1,"Back",sel==0?C_SELECTED:C_FOLDER,MENU_WIDTH-4);
      item_y=y+2;
    }
    for(i=0;i<n;i++){
      int visual=i+(parent>=0);node=list[i];
      if(nodes[node].separator)textout(x+1,item_y+i,"------------------",C_BORDER,MENU_WIDTH-2);
      else {textout(x+1,item_y+i,nodes[node].title,visual==sel?C_SELECTED:(nodes[node].folder?C_FOLDER:C_ITEM),MENU_WIDTH-3);if(nodes[node].folder)cell(x+MENU_WIDTH-2,item_y+i,16,visual==sel?C_SELECTED:C_FOLDER);}
    }
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      int newsel=-1;
      if(mx>x && mx<x+MENU_WIDTH-1){
        if(parent>=0 && my==y+1)newsel=0;
        else if(my>=item_y && my<item_y+n){
          i=my-item_y;node=list[i];
          if(node>=0 && !nodes[node].separator)newsel=i+(parent>=0);
        }
      }
      if(newsel>=0 && newsel!=sel)sel=newsel;
      continue;
    }
    if(mb&1){
      if(parent>=0&&my==y+1&&mx>x&&mx<x+MENU_WIDTH-1){parent=nodes[parent].parent;sel=0;continue;}
      if(my>=item_y&&my<item_y+n&&mx>x&&mx<x+MENU_WIDTH-1){i=my-item_y;node=list[i];if(node<0||nodes[node].separator)continue;if(nodes[node].folder){parent=node;sel=0;continue;}return node;}
      continue;
    }
    if(k==27){if(parent<0)return -1;parent=nodes[parent].parent;sel=0;continue;}
    if(parent>=0 && (k==8||k==0x4B00)){parent=nodes[parent].parent;sel=0;continue;}
    if(k==0x4800){if(sel>0)sel--;continue;}if(k==0x5000){if(sel+1<n+(parent>=0))sel++;continue;}
    if(k==13||k==0x4D00){if(parent>=0&&sel==0){parent=nodes[parent].parent;sel=0;continue;}i=sel-(parent>=0);node=list[i];if(node<0||nodes[node].separator)continue;if(nodes[node].folder){parent=node;sel=0;continue;}return node;}
  }
}

static int collection_launcher_choose(const char *ext)
{
  unsigned short far *behind;
  unsigned i,n=(unsigned)(screen_cols*screen_rows);
  int result;
  int old_close_x=dialog_close_x,old_close_y=dialog_close_y;
  /* The association editor activates Choose on the mouse-down edge.  Do not
     let that same physical click enter the nested launcher menu: wait for an
     actual button release using INT 33h state (not edge-triggered mouse_poll),
     then reset our edge state before the picker becomes live. */
  if(mouse_present){
    union REGS mr;
    /* Synchronise to the current physical button state instead of spinning
       until release.  Some DOS mouse drivers can keep button 1 asserted
       while a modal is being redrawn, which made File Open appear to lock
       the entire machine.  The normal edge detector will see the next
       press only after the current button has been released. */
    mr.x.ax=3;int86(0x33,&mr,&mr);
    mouse_last_buttons=mr.x.bx;
  }
  /* Disable the parent dialog close hit target while the nested picker is live. */
  dialog_close_x=-1;dialog_close_y=-1;
  behind=(unsigned short far *)_fmalloc(n*2U);
  if(!behind){result=collection_launcher_choose_core(ext);}
  else {
    for(i=0;i<n;i++)behind[i]=video[i];
    result=collection_launcher_choose_core(ext);
    wait_vertical_retrace();
    for(i=0;i<n;i++)video[i]=behind[i];
    _ffree(behind);
  }
  /* Restore the association dialog's titlebar Close hit target. */
  dialog_close_x=old_close_x;dialog_close_y=old_close_y;
  return result;
}

/* -------------------------------------------------------------------------
   Association editor

   Fresh 3.5 implementation.  This is deliberately self-contained: the form
   owns its focus, mouse hit-testing and field editing, and all activation
   paths call the same action directly.  In particular the Choose control does
   NOT use press_button() or any inherited mouse-release state before entering
   the modal launcher picker.  Keep this form independent of generic menu
   button timing so future UI changes cannot reintroduce the old intermittent
   Choose regression.
   ------------------------------------------------------------------------- */

#pragma code_seg("ASSOC_TEXT")

static int assoc_normalize_extensions(const char *src,char *dst,int dstsz)
{
  const char *p=src;int used=0,first=1;
  while(*p){
    char e[5];int n=0;
    while(*p==' '||*p=='\t'||*p==',')p++;
    if(!*p)break;
    if(*p=='.')p++;
    while(*p&&*p!=','){
      if(*p!=' '&&*p!='\t'){
        if(!isalnum((unsigned char)*p)||n>=3)return 0;
        e[n++]=(char)toupper((unsigned char)*p);
      }
      p++;
    }
    if(!n)return 0;
    e[n]=0;
    if(used+(first?0:1)+n+1>=dstsz)return 0;
    if(!first)dst[used++]=',';
    dst[used++]='.';
    memcpy(dst+used,e,n);used+=n;dst[used]=0;first=0;
    if(*p==',')p++;
  }
  return used>0;
}

static void assoc_caption_extension(const char *src,char *out)
{
  const char *p=src;int n=0;
  out[0]=0;
  while(*p==' '||*p=='\t')p++;
  out[n++]='.';if(*p=='.')p++;
  while(*p&&*p!=','&&n<4){
    if(*p!=' '&&*p!='\t'){
      if(!isalnum((unsigned char)*p)){out[0]=0;return;}
      out[n++]=(char)toupper((unsigned char)*p);
    }
    p++;
  }
  if(n==1){out[0]=0;return;}
  out[n]=0;
}

static int assoc_scroll_for_cursor(int pos,int width)
{
  return pos>=width?pos-width+1:0;
}

static void assoc_draw_field(int lx,int fx,int row,const char *label,
                             const char *value,int width,int pos,int focused,int hovered)
{
  int scroll=assoc_scroll_for_cursor(pos,width),cursor=pos-scroll;
  int attr=(focused||hovered)?C_SELECTED:C_INPUT_FIELD;
  textout(lx,row,label,C_INPUT_LABEL,(int)strlen(label));
  textout(fx,row,value+scroll,attr,width);
  if(focused){
    int len=(int)strlen(value);
    if(cursor<0)cursor=0;if(cursor>=width)cursor=width-1;
    cell(fx+cursor,row,pos<len?value[pos]:' ',C_INPUT_FIELD);edit_caret_set(fx+cursor,row);
  }
}

static int assoc_next_control(int f,int dir,int save_ok)
{
  int tries;
  for(tries=0;tries<5;tries++){
    f+=dir;if(f<0)f=4;if(f>4)f=0;
    if(f!=3||save_ok)return f;
  }
  return 4;
}

static int assoc_pick_launcher(const char *extensions,int current)
{
  char ext[5];int chosen;
  assoc_caption_extension(extensions,ext);
  chosen=collection_launcher_choose(ext);
  return chosen>=0?chosen:current;
}

static int collection_edit_dialog(int edit_index)
{
  enum { A_NAME=0,A_EXT=1,A_CHOOSE=2,A_SAVE=3,A_CANCEL=4 };
  int x=(screen_cols-60)/2,y=(screen_rows-14)/2;
  int focus=A_NAME,hover=-1,redraw=2,k=0,mx=0,my=0;
  int pos_name=0,pos_ext=0,launcher=-1,save_ok,len,scroll;
  unsigned mb=0;
  static char name[17],ext[49],normalized[49],chosen[25];

  name[0]=ext[0]=normalized[0]=chosen[0]=0;
  if(edit_index>=0&&edit_index<collection_count){
    strcpy(name,collections[edit_index].name);
    strcpy(ext,collections[edit_index].ext);
    launcher=collections[edit_index].launcher;
    pos_name=(int)strlen(name);pos_ext=(int)strlen(ext);
  }

  for(;;){
    save_ok=(name[0]&&ext[0]&&launcher>=0);
    if(focus==A_SAVE&&!save_ok)focus=A_CANCEL;

    if(redraw){
      edit_caret_hide();if(redraw==2)subdialog_box(x,y,60,14,edit_index>=0?"Edit association":"Create association");
      assoc_draw_field(x+3,x+29,y+2,"Association name:",name,16,pos_name,focus==A_NAME,hover==A_NAME);
      assoc_draw_field(x+3,x+29,y+4,"File extension/s:",ext,26,pos_ext,focus==A_EXT,hover==A_EXT);
      textout(x+29,y+5,"Separate multiple with ,",C_INPUT_LABEL,24);
      textout(x+3,y+7,"Open with launcher...",C_INPUT_LABEL,21);
      draw_button(x+29,y+7,"  Choose  ",10,focus==A_CHOOSE||hover==A_CHOOSE);
      if(launcher>=0){sprintf(chosen,"Open with %.18s",nodes[launcher].title);textout(x+29,y+9,chosen,C_TITLE,25);}
      else textout(x+29,y+9,"",C_MENU_BACKGROUND,25);
      if(save_ok)draw_button(x+3,y+11,"  OK  ",8,focus==A_SAVE||hover==A_SAVE);
      else draw_button_disabled(x+3,y+11,"  OK  ",8);
      draw_button(x+13,y+11,"  Cancel  ",10,focus==A_CANCEL||hover==A_CANCEL);
      redraw=0;
    }

    wait_input(&k,&mx,&my,&mb);

    /* Mouse press is handled before hover/state changes.  A click has one and
       only one destination, and Choose enters the picker directly. */
    if(mb&1){
      if(my==y+2&&mx>=x+29&&mx<x+45){
        focus=A_NAME;len=(int)strlen(name);scroll=assoc_scroll_for_cursor(pos_name,16);
        pos_name=scroll+mx-(x+29);if(pos_name>len)pos_name=len;hover=-1;redraw=1;continue;
      }
      if(my==y+4&&mx>=x+29&&mx<x+55){
        focus=A_EXT;len=(int)strlen(ext);scroll=assoc_scroll_for_cursor(pos_ext,26);
        pos_ext=scroll+mx-(x+29);if(pos_ext>len)pos_ext=len;hover=-1;redraw=1;continue;
      }
      if(my==y+7&&mx>=x+29&&mx<x+39){
        focus=A_CHOOSE;hover=-1;
        launcher=assoc_pick_launcher(ext,launcher);
        redraw=1;continue;
      }
      if(save_ok&&my==y+11&&mx>=x+3&&mx<x+11){focus=A_SAVE;k=13;hover=-1;}
      else if(my==y+11&&mx>=x+13&&mx<x+23){focus=A_CANCEL;k=13;hover=-1;}
      else continue;
    } else if(mb&MOUSE_MOVED){
      int h=-1;
      if(my==y+2&&mx>=x+29&&mx<x+45)h=A_NAME;
      else if(my==y+4&&mx>=x+29&&mx<x+55)h=A_EXT;
      else if(my==y+7&&mx>=x+29&&mx<x+39)h=A_CHOOSE;
      else if(save_ok&&my==y+11&&mx>=x+3&&mx<x+11)h=A_SAVE;
      else if(my==y+11&&mx>=x+13&&mx<x+23)h=A_CANCEL;
      if(h!=hover){hover=h;redraw=1;}
      continue;
    }

    if(k==27)return 0;
    if(k==9||k==0x0F00){focus=assoc_next_control(focus,k==0x0F00?-1:1,save_ok);hover=-1;redraw=1;continue;}

    if((focus==A_NAME||focus==A_EXT)&&(k==0x4800||k==0x5000)){
      focus=(focus==A_NAME)?A_EXT:A_NAME;hover=-1;redraw=1;continue;
    }

    if(focus==A_NAME||focus==A_EXT){
      char *field=(focus==A_NAME)?name:ext;
      int *ppos=(focus==A_NAME)?&pos_name:&pos_ext;
      int max=(focus==A_NAME)?16:48;
      len=(int)strlen(field);
      if(k==0x4B00){if(*ppos>0)(*ppos)--;}
      else if(k==0x4D00){if(*ppos<len)(*ppos)++;}
      else if(k==0x4700)*ppos=0;
      else if(k==0x4F00)*ppos=len;
      else if(k==8&&*ppos>0){memmove(field+*ppos-1,field+*ppos,len-*ppos+1);(*ppos)--;}
      else if(k==0x5300&&*ppos<len)memmove(field+*ppos,field+*ppos+1,len-*ppos);
      else if(k==13)focus=(focus==A_NAME)?A_EXT:A_CHOOSE;
      else if(k>=32&&k<127&&len<max){
        if(focus==A_EXT&&k!='.'&&k!=','&&k!=' '&&!isalnum((unsigned char)k)){redraw=1;continue;}
        memmove(field+*ppos+1,field+*ppos,len-*ppos+1);field[(*ppos)++]=(char)k;
      }
      redraw=1;continue;
    }

    if((k==13||k==' ')&&focus==A_CHOOSE){launcher=assoc_pick_launcher(ext,launcher);hover=-1;redraw=1;continue;}
    if((k==13||k==' ')&&focus==A_CANCEL)return 0;
    if((k==13||k==' ')&&focus==A_SAVE&&save_ok){
      COLLECTION *c;
      if(!assoc_normalize_extensions(ext,normalized,sizeof(normalized))){notice_box("Association","Enter valid extension/s first.");redraw=1;continue;}
      if(edit_index<0&&collection_count>=MAX_COLLECTIONS){notice_box("Associations Full","The association list is full.");return 0;}
      c=(edit_index>=0)?&collections[edit_index]:&collections[collection_count++];
      strncpy(c->name,name,16);c->name[16]=0;
      strncpy(c->ext,normalized,48);c->ext[48]=0;c->launcher=launcher;
      collections_sort();collection_last_changed=collection_find_exact(name,normalized,launcher);
      if(!collections_save())notice_box("Write Error","Could not save ASSOC.CFG.");
      return 1;
    }
  }
}

static int collection_create(void){return collection_edit_dialog(-1);}
static int collection_edit(int index){return collection_edit_dialog(index);}

#pragma code_seg("OPENFILE_TEXT")

static void openfile_path_field(int x,int y,const char *path,int width)
{
  int length=strlen(path);char shown[MAX_CMD];
  if(length<=width)textout(x,y,path,C_CONSOLE_PATH,width);
  else {strcpy(shown,"...");strncpy(shown+3,path+length-(width-3),width-3);shown[width]=0;textout(x,y,shown,C_CONSOLE_PATH,width);}
}
static void openfile_selection_field(int x,int y,const char *path,int selected,int width)
{
  char preview[MAX_CMD];int needed;strcpy(preview,path);
  if(selected>=0&&selected<explore_count){if(explore_entries[selected].directory&&!strcmp(explore_entries[selected].name,".."))explore_parent(preview);else{needed=strlen(preview)+strlen(explore_entries[selected].name)+2;if(needed<MAX_CMD){strcat(preview,explore_entries[selected].name);if(explore_entries[selected].directory)strcat(preview,"\\");}}}
  openfile_path_field(x,y,preview,width);
}
static void openfile_drive_bar(int x,int y,int focused_drive,int width)
{
  int drive,pos=0;unsigned char symbol;int label_attr=ATTR(appearance.background,appearance.labels),drive_attr=ATTR(appearance.background,appearance.launchers),symbol_attr=ATTR(appearance.background,appearance.titles);
  textout(x,y,"",label_attr,width);
  for(drive=0;drive<26;drive++){explore_drive_positions[drive]=-1;symbol=explore_drive_symbols[drive];if(!symbol)continue;if(pos+6>width)break;explore_drive_positions[drive]=x+pos;cell(x+pos++,y,'[',drive==focused_drive?C_SELECTED:label_attr);cell(x+pos++,y,'A'+drive,drive==focused_drive?C_SELECTED:drive_attr);cell(x+pos++,y,':',drive==focused_drive?C_SELECTED:drive_attr);cell(x+pos++,y,' ',drive==focused_drive?C_SELECTED:label_attr);cell(x+pos++,y,symbol,drive==focused_drive?C_SELECTED:symbol_attr);cell(x+pos++,y,']',drive==focused_drive?C_SELECTED:label_attr);if(pos<width)cell(x+pos++,y,' ',label_attr);}
}

static int collection_file_match(const char *name,const char *exts)
{
  const char *p=strrchr(name,'.'),*q=exts;char one[5];int n;
  if(!p)return 0;
  while(*q){
    while(*q==','||*q==' '||*q=='\t')q++;
    n=0;while(*q&&*q!=','&&n<4)one[n++]=*q++;one[n]=0;
    if(n&&!stricmp(p,one))return 1;
    while(*q&&*q!=',')q++;
  }
  return 0;
}

static int openfile_load(const char *path,const char *ext)
{
  struct find_t found;char mask[MAX_CMD];unsigned result;explore_count=0;
  openfile_result_mode=0;memset(openfile_mark,0,sizeof(openfile_mark));
  if(!explore_is_root(path))explore_add("..",1);
  if(strlen(path)>MAX_CMD-4)return 0;strcpy(mask,path);strcat(mask,"*.*");
  result=_dos_findfirst(mask,_A_NORMAL|_A_RDONLY|_A_SUBDIR,&found);
  while(!result){
    if((found.attrib&_A_SUBDIR)&&strcmp(found.name,".")&&strcmp(found.name,".."))explore_add(found.name,1);
    else if(!(found.attrib&_A_SUBDIR)&&collection_file_match(found.name,ext))explore_add(found.name,0);
    result=_dos_findnext(&found);
  }
  qsort(explore_entries,explore_count,sizeof(explore_entries[0]),explore_compare);return 1;
}

/* Keep the DOS/LFN quoting policy outside the browser code segment.  The
   association editor is isolated in ASSOC_TEXT and uses the standard Launch!
   form/button path; filename quoting is deliberately unrelated to it. */
static void openfile_apply_dos_quoting(char *cmd,const char *path,const char *name);

static void openfile_far_to_near(char *dst,const char far *src,int max)
{int i=0;if(max<=0)return;while(i<max-1&&src[i]){dst[i]=src[i];i++;}dst[i]=0;}
static void openfile_near_to_far(char far *dst,const char *src,int max)
{int i=0;if(max<=0)return;while(i<max-1&&src[i]){dst[i]=src[i];i++;}dst[i]=0;}

static void openfile_entry_fullpath(int index,const char *browser_path,char *out)
{
  if(openfile_result_mode){
    char result_name[13];
    openfile_far_to_near(out,collection_results[index].path,COLLECTION_PATH);
    openfile_far_to_near(result_name,collection_results[index].name,13);
    if(out[0]&&out[strlen(out)-1]!='\\')strcat(out,"\\");
    strcat(out,result_name);
  }else{
    strcpy(out,browser_path);strcat(out,explore_entries[index].name);
  }
}

static int openfile_command(const char *path,int selected,int collection,char *out)
{
  int i,count=0,needed;char full[MAX_CMD];
  if(selected<0||selected>=explore_count||explore_entries[selected].directory){notice_box("File Open","Select a file first.");return 0;}
  strcpy(out,nodes[collections[collection].launcher].command);
  for(i=0;i<explore_count;i++)if(openfile_mark[i]&&!explore_entries[i].directory)count++;
  if(!count)openfile_mark[selected]=1;
  for(i=0;i<explore_count;i++)if(openfile_mark[i]&&!explore_entries[i].directory){
    openfile_entry_fullpath(i,path,full);
    needed=(int)strlen(out)+(int)strlen(full)+4;
    if(needed>=MAX_CMD){notice_box("Command Too Long","The selected files do not fit on the DOS command line.");return 0;}
    strcat(out," ");strcat(out,full);
  }
  /* DOS Launch! targets use ordinary unquoted path arguments.  In
     particular, many period DOS programs do not accept quote characters. */
  return 1;
}

static int search_wild_match(const char *pat,const char *name)
{
  while(*pat){
    if(*pat=='*'){pat++;if(!*pat)return 1;while(*name){if(search_wild_match(pat,name))return 1;name++;}return 0;}
    if(*pat=='?'||*pat=='!'){if(!*name)return 0;pat++;name++;continue;}
    if(toupper((unsigned char)*pat)!=toupper((unsigned char)*name))return 0;
    pat++;name++;
  }
  return *name==0;
}
static int search_contains_ci(const char *a,const char *b)
{
  int i;if(!*b)return 1;for(;*a;a++){for(i=0;b[i]&&a[i]&&toupper((unsigned char)a[i])==toupper((unsigned char)b[i]);i++);if(!b[i])return 1;}return 0;
}
static int search_binary(const char *file)
{
  FILE*f=fopen(file,"rb");unsigned char b[256];size_t n,i;if(!f)return 1;n=fread(b,1,sizeof(b),f);fclose(f);
  for(i=0;i<n;i++)if(b[i]==0)return 1;return 0;
}
static int search_file_contains(const char *file,const char *needle)
{
  FILE*f;char line[256];if(!needle[0]||search_binary(file))return 0;f=fopen(file,"rt");if(!f)return 0;
  while(fgets(line,sizeof(line),f))if(search_contains_ci(line,needle)){fclose(f);return 1;}fclose(f);return 0;
}
static void search_literal_from_pattern(const char *pattern,char *out,int max)
{
  int n=0;while(*pattern&&n<max){if(*pattern!='*'&&*pattern!='?'&&*pattern!='!')out[n++]=*pattern;pattern++;}out[n]=0;trim(out);
}
static void openfile_search_live_draw(void)
{
  int dx=(screen_cols-78)/2,dy=(screen_rows-21)/2,rx=21,row,index,start;
  char rn[13],rp[COLLECTION_PATH],line[53];int avail,plen;
  /* The search form has already been removed.  Repaint the result pane as
     matches arrive so a recursive disk search never looks frozen. */
  start=((collection_result_count-1)/EXPLORE_ROWS)*EXPLORE_ROWS;
  for(row=0;row<EXPLORE_ROWS;row++){
    index=start+row;textout(dx+rx,dy+6+row,"",C_MENU_BACKGROUND,52);
    if(index>=collection_result_count)continue;
    openfile_far_to_near(rn,collection_results[index].name,13);
    openfile_far_to_near(rp,collection_results[index].path,COLLECTION_PATH);
    line[0]=0;strncat(line,rn,12);strcat(line,"  ");
    avail=52-(int)strlen(line);
    plen=(int)strlen(rp);
    if(plen<=avail)strncat(line,rp,avail);
    else if(avail>3){strncat(line,rp,avail-3);strcat(line,"...");}
    textout(dx+rx,dy+6+row,line,C_ITEM,52);
  }
  render_end();
}

static void openfile_search_dir(const char *dir,const char *pattern,const char *exts,int recurse,int deep,int depth)
{
  struct find_t found;char mask[MAX_CMD],full[MAX_CMD],literal[64];unsigned r;int namehit,contenthit;
  if(collection_result_count>=MAX_COLLECTION_RESULTS||depth>24)return;
  strcpy(mask,dir);if(mask[0]&&mask[strlen(mask)-1]!='\\')strcat(mask,"\\");strcat(mask,"*.*");
  search_literal_from_pattern(pattern,literal,63);
  r=_dos_findfirst(mask,_A_NORMAL|_A_RDONLY|_A_SUBDIR,&found);
  while(!r&&collection_result_count<MAX_COLLECTION_RESULTS){
    if((found.attrib&_A_SUBDIR)&&strcmp(found.name,".")&&strcmp(found.name,"..")){
      if(recurse){strcpy(full,dir);if(full[strlen(full)-1]!='\\')strcat(full,"\\");strcat(full,found.name);openfile_search_dir(full,pattern,exts,recurse,deep,depth+1);}
    }else if(!(found.attrib&_A_SUBDIR)&&collection_file_match(found.name,exts)){
      /* Plain search text is a case-insensitive filename fragment.  Wildcard
         characters opt into DOS-style wildcard matching.  Thus "globe"
         finds GLOBE.BMP while "glob?.*" remains available when wanted. */
      if(strchr(pattern,'*')||strchr(pattern,'?')||strchr(pattern,'!'))
        namehit=search_wild_match(pattern,found.name);
      else namehit=search_contains_ci(found.name,pattern);
      contenthit=0;
      strcpy(full,dir);if(full[strlen(full)-1]!='\\')strcat(full,"\\");strcat(full,found.name);
      if(deep&&!namehit&&literal[0])contenthit=search_file_contains(full,literal);
      if(namehit||contenthit){
        COLLECTION_RESULT far *cr=&collection_results[collection_result_count++];
        openfile_near_to_far(cr->name,found.name,13);cr->date[0]=0;openfile_near_to_far(cr->path,dir,COLLECTION_PATH);
        openfile_search_live_draw();
      }
    }
    r=_dos_findnext(&found);
  }
}
static int openfile_search_dialog(const char *assoc_name,const char *exts,const char *current)
{
  int w=62,h=15,x=(screen_cols-w)/2,y=(screen_rows-h)/2,focus=0,k,mx,my,posn,posp,recurse=1,deep=0,i;unsigned mb;
  unsigned short far *behind=0;unsigned cells=(unsigned)(screen_cols*screen_rows);
  char pattern[32]="*",look[COLLECTION_PATH],heading[48];
  behind=(unsigned short far *)_fmalloc(cells*2U);
  if(behind)for(i=0;i<(int)cells;i++)behind[i]=video[i];
  strncpy(look,current,COLLECTION_PATH-1);look[COLLECTION_PATH-1]=0;posn=strlen(pattern);posp=strlen(look);
  strcpy(heading,"Search files for ");
  if(assoc_name)strncat(heading,assoc_name,sizeof(heading)-strlen(heading)-1);
  for(;;){
    subdialog_box(x,y,w,h,"Search Files");
    textout(x+3,y+2,heading,C_TITLE,w-6);
    textout(x+3,y+4,"Search for:",C_INPUT_LABEL,12);textout(x+16,y+4,"                                      ",focus==0?C_SELECTED:C_INPUT_FIELD,38);textout(x+16,y+4,pattern,focus==0?C_SELECTED:C_INPUT_FIELD,38);
    textout(x+3,y+6,"Look in:",C_INPUT_LABEL,12);textout(x+16,y+6,"                                      ",focus==1?C_SELECTED:C_INPUT_FIELD,38);textout(x+16,y+6,look,focus==1?C_SELECTED:C_INPUT_FIELD,38);
    check_line(x+16,y+8,"Search sub-directories",recurse,focus==2);
    check_line(x+16,y+9,"Go deep",deep,focus==3);
    draw_button(x+3,y+12," Search ",8,focus==4);draw_button(x+12,y+12,"  Cancel  ",10,focus==5);
    wait_input(&k,&mx,&my,&mb);
    if(mb&1){
      if(my==y+8&&mx>=x+16&&mx<x+40){recurse=!recurse;continue;}
      if(my==y+9&&mx>=x+16&&mx<x+40){deep=!deep;continue;}
      if(my==y+12&&mx>=x+3&&mx<x+11){focus=4;k=13;}
      else if(my==y+12&&mx>=x+12&&mx<x+22){focus=5;k=13;}
      else if(my==y+4&&mx>=x+16&&mx<x+54)focus=0;
      else if(my==y+6&&mx>=x+16&&mx<x+54)focus=1;
    }
    if(k==27){if(behind){for(i=0;i<(int)cells;i++)video[i]=behind[i];_ffree(behind);}return 0;}
    if(k==9||k==0x0F00){focus+=(k==0x0F00?-1:1);if(focus<0)focus=5;if(focus>5)focus=0;continue;}
    if((k==13||k==' ')&&focus==2){recurse=!recurse;continue;}
    if((k==13||k==' ')&&focus==3){deep=!deep;continue;}
    if((k==13||k==' ')&&focus==5){if(behind){for(i=0;i<(int)cells;i++)video[i]=behind[i];_ffree(behind);}return 0;}
    if((k==13||k==' ')&&focus==4)break;
    if(focus==0){int len=strlen(pattern);if(k==0x4700){posn=0;}else if(k==0x4F00){posn=len;}else if(k==8&&posn){memmove(pattern+posn-1,pattern+posn,len-posn+1);posn--;}else if(k>=32&&k<127&&len<30){memmove(pattern+posn+1,pattern+posn,len-posn+1);pattern[posn++]=(char)k;}}
    else if(focus==1){int len=strlen(look);if(k==8&&posp){memmove(look+posp-1,look+posp,len-posp+1);posp--;}else if(k>=32&&k<127&&len<COLLECTION_PATH-2){memmove(look+posp+1,look+posp,len-posp+1);look[posp++]=(char)k;}}
  }
  trim(pattern);trim(look);if(!pattern[0])strcpy(pattern,"*");if(!look[0])strcpy(look,current);
  /* Close the modal form before touching the disk.  The File Open window is
     restored immediately and the result pane is then populated live. */
  if(behind){for(i=0;i<(int)cells;i++)video[i]=behind[i];_ffree(behind);behind=0;}
  /* Microsoft C 7.0 medium model keeps ordinary static data in DGROUP.  A
     static `far` result array proved unsafe here: writes landed in near data
     on the real compiler, corrupting associations and leaving blank result
     names.  Allocate the search table explicitly from the far heap instead. */
  if(!collection_results){
    collection_results=(COLLECTION_RESULT far *)_fmalloc((unsigned)(MAX_COLLECTION_RESULTS*sizeof(COLLECTION_RESULT)));
    if(!collection_results){notice_box("Search Files","Not enough memory for search results.");return 0;}
  }
  collection_result_count=0;explore_count=0;memset(openfile_mark,0,sizeof(openfile_mark));openfile_result_mode=1;
  {int dx=(screen_cols-78)/2,dy=(screen_rows-21)/2,row;render_begin();for(row=0;row<EXPLORE_ROWS;row++)textout(dx+21,dy+6+row,"",C_MENU_BACKGROUND,52);textout(dx+21,dy+6,"Searching...",C_BORDER,12);render_end();}
  openfile_search_dir(look,pattern,exts,recurse,deep,0);
  for(i=0;i<collection_result_count&&i<MAX_EXPLORE_ENTRIES;i++){
    openfile_far_to_near(explore_entries[i].name,collection_results[i].name,13);explore_entries[i].directory=0;
    explore_count++;
  }
  openfile_result_mode=1;return 1;
}

static int collections_dialog(void)
{
  int x=(screen_cols-78)/2,y=(screen_rows-21)/2,k,mx=0,my=0,selc=0,ctop=0,selected=-1,top=0,page=EXPLORE_ROWS*3;
  int i,row,column,index,drive,focus=0,pane=0,redraw=1,last_click=-1,hover_control=-1,hover_entry=-1;unsigned mb;unsigned long last_click_tick=0,tick;static char path[MAX_CMD]="C:\\";char cmd[MAX_CMD];
  const int rx=21,rw=55;
  openfile_result_mode=0;collection_result_count=0;memset(openfile_mark,0,sizeof(openfile_mark));
  collections_load();explore_detect_drives();strcpy(path,"C:\\");
  if(collection_count){openfile_load(path,collections[0].ext);selected=explore_count?0:-1;}else explore_count=0;
  for(;;){
    page=openfile_result_mode?EXPLORE_ROWS:EXPLORE_ROWS*3;
    if(redraw){render_begin();if(redraw==1){dialog_box(x,y,78,21,"Launch! File Open");textout(x+3,y,"Launch!",ATTR(appearance.titlebar_bg,appearance.main_title),7);}
      draw_button(x+2,y+2,"  Add  ",8,focus==10||hover_control==10);
      /* File Types is a 13-row scrolling pane.  The selected type carries a
         right-pointing marker in the final label cell to associate it with
         the browser pane. */
      for(i=0;i<13;i++){
        int ci=ctop+i,at=C_ITEM;
        textout(x+2,y+4+i,"",C_MENU_BACKGROUND,16);
        if(ci<collection_count){
          at=(ci==selc)?(pane==0&&focus==0?C_SELECTED:C_TITLE):C_ITEM;
          textout(x+2,y+4+i,collections[ci].name,at,15);
          if(ci==selc)cell(x+17,y+4+i,16,at);
        }
      }
      if(!collection_count){textout(x+2,y+4,"No file",C_EMPTY,16);textout(x+2,y+5,"associations",C_EMPTY,16);textout(x+2,y+6,"created.",C_EMPTY,16);}
      if(collection_count>13){cell(x+18,y+4,ctop>0?30:' ',C_BUTTON);cell(x+18,y+16,ctop+13<collection_count?31:' ',C_BUTTON);}
      else for(i=4;i<=16;i++)cell(x+18,y+i,' ',C_MENU_BACKGROUND);
      /* Continuous pane separator.  The browser rules stop before it rather
         than joining to it. */
      for(i=2;i<17;i++)cell(x+19,y+i,179,C_BORDER);
      if(collection_count){
        if(openfile_result_mode&&selected>=0&&selected<explore_count){
          char sf[MAX_CMD];openfile_entry_fullpath(selected,path,sf);openfile_path_field(x+rx,y+2,sf,rw);
        }else openfile_selection_field(x+rx,y+2,path,selected,rw);
        for(i=x+rx;i<77;i++)cell(i,y+3,196,C_BORDER);
        openfile_drive_bar(x+rx,y+4,focus>=5?focus-5:-1,rw);for(i=x+rx;i<77;i++)cell(i,y+5,196,C_BORDER);
        toolbar_divider(x,y+17,78);
        if(openfile_result_mode){
          for(row=0;row<EXPLORE_ROWS;row++){
            index=top+row;
            if(index<explore_count){
              int marked=openfile_mark[index]!=0;
              int active=(index==selected&&pane==1&&focus==0);
              int fat=(marked||active)?C_SELECTED:C_ITEM;
              char rp[COLLECTION_PATH],rn[13],shown[51];int avail,plen;
              textout(x+rx,y+6+row,"",C_MENU_BACKGROUND,52);
              openfile_far_to_near(rn,collection_results[index].name,13);
              openfile_far_to_near(rp,collection_results[index].path,COLLECTION_PATH);
              cell(x+rx,y+6+row,launchui_browser_codes[0],fat);
              cell(x+rx+1,y+6+row,launchui_browser_codes[1],fat);
              textout(x+rx+2,y+6+row,rn,fat,12);
              avail=36;plen=(int)strlen(rp);shown[0]=0;if(plen<=avail){strncpy(shown,rp,avail);shown[plen]=0;}else if(avail>3){strncpy(shown,rp,avail-3);shown[avail-3]=0;strcat(shown,"...");}shown[avail]=0;
              textout(x+rx+16,y+6+row,shown,marked||active?C_SELECTED:ATTR(appearance.background,appearance.labels),36);
              if(openfile_mark[index])cell(x+rx+1,y+6+row,251,fat);
            }else textout(x+rx,y+6+row,"",C_MENU_BACKGROUND,52);
          }
        }else{
          for(column=0;column<3;column++)for(row=0;row<EXPLORE_ROWS;row++){
            index=top+column*EXPLORE_ROWS+row;
            if(index<explore_count){
              int at=openfile_mark[index]?C_SELECTED:((index==selected&&pane==1&&focus==0)?C_SELECTED:explore_entry_attribute(index,0));
              draw_browser_entry(x+rx+column*18,y+6+row,index,at,16,0);
              if(openfile_mark[index])cell(x+rx+column*18+15,y+6+row,251,at);
            }else textout(x+rx+column*18,y+6+row,"",C_MENU_BACKGROUND,16);
          }
        }
        cell(x+75,y+6,top>0?30:' ',C_BUTTON);
        cell(x+75,y+16,top+page<explore_count?31:' ',C_BUTTON);
        if(!explore_count){textout(x+rx,y+6,"",C_MENU_BACKGROUND,52);textout(x+rx,y+6,"No matching files found.",C_BORDER,24);}
      } else {
        textout(x+rx,y+4,"Select a file association to browse...",C_EMPTY,rw);
      }
      {int file_ok=collection_count&&selected>=0&&selected<explore_count&&!explore_entries[selected].directory;if(collection_count)draw_button(x+3,y+18," Search ",8,focus==11||hover_control==11);else draw_button_disabled(x+3,y+18," Search ",8);cell(x+12,y+18,179,C_BORDER);if(file_ok){draw_button(x+14,y+18,"  Open  ",9,focus==1||hover_control==1);draw_button(x+25,y+18,"  Params  ",10,focus==2||hover_control==2);draw_button(x+37,y+18,"  Locate  ",10,focus==3||hover_control==3);}else{draw_button_disabled(x+14,y+18,"  Open  ",9);draw_button_disabled(x+25,y+18,"  Params  ",10);draw_button_disabled(x+37,y+18,"  Locate  ",10);}draw_button(x+68,y+18,"  Exit  ",9,focus==4||hover_control==4);}render_end();redraw=0;}
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      int h=-1,next_hover=-1;
      if(collection_count&&my>=y+6&&my<y+17&&mx>=x+rx&&mx<x+75){
        if(openfile_result_mode){row=my-(y+6);index=(row>=0)?top+row:-1;}
        else {column=(mx-(x+rx))/18;row=my-(y+6);index=top+column*EXPLORE_ROWS+row;}
        if(index>=0&&index<explore_count&&!openfile_mark[index])next_hover=-1;
      }
      if(my==y+2&&mx>=x+2&&mx<x+10)h=10;
      else if(collection_count&&my==y+18&&mx>=x+3&&mx<x+11)h=11;
      else if(my==y+18&&mx>=x+14&&mx<x+23)h=1;
      else if(my==y+18&&mx>=x+25&&mx<x+35)h=2;
      else if(my==y+18&&mx>=x+37&&mx<x+47)h=3;
      else if(my==y+18&&mx>=x+68&&mx<x+77)h=4;
      if(next_hover!=hover_entry){
        int old_hover=hover_entry;
        if(openfile_result_mode){hover_entry=next_hover;redraw=1;continue;}
        wait_vertical_retrace();
        if(old_hover>=0){
          int rel=old_hover-top,rr=rel%EXPLORE_ROWS,cc=rel/EXPLORE_ROWS;
          if(rel>=0&&rel<page)draw_browser_entry(x+rx+cc*18,y+6+rr,old_hover,
            openfile_mark[old_hover]?C_SELECTED:explore_entry_attribute(old_hover,old_hover==selected&&pane==1&&focus==0),16,0);
          if(rel>=0&&rel<page&&openfile_mark[old_hover])cell(x+rx+cc*18+15,y+6+rr,251,C_SELECTED);
        }
        hover_entry=next_hover;
        if(hover_entry>=0){
          int rel=hover_entry-top,rr=rel%EXPLORE_ROWS,cc=rel/EXPLORE_ROWS;
          if(rel>=0&&rel<page)draw_browser_entry(x+rx+cc*18,y+6+rr,hover_entry,
            explore_entry_attribute(hover_entry,1),16,0);
        }
      }
      if(h!=hover_control){hover_control=h;redraw=1;}
      continue;
    }
    if(mb&2){
      if(my>=y+4&&my<y+17&&mx>=x+2&&mx<x+18){
        i=ctop+my-(y+4);
        if(i<collection_count){selc=i;pane=0;if(collection_edit(selc)){selc=collection_last_changed;if(selc<ctop)ctop=selc;if(selc>=ctop+13)ctop=selc-12;openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;}redraw=1;}
      }
      continue;
    }
    if(mb&1){
      if(my==y+2&&mx>=x+2&&mx<x+10){if(collection_create()){selc=collection_last_changed;ctop=(selc/13)*13;strcpy(path,"C:\\");openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;pane=0;}redraw=1;continue;}
      if(collection_count>12&&mx==x+18&&my>=y+5&&my<=y+16){pane=0;if(my<y+11&&ctop>0){ctop-=13;if(ctop<0)ctop=0;selc=ctop;}else if(my>=y+11&&ctop+13<collection_count){ctop+=13;if(ctop>collection_count-1)ctop=collection_count-1;selc=ctop;}openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;redraw=1;continue;}
      if(my>=y+4&&my<y+17&&mx>=x+2&&mx<x+18){i=ctop+my-(y+4);if(i<collection_count){pane=0;if(i!=selc){selc=i;openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;}}redraw=1;continue;}
      if(collection_count&&my==y+4&&mx>=x+rx&&mx<x+rx+rw){drive=explore_drive_at(mx);if(drive>=0){path[0]=(char)('A'+drive);strcpy(path+1,":\\");openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;pane=1;redraw=1;}continue;}
      if(collection_count&&my>=y+6&&my<y+17&&mx>=x+rx&&mx<=x+75){
        if(mx==x+75){
          pane=1;
          if(my<y+12&&top>0){top-=page;if(top<0)top=0;selected=top;}
          else if(my>=y+12&&top+page<explore_count){top+=page;selected=top;}
          redraw=1;continue;
        }
        if(openfile_result_mode){row=my-(y+6);index=(row>=0)?top+row:-1;}
        else {column=(mx-(x+rx))/18;row=my-(y+6);index=top+column*EXPLORE_ROWS+row;}
        if(index>=0&&index<explore_count){
          pane=1;focus=0;hover_entry=-1;
          tick=*(unsigned long far *)MAKE_FP(0x40,0x6C);
          if(!explore_entries[index].directory&&openfile_mark[index]){
            openfile_mark[index]=0;selected=index;last_click=-1;redraw=1;continue;
          }
          if(core_ctrl_down()&&!explore_entries[index].directory){
            if(selected>=0&&selected<explore_count&&selected!=index&&!explore_entries[selected].directory&&!openfile_mark[selected])openfile_mark[selected]=1;
            openfile_mark[index]=1;selected=index;last_click=-1;redraw=1;continue;
          }
          if(index==last_click && tick>=last_click_tick && tick-last_click_tick<=9UL){
            if(explore_entries[index].directory){
              if(!strcmp(explore_entries[index].name,".."))explore_parent(path);
              else{strcat(path,explore_entries[index].name);strcat(path,"\\");}
              openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;last_click=-1;
            } else if(openfile_command(path,index,selc,run_command)){
              collection_macro_direct=0;collection_run_launcher=collections[selc].launcher;return 1;
            }
          } else {
            selected=index;last_click=index;last_click_tick=tick;
          }
          redraw=2;
        }
        continue;
      }
      if(collection_count&&my==y+18&&mx>=x+3&&mx<x+11){focus=11;k=13;}else if(my==y+18&&mx>=x+14&&mx<x+23){focus=1;k=13;}else if(my==y+18&&mx>=x+25&&mx<x+35){focus=2;k=13;}else if(my==y+18&&mx>=x+37&&mx<x+47){focus=3;k=13;}else if(my==y+18&&mx>=x+68)return 0;else continue;
    }
    if(k==27)return 0;if(!collection_count){
      if(k==9){focus=(focus==10)?4:10;redraw=1;continue;}
      if(k==0x0F00){focus=(focus==4)?10:4;redraw=1;continue;}
      if((focus==10&&(k==13||k==' '))||k=='a'||k=='A'){if(collection_create()){selc=collection_last_changed;ctop=(selc/13)*13;openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;pane=0;focus=0;}redraw=1;continue;}
      if(focus==4&&(k==13||k==' '))return 0;
      continue;
    }
    if((k&0xFF)==1){ /* Ctrl+A: add association */
      if(collection_create()){selc=collection_last_changed;if(selc<ctop)ctop=selc;if(selc>=ctop+13)ctop=selc-12;openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;pane=0;focus=0;}redraw=1;continue;
    }
    if((k&0xFF)==5 && collection_count){ /* Ctrl+E: edit association */
      if(collection_edit(selc)){selc=collection_last_changed;if(selc<ctop)ctop=selc;if(selc>=ctop+13)ctop=selc-12;openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;}redraw=1;continue;
    }
    if((k&0xFF)==4 && collection_count){ /* Ctrl+D: delete association */
      char q[64];sprintf(q,"Remove association %s?",collections[selc].name);
      if(confirm_box("Remove Association",q)){memmove(&collections[selc],&collections[selc+1],(collection_count-selc-1)*sizeof(COLLECTION));collection_count--;if(selc>=collection_count)selc=collection_count-1;if(selc<0)selc=0;if(selc<ctop)ctop=selc;if(ctop>0&&ctop>=collection_count)ctop=((collection_count-1)/13)*13;collections_save();if(collection_count){openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;}else{explore_count=0;selected=-1;}top=0;}redraw=1;continue;
    }
    /* File Open is a two-pane browser. Tab/Shift-Tab changes pane; browser
       navigation otherwise follows Explore & Run, including column movement. */
    if(k==9||k==0x0F00){
      int back=(k==0x0F00),d;
      if(!back){
        if(focus==0&&pane==0){focus=10;}
        else if(focus==10){focus=0;pane=1;}
        else if(focus==0&&pane==1){d=explore_first_drive();if(d>=0)focus=d+5;else focus=1;}
        else if(focus>=5&&focus<10)focus=1;
        else if(focus>=1&&focus<3)focus++;
        else if(focus==3)focus=collection_count?11:4;
        else if(focus==11)focus=4;
        else {focus=0;pane=0;}
      } else {
        if(focus==0&&pane==0)focus=4;
        else if(focus==10){focus=0;pane=0;}
        else if(focus==0&&pane==1)focus=10;
        else if(focus>=5&&focus<10){focus=0;pane=1;}
        else if(focus==1){d=explore_first_drive();if(d>=0){int n=d;while(explore_adjacent_drive(n,1)!=n)n=explore_adjacent_drive(n,1);focus=n+5;}else{focus=0;pane=1;}}
        else if(focus==4)focus=collection_count?11:3;
        else if(focus==11)focus=3;
        else if(focus>1&&focus<=3)focus--;
        else {focus=0;pane=0;}
      }
      redraw=2;continue;
    }
    if(focus==10){
      if(k==13||k==' '){
        if(focus==10){if(collection_create()){selc=collection_last_changed;ctop=(selc/13)*13;strcpy(path,"C:\\");openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;pane=0;focus=0;}redraw=1;continue;}
      }
      continue;
    }
    if(focus==11){
      if(collection_count&&(k==13||k==' ')){if(openfile_search_dialog(collections[selc].name,collections[selc].ext,path)){selected=explore_count?0:-1;top=0;pane=1;focus=0;}redraw=1;}
      continue;
    }
    if(collection_count&&((k&0xFF)==19)){ /* Ctrl+S: Search */
      if(openfile_search_dialog(collections[selc].name,collections[selc].ext,path)){selected=explore_count?0:-1;top=0;pane=1;focus=0;}redraw=1;continue;
    }
    if(focus>=5&&focus<10){
      int oldd=focus-5,newd=oldd;
      if(k==0x4B00)newd=explore_adjacent_drive(oldd,-1);
      else if(k==0x4D00)newd=explore_adjacent_drive(oldd,1);
      else if(k==13||k==' '){path[0]=(char)('A'+oldd);strcpy(path+1,":\\");openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;pane=1;focus=0;redraw=1;continue;}
      if(newd!=oldd){focus=newd+5;redraw=2;}
      continue;
    }
    if(focus>=1&&focus<=4){
      if(k==13||k==' '){
        if(focus==4)return 0;
        if(focus==3){if(selected>=0&&!explore_entries[selected].directory){strcpy(run_command,path);collection_macro_direct=1;return 1;}redraw=1;continue;}
        if(focus==2){if(openfile_command(path,selected,selc,cmd)&&parameter_prompt_command(nodes[collections[selc].launcher].title,cmd)){collection_macro_direct=0;collection_run_launcher=collections[selc].launcher;return 1;}redraw=1;continue;}
        if(focus==1){if(selected>=0&&explore_entries[selected].directory){if(!strcmp(explore_entries[selected].name,".."))explore_parent(path);else{strcat(path,explore_entries[selected].name);strcat(path,"\\");}openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;focus=0;pane=1;redraw=1;continue;}if(openfile_command(path,selected,selc,run_command)){collection_macro_direct=0;collection_run_launcher=collections[selc].launcher;return 1;}redraw=1;continue;}
      }
      continue;
    }
    if(k==0x4800){
      if(pane==0){
        if(selc>0){selc--;if(selc<ctop)ctop=selc;openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;}
      } else if(selected>0){selected--;top=(selected/page)*page;}
      redraw=2;continue;
    }
    if(k==0x5000){
      if(pane==0){
        if(selc+1<collection_count){selc++;if(selc>=ctop+13)ctop=selc-12;openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;}
      } else if(selected+1<explore_count){selected++;top=(selected/page)*page;}
      redraw=2;continue;
    }
    if(pane==1 && k==0x4B00){
      if(openfile_result_mode){redraw=1;continue;}
      if(selected>=EXPLORE_ROWS)selected-=EXPLORE_ROWS;
      else if(!explore_is_root(path)){explore_parent(path);openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;}
      if(selected>=0)top=(selected/page)*page;redraw=1;continue;
    }
    if(pane==1 && k==0x4D00){if(!openfile_result_mode&&selected>=0&&selected+EXPLORE_ROWS<explore_count){selected+=EXPLORE_ROWS;top=(selected/page)*page;}redraw=2;continue;}
    if(pane==1 && k==0x4900){top=top>=page?top-page:0;selected=explore_count?top:-1;redraw=2;continue;}
    if(pane==1 && k==0x5100 && top+page<explore_count){top+=page;selected=top;redraw=2;continue;}
    if(pane==1 && k==8 && !explore_is_root(path)){explore_parent(path);openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;redraw=1;continue;}
    if(k=='l'||k=='L'){if(selected>=0&&!explore_entries[selected].directory){strcpy(run_command,path);collection_macro_direct=1;return 1;}notice_box("Locate","Select a file first.");redraw=1;continue;}
    if(k=='p'||k=='P'){if(openfile_command(path,selected,selc,cmd)&&parameter_prompt_command(nodes[collections[selc].launcher].title,cmd)){collection_macro_direct=0;collection_run_launcher=collections[selc].launcher;return 1;}redraw=1;continue;}
    if(k==' '&&pane==1&&focus==0&&selected>=0&&!explore_entries[selected].directory){openfile_mark[selected]=!openfile_mark[selected];redraw=1;continue;}
    if(k==13){if(selected>=0&&explore_entries[selected].directory){if(!strcmp(explore_entries[selected].name,".."))explore_parent(path);else{strcat(path,explore_entries[selected].name);strcat(path,"\\");}openfile_load(path,collections[selc].ext);selected=explore_count?0:-1;top=0;redraw=1;continue;}if(openfile_command(path,selected,selc,run_command)){collection_macro_direct=0;collection_run_launcher=collections[selc].launcher;return 1;}redraw=1;}
  }
}

/* --- end Collections --------------------------------------------------- */

#pragma code_seg()



static int menu_entry_attribute(int node,int selected)
{
  if(selected){
    if(node>=0 && nodes[node].separator)
      return ATTR(appearance.selected_bg,appearance.border);
    return C_SELECTED;
  }
  if(node<0)return C_ITEM;
  if(nodes[node].separator)return C_BORDER;
  return nodes[node].folder?C_FOLDER:C_ITEM;
}

static int menu_item_offset(int depth,int node,int index)
{
  if(depth)return 1+index;
  return 2+index+(node<0?1:0);
}

#ifndef __GNUC__
#pragma code_seg("SYSBAR_TEXT")
#endif
static unsigned sysbar_first_mcb(void){union REGS r;struct SREGS sr;memset(&r,0,sizeof(r));memset(&sr,0,sizeof(sr));r.h.ah=0x52;int86x(0x21,&r,&r,&sr);return *(unsigned short far *)MAKE_FP(sr.es,r.x.bx-2);}
static unsigned sysbar_largest_kb(void){unsigned seg=sysbar_first_mcb(),owner,size;unsigned char type;unsigned long run=0,best=0;int guard=0;if(!seg)return 0;while(guard++<256){type=*(unsigned char far*)MAKE_FP(seg,0);owner=*(unsigned short far*)MAKE_FP(seg,1);size=*(unsigned short far*)MAKE_FP(seg,3);if(owner==0||owner==_psp){run+=(unsigned long)size+1UL;if(run>best)best=run;}else run=0;if(type=='Z')break;if(type!='M')break;seg=(unsigned)(seg+size+1);}return(unsigned)(best/64UL);}
static unsigned sysbar_env_free_kb(void){unsigned parent=*(unsigned short far*)MAKE_FP(_psp,0x16),eseg,paras;unsigned long bytes,used=0;char far*p;if(!parent)parent=_psp;eseg=*(unsigned short far*)MAKE_FP(parent,0x2C);if(!eseg)return 0;paras=*(unsigned short far*)MAKE_FP(eseg-1,3);bytes=(unsigned long)paras*16UL;p=(char far*)MAKE_FP(eseg,0);while(used+1<bytes){if(p[used]==0&&p[used+1]==0){used+=2;break;}used++;}return bytes>used?(unsigned)((bytes-used+1023UL)/1024UL):0;}
static int sysbar_packet_driver(void){unsigned v,off,seg,i;static const char sig[]="PKT DRVR";for(v=0x60;v<=0x80;v++){off=*(unsigned short far*)MAKE_FP(0,v*4);seg=*(unsigned short far*)MAKE_FP(0,v*4+2);if(!seg)continue;for(i=0;i<8;i++)if(*(unsigned char far*)MAKE_FP(seg,off+3+i)!=(unsigned char)sig[i])break;if(i==8)return 1;}return 0;}
static void sysbar_drive_free(char*out){char*cs=getenv("COMSPEC");unsigned drive;union REGS r;unsigned long bytes;if(cs&&cs[0]&&cs[1]==':')drive=(unsigned)(toupper((unsigned char)cs[0])-'A'+1);else{memset(&r,0,sizeof(r));r.h.ah=0x19;int86(0x21,&r,&r);drive=(unsigned)r.h.al+1;}memset(&r,0,sizeof(r));r.h.ah=0x36;r.h.dl=(unsigned char)drive;int86(0x21,&r,&r);if(r.x.ax==0xFFFF){sprintf(out,"%c: ?",(char)('A'+drive-1));return;}bytes=(unsigned long)r.x.ax*(unsigned long)r.x.bx*(unsigned long)r.x.cx;if(bytes>=1024UL*1024UL*1024UL)sprintf(out,"%c: %luGB",(char)('A'+drive-1),(bytes+(512UL*1024UL*1024UL))/(1024UL*1024UL*1024UL));else if(bytes>=1024UL*1024UL)sprintf(out,"%c: %luMB",(char)('A'+drive-1),(bytes+512UL*1024UL)/(1024UL*1024UL));else sprintf(out,"%c: %luKB",(char)('A'+drive-1),(bytes+512UL)/1024UL);}
#ifndef __GNUC__
#pragma code_seg()
#endif

static void draw_sysbar(void){char s[80],cc[8]="INT",video[4],disk[16];unsigned char country_info[34];union REGS r;unsigned mem,env,locks,ctry=0;int net,w,x,base=ATTR(appearance.controls_bg,appearance.controls_fg),dim=ATTR(appearance.controls_bg,(appearance.controls_fg&7)|8),active=ATTR(appearance.controls_bg,appearance.launchers);mem=sysbar_largest_kb();env=sysbar_env_free_kb();sysbar_drive_free(disk);net=sysbar_packet_driver();locks=*(unsigned char far *)MAKE_FP(0x40,0x17);memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);strcpy(video,r.h.al==0x1A?"VGA":"EGA");memset(country_info,0,sizeof(country_info));memset(&r,0,sizeof(r));r.h.ah=0x38;r.h.al=0;r.x.dx=(unsigned)country_info;int86(0x21,&r,&r);ctry=r.x.bx;if(ctry==61)strcpy(cc,"AUS");else if(ctry==1)strcpy(cc,"USA");else if(ctry==44)strcpy(cc,"GBR");sprintf(s,"  MEM %uKB | ENV %uKB | %s | CL NL SL | MOUSE %s NET | %u-%s  ",mem,env,disk,video,ctry,cc);w=(int)strlen(s);if(w>screen_cols)w=screen_cols;x=screen_cols-w;textout(x,0,s,base,w);{char*p;p=strstr(s,"|");while(p){cell(x+(int)(p-s),0,179,ATTR(appearance.controls_bg,0));p=strstr(p+1,"|");}p=strstr(s,"CL");if(p)textout(x+(int)(p-s),0,"CL",(locks&0x40)?active:dim,2);p=strstr(s,"NL");if(p)textout(x+(int)(p-s),0,"NL",(locks&0x20)?active:dim,2);p=strstr(s,"SL");if(p)textout(x+(int)(p-s),0,"SL",(locks&0x10)?active:dim,2);p=strstr(s,"MOUSE");if(p)textout(x+(int)(p-s),0,"MOUSE",mouse_present?active:dim,5);p=strstr(s,video);if(p)textout(x+(int)(p-s),0,video,active,3);p=strstr(s,"NET");if(p)textout(x+(int)(p-s),0,"NET",net?active:dim,3);}}

static int menu_position_at(int depth,int *list,int n,int panel_y,int mouse_y)
{
  int i;for(i=0;i<n;i++)if(mouse_y==panel_y+menu_item_offset(depth,list[i],i))return i;
  return -1;
}

static void change_menu_selection(int depth,int *list,int n,int *selection,
                                  int direction,int panel_y)
{
  int old=*selection,next,x=depth*MENU_WIDTH;
  if(!n)return;
  next=(old+n+direction)%n;if(next==old)return;
  if(x+MENU_WIDTH>screen_cols)x=screen_cols-MENU_WIDTH;
  wait_vertical_retrace();
  row_attribute(x+1,panel_y+menu_item_offset(depth,list[old],old),18,menu_entry_attribute(list[old],0));
  *selection=next;
  row_attribute(x+1,panel_y+menu_item_offset(depth,list[next],next),18,menu_entry_attribute(list[next],1));
}

static void set_menu_selection(int depth,int *list,int *selection,
                               int next,int panel_y)
{
  int old=*selection,x=depth*MENU_WIDTH;
  if(next==old)return;
  if(x+MENU_WIDTH>screen_cols)x=screen_cols-MENU_WIDTH;
  wait_vertical_retrace();
  row_attribute(x+1,panel_y+menu_item_offset(depth,list[old],old),18,menu_entry_attribute(list[old],0));
  *selection=next;
  row_attribute(x+1,panel_y+menu_item_offset(depth,list[next],next),18,menu_entry_attribute(list[next],1));
}

static int menu_open_folder(const char *name,int *parent,int *sel)
{
  int target=-1,chain[MAX_DEPTH],count=0,i,j,n,list[MAX_CHILD],p;
  if(!name||!*name)return 0;
  for(i=0;i<node_count;i++)if(nodes[i].active&&nodes[i].folder&&
     !stricmp(nodes[i].title,name)){target=i;break;}
  if(target<0)return 0;
  p=target;while(p>=0&&count<MAX_DEPTH){chain[count++]=p;p=nodes[p].parent;}
  if(p>=0)return 0;
  parent[0]=-1;sel[0]=0;
  for(i=count-1,j=0;i>=0&&j<MAX_DEPTH-1;i--,j++){
    n=menu_children(parent[j],list);sel[j]=0;
    for(p=0;p<n;p++)if(list[p]==chain[i]){sel[j]=p;break;}
    parent[j+1]=chain[i];sel[j+1]=0;
  }
  return count<MAX_DEPTH?count:MAX_DEPTH-1;
}

static int quick_title_match(const char *title,const char *find)
{
  int i,j,n;
  if(!title||!find)return 0;
  n=strlen(find);if(n<2)return 0;
  for(i=0;title[i];i++){
    for(j=0;j<n;j++){
      if(!title[i+j])break;
      if(toupper((unsigned char)title[i+j])!=toupper((unsigned char)find[j]))break;
    }
    if(j==n)return 1;
  }
  return 0;
}

static int quick_launchers(const char *find,int *list)
{
  int i,j,t,n=0;
  if(!find||strlen(find)<2)return 0;
  for(i=0;i<node_count && n<MAX_CHILD;i++)
    if(nodes[i].active&&!nodes[i].folder&&!nodes[i].separator&&quick_title_match(nodes[i].title,find))list[n++]=i;
  for(i=0;i<n-1;i++)for(j=i+1;j<n;j++)
    if(stricmp(nodes[list[j]].title,nodes[list[i]].title)<0){t=list[i];list[i]=list[j];list[j]=t;}
  return n;
}

static int menu(const char *open_to)
{
  static int parent[MAX_DEPTH],sel[MAX_DEPTH],list[MAX_CHILD];
  static int panel_y[MAX_DEPTH],panel_h[MAX_DEPTH];
  static int draw_list[MAX_CHILD],hitlist[MAX_CHILD];
  union REGS clock_regs;
  int depth=0,n,k,i,h,x,y,node,redraw=2,mx=0,my=0,hit,pos;
  int quick=0,qfocus=1,qn=0,qsel=0,qlist[MAX_CHILD],qh,qy,qinput_y;
  char qtext[MAX_TITLE];
  unsigned mb,last_mouse_x,last_mouse_y;unsigned char last_second=255;
  unsigned long last_activity,now;
  parent[0]=-1; sel[0]=0;qtext[0]=0;depth=menu_open_folder(open_to,parent,sel);
  video_init();if(!save_screen()){puts("Launch!: insufficient memory");return -1;}
  cursor_hide();mouse_present=mouse_start();mouse_stop();
  last_mouse_x=mouse_raw_x;last_mouse_y=mouse_raw_y;last_activity=bios_ticks();
  for(;;){
    if(quick){
      qn=quick_launchers(qtext,qlist);if(qsel>=qn)qsel=qn?qn-1:0;
      qh=(qn?qn:1)+5;if(qh>screen_rows)qh=screen_rows;
      qy=appearance.menu_top?0:screen_rows-qh;qinput_y=qy+qh-2;
      if(redraw){
        /* Quick Launch can shrink as the result count changes.  Always restore
           the saved DOS screen before redrawing it so pixels from a taller
           previous Quick Launch/menu panel cannot remain exposed behind the
           new, shorter panel. */
        restore_screen();
        if(appearance.show_sysbar)draw_sysbar();
        menu_box(0,qy,MENU_WIDTH,qh,"Launch!",C_ROOT_TITLE,1);
        if(qn){
          int first=0,visible=qh-5;
          if(!qfocus&&qsel>=visible)first=qsel-visible+1;
          for(i=0;i<visible&&first+i<qn;i++){
            node=qlist[first+i];
            textout(1,qy+2+i,nodes[node].title,(!qfocus&&first+i==qsel)?C_SELECTED:C_ITEM,18);
          }
        } else textout(1,qy+2,strlen(qtext)>=2?"No matches":"Type to search",C_EMPTY,18);
        for(i=1;i<MENU_WIDTH-1;i++)cell(i,qinput_y-1,196,C_BORDER);
        for(i=1;i<MENU_WIDTH-1;i++)cell(i,qinput_y,' ',C_INPUT_FIELD);
        textout(1,qinput_y,qtext,C_INPUT_FIELD,MENU_WIDTH-2);
        if(qfocus){int cx=1+strlen(qtext);if(cx>MENU_WIDTH-2)cx=MENU_WIDTH-2;cell(cx,qinput_y,'_',C_INPUT_FIELD);}
        redraw=0;
      }
      k=keyread();cursor_hide();last_activity=bios_ticks();
      if(k==27){quick=0;depth=0;parent[0]=-1;sel[0]=0;redraw=2;continue;}
      if(qfocus){
        if(k==0x4800&&qn){qfocus=0;qsel=qn-1;redraw=1;continue;}
        if(k==0x5000&&qn){qfocus=0;qsel=0;redraw=1;continue;}
        if(k==8){int l=strlen(qtext);if(l){qtext[l-1]=0;qsel=0;redraw=1;}continue;}
        if(k>=32&&k<127){int l=strlen(qtext);if(l<MAX_TITLE-1){qtext[l]=(char)k;qtext[l+1]=0;qsel=0;redraw=1;}continue;}
        continue;
      }
      if(k=='\\'){qfocus=1;redraw=1;continue;}
      if(k==0x4800&&qn){qsel=(qsel+qn-1)%qn;redraw=1;continue;}
      if(k==0x5000&&qn){qsel=(qsel+1)%qn;redraw=1;continue;}
      if((key_shift&0x04)&&key_scan==0x12&&qn){node=qlist[qsel];if(edit_dialog(node)&&!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");redraw=2;continue;}
      if((key_shift&0x04)&&key_scan==0x20&&qn){
        char question[64];node=qlist[qsel];sprintf(question,"Remove %s from the menu?",nodes[node].title);
        strncpy(remove_item_name,nodes[node].title,MAX_TITLE-1);remove_item_name[MAX_TITLE-1]=0;remove_item_colour=C_ITEM;
        if(confirm_box("Remove Item",question)){int pp=nodes[node].parent;delete_tree(node);normalize_order(pp);if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");}
        remove_item_name[0]=0;qsel=0;redraw=2;continue;
      }
      if(k==13&&qn){node=qlist[qsel];if(prepare_launcher(node)){run_node=node;close_menu();return node;}redraw=2;continue;}
      continue;
    }
    n=menu_children(parent[depth],list);
    if(sel[depth]>=n) sel[depth]=n ? n-1 : 0;
    if(redraw){
      if(redraw==2) restore_screen();
      if(appearance.show_sysbar)draw_sysbar();
      for(i=0;i<=depth;i++){
        int tn,j;
        tn=menu_children(parent[i],draw_list);h=tn?tn+2:3;
        if(i==0){h+=2;if(appearance.show_collections||appearance.show_explore||appearance.show_power)h++;}
        if(h>screen_rows) h=screen_rows;
        if(i==0) y=appearance.menu_top?0:screen_rows-h;
        else {
          int pn=menu_children(parent[i-1],hitlist);
          y=panel_y[i-1]+menu_item_offset(i-1,pn?hitlist[sel[i-1]]:0,sel[i-1])-1;
          if(y+h>screen_rows) y=screen_rows-h;
          if(y<0) y=0;
        }
        panel_y[i]=y;panel_h[i]=h;
        x=i*MENU_WIDTH;
        if(x+MENU_WIDTH>screen_cols) x=screen_cols-MENU_WIDTH;
        menu_box(x,y,MENU_WIDTH,h,(i==0)?"Launch!":0,
                 (i==0)?C_ROOT_TITLE:C_TITLE,i==0);
        for(j=0;j<tn && j<h-2;j++){
          int item_y;
          node=draw_list[j];
          item_y=y+menu_item_offset(i,node,j);
          if(i==0&&node<0&&(j==0||draw_list[j-1]>=0)){
            int sx;for(sx=1;sx<MENU_WIDTH-1;sx++)cell(x+sx,item_y-1,196,C_BORDER);
          }
          if(node==BUILTIN_COLLECTIONS){
            textout(x+1,item_y,"File Open",(j==sel[i])?C_SELECTED:C_ITEM,18);
          } else if(node==BUILTIN_EXPLORE){
	    textout(x+1,item_y,"Explore & Run",(j==sel[i])?C_SELECTED:C_ITEM,18);
          } else if(node==BUILTIN_POWER){
	    textout(x+1,item_y,"Shutdown...",(j==sel[i])?C_SELECTED:C_ITEM,18);
          } else if(nodes[node].separator){
            int a=(j==sel[i])?ATTR(appearance.selected_bg,appearance.border):C_BORDER;
            int sx;for(sx=0;sx<18;sx++)cell(x+1+sx,item_y,196,a);
          } else if(nodes[node].folder){
            textout(x+1,item_y,nodes[node].title,(j==sel[i])?C_SELECTED:C_FOLDER,16);
            cell(x+17,item_y,' ',(j==sel[i])?C_SELECTED:C_FOLDER);
            cell(x+18,item_y,16,(j==sel[i])?C_SELECTED:C_FOLDER);
          } else textout(x+1,item_y,nodes[node].title,(j==sel[i])?C_SELECTED:C_ITEM,18);
        }
        if(!tn)textout(x+1,y+(i==0?2:1),"Empty",C_EMPTY,18);
      }
      if(appearance.show_time)last_second=draw_clock(panel_y[0]+panel_h[0]-1,255);
      else last_second=255;
      last_activity=bios_ticks();
      redraw=0;
    }
    k=0;mb=0;mouse_show();
    do {
      if(key_waiting()){k=keyread();break;}
      mb=mouse_poll(&mx,&my);
      if(mouse_raw_x!=last_mouse_x || mouse_raw_y!=last_mouse_y){
        last_mouse_x=mouse_raw_x;last_mouse_y=mouse_raw_y;last_activity=bios_ticks();
        hit=-1;pos=-1;
        for(i=depth;i>=0;i--){
          x=i*MENU_WIDTH;if(x+MENU_WIDTH>screen_cols)x=screen_cols-MENU_WIDTH;
          if(mx>=x && mx<x+MENU_WIDTH && my>panel_y[i] &&
             my<panel_y[i]+panel_h[i]){
            int hn=menu_children(parent[i],hitlist);
            pos=menu_position_at(i,hitlist,hn,panel_y[i],my);
            if(pos>=0){hit=i;break;}
          }
        }
        if(hit>=0 && pos>=0){
          int hn=menu_children(parent[hit],hitlist);
          if(pos<hn && pos!=sel[hit]){
            if(hit<depth){depth=hit;sel[hit]=pos;redraw=2;break;}
            mouse_stop();set_menu_selection(hit,hitlist,&sel[hit],pos,panel_y[hit]);
            mouse_show();
          }
        }
      }
      if(appearance.show_time){
        memset(&clock_regs,0,sizeof(clock_regs));clock_regs.h.ah=0x2C;
        int86(0x21,&clock_regs,&clock_regs);
        if(clock_regs.h.dh!=last_second){
          mouse_stop();
          last_second=draw_clock(panel_y[0]+panel_h[0]-1,last_second);
          mouse_show();
        }
      }
      now=bios_ticks();
      if(appearance.screensaver &&
         elapsed_ticks(last_activity,now)>=saver_delay_ticks()){
        run_screensaver();
        last_mouse_x=mouse_raw_x;last_mouse_y=mouse_raw_y;
        last_activity=bios_ticks();redraw=2;break;
      }
    } while(!mb);
    mouse_stop();

    if(k || mb)last_activity=bios_ticks();
    if(!k && !mb && redraw==2)continue;

    if(mb){
      hit=-1;pos=-1;
      for(i=depth;i>=0;i--){
        x=i*MENU_WIDTH;if(x+MENU_WIDTH>screen_cols)x=screen_cols-MENU_WIDTH;
        if(mx>=x && mx<x+MENU_WIDTH && my>=panel_y[i] && my<panel_y[i]+panel_h[i]){
          int hn=menu_children(parent[i],hitlist);
          hit=i;pos=menu_position_at(i,hitlist,hn,panel_y[i],my);break;
        }
      }
      if((mb&2) && appearance.show_time && my==panel_y[0]+panel_h[0]-1 && mx>=8 && mx<=18){
        /* Right-clicking the clock behaves exactly like launching a command:
           close Launch!, restore the command prompt, then feed TIME + Enter
           through the resident keyboard-macro helper.  Do not execute a child
           shell here and do not redraw/reopen the menu afterward. */
        time_macro_pending=1;
        close_menu();
        return -1;
      }
      if((mb&2) && hit==0 && my==panel_y[0] && mx>=3 && mx<10){
        close_menu();return BUILTIN_CONFIG;
      }
      if((mb&1) && hit<0){close_menu();return -1;}
      if(hit>=0 && pos>=0){
        int hn=menu_children(parent[hit],hitlist);
        if(pos<hn){
          depth=hit;sel[hit]=pos;node=hitlist[pos];
          if(node==BUILTIN_COLLECTIONS && (mb&1)){
            if(collections_dialog()){run_node=BUILTIN_COLLECTIONS;close_menu();return 0;}
            redraw=2;continue;
          }
          if(node==BUILTIN_EXPLORE && (mb&1)){
            if(explore_dialog()){run_node=BUILTIN_EXPLORE;close_menu();return 0;}
            redraw=2;continue;
          }
          if(node==BUILTIN_POWER && (mb&1)){
            int action=power_dialog();
            if(action){
              close_menu();
              if(action==1){if(!power_off())safe_to_turn_off();}
              else cold_reboot();
              return -1;
            }
            redraw=2;continue;
          }
          if(mb&2){
            if(node>=0 && !nodes[node].separator && edit_dialog(node) && !save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
            redraw=2;continue;
          }
          if(mb&1){
            if(nodes[node].folder && depth<MAX_DEPTH-1){depth++;parent[depth]=node;sel[depth]=0;redraw=2;continue;}
            if(!nodes[node].folder && !nodes[node].separator){
              if(prepare_launcher(node)){run_node=node;close_menu();return node;}
              redraw=2;continue;
            }
          }
        }
      }
      continue;
    }

    if(k=='\\'){quick=1;qfocus=1;qtext[0]=0;qsel=0;depth=0;redraw=2;continue;}
    if(k==27){close_menu();return -1;}
    if((key_shift&0x04) && key_scan==0x1E){
      if(add_dialog(parent[depth],depth)){
        if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
        n=menu_children(parent[depth],list);for(i=0;i<n;i++)if(list[i]==added_visible_node)sel[depth]=i;
      }
      redraw=2;
    }
    else if((key_shift&0x04) && key_scan==0x20 && n>0){
      node=list[sel[depth]];
      if(node<0){redraw=1;continue;}
      {
        char question[64];
        remove_item_name[0]=0;
        if(nodes[node].separator)strcpy(question,"Remove this separator from the menu?");
        else {sprintf(question,"Remove %s from the menu?",nodes[node].title);strncpy(remove_item_name,nodes[node].title,MAX_TITLE-1);remove_item_name[MAX_TITLE-1]=0;remove_item_colour=nodes[node].folder?C_FOLDER:C_ITEM;}
        if(confirm_box("Remove Item",question)){
          delete_tree(node);normalize_order(parent[depth]);if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
        }
        remove_item_name[0]=0;
      }
      redraw=2;
    }
    else if((key_shift&0x04) && key_scan==0x12 && n>0){
      node=list[sel[depth]];
      if(node>=0 && !nodes[node].separator && edit_dialog(node) && !save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
      redraw=2;
    }
    else if((key_shift&0x04) && (key_scan==0x48 || key_scan==0x8D) && n>0){
      node=list[sel[depth]];
      if(node>=0){sel[depth]=move_item(parent[depth],sel[depth],-1);if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");}
      redraw=1;
    }
    else if((key_shift&0x04) && (key_scan==0x50 || key_scan==0x91) && n>0){
      node=list[sel[depth]];
      if(node>=0){sel[depth]=move_item(parent[depth],sel[depth],1);if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");}
      redraw=1;
    }
    else if((key_shift&0x04) && key_scan==0x1F){
      if(n>0){node=list[sel[depth]];sort_menu(parent[depth]);n=menu_children(parent[depth],list);for(i=0;i<n;i++)if(list[i]==node)sel[depth]=i;if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");}
      redraw=1;
    }
    else if(k==0x4800 && n>0)change_menu_selection(depth,list,n,&sel[depth],-1,panel_y[depth]);
    else if(k==0x5000 && n>0)change_menu_selection(depth,list,n,&sel[depth],1,panel_y[depth]);
    else if(k==0x4B00){if(depth){depth--;redraw=2;}}
    else if((k==13 || k==0x4D00) && n>0){
      node=list[sel[depth]];
      if(node==BUILTIN_COLLECTIONS && k==13){
        if(collections_dialog()){run_node=BUILTIN_COLLECTIONS;close_menu();return 0;}
        redraw=2;
      }
      else if(node==BUILTIN_EXPLORE && k==13){
        if(explore_dialog()){run_node=BUILTIN_EXPLORE;close_menu();return 0;}
        redraw=2;
      }
      else if(node==BUILTIN_POWER && k==13){
        int action=power_dialog();
        if(action){close_menu();if(action==1){if(!power_off())safe_to_turn_off();}else cold_reboot();return -1;}
        redraw=2;
      }
      else if(node<0){redraw=1;}
      else if(nodes[node].folder && depth<MAX_DEPTH-1){depth++;parent[depth]=node;sel[depth]=0;redraw=1;}
      else if(k==13 && !nodes[node].folder && !nodes[node].separator){
        if(prepare_launcher(node)){run_node=node;close_menu();return node;}
        redraw=2;
      }
    }
  }
}

static void macro_append(char *text,const char *part)
{
  strncat(text,part,MAX_MACRO-strlen(text)-1);
}

static void command_directory(const char *command,char *directory)
{
  const char *p=command,*end,*slash,*other;int n;
  while(*p==' ')p++;
  if(*p=='\"'){p++;end=strchr(p,'\"');if(!end)end=p+strlen(p);}
  else {end=p;while(*end && *end!=' ' && *end!='\t')end++;}
  slash=0;other=p;
  while(other<end){if(*other=='\\' || *other=='/')slash=other;other++;}
  if(!slash){directory[0]=0;return;}
  n=(int)(slash-p);
  if(n==2 && p[1]==':')n++;
  if(n>=MAX_CMD)n=MAX_CMD-1;
  strncpy(directory,p,n);directory[n]=0;
}

static int path_has_directory(const char *directory)
{
  const char *path=getenv("PATH"),*p,*q;int n;if(!path||!*directory)return 0;n=strlen(directory);
  for(p=path;*p;){while(*p==';'||*p==' ')p++;q=p;while(*q&&*q!=';')q++;
    while(q>p&&q[-1]==' ')q--;if((int)(q-p)==n&&!strnicmp(p,directory,n))return 1;
    p=*q?q+1:q;}return 0;
}

static void build_macro(int node,const char *command,char *text)
{
  char directory[MAX_CMD];text[0]=0;
  if(node>=0 && nodes[node].add_path){
    command_directory(command,directory);
    if(*directory && !path_has_directory(directory)){macro_append(text,"SET PATH=%PATH%;");macro_append(text,directory);macro_append(text,"\r");}
  }
  if(node<0 || nodes[node].change_dir){
    command_directory(command,directory);
    if(*directory){
      if(directory[1]==':'){
        char drive[4];drive[0]=directory[0];drive[1]=':';drive[2]='\r';drive[3]=0;
        macro_append(text,drive);
      }
      if(!(directory[1]==':' && directory[2]==0)){
        macro_append(text,"CD ");macro_append(text,directory);macro_append(text,"\r");
      }
    }
  }
  macro_append(text,command);
  if(node<0 || nodes[node].press_enter)macro_append(text,"\r");
}

static void far_write_long(unsigned seg,unsigned off,unsigned long value)
{
  unsigned long far *p=(unsigned long far *)MAKE_FP(seg,off);*p=value;
}

static unsigned long dos_get_vector(unsigned char vector)
{
  union REGS r;struct SREGS s;
  segread(&s);
  r.h.ah=0x35;r.h.al=vector;
  int86x(0x21,&r,&r,&s);
  return ((unsigned long)s.es<<16)|r.x.bx;
}

static void dos_set_vector(unsigned char vector,unsigned seg,unsigned off)
{
  union REGS r;struct SREGS s;
  segread(&s);s.ds=seg;
  r.h.ah=0x25;r.h.al=vector;r.x.dx=off;
  int86x(0x21,&r,&r,&s);
}

#define FONT_OLD10_OFF       0
#define FONT_ACTIVE_ID_OFF   4
#define FONT_MARK_OFF        5
#define FONT_ACTIVE_FONT_OFF 0x000E
#define FONT_INT10_OFF       0x1064

static int font_is_vga(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);
  return r.h.al==0x1A && (r.h.bl==7 || r.h.bl==8 || r.h.bh==7 || r.h.bh==8);
}

static int font_read(unsigned char id,unsigned segment)
{
  FILE *f;long offset;size_t got;unsigned total=0;struct SREGS s;
  if(id==0)return 1;
  if(id>=BUILTIN_FONT_COUNT){char p[MAX_CMD];int ex=(int)id-BUILTIN_FONT_COUNT;if(ex<0||ex>=external_font_count)return 0;strcpy(p,program_dir);strcat(p,"APPDATA\\");strcat(p,external_font_file[ex]);f=fopen(p,"rb");offset=0;}
  else {f=fopen(font_file,"rb");offset=((long)id-1L)*4096L;}
  if(!f)return 0;
  if(fseek(f,offset,SEEK_SET)!=0){fclose(f);return 0;}
  segread(&s);
  while(total<4096){
    got=fread(copy_buffer,1,sizeof(copy_buffer),f);
    if(!got)break;
    movedata(s.ds,(unsigned)copy_buffer,segment,total,(unsigned)got);
    total+=(unsigned)got;
  }
  fclose(f);return total==4096;
}

static int font_resident(unsigned *resident)
{
  unsigned long vector=dos_get_vector(0x10);unsigned seg=(unsigned)(vector>>16);
  unsigned char far *p;
  if((unsigned)vector!=FONT_INT10_OFF)return 0;
  /* LAFONT22 is the stable resident ABI signature, not the release number. */
  p=(unsigned char far *)MAKE_FP(seg,FONT_MARK_OFF);
  if(p[0]!='L'||p[1]!='A'||p[2]!='F'||p[3]!='O'||p[4]!='N'||p[5]!='T'||
     p[6]!='2'||p[7]!='2')return 0;
  *resident=seg;return 1;
}

static int font_manager(unsigned *resident)
{
  union REGS r;unsigned seg,paragraphs,old_strategy;unsigned long old10;
  const unsigned char far *source=font_resident_blob;int alloc_error;
  if(font_resident(resident))return 1;
  paragraphs=(FONT_RESIDENT_SIZE+15)/16;
  r.x.ax=0x5800;int86(0x21,&r,&r);old_strategy=r.x.ax;
  if(old_strategy<=2){r.x.ax=0x5801;r.x.bx=2;int86(0x21,&r,&r);}
  alloc_error=_dos_allocmem(paragraphs,&seg);
  if(old_strategy<=2){r.x.ax=0x5801;r.x.bx=old_strategy;int86(0x21,&r,&r);}
  if(alloc_error!=0)return 0;
  movedata(FP_SEG(source),FP_OFF(source),seg,0,FONT_RESIDENT_SIZE);
  if(*(unsigned char far *)MAKE_FP(seg,FONT_MARK_OFF)!='L' ||
     *(unsigned char far *)MAKE_FP(seg,FONT_MARK_OFF+7)!='2'){
    _dos_freemem(seg);return 0;
  }
  old10=dos_get_vector(0x10);far_write_long(seg,FONT_OLD10_OFF,old10);
  *(unsigned far *)MAKE_FP(seg-1,1)=8;
  dos_set_vector(0x10,seg,FONT_INT10_OFF);*resident=seg;return 1;
}

static int font_unload(void)
{
  union REGS r;unsigned resident,psp;unsigned long old10;
  unsigned far *owner;
  if(!font_resident(&resident))return 1;
  old10=*(unsigned long far *)MAKE_FP(resident,FONT_OLD10_OFF);
  dos_set_vector(0x10,(unsigned)(old10>>16),(unsigned)old10);
  memset(&r,0,sizeof(r));r.h.ah=0x51;int86(0x21,&r,&r);psp=r.x.bx;
  owner=(unsigned far *)MAKE_FP(resident-1,1);*owner=psp;
  if(_dos_freemem(resident)==0)return 1;
  *owner=8;dos_set_vector(0x10,resident,FONT_INT10_OFF);return 0;
}

static void font_bios_load(unsigned font_segment,unsigned font_offset)
{
#ifndef __GNUC__
  _asm {
    push bp
    push es
    mov ax,font_segment
    mov es,ax
    mov bp,font_offset
    mov ax,1100h
    mov bx,1000h
    mov cx,256
    xor dx,dx
    int 10h
    pop es
    pop bp
  }
#else
  (void)font_segment;(void)font_offset;
#endif
}

static void font_bios_standard(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1104;r.x.bx=0;
  int86(0x10,&r,&r);
}

static int font_preview(unsigned char id)
{
  unsigned segment;
  if(!font_is_vga())return 1;
  if(!id){font_bios_standard();launchui_rebase();return 1;}
  if(_dos_allocmem(256,&segment)!=0)return 0;
  if(!font_read(id,segment)){_dos_freemem(segment);return 0;}
  font_bios_load(segment,0);_dos_freemem(segment);launchui_rebase();return 1;
}

static int font_commit(unsigned char id)
{
  unsigned resident,temporary;
  if(!font_is_vga())return 1;
  if(!id){
    if(!font_unload())return 0;
    font_bios_standard();launchui_rebase();return 1;
  }
  if(!appearance.font_persist){
    if(!font_unload())return 0;
    return font_preview(id);
  }
  if(!font_manager(&resident))return 0;
  if(_dos_allocmem(256,&temporary)!=0)return 0;
  if(!font_read(id,temporary)){_dos_freemem(temporary);return 0;}
  movedata(temporary,0,resident,FONT_ACTIVE_FONT_OFF,4096);
  _dos_freemem(temporary);
  *(unsigned char far *)MAKE_FP(resident,FONT_ACTIVE_ID_OFF)=id;
  font_bios_load(resident,FONT_ACTIVE_FONT_OFF);launchui_rebase();return 1;
}

static void font_restore(void)
{
  if(font_is_vga())font_preview(appearance.font_id);
}

static int helper_signature(unsigned seg)
{
  unsigned char far *p=(unsigned char far *)MAKE_FP(seg,0);
  unsigned char far *m=(unsigned char far *)MAKE_FP(seg-1,0);
  unsigned owner=*(unsigned short far *)(m+1);
  unsigned size=*(unsigned short far *)(m+3);
  return (m[0]=='M' || m[0]=='Z') && owner==8 &&
         size>=(MACRO_BLOB_SIZE+15)/16 &&
         p[0x00]==0xE8 && p[0x01]==0x05 && p[0x02]==0x00 &&
         p[MACRO_SIGNATURE_OFF+0]=='L' &&
         p[MACRO_SIGNATURE_OFF+1]=='H' &&
         p[MACRO_SIGNATURE_OFF+2]=='1' &&
         p[MACRO_SIGNATURE_OFF+3]=='1';
}

static unsigned find_resident_helper(void)
{
  unsigned seg,top;
  top=(*(unsigned short far *)MAKE_FP(0x40,0x13))*64U;
  if(top<0x60 || top>0xA000)top=0xA000;
  for(seg=0x60;seg<top;seg++)if(helper_signature(seg))return seg;
  return 0;
}

static int macro_helper(void)
{
  union REGS r;unsigned seg,paragraphs,vector_seg,vector_off;
  unsigned int16_seg,int16_off,old_strategy;int detected,direct,alloc_error;
  struct SREGS s;
  unsigned long old16,old2f;
  r.x.ax=0xD5A0;r.x.bx=0;int86(0x2F,&r,&r);
  detected=r.x.bx==0x4C48;
  old2f=dos_get_vector(0x2F);
  vector_seg=(unsigned)(old2f>>16);vector_off=(unsigned)old2f;
  direct=vector_off==MACRO_INT2F_OFF && helper_signature(vector_seg);
  seg=direct?vector_seg:find_resident_helper();
  if(seg){
    old16=dos_get_vector(0x16);
    int16_seg=(unsigned)(old16>>16);int16_off=(unsigned)old16;
    if(int16_seg!=seg || int16_off!=MACRO_INT16_OFF){
      far_write_long(seg,MACRO_OLD16_OFF,old16);
      dos_set_vector(0x16,seg,MACRO_INT16_OFF);
    }
    if(!direct && !detected){
      far_write_long(seg,MACRO_OLD2F_OFF,old2f);
      dos_set_vector(0x2F,seg,MACRO_INT2F_OFF);
    }
    return 1;
  }
  if(detected)return 1;
  paragraphs=(MACRO_BLOB_SIZE+15)/16;
  r.x.ax=0x5800;int86(0x21,&r,&r);old_strategy=r.x.ax;
  if(old_strategy<=2){r.x.ax=0x5801;r.x.bx=2;int86(0x21,&r,&r);}
  alloc_error=_dos_allocmem(paragraphs,&seg);
  if(old_strategy<=2){r.x.ax=0x5801;r.x.bx=old_strategy;int86(0x21,&r,&r);}
  if(alloc_error!=0)return 0;
  segread(&s);movedata(s.ds,(unsigned)macro_blob,seg,0,MACRO_BLOB_SIZE);
  old16=dos_get_vector(0x16);
  far_write_long(seg,MACRO_OLD16_OFF,old16);
  far_write_long(seg,MACRO_OLD2F_OFF,old2f);
  *(unsigned far *)MAKE_FP(seg-1,1)=8; /* DOS-owned: survive !.EXE */
  dos_set_vector(0x16,seg,MACRO_INT16_OFF);
  dos_set_vector(0x2F,seg,MACRO_INT2F_OFF);
  return 1;
}

static int queue_macro(const char *text)
{
  union REGS r;struct SREGS s;
  r.x.ax=0xD5B3;r.x.bx=0;int86(0x2F,&r,&r);
  if(r.x.bx==0x4D51 && r.x.cx>=MAX_MACRO){
    segread(&s);r.x.ax=0xD5B2;r.x.si=(unsigned)text;
    r.x.cx=(unsigned)strlen(text);
    int86x(0x2F,&r,&r,&s);return r.h.al==0;
  }
  if(!macro_helper())return 0;
  segread(&s);r.x.ax=0xD5A1;r.x.si=(unsigned)text;r.x.cx=(unsigned)strlen(text);
  int86x(0x2F,&r,&r,&s);return r.h.al==0;
}

#pragma code_seg("SHORTCUT_TEXT")
static unsigned char far *setkey_bios_byte(unsigned offset)
{
  return (unsigned char far *)(((unsigned long)0x40<<16)|offset);
}

static unsigned short far *setkey_bios_word(unsigned offset)
{
  return (unsigned short far *)(((unsigned long)0x40<<16)|offset);
}

static int setkey_ready(void)
{
  return *setkey_bios_word(0x1A)!=*setkey_bios_word(0x1C);
}

static unsigned setkey_read(unsigned char *shift)
{
  unsigned short far *head=setkey_bios_word(0x1A);unsigned pos,next,word;
  _disable();pos=*head;word=*setkey_bios_word(pos);
  next=pos+2;if(next>=0x3E)next=0x1E;*head=next;
  *shift=*setkey_bios_byte(0x17);_enable();return word;
}

static void setkey_label(unsigned scan,char *label)
{
  static const unsigned char letters[26]={
    0x1E,0x30,0x2E,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,
    0x31,0x18,0x19,0x10,0x13,0x1F,0x14,0x16,0x2F,0x11,0x2D,0x15,0x2C
  };
  static const char digits[]="1234567890";int i;
  for(i=0;i<26;i++)if(scan==letters[i]){label[0]=(char)('A'+i);label[1]=0;return;}
  if(scan>=2 && scan<=11){label[0]=digits[scan-2];label[1]=0;return;}
  if(scan>=0x3B && scan<=0x44){sprintf(label,"F%d",scan-0x3A);return;}
  if(scan==0x57){strcpy(label,"F11");return;}
  if(scan==0x58){strcpy(label,"F12");return;}
  switch(scan){
    case 0x0C:strcpy(label,"-");return;case 0x0D:strcpy(label,"=");return;
    case 0x1A:strcpy(label,"[");return;case 0x1B:strcpy(label,"]");return;
    case 0x27:strcpy(label,";");return;case 0x29:strcpy(label,"`");return;
    case 0x2B:strcpy(label,"\\");return;case 0x33:strcpy(label,",");return;
    case 0x34:strcpy(label,".");return;case 0x35:strcpy(label,"/");return;
    case 0x39:strcpy(label,"SPACE");return;case 0x48:strcpy(label,"UP");return;
    case 0x50:strcpy(label,"DOWN");return;case 0x4B:strcpy(label,"LEFT");return;
    case 0x4D:strcpy(label,"RIGHT");return;case 0x5B:strcpy(label,"LWIN");return;
    case 0x5C:strcpy(label,"RWIN");return;case 0x5D:strcpy(label,"MENU");return;
  }
  sprintf(label,"SCAN %02X",scan);
}

static void setkey_append_display(char *text,const char *name)
{
  if(*text)strcat(text," + ");
  strcat(text,"[");strcat(text,name);strcat(text,"]");
}

static void setkey_display(unsigned char shift,unsigned scan,char *text)
{
  char label[16];text[0]=0;
  if(shift&4)setkey_append_display(text,"CTRL");
  if(shift&8)setkey_append_display(text,"ALT");
  if(shift&3)setkey_append_display(text,"SHIFT");
  if(scan){setkey_label(scan,label);setkey_append_display(text,label);}
}

static void setkey_spec(unsigned char shift,unsigned scan,char *spec)
{
  spec[0]=0;
  if(shift&4)strcat(spec,"1D+");
  if(shift&8)strcat(spec,"38+");
  if(shift&3)strcat(spec,"2A+");
  if(scan==0x5B)strcat(spec,"LWIN");
  else if(scan==0x5C)strcat(spec,"RWIN");
  else if(scan==0x5D)strcat(spec,"MENU");
  else sprintf(spec+strlen(spec),"%02X",scan);
}

static void setkey_dialog_display(int x,int y,unsigned char shift,unsigned scan)
{
  char text[80];setkey_display(shift,scan,text);
  textout(x+4,y+4,"Detected:",C_INPUT_LABEL,10);
  textout(x+14,y+4,text,C_TITLE,36);
}

static int capture_setkey_dialog(char *spec)
{
  unsigned word,scan,buttons;unsigned char shift,captured_shift,last_shift=0xFF;
  int x=(screen_cols-54)/2,y=(screen_rows-10)/2,mx=0,my=0,focus=0,k;
  subdialog_box(x,y,54,10,"Set Keyboard Shortcut");
  textout(x+4,y+2,"Press the keys you want to use now",C_INPUT_LABEL,45);
  draw_button(x+14,y+7,"  Save  ",8,0);
  draw_button(x+32,y+7,"  Cancel  ",10,0);
  while(setkey_ready()){shift=0;setkey_read(&shift);}
  setkey_scan=setkey_e0=setkey_mods=setkey_key=setkey_key_mods=0;
  setkey_old_int09=_dos_getvect(0x09);_dos_setvect(0x09,setkey_int09);
  for(;;){
    shift=*setkey_bios_byte(0x17);
    if((shift&15)!=(last_shift&15)){
      mouse_stop();setkey_dialog_display(x,y,shift,0);mouse_show();last_shift=shift;
    }
    buttons=mouse_poll(&mx,&my);
    if((buttons&1)&&my==y+7&&mx>=x+32&&mx<x+42){
      _dos_setvect(0x09,setkey_old_int09);mouse_stop();
      press_button(x+32,y+7,"  Cancel  ",10);return 0;
    }
    if(setkey_key){
      _disable();scan=setkey_key;captured_shift=setkey_key_mods;
      setkey_key=0;_enable();_dos_setvect(0x09,setkey_old_int09);
      while(setkey_ready()){word=setkey_read(&shift);(void)word;}
      shift=captured_shift;if(scan==1)return 0;
    } else if(setkey_ready()){
      word=setkey_read(&shift);scan=word>>8;
      if(scan==1){_dos_setvect(0x09,setkey_old_int09);return 0;}
      if(!scan)continue;
      _dos_setvect(0x09,setkey_old_int09);
      if(scan>=0x85 && scan<=0x8C){
        if(scan>=0x87 && scan<=0x88)shift|=1;
        else if(scan>=0x89 && scan<=0x8A)shift|=4;
        else if(scan>=0x8B)shift|=8;
        scan=(scan&1)?0x57:0x58;
      }
    } else continue;
    mouse_stop();setkey_dialog_display(x,y,shift,scan);
    setkey_spec(shift,scan,spec);
    while((*setkey_bios_byte(0x17)&15)!=0) ;
    break;
  }
  for(;;){
    draw_button(x+14,y+7,"  Save  ",8,focus==0);
    draw_button(x+32,y+7,"  Cancel  ",10,focus==1);
    wait_input(&k,&mx,&my,&buttons);
    if(buttons&MOUSE_MOVED){
      if(my==y+7&&mx>=x+14&&mx<x+22)focus=0;
      else if(my==y+7&&mx>=x+32&&mx<x+42)focus=1;
      continue;
    }
    if((buttons&1)&&my==y+7&&mx>=x+14&&mx<x+22){
      press_button(x+14,y+7,"  Save  ",8);return 1;
    }
    if((buttons&1)&&my==y+7&&mx>=x+32&&mx<x+42){
      press_button(x+32,y+7,"  Cancel  ",10);return 0;
    }
    if(k==27)return 0;
    if(k==9||k==0x4B00||k==0x4D00)focus=!focus;
    else if(k==13){
      if(!focus){press_button(x+14,y+7,"  Save  ",8);return 1;}
      press_button(x+32,y+7,"  Cancel  ",10);return 0;
    }
  }
}

static char *setkey_stristr(char *text,const char *find)
{
  char *p;const char *a,*b;
  for(p=text;*p;p++){
    for(a=p,b=find;*a && *b && toupper((unsigned char)*a)==
        toupper((unsigned char)*b);a++,b++) ;
    if(!*b)return p;
  }
  return 0;
}

static int update_shortcut_key(const char *spec)
{
  static char autoexec[20],temp[20],old[20],line[256],output[256];
  char *comspec,*hit,*key,*end,*p;FILE *in,*out;int found=0,ok=1;
  comspec=getenv("COMSPEC");autoexec[0]=(comspec && comspec[1]==':')?
    (char)toupper((unsigned char)comspec[0]):'C';
  strcpy(autoexec+1,":\\AUTOEXEC.BAT");strcpy(temp,autoexec);
  strcpy(strrchr(temp,'.'),".$$$");strcpy(old,autoexec);strcpy(strrchr(old,'.'),".L!$");
  in=fopen(autoexec,"rt");if(!in)return 0;
  out=fopen(temp,"wt");if(!out){fclose(in);return 0;}
  while(fgets(line,sizeof(line),in)){
    strcpy(output,line);p=output;while(*p==' ' || *p=='\t')p++;
    hit=setkey_stristr(p,"!KEY.COM");
    if(!found && hit && strnicmp(p,"REM",3)!=0){
      key=setkey_stristr(hit,"/KEY=");
      if(key){end=key+5;while(*end && !isspace((unsigned char)*end))end++;
        memmove(key,end,strlen(end)+1);
      }
      if(!shortcut_loadhigh_supported()){char *lh=output;while(*lh==' '||*lh=='\t')lh++;if(!strnicmp(lh,"LOADHIGH",8)&&isspace((unsigned char)lh[8]))memmove(lh,lh+9,strlen(lh+9)+1);}
      end=output+strlen(output);while(end>output && (end[-1]=='\r'||end[-1]=='\n'))end--;
      *end=0;if(end>output && !isspace((unsigned char)end[-1]))strcat(output," ");
      strcat(output,"/KEY=");strcat(output,spec);strcat(output,"\n");found=1;
    }
    if(fputs(output,out)==EOF){ok=0;break;}
  }
  if(ferror(in))ok=0;
  if(fclose(in)!=0)ok=0;
  if(fclose(out)!=0)ok=0;
  if(!ok || !found){remove(temp);return found?-1:0;}
  remove(old);if(rename(autoexec,old)!=0){remove(temp);return -1;}
  if(rename(temp,autoexec)!=0){rename(old,autoexec);remove(temp);return -1;}
  remove(old);return 1;
}

static void shortcut_append(char *text,const char *part)
{
  if(*text)strcat(text,"+");
  strcat(text,part);
}

static void shortcut_format_spec(const char *spec,char *display)
{
  char copy[64],*token,*end,label[16];unsigned long scan;
  strncpy(copy,spec,sizeof(copy)-1);copy[sizeof(copy)-1]=0;display[0]=0;
  token=strtok(copy,"+");
  while(token){
    if(!stricmp(token,"CTRL")||!stricmp(token,"1D"))shortcut_append(display,"CTRL");
    else if(!stricmp(token,"ALT")||!stricmp(token,"38"))shortcut_append(display,"ALT");
    else if(!stricmp(token,"SHIFT")||!stricmp(token,"2A")||!stricmp(token,"36"))
      shortcut_append(display,"SHIFT");
    else if(!stricmp(token,"WIN")||!stricmp(token,"LWIN"))shortcut_append(display,"LWIN");
    else if(!stricmp(token,"RWIN"))shortcut_append(display,"RWIN");
    else if(!stricmp(token,"MENU"))shortcut_append(display,"MENU");
    else {
      scan=strtoul(token,&end,16);
      if(*token&&!*end&&scan<=255){setkey_label((unsigned)scan,label);shortcut_append(display,label);}
      else shortcut_append(display,token);
    }
    token=strtok(0,"+");
  }
  if(!*display)strcpy(display,"CTRL+ALT+.");
}

static void shortcut_refresh(void)
{
  union REGS r;FILE *f;char line[256],spec[64],*p,*hit,*key,*end,*comspec;
  char autoexec[20];
  memset(&r,0,sizeof(r));r.x.ax=0xD5B0;int86(0x2F,&r,&r);
  config_shortcut_active=r.x.bx==0x5343;
  strcpy(spec,"1D+38+34");
  comspec=getenv("COMSPEC");autoexec[0]=(comspec&&comspec[1]==':')?
    (char)toupper((unsigned char)comspec[0]):'C';
  strcpy(autoexec+1,":\\AUTOEXEC.BAT");f=fopen(autoexec,"rt");
  if(f){
    while(fgets(line,sizeof(line),f)){
      p=line;while(*p==' '||*p=='\t')p++;
      hit=setkey_stristr(p,"!KEY.COM");
      if(hit&&strnicmp(p,"REM",3)!=0){
        key=setkey_stristr(hit,"/KEY=");
        if(key){
          key+=5;end=key;while(*end&&!isspace((unsigned char)*end))end++;
          if(end-key<(int)sizeof(spec)){memcpy(spec,key,end-key);spec[end-key]=0;}
        }
        break;
      }
    }
    fclose(f);
  }
  shortcut_format_spec(spec,config_shortcut_combination);
}

static int shortcut_set_dialog(void)
{
  char spec[64];int result;
  if(!capture_setkey_dialog(spec))return 0;
  result=update_shortcut_key(spec);
  if(result==0){
    notice_box("Shortcut Error","No active !KEY.COM entry exists in AUTOEXEC.BAT.");
    return 0;
  }
  if(result<0){
    notice_box("Shortcut Error","AUTOEXEC.BAT could not be updated.");return 0;
  }
  return 1;
}


static int shortcut_loadhigh_supported(void)
{
  char bat[MAX_CMD],ok[MAX_CMD],cmd[MAX_CMD*2],*comspec;FILE *f;int found;
  comspec=getenv("COMSPEC");if(!comspec||!*comspec||!program_dir[0])return 0;
  strcpy(bat,program_dir);strcat(bat,"LHTEST.BAT");strcpy(ok,program_dir);strcat(ok,"LHTEST.$$$");
  remove(bat);remove(ok);f=fopen(bat,"wt");if(!f)return 0;
  fprintf(f,"@ECHO OFF\nECHO Y>%s\n",ok);if(fclose(f)!=0){remove(bat);return 0;}
  sprintf(cmd,"LOADHIGH %s /C %s >NUL",comspec,bat);system(cmd);remove(bat);
  f=fopen(ok,"rb");found=f!=0;if(f)fclose(f);remove(ok);return found;
}

static int ensure_shortcut_autoexec(void)
{
  char autoexec[20],line[256],*p,*comspec;FILE *f;long size;int last=0;
  comspec=getenv("COMSPEC");autoexec[0]=(comspec&&comspec[1]==':')?
    (char)toupper((unsigned char)comspec[0]):'C';
  strcpy(autoexec+1,":\\AUTOEXEC.BAT");f=fopen(autoexec,"rt");
  if(f){
    while(fgets(line,sizeof(line),f)){
      p=line;while(*p==' '||*p=='\t')p++;
      if(strnicmp(p,"REM",3)!=0&&setkey_stristr(p,"!KEY.COM")){
        fclose(f);return 1;
      }
    }
    fclose(f);
  }
  f=fopen(autoexec,"a+b");if(!f)return 0;
  fseek(f,0L,SEEK_END);size=ftell(f);
  if(size>0){fseek(f,-1L,SEEK_END);last=fgetc(f);fseek(f,0L,SEEK_END);}
  if(size>0&&last!='\n'&&fputs("\r\n",f)==EOF){fclose(f);return 0;}
  if(fprintf(f,"%s%s!KEY.COM\r\n",shortcut_loadhigh_supported()?"LOADHIGH ":"",program_dir)<0){fclose(f);return 0;}
  return fclose(f)==0;
}

static int shortcut_activate(void)
{
  if(!confirm_box("Activate Shortcut","Do you want to reboot to activate the shortcut?"))return 0;
  if(!ensure_shortcut_autoexec()){
    notice_box("Shortcut Error","AUTOEXEC.BAT could not be updated.");return 0;
  }
  cold_reboot();return 1;
}

static int shortcut_unload(void)
{
  char shortcut[MAX_CMD];int result,saved_out=-1,saved_err=-1;FILE *nullout;
  strcpy(shortcut,program_dir);strcat(shortcut,"!KEY.COM");
  mouse_stop();fflush(stdout);fflush(stderr);nullout=fopen("NUL","wt");
  if(nullout){
    saved_out=_dup(1);saved_err=_dup(2);
    if(saved_out>=0)_dup2(_fileno(nullout),1);
    if(saved_err>=0)_dup2(_fileno(nullout),2);
  }
  result=spawnl(P_WAIT,shortcut,shortcut,"/UNLOAD",NULL);
  fflush(stdout);fflush(stderr);
  if(saved_out>=0){_dup2(saved_out,1);_close(saved_out);}
  if(saved_err>=0){_dup2(saved_err,2);_close(saved_err);}
  if(nullout)fclose(nullout);
  if(result==-1){notice_box("Shortcut Error","!KEY.COM could not be started.");return 0;}
  shortcut_refresh();
  if(config_shortcut_active){notice_box("Shortcut Error","The resident shortcut could not be unloaded.");return 0;}
  return 1;
}

static void shortcut_idle_sync(void)
{
  union REGS r;unsigned long ticks=saver_delay_ticks();
  memset(&r,0,sizeof(r));r.x.ax=0xD5B0;int86(0x2F,&r,&r);
  if(r.x.bx!=0x5343)return;
  memset(&r,0,sizeof(r));r.x.ax=0xD5B4;
  r.h.bl=(unsigned char)(appearance.screensaver!=0);
  r.x.cx=(unsigned)(ticks&0xFFFFUL);r.x.dx=(unsigned)(ticks>>16);
  int86(0x2F,&r,&r);
}

#pragma code_seg()

static void show_help(void)
{
  puts("Launch! 3.73 - a lightweight command menu for DOS\n");
  puts("Usage: ! [menu.mnu] [/CONFIG | /EXPLORE | /OPEN | /BYE | /NOW | /OPENTO=folder | /?]\n");
  puts("Menu management shortcuts:");
  puts("  Ctrl+A        Add a folder, launcher or separator");
  puts("  Ctrl+D        Delete the selected item");
  puts("  Ctrl+E        Edit the selected item");
  puts("  Ctrl+Up/Down  Move the selected item");
  puts("  Ctrl+S        Sort the current menu\n");
  puts("Command-line parameters:");
  puts("  /CONFIG       Configure menu appearance and options");
  puts("  /EXPLORE      Open Explore & Run directly");
  puts("  /OPEN         Open the File Open dialog");
  puts("  /BYE          Open Shutdown... directly");
  puts("  /NOW          Start the selected screensaver immediately");
  puts("  menu.mnu      Use another menu file beside !.EXE (or a full path)");
  puts("  /OPENTO=name  Open directly to the first folder with this name");
  puts("  /?            Show this help");
}

int main(int argc,char **argv)
{
  int i,result,config_status,config_mode=0,explore_mode=0,open_mode=0,bye_mode=0,now_mode=0,initmenu_mode=0;
  static char macro[MAX_MACRO],open_to[MAX_TITLE];
  config_path(argv[0]);
  scan_external_fonts();
  if(!load_appearance())puts("Launch!: LAUNCH.CFG is invalid; using default appearance.");
  shortcut_idle_sync();
  for(i=1;i<argc;i++){
    if(!stricmp(argv[i],"/?") || !stricmp(argv[i],"-?")){show_help();return 0;}
    if(!stricmp(argv[i],"/CONFIG"))config_mode=1;
    else if(!stricmp(argv[i],"/EXPLORE"))explore_mode=1;
    else if(!stricmp(argv[i],"/OPEN"))open_mode=1;
    else if(!stricmp(argv[i],"/BYE"))bye_mode=1;
    else if(!stricmp(argv[i],"/NOW"))now_mode=1;
    else if(!stricmp(argv[i],"/INITMENU"))initmenu_mode=1;
    else if(i==1&&argv[i][0]!='/'&&argv[i][0]!='-'){
      if(!select_menu_file(argv[i])){puts("Launch!: invalid menu filename.");return 1;}
    }
    else if(!strnicmp(argv[i],"/OPENTO=",8)){
      strncpy(open_to,argv[i]+8,MAX_TITLE-1);open_to[MAX_TITLE-1]=0;
    }
    else {printf("Launch!: unknown option %s (use ! /?)\n",argv[i]);return 1;}
  }
  if(initmenu_mode){
    if(!default_menu_file(0)){puts("Launch!: could not create or update the default menu.");return 1;}
    return 0;
  }
  /* A missing primary menu always means rebuild the installed defaults, even
     when Launch! was invoked directly in Configuration/Open/Explore/etc. */
  if(!file_exists(config_file)){
    config_status=prepare_config();
    if(!config_status){printf("Launch!: cannot recover %s\n",config_file);return 1;}
    if(config_status==3)puts("Launch!: no menu file was found; rebuilt the default menu.");
  }
  if(!font_commit(appearance.font_id)){
    font_commit(0);puts("Launch!: FONT.DAT could not be read; using the standard VGA font.");
  }
  if(config_mode){
    result=configure_appearance();
    if(result==2 && prompt_macro_pending){
      build_macro(-1,run_command,macro);
      if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
    }
    return 0;
  }
  if(bye_mode){
    int action;
    video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 1;}
    cursor_hide();mouse_present=mouse_start();mouse_stop();
    action=power_dialog();
    close_menu();
    if(action==1){if(!power_off())safe_to_turn_off();}
    else if(action==2)cold_reboot();
    return 0;
  }
  if(open_mode){
    /* /OPEN needs the launcher tree just as much as File Open launched from
       the main menu does.  The normal menu path calls prepare_config() before
       entering File Open, but the direct /OPEN path used to bypass that load.
       As a result node_count stayed empty and Create Association > Choose had
       no launcher nodes to display.  Load/recover LAUNCH.MNU here before the
       File Open dialog is created. */
    config_status=prepare_config();
    if(!config_status){printf("Launch!: cannot recover %s\n",config_file);return 1;}
    if(config_status==2)puts("Launch!: LAUNCH.MNU was invalid; restored LAUNCH.BAK.");
    else if(config_status==3)puts("Launch!: no valid menu file was found; rebuilt the default menu.");
    video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 1;}
    cursor_hide();mouse_present=mouse_start();mouse_stop();
    result=collections_dialog();
    if(result)run_node=BUILTIN_COLLECTIONS;
    close_menu();
    if(result){
      if(collection_macro_direct){char d[4];macro[0]=0;d[0]=run_command[0];d[1]=':';d[2]='\r';d[3]=0;macro_append(macro,d);macro_append(macro,"CD ");macro_append(macro,run_command);macro_append(macro,"\r");}
      else build_macro(collection_run_launcher,run_command,macro);
      if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
    }
    return 0;
  }
  if(explore_mode){
    video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 1;}
    cursor_hide();mouse_present=mouse_start();mouse_stop();
    result=explore_dialog();
    if(result)run_node=BUILTIN_EXPLORE;
    close_menu();
    if(result){
      build_macro(run_node,run_command,macro);
      if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
    }
    return 0;
  }
  if(now_mode){
    video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 1;}
    cursor_hide();mouse_present=mouse_start();run_screensaver();close_menu();return 0;
  }
  config_status=prepare_config();
  if(!config_status){printf("Launch!: cannot recover %s\n",config_file);return 1;}
  if(config_status==2)puts("Launch!: LAUNCH.MNU was invalid; restored LAUNCH.BAK.");
  else if(config_status==3)puts("Launch!: no valid menu file was found; rebuilt the default menu.");
  do {
    result=menu(open_to);open_to[0]=0;
    if(result==BUILTIN_CONFIG){
      config_status=configure_appearance();
      if(config_status==2 && prompt_macro_pending){result=0;break;}
    }
  } while(result==BUILTIN_CONFIG);
  if(time_macro_pending){
    strcpy(macro,"TIME\r");
    if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
  } else if(prompt_macro_pending){
    build_macro(-1,run_command,macro);
    if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
  } else if(result>=0){
    if(run_node==BUILTIN_COLLECTIONS && collection_macro_direct){char d[4];macro[0]=0;d[0]=run_command[0];d[1]=':';d[2]='\r';d[3]=0;macro_append(macro,d);macro_append(macro,"CD ");macro_append(macro,run_command);macro_append(macro,"\r");}
    else build_macro(run_node==BUILTIN_COLLECTIONS?collection_run_launcher:run_node,run_command,macro);
    if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
  }
  return 0;
}
