/* Launch! 2.7 - modal command menu for DOS
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

#define MAX_NODES 96
#define MAX_TITLE 24
#define MAX_CMD 128
#define MAX_MACRO 384
#define MAX_DEPTH 4
#define MAX_CHILD 22
#define MENU_CAPACITY 20
#define MENU_WIDTH 20
#define MAX_HELP_LINES 96
#define HELP_WIDTH 70
#define MAX_EXPLORE_ENTRIES 256
#define EXPLORE_ROWS 11
#define EXPLORE_COLS 4
#define MAKE_FP(seg,off) ((void far *)((((unsigned long)(seg))<<16) | \
                                      (unsigned short)(off)))

typedef struct {
  unsigned char background,border,main_title,titles,folders,launchers;
  unsigned char selected_fg,selected_bg,controls_fg,controls_bg,labels;
  unsigned char menu_top,show_explore,show_power,show_time;
  unsigned char screensaver,saver_color,saver_delay,hour_12;
  unsigned char font_id,font_persist,mouse_cursor,prompt_inactivity;
} APPEARANCE;

static const APPEARANCE default_appearance={1,11,12,14,15,10,15,3,0,7,7,0,1,1,1,1,10,0,1,1,1,0,0};
static APPEARANCE appearance={1,11,12,14,15,10,15,3,0,7,7,0,1,1,1,1,10,0,1,1,1,0,0};

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
static unsigned char cursor_start,cursor_end;
static int run_node;
static char run_command[MAX_CMD];
static char config_file[MAX_CMD];
static char backup_file[MAX_CMD];
static char temp_file[MAX_CMD];
static char backup_temp_file[MAX_CMD];
static char bad_file[MAX_CMD];
static char appearance_file[MAX_CMD];
static char appearance_temp_file[MAX_CMD];
static char logos_file[MAX_CMD];
static char help_file[MAX_CMD];
static char font_file[MAX_CMD];
static char program_dir[MAX_CMD];
static int font_is_vga(void);
static int font_preview(unsigned char id);
static int font_commit(unsigned char id);
static void font_restore(void);
static void shortcut_refresh(void);
static int shortcut_set_dialog(void);
static int shortcut_unload(void);
static int shortcut_activate(void);
static void shortcut_idle_sync(void);
static char write_path[MAX_CMD];
static unsigned char copy_buffer[512];
static unsigned char key_shift;
static unsigned char key_scan;
static int mouse_present;
static int mouse_visible;
static int mouse_glyph_saved;
static unsigned char mouse_old_glyph[32];
static int screensaver_ran;
static int dialog_close_x=-1,dialog_close_y=-1;
static unsigned mouse_last_buttons;
static unsigned mouse_raw_x,mouse_raw_y;
static int added_visible_node;
static char help_lines[MAX_HELP_LINES][HELP_WIDTH+1];
static int help_line_count;
static int config_shortcut_active,config_shortcut_changed;
static char config_shortcut_combination[64];

#define BUILTIN_EXPLORE (-3)
#define BUILTIN_POWER (-2)
#define BUILTIN_CONFIG (-4)
#define MINUTE_TICKS 1092UL
#define MOUSE_MOVED 0x8000U
#define CONFIG_TAB_COUNT 5
#define CONFIG_CONTROL_BASE 5

typedef struct {
  char name[13];
  unsigned char directory;
} EXPLORE_ENTRY;

static EXPLORE_ENTRY explore_entries[MAX_EXPLORE_ENTRIES];
static int explore_count;
static unsigned char explore_drive_symbols[26];
static int explore_drive_positions[26];

static const char *sample_config[] = {
  "; Launch! 2.7 initial menu definition\n",
  "; ITEM=title|command and parameters|press Enter|change directory|prompt (0/1)\n",
  "; SEPARATOR= adds a movable horizontal separator\n",
  "\n",
  "[Launcher]\n",
  "FOLDER=DOS Commands\n",
  "SEPARATOR=\n",
  "\n",
  "[Launcher\\DOS Commands]\n",
  "ITEM=Check Disk|CHKDSK|1|0|0\n",
  "ITEM=Format Disk|FORMAT|0|0|0\n",
  "ITEM=Partition Disk|FDISK|1|0|0\n",
  "ITEM=Memory Information|MEM|1|0|0\n",
  "ITEM=File Attributes|ATTRIB|0|0|0\n",
  "ITEM=Find Text|FIND|0|0|0\n",
  "ITEM=Sort Text|SORT|0|0|0\n",
  0
};

static void cursor_restore(void);
static void mouse_stop(void);
static void mouse_pointer_restore(void);
static unsigned mouse_poll(int *column,int *row);

static void video_init(void)
{
  unsigned char far *m = (unsigned char far *)MAKE_FP(0x40,0x49);
  unsigned short far *c = (unsigned short far *)MAKE_FP(0x40,0x4A);
  unsigned char far *r = (unsigned char far *)MAKE_FP(0x40,0x84);
  screen_cols = *c;
  if (screen_cols <= 0 || screen_cols > 80) screen_cols = 80;
  screen_rows = (*r >= 24 && *r < 60) ? *r + 1 : 25;
  video = (unsigned short far *)MAKE_FP((*m == 7) ? 0xB000 : 0xB800,0);
}

static int save_screen(void)
{
  int i, n = screen_cols * screen_rows;
  saved=(unsigned short far *)_fmalloc((unsigned long)n*sizeof(unsigned short));
  if(!saved)return 0;
  for (i=0; i<n; ++i) saved[i] = video[i];
  return 1;
}

static void restore_screen(void)
{
  int i, n = screen_cols * screen_rows;
  if(!saved)return;
  for (i=0; i<n; ++i) video[i] = saved[i];
}

static void clear_text_screen(void)
{
  union REGS r;
  memset(&r,0,sizeof(r));r.h.ah=6;r.h.al=0;r.h.bh=7;
  r.x.cx=0;r.h.dh=(unsigned char)(screen_rows-1);
  r.h.dl=(unsigned char)(screen_cols-1);int86(0x10,&r,&r);
  memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=0;r.x.dx=0;int86(0x10,&r,&r);
}

static void close_menu(void)
{
  mouse_stop();mouse_pointer_restore();
  if(screensaver_ran)clear_text_screen();else restore_screen();
  cursor_restore();
  _ffree(saved);saved=0;
}

static void cell(int x,int y,int ch,int at)
{
  if (x>=0 && x<screen_cols && y>=0 && y<screen_rows)
    video[y*screen_cols+x] = (unsigned short)((at<<8)|(ch&255));
}

static void wait_vertical_retrace(void)
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
    video[y*screen_cols+x+i]=(unsigned short)((value&255)|(at<<8));
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
  cursor_start=r.h.ch; cursor_end=r.h.cl;
  r.h.ah=1; r.h.ch=0x20; r.h.cl=0; int86(0x10,&r,&r);
}

static void cursor_restore(void)
{
  union REGS r;
  r.h.ah=1; r.h.ch=cursor_start; r.h.cl=cursor_end;
  int86(0x10,&r,&r);
}

static void box(int x,int y,int w,int h,const char *title,int title_attr)
{
  int i,j,len;
  for (j=0;j<h;j++) for (i=0;i<w;i++) cell(x+i,y+j,' ',C_MENU_BACKGROUND);
  cell(x,y,218,C_BORDER); cell(x+w-1,y,191,C_BORDER);
  cell(x,y+h-1,192,C_BORDER); cell(x+w-1,y+h-1,217,C_BORDER);
  for(i=1;i<w-1;i++){cell(x+i,y,196,C_BORDER);cell(x+i,y+h-1,196,C_BORDER);}
  for(j=1;j<h-1;j++){cell(x,y+j,179,C_BORDER);cell(x+w-1,y+j,179,C_BORDER);}
  if(title && *title){
    len=strlen(title); if(len>w-6) len=w-6;
    cell(x+2,y,180,C_BORDER); textout(x+3,y,title,title_attr,len);
    cell(x+3+len,y,195,C_BORDER);
  }
}

static void dialog_box(int x,int y,int w,int h,const char *title)
{
  int i; unsigned short v;
  box(x,y,w,h,title,C_TITLE);
  cell(x+w-5,y,180,C_BORDER);cell(x+w-4,y,'X',C_ROOT_TITLE);
  cell(x+w-3,y,195,C_BORDER);dialog_close_x=x+w-4;dialog_close_y=y;
  for(i=1;i<=h;i++){
    v=video[(y+i)*screen_cols+x+w];
    cell(x+w,y+i,v&255,C_FAUX_SHADOW);
  }
  for(i=1;i<=w;i++){
    v=video[(y+h)*screen_cols+x+i];
    cell(x+i,y+h,v&255,C_FAUX_SHADOW);
  }
}

static void menu_box(int x,int y,int w,int h,const char *title,int title_attr)
{
  int i; unsigned short v;
  box(x,y,w,h,title,title_attr);
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
  strncpy(help_file,exe,n);help_file[n]=0;strcat(help_file,"LAUNCH.HLP");
  strncpy(font_file,exe,n);font_file[n]=0;strcat(font_file,"FONT.DAT");
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
  else if(!stricmp(key,"SHOW_EXPLORE")){field=&a->show_explore;limit=1;}
  else if(!stricmp(key,"SHOW_POWER")){field=&a->show_power;limit=1;}
  else if(!stricmp(key,"SHOW_TIME")){field=&a->show_time;limit=1;}
  else if(!stricmp(key,"SCREENSAVER")){field=&a->screensaver;limit=8;}
  else if(!stricmp(key,"SAVER_COLOR"))field=&a->saver_color;
  else if(!stricmp(key,"SAVER_DELAY")){field=&a->saver_delay;limit=3;}
  else if(!stricmp(key,"HOUR_12")){field=&a->hour_12;limit=1;}
  else if(!stricmp(key,"FONT_ID")){field=&a->font_id;limit=21;}
  else if(!stricmp(key,"FONT_PERSIST")){field=&a->font_persist;limit=1;}
  else if(!stricmp(key,"MOUSE_CURSOR")){field=&a->mouse_cursor;limit=2;}
  else if(!stricmp(key,"PROMPT_INACTIVITY")){field=&a->prompt_inactivity;limit=1;}
  if(!field || value<0 || value>limit)return 0;
  *field=(unsigned char)value;return 1;
}

static int load_appearance(void)
{
  FILE *f;static char line[80];char *p,*q,*end;long value;APPEARANCE loaded;
  appearance=default_appearance;
  f=fopen(appearance_file,"rt");if(!f)return 1;
  loaded=default_appearance;
  while(fgets(line,sizeof(line),f)){
    p=trim(line);if(!*p || *p==';' || *p=='#')continue;
    q=strchr(p,'=');if(!q){fclose(f);return 0;}
    *q++=0;q=trim(q);p=trim(p);
    value=strtol(q,&end,10);end=trim(end);
    if(!*q || *end || !appearance_value(&loaded,p,(int)value)){fclose(f);return 0;}
  }
  if(ferror(f)){fclose(f);return 0;}
  fclose(f);appearance=loaded;return 1;
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
  nodes[n].press_enter=1;nodes[n].change_dir=0;nodes[n].prompt_params=0;
  return n;
}

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
      char *r;int node,enter=1,cd=0,prompt=0,flags[3],flag_count=0;
      p=trim(p+5); q=strchr(p,'|');
      if(q && *trim(p)){
        *q++=0;
        while(flag_count<3 && (r=strrchr(q,'|'))!=0 &&
              (r[1]=='0' || r[1]=='1') && r[2]==0){
          flags[flag_count++]=r[1]-'0';*r=0;
        }
        if(flag_count>=1)cd=flags[0];
        if(flag_count>=2){enter=flags[1];cd=flags[0];}
        if(flag_count>=3){enter=flags[2];cd=flags[1];prompt=flags[0];}
        p=trim(p);q=trim(q);
        if(!*p || !*q){valid=0;break;}
        node=add_node(p,q,parent,0);
        if(node<0){valid=0;break;}
        nodes[node].press_enter=(unsigned char)enter;nodes[node].change_dir=(unsigned char)cd;
        nodes[node].prompt_params=(unsigned char)prompt;
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

static unsigned long bios_ticks(void)
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

static void ega_span(int y,int left,int right,unsigned char colour);
static void ega_rectangle(int x,int y,int width,int height,unsigned char colour);

static unsigned saver_random(unsigned limit)
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

static void ega_span(int y,int left,int right,unsigned char colour)
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

static void ega_rectangle(int x,int y,int width,int height,unsigned char colour)
{
  int row;for(row=0;row<height;row++)ega_span(y+row,x,x+width-1,colour);
}

static void ega_line(int x0,int y0,int x1,int y1,unsigned char colour)
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

static void mouse_glyph_write(const unsigned char *glyph)
{
  FONT_REGS old;unsigned char far *font;
  int i;
  font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,127*32);
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
    if(mouse_glyph_saved)mouse_pointer_restore();
    memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;
    if(appearance.mouse_cursor==1){
      /* Retain the underlying glyph and invert its foreground/background. */
      r.x.cx=0xFFFF;r.x.dx=0x7700;
    } else {
      /* Target retains the underlying attribute but substitutes glyph 8. */
      r.x.cx=0xFF00;r.x.dx=0x7708;
    }
    int86(0x33,&r,&r);return;
  }
  if(!mouse_glyph_saved){
    font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,127*32);
    for(i=0;i<32;i++)mouse_old_glyph[i]=font[i];
    font_plane_close(&old);mouse_glyph_saved=1;
  }
  memset(arrow,0,sizeof(arrow));
  height_ptr=(unsigned char far *)MAKE_FP(0x40,0x85);height=*height_ptr;
  if(height<8 || height>32)height=16;
  for(i=0;i<height;i++){
    source=i*16/height;if(source>15)source=15;arrow[i]=arrow16[source];
  }
  mouse_glyph_write(arrow);
  memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;
  r.x.cx=0xF000;r.x.dx=0x0F7F;int86(0x33,&r,&r);
}

static void mouse_pointer_restore(void)
{
  union REGS r;if(!mouse_glyph_saved)return;
  mouse_glyph_write(mouse_old_glyph);
  memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;
  r.x.cx=0xFFFF;r.x.dx=0x7700;int86(0x33,&r,&r);
  mouse_glyph_saved=0;
}

static void mouse_show(void)
{
  union REGS r;if(!mouse_present || mouse_visible)return;
  memset(&r,0,sizeof(r));r.x.ax=1;int86(0x33,&r,&r);
  mouse_visible=1;
}

static int saver_input(unsigned start_x,unsigned start_y)
{
  int mx=0,my=0;unsigned buttons;
  if(key_waiting()){keyread();return 1;}
  buttons=mouse_poll(&mx,&my);
  return buttons || mouse_raw_x!=start_x || mouse_raw_y!=start_y;
}

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

static int boing_isqrt(int value)
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

static void run_screensaver(void)
{
  union REGS r;int mx=0,my=0,old_mode,old_rows=screen_rows;
  unsigned start_x,start_y;
  if(!appearance.screensaver)return;
  screensaver_ran=1;
  mouse_stop();
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);old_mode=r.h.al;
  set_ega_saver_mode(appearance.screensaver==1);
  saver_random_state=bios_ticks()^0xA5A55A5AUL;
  /* A mode change can rescale or reset the mouse driver's coordinates. */
  (void)mouse_poll(&mx,&my);start_x=mouse_raw_x;start_y=mouse_raw_y;
  if(appearance.screensaver==1)clock_saver_loop(start_x,start_y);
  else if(appearance.screensaver==2)starry_saver_loop(start_x,start_y);
  else if(appearance.screensaver==3)warp_saver_loop(start_x,start_y);
  else if(appearance.screensaver==4)logo_saver_loop(start_x,start_y);
  else if(appearance.screensaver==5)pipes_saver_loop(start_x,start_y);
  else if(appearance.screensaver==6)mystify_saver_loop(start_x,start_y);
  else if(appearance.screensaver==7)boing_saver_loop(start_x,start_y);
  else halftone_saver_loop(start_x,start_y);
  memset(&r,0,sizeof(r));r.h.al=(unsigned char)old_mode;int86(0x10,&r,&r);
  if(old_rows>25){memset(&r,0,sizeof(r));r.x.ax=0x1112;r.h.bl=0;int86(0x10,&r,&r);}
  video_init();
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
    if(key_waiting()){*key=keyread();mouse_stop();return;}
    *buttons=mouse_poll(column,row);
    if(!*buttons && (*column!=start_column || *row!=start_row)){
      *buttons=MOUSE_MOVED;mouse_stop();return;
    }
  } while(!*buttons);
  mouse_stop();
  if((*buttons&1) && *column==dialog_close_x && *row==dialog_close_y){
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
    else if(fprintf(f,"ITEM=%s|%s|%u|%u|%u\n",nodes[node].title,nodes[node].command,
                    nodes[node].press_enter,nodes[node].change_dir,
                    nodes[node].prompt_params)<0)return 0;
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
  ok=fputs("; Launch! 2.7 menu definition\n; ITEM=title|command and parameters|press Enter|change directory|prompt (0/1)\n; SEPARATOR= adds a movable horizontal separator\n\n",f)!=EOF;
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
  if(fputs("; Launch! 2.7 appearance settings\n",f)==EOF)ok=0;
  if(ok && fprintf(f,"BACKGROUND=%u\nBORDER=%u\nMAIN_TITLE=%u\nTITLES=%u\n"
      "FOLDERS=%u\nLAUNCHERS=%u\nSELECTED_FG=%u\nSELECTED_BG=%u\n"
      "CONTROLS_FG=%u\nCONTROLS_BG=%u\nLABELS=%u\nMENU_TOP=%u\n"
      "SCREENSAVER=%u\nSAVER_COLOR=%u\nSAVER_DELAY=%u\nHOUR_12=%u\n"
      "SHOW_EXPLORE=%u\n"
      "SHOW_POWER=%u\nSHOW_TIME=%u\nFONT_ID=%u\nFONT_PERSIST=%u\n"
      "MOUSE_CURSOR=%u\n",
      appearance.background,appearance.border,appearance.main_title,appearance.titles,
      appearance.folders,appearance.launchers,appearance.selected_fg,appearance.selected_bg,
      appearance.controls_fg,appearance.controls_bg,appearance.labels,
      appearance.menu_top,appearance.screensaver,appearance.saver_color,
      appearance.saver_delay,appearance.hour_12,
      appearance.show_explore,appearance.show_power,
      appearance.show_time,appearance.font_id,appearance.font_persist,
      appearance.mouse_cursor)<0)ok=0;
  if(fclose(f)!=0)ok=0;
  if(!ok){remove(appearance_temp_file);return 0;}
  remove(appearance_file);
  if(rename(appearance_temp_file,appearance_file)!=0)return 0;
  shortcut_idle_sync();
  return 1;
}

static int write_sample(const char *name)
{
  FILE *f;int i,ok=1;
  f=fopen(name,"wt");if(!f)return 0;
  for(i=0;sample_config[i];i++)if(fputs(sample_config[i],f)==EOF){ok=0;break;}
  if(fclose(f)!=0)ok=0;
  if(!ok)remove(name);
  return ok;
}

/* 1=normal, 2=restored backup, 3=created sample, 0=failure. */
static int prepare_config(void)
{
  int primary_exists=file_exists(config_file);
  int backup_exists=file_exists(backup_file);
  if(primary_exists && load_config(config_file))return 1;
  if(backup_exists && load_config(backup_file)){
    if(primary_exists){remove(bad_file);rename(config_file,bad_file);}
    if(copy_file(backup_file,config_file))return 2;
    return 0;
  }
  if(primary_exists){
    remove(bad_file);
    if(rename(config_file,bad_file)!=0)return 0;
  } else if(backup_exists){
    remove(bad_file);rename(backup_file,bad_file);
  }
  if(!write_sample(config_file) || !load_config(config_file))return 0;
  remove(backup_file);
  if(!copy_file(config_file,backup_file))return 0;
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

static void draw_button(int x,int y,const char *label,int width,int focused)
{
  int i; unsigned short v;
  for(i=1;i<=width;i++){
    v=video[(y+1)*screen_cols+x+i];
    cell(x+i,y+1,223,((v>>8)&0xF0)|C_BLOCK_SHADOW_FG);
  }
  v=video[y*screen_cols+x+width];
  cell(x+width,y,220,((v>>8)&0xF0)|C_BLOCK_SHADOW_FG);
  textout(x,y,label,C_BUTTON,width);
  if(focused){cell(x,y,16,C_BUTTON);cell(x+width-1,y,17,C_BUTTON);}
}

static void press_button(int x,int y,const char *label,int width)
{
  int i,mx=0,my=0;
  /* Remove the right and lower shadows while the physical button is held. */
  cell(x+width,y,' ',C_MENU_BACKGROUND);
  for(i=1;i<=width;i++)cell(x+i,y+1,' ',C_MENU_BACKGROUND);
  textout(x,y,label,C_BUTTON,width);
  mouse_show();
  do {(void)mouse_poll(&mx,&my);} while(mouse_last_buttons&1);
  mouse_stop();
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

static int confirm_box(const char *title,const char *message)
{
  int x=(screen_cols-48)/2,y=(screen_rows-7)/2,k,yes=0,mx=0,my=0;
  unsigned mb;
  char line1[43],line2[43];
  wrap_message(message,line1,line2,42);
  dialog_box(x,y,48,7,title);textout(x+3,y+2,line1,C_FOLDER,42);textout(x+3,y+3,line2,C_FOLDER,42);
  for(;;){
    wait_vertical_retrace();
    draw_button(x+14,y+4,"  Yes  ",7,yes);
    draw_button(x+28,y+4,"  No  ",6,!yes);
    wait_input(&k,&mx,&my,&mb);
    if((mb&MOUSE_MOVED) && my==y+4){
      if(mx>=x+14 && mx<x+21)yes=1;
      else if(mx>=x+28 && mx<x+34)yes=0;
      continue;
    }
    if((mb&1) && my==y+4){
      if(mx>=x+14 && mx<x+21){press_button(x+14,y+4,"  Yes  ",7);return 1;}
      if(mx>=x+28 && mx<x+34){press_button(x+28,y+4,"  No  ",6);return 0;}
    }
    if(k==27) return 0;
    if(k==0x4B00 || k==0x4D00 || k==9) yes=!yes;
    else if(k==13) return yes;
    else if(k=='y' || k=='Y') return 1;
    else if(k=='n' || k=='N') return 0;
  }
}

static void notice_box(const char *title,const char *message)
{
  int w=strlen(message)>42?54:48;
  int x=(screen_cols-w)/2,y=(screen_rows-7)/2,k,mx,my,button_x=x+(w-6)/2;
  unsigned mb;
  dialog_box(x,y,w,7,title);textout(x+3,y+2,message,C_FOLDER,w-6);
  draw_button(button_x,y+4,"  OK  ",6,1);
  for(;;){
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED)continue;
    if(mb&1){
      if(my==y+4 && mx>=button_x && mx<button_x+6){
        press_button(button_x,y+4,"  OK  ",6);return;
      }
      continue;
    }
    if(k)return;
  }
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
  int x=(screen_cols-54)/2,y=(screen_rows-7)/2,k,choice=2,mx=0,my=0;unsigned mb;
  dialog_box(x,y,54,7,"Shutdown...");textout(x+3,y+2,"What do you want to do?",C_FOLDER,46);
  for(;;){
    wait_vertical_retrace();
    draw_button(x+3,y+4,"  Power Off  ",13,choice==0);
    draw_button(x+20,y+4,"  Reboot  ",10,choice==1);
    draw_button(x+39,y+4,"  Cancel  ",10,choice==2);
    wait_input(&k,&mx,&my,&mb);
    if((mb&MOUSE_MOVED) && my==y+4){
      if(mx>=x+3 && mx<x+16)choice=0;
      else if(mx>=x+20 && mx<x+30)choice=1;
      else if(mx>=x+39 && mx<x+49)choice=2;
      continue;
    }
    if((mb&1) && my==y+4){
      if(mx>=x+3 && mx<x+16){press_button(x+3,y+4,"  Power Off  ",13);return 1;}
      if(mx>=x+20 && mx<x+30){press_button(x+20,y+4,"  Reboot  ",10);return 2;}
      if(mx>=x+39 && mx<x+49){press_button(x+39,y+4,"  Cancel  ",10);return 0;}
    }
    if(k==27)return 0;
    if(k==0x4B00)choice=(choice+2)%3;
    else if(k==0x4D00 || k==9)choice=(choice+1)%3;
    else if(k==13)return choice==0?1:(choice==1?2:0);
  }
}

static int choose_type(void)
{
  int x=(screen_cols-56)/2,y=(screen_rows-7)/2,k,choice=0,mx=0,my=0;unsigned mb;
  dialog_box(x,y,56,7,"Add Item");textout(x+3,y+2,"Choose the type of item to add:",C_FOLDER,48);
  for(;;){
    wait_vertical_retrace();
    draw_button(x+5,y+4,"  Folder  ",10,choice==0);
    draw_button(x+20,y+4,"  Launcher  ",12,choice==1);
    draw_button(x+37,y+4,"  Separator  ",13,choice==2);
    wait_input(&k,&mx,&my,&mb);
    if((mb&MOUSE_MOVED) && my==y+4){
      if(mx>=x+5 && mx<x+15)choice=0;
      else if(mx>=x+20 && mx<x+32)choice=1;
      else if(mx>=x+37 && mx<x+50)choice=2;
      continue;
    }
    if((mb&1) && my==y+4){
      if(mx>=x+5 && mx<x+15){press_button(x+5,y+4,"  Folder  ",10);return 1;}
      if(mx>=x+20 && mx<x+32){press_button(x+20,y+4,"  Launcher  ",12);return 0;}
      if(mx>=x+37 && mx<x+50){press_button(x+37,y+4,"  Separator  ",13);return 2;}
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
  if(focused){i=pos-scroll;cell(x+13+i,y,(pos<vlen)?value[pos]:' ',C_INPUT_FIELD);}
}

static void check_line(int x,int y,const char *label,int checked,int focused)
{
  char mark[4];
  sprintf(mark,"[%c]",checked?(char)254:' ');
  textout(x,y,mark,C_BUTTON,3);
  textout(x+4,y,label,focused?C_SELECTED:C_INPUT_LABEL,34);
}

static int item_form(int folder,char *name,char *exe,char *params,
                     int *press_enter,int *change_dir,int *prompt_params,int editing)
{
  char *fields[3]; int limits[3],pos[3],count,controls,focus=0,hover=-1;
  int k,x,y,i,len,mx=0,my=0,scroll;
  unsigned mb;
  fields[0]=name;fields[1]=exe;fields[2]=params;
  limits[0]=folder?16:18;limits[1]=MAX_CMD-2;limits[2]=MAX_CMD-1;
  name[limits[0]]=0;
  pos[0]=pos[1]=pos[2]=0; count=folder?1:3;
  controls=folder?count:count+3;
  x=(screen_cols-64)/2;y=(screen_rows-(folder?10:14))/2;
  dialog_box(x,y,64,folder?10:14,editing?(folder?"Edit Folder":"Edit Launcher"):(folder?"Add Folder":"Add Launcher"));
  for(;;){
    wait_vertical_retrace();
    field_line(x+3,y+2,"Name:",name,focus==0,hover==0,pos[0],42);
    if(!folder){
      field_line(x+3,y+4,"Command:",exe,focus==1,hover==1,pos[1],42);
      field_line(x+3,y+6,"Parameters:",params,focus==2,hover==2,pos[2],28);
      textout(x+47,y+6,"Prompt?",(focus==3||hover==3)?C_SELECTED:C_INPUT_LABEL,7);
      cell(x+55,y+6,'[',C_BUTTON);
      cell(x+56,y+6,*prompt_params?(char)254:' ',C_BUTTON);
      cell(x+57,y+6,']',C_BUTTON);
    }
    if(!folder){
      check_line(x+16,y+8,"Provide \021\331 after launcher command",*press_enter,focus==4||hover==4);
      check_line(x+16,y+9,"Change directory first",*change_dir,focus==5||hover==5);
    }
    draw_button(x+16,y+(folder?7:11),"  Save  ",8,focus==controls||hover==controls);
    draw_button(x+28,y+(folder?7:11),"  Cancel  ",10,focus==controls+1||hover==controls+1);
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      hover=-1;
      if(my==y+2 && mx>=x+16 && mx<x+58)hover=0;
      else if(!folder && my==y+4 && mx>=x+16 && mx<x+58)hover=1;
      else if(!folder && my==y+6 && mx>=x+16 && mx<x+44)hover=2;
      else if(!folder && my==y+6 && mx>=x+47 && mx<x+58)hover=3;
      else if(!folder && my==y+8 && mx>=x+16 && mx<x+54)hover=4;
      else if(!folder && my==y+9 && mx>=x+16 && mx<x+54)hover=5;
      else if(my==y+(folder?7:11) && mx>=x+16 && mx<x+24)hover=controls;
      else if(my==y+(folder?7:11) && mx>=x+28 && mx<x+38)hover=controls+1;
      continue;
    }
    if(mb&1){
      if(my==y+2 && mx>=x+16 && mx<x+58){focus=0;i=0;}
      else if(!folder && my==y+4 && mx>=x+16 && mx<x+58){focus=1;i=1;}
      else if(!folder && my==y+6 && mx>=x+16 && mx<x+44){focus=2;i=2;}
      else if(!folder && my==y+6 && mx>=x+47 && mx<x+58){*prompt_params=!*prompt_params;focus=3;continue;}
      else if(!folder && my==y+8 && mx>=x+16 && mx<x+54){*press_enter=!*press_enter;focus=4;continue;}
      else if(!folder && my==y+9 && mx>=x+16 && mx<x+54){*change_dir=!*change_dir;focus=5;continue;}
      else if(my==y+(folder?7:11) && mx>=x+16 && mx<x+24){
        press_button(x+16,y+(folder?7:11),"  Save  ",8);
        if(*name && (folder || *exe))return 1;
        focus=controls;continue;
      }
      else if(my==y+(folder?7:11) && mx>=x+28 && mx<x+38){
        press_button(x+28,y+(folder?7:11),"  Cancel  ",10);return 0;
      }
      else continue;
      len=strlen(fields[i]);scroll=pos[i]>(i==2?27:41)?pos[i]-(i==2?27:41):0;
      pos[i]=scroll+mx-(x+16);if(pos[i]>len)pos[i]=len;
      continue;
    }
    if(k==27) return 0;
    if(k==9 || k==0x5000){focus=(focus+1)%(controls+2);continue;}
    if(k==0x4800){focus=(focus+controls+1)%(controls+2);continue;}
    if(!folder && (focus==3 || focus==4 || focus==5)){
      if(k==' ' || k==13){
        if(focus==3)*prompt_params=!*prompt_params;
        else if(focus==4)*press_enter=!*press_enter;
        else *change_dir=!*change_dir;
      }
      continue;
    }
    if(focus>=controls){
      if(k==0x4B00 || k==0x4D00) focus=(focus==controls)?controls+1:controls;
      else if(k==13){if(focus==controls && *name && (folder || *exe))return 1;if(focus==controls+1)return 0;}
      continue;
    }
    i=focus;len=strlen(fields[i]);
    if(k==0x4B00){if(pos[i]>0)pos[i]--;}
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
  unsigned char colour[11];
} COLOUR_SCHEME;

static const COLOUR_SCHEME colour_schemes[8]={
  {"Standard", {1,11,12,14,15,10,15,3,0,7,7}},
  {"Hot Dog",  {4,0,14,14,7,15,14,0,4,7,12}},
  {"Mono",     {7,8,0,0,8,0,15,0,7,0,0}},
  {"Pretty",   {5,12,14,13,7,12,5,3,5,7,15}},
  {"Pro Style",{7,15,9,0,8,0,11,3,9,1,8}},
  {"Stealth",  {0,9,14,14,7,15,12,0,9,0,15}},
  {"Swiss",    {7,8,4,4,8,0,15,0,7,4,8}},
  {"Tranquil", {3,11,10,10,8,0,10,0,15,2,15}}
};

static int colour_scheme_index(void)
{
  const unsigned char *current=&appearance.background;int i,j;
  for(i=0;i<8;i++){
    for(j=0;j<11;j++)if(current[j]!=colour_schemes[i].colour[j])break;
    if(j==11)return i;
  }
  return -1;
}

static void apply_colour_scheme(int index)
{
  unsigned char *current=&appearance.background;int i;
  for(i=0;i<11;i++)current[i]=colour_schemes[index].colour[i];
}

static const char *screensaver_names[9]={
  "None","Clock","Starry Nite","Warp","Logo","Pipes","Mystify","Boing","Halftone"
};

static const char *saver_delay_names[4]={
  "1 minute","5 minutes","15 minutes","30 minutes"
};
static const char *mouse_cursor_names[3]={"Pointer","Block","Target"};

static const char *font_names[22]={
  "Standard","Bold Sans","Tall Sans","IBM ISO","CGAlike","Elite",
  "Oakley","Oakley Big","Sans Serif","Howard","Neil","Italic",
  "Olde Eng","DOS/V","MSDOS/V","Roman","Fatscii","Elergon",
  "Police","Espy","Scribble","Script"
};

static void cycle_control(int x,int y,const char *value,int focused)
{
  char field[24];
  sprintf(field,"%c %-11.11s %c",(char)174,value,(char)175);
  textout(x,y,field,focused?C_SELECTED:C_BUTTON,15);
}

static void draw_config_tabs(int x,int y,int active,int focus,int hover)
{
  static const int edge[6]={3,15,23,33,48,56};
  static const char *name[5]={"Shortcut","Menu","Colors","Screensaver","Font"};
  int i,j,focused,base,text_attr;

  /* Upper edge of the single, joined tab strip. */
  cell(x+edge[0],y,218,C_BORDER);
  for(i=0;i<5;i++){
    for(j=edge[i]+1;j<edge[i+1];j++)cell(x+j,y,196,C_BORDER);
    cell(x+edge[i+1],y,i==4?191:194,C_BORDER);
  }

  /* The lower edge forms the divider across the complete dialog. */
  for(j=1;j<65;j++)cell(x+j,y+1,196,C_BORDER);
  for(i=0;i<6;i++)cell(x+edge[i],y+1,179,C_BORDER);
  for(i=0;i<5;i++){
    focused=(focus==i||hover==i);
    base=focused?C_SELECTED:ATTR(appearance.background,appearance.labels);
    text_attr=focused?C_SELECTED:
      (active==i?ATTR(appearance.background,appearance.titles):
                 ATTR(appearance.background,appearance.labels));
    for(j=edge[i]+1;j<edge[i+1];j++)cell(x+j,y+1,' ',base);
    textout(x+edge[i]+2,y+1,name[i],text_attr,(int)strlen(name[i]));
  }
}

static void draw_character_preview(int x,int y)
{
  int code,row,column;
  for(code=0;code<256;code++){
    row=code/56;column=code%56;cell(x+column,y+row,code,C_BORDER);
  }
}

static int config_count(int tab)
{
  if(tab==0)return 2;
  if(tab==1)return 6;
  if(tab==2)return 12;
  if(tab==3)return appearance.screensaver==1?4:3;
  return font_is_vga()?2:0;
}

static unsigned char *config_field(int tab,int item,int *limit)
{
  *limit=15;
  if(tab==2)switch(item){
    case 1:*limit=7;return &appearance.background;
    case 2:return &appearance.border;case 3:return &appearance.main_title;
    case 4:return &appearance.titles;case 5:return &appearance.folders;
    case 6:return &appearance.launchers;case 7:return &appearance.selected_fg;
    case 8:*limit=7;return &appearance.selected_bg;
    case 9:return &appearance.controls_fg;
    case 10:*limit=7;return &appearance.controls_bg;
    case 11:return &appearance.labels;
  }
  if(tab==1){
    if(item==0){*limit=1;return &appearance.menu_top;}
    if(item==1)return &appearance.show_explore;
    if(item==2)return &appearance.show_power;
    if(item==3)return &appearance.show_time;
    if(item==4){*limit=1;return &appearance.hour_12;}
    if(item==5){*limit=2;return &appearance.mouse_cursor;}
  }
  if(tab==3){
    if(item==0){*limit=8;return &appearance.screensaver;}
    if(appearance.screensaver==1){
      if(item==1)return &appearance.saver_color;
      if(item==2){*limit=3;return &appearance.saver_delay;}
    } else {
      if(item==1){*limit=3;return &appearance.saver_delay;}
    }
  }
  if(tab==4){
    if(item==0){*limit=21;return &appearance.font_id;}
    if(item==1){*limit=1;return &appearance.font_persist;}
  }
  return 0;
}

static void change_config_value(int tab,int item,int direction)
{
  unsigned char *field;int limit,value;
  if(tab==2 && item==0){
    value=colour_scheme_index();
    if(value<0)value=direction>0?0:7;
    else {value+=direction;if(value<0)value=7;if(value>7)value=0;}
    apply_colour_scheme(value);return;
  }
  field=config_field(tab,item,&limit);if(!field)return;
  if((tab==1 && item>=1 && item<=3) || (tab==4 && item==1)){
    *field=!*field;return;
  }
  value=(int)*field+direction;if(value<0)value=limit;if(value>limit)value=0;
  *field=(unsigned char)value;
  if(tab==4 && item==0){font_preview(appearance.font_id);mouse_pointer_install();}
  if(tab==1 && item==5)mouse_pointer_install();
}

static void draw_config_page(int x,int y,int tab,int focus,int hover,int full)
{
  int f=focus-CONFIG_CONTROL_BASE,h=hover-CONFIG_CONTROL_BASE,i;
  if(full)dialog_box(x,y,66,20,"Launch! Configuration");
  draw_config_tabs(x,y+1,tab,focus,hover);
  if(tab==0){
    textout(x+5,y+5,"Keyboard shortcut status:",C_INPUT_LABEL,25);
    textout(x+31,y+5,config_shortcut_active?"Active":"Inactive",C_ITEM,12);
    if(config_shortcut_active)
      draw_button(x+45,y+5,"  Unload  ",10,f==0||h==0);
    else draw_button(x+45,y+5,"  Activate  ",12,f==0||h==0);
    textout(x+5,y+8,"Current combination:",C_INPUT_LABEL,25);
    textout(x+31,y+8,config_shortcut_combination,C_ITEM,28);
    textout(x+5,y+11,"Set new combination:",C_INPUT_LABEL,25);
    draw_button(x+31,y+11,"  Choose  ",10,f==1||h==1);
    if(config_shortcut_changed)
      textout(x+5,y+14,"Shortcut combination changed. Restart to take effect.",
              C_INPUT_LABEL,54);
  } else if(tab==2){
    static const char *labels[9]={"Panels","Border","Main Title","Titles","Folders","Launchers","Selected items","Controls","Labels"};
    int scheme=colour_scheme_index();
    textout(x+5,y+4,"Color scheme",C_INPUT_LABEL,18);
    cycle_control(x+25,y+4,scheme<0?"Custom":colour_schemes[scheme].name,f==0||h==0);
    textout(x+25,y+6,"Foreground",C_INPUT_LABEL,15);
    textout(x+45,y+6,"Background",C_INPUT_LABEL,15);
    for(i=0;i<9;i++)textout(x+5,y+7+i,labels[i],C_INPUT_LABEL,18);
    cycle_control(x+45,y+7,colour_names[appearance.background],f==1||h==1);
    cycle_control(x+25,y+8,colour_names[appearance.border],f==2||h==2);
    cycle_control(x+25,y+9,colour_names[appearance.main_title],f==3||h==3);
    cycle_control(x+25,y+10,colour_names[appearance.titles],f==4||h==4);
    cycle_control(x+25,y+11,colour_names[appearance.folders],f==5||h==5);
    cycle_control(x+25,y+12,colour_names[appearance.launchers],f==6||h==6);
    cycle_control(x+25,y+13,colour_names[appearance.selected_fg],f==7||h==7);
    cycle_control(x+45,y+13,colour_names[appearance.selected_bg],f==8||h==8);
    cycle_control(x+25,y+14,colour_names[appearance.controls_fg],f==9||h==9);
    cycle_control(x+45,y+14,colour_names[appearance.controls_bg],f==10||h==10);
    cycle_control(x+25,y+15,colour_names[appearance.labels],f==11||h==11);
  } else if(tab==1){
    textout(x+5,y+4,"Menu position",C_INPUT_LABEL,18);
    cycle_control(x+25,y+4,appearance.menu_top?"Top":"Bottom",f==0||h==0);
    check_line(x+25,y+6,"Show 'Explore & Run' menu item",appearance.show_explore,f==1||h==1);
    check_line(x+25,y+8,"Show 'Shutdown...' menu item",appearance.show_power,f==2||h==2);
    check_line(x+25,y+10,"Show the time",appearance.show_time,f==3||h==3);
    textout(x+5,y+12,"Time format",C_INPUT_LABEL,18);
    cycle_control(x+25,y+12,appearance.hour_12?"12-hour":"24-hour",f==4||h==4);
    textout(x+5,y+14,"Mouse cursor",C_INPUT_LABEL,18);
    cycle_control(x+25,y+14,mouse_cursor_names[appearance.mouse_cursor],f==5||h==5);
  } else if(tab==3){
    textout(x+5,y+4,"Screensaver",C_INPUT_LABEL,18);
    cycle_control(x+25,y+4,screensaver_names[appearance.screensaver],f==0||h==0);
    if(appearance.screensaver==1){
      textout(x+5,y+6,"Clock colour",C_INPUT_LABEL,18);
      cycle_control(x+25,y+6,colour_names[appearance.saver_color],f==1||h==1);
      textout(x+5,y+8,"Inactivity period",C_INPUT_LABEL,18);
      cycle_control(x+25,y+8,saver_delay_names[appearance.saver_delay],f==2||h==2);
      draw_button(x+25,y+10,"  Preview   ",11,f==3||h==3);
      textout(x+5,y+12,"Monitoring",C_INPUT_LABEL,18);
      textout(x+25,y+12,config_shortcut_active?"Command Prompt and Menu":"Menu only",C_ITEM,23);
    } else {
      textout(x+5,y+6,"Inactivity period",C_INPUT_LABEL,18);
      cycle_control(x+25,y+6,saver_delay_names[appearance.saver_delay],f==1||h==1);
      draw_button(x+25,y+8,"  Preview   ",11,f==2||h==2);
      textout(x+5,y+10,"Monitoring",C_INPUT_LABEL,18);
      textout(x+25,y+10,config_shortcut_active?"Command Prompt and Menu":"Menu only",C_ITEM,23);
    }
  } else if(tab==4&&font_is_vga()){
    textout(x+5,y+4,"VGA display font",C_INPUT_LABEL,18);
    cycle_control(x+25,y+4,font_names[appearance.font_id],f==0||h==0);
    check_line(x+25,y+6,"Persist",appearance.font_persist,f==1||h==1);
    draw_character_preview(x+5,y+9);
  } else textout(x+7,y+9,"Font customization requires a VGA display adapter",C_INPUT_LABEL,52);
  for(i=1;i<65;i++)cell(x+i,y+16,196,C_BORDER);
  draw_button(x+5,y+17,"  Save  ",8,focus==20||hover==20);
  draw_button(x+16,y+17,"  Cancel  ",10,focus==21||hover==21);
  textout(x+50,y+17,"Version 2.7",C_INPUT_LABEL,11);
}

static int config_hit(int x,int y,int tab,int mx,int my)
{
  static const int left[5]={3,15,23,33,48},right[5]={15,23,33,48,56};int i;
  if(my==y+1||my==y+2)
    for(i=0;i<5;i++)if(mx>x+left[i]&&mx<x+right[i])return i;
  if(tab==0){
    if(my==y+5&&mx>=x+45&&
       mx<(config_shortcut_active?x+55:x+57))return CONFIG_CONTROL_BASE;
    if(my==y+11&&mx>=x+31&&mx<x+41)return CONFIG_CONTROL_BASE+1;
  } else if(tab==2){
    static const int rows[9]={4,8,9,10,11,12,13,14,15};
    static const int items[9]={0,2,3,4,5,6,7,9,11};
    if(mx>=x+25&&mx<x+40)
      for(i=0;i<9;i++)if(my==y+rows[i])return CONFIG_CONTROL_BASE+items[i];
    if(mx>=x+45&&mx<x+60&&my==y+7)return CONFIG_CONTROL_BASE+1;
    if(mx>=x+45&&mx<x+60&&my==y+13)return CONFIG_CONTROL_BASE+8;
    if(mx>=x+45&&mx<x+60&&my==y+14)return CONFIG_CONTROL_BASE+10;
  } else if(tab==1){
    static const int rows[6]={4,6,8,10,12,14};
    for(i=0;i<6;i++)if(my==y+rows[i]&&mx>=x+25&&mx<x+63)
      return CONFIG_CONTROL_BASE+i;
  } else if(tab==3){
    if(my==y+4&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE;
    if(my==y+6&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE+1;
    if(appearance.screensaver==1&&my==y+8&&mx>=x+25&&mx<x+40)
      return CONFIG_CONTROL_BASE+2;
    if(appearance.screensaver==1&&my==y+10&&mx>=x+25&&mx<x+36)return CONFIG_CONTROL_BASE+3;
    if(appearance.screensaver!=1&&my==y+8&&mx>=x+25&&mx<x+36)return CONFIG_CONTROL_BASE+2;
  } else if(tab==4&&font_is_vga()){
    if(my==y+4&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE;
    if(my==y+6&&mx>=x+25&&mx<x+36)return CONFIG_CONTROL_BASE+1;
  }
  if(my==y+17&&mx>=x+5&&mx<x+13)return 20;
  if(my==y+17&&mx>=x+16&&mx<x+26)return 21;
  return -1;
}

static int configure_appearance(void)
{
  APPEARANCE original=appearance;int x,y,k=0,mx=0,my=0,tab=0,focus=0,hover=-1;
  int hit,count,item,redraw=2;unsigned mb=0;
  video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 0;}
  cursor_hide();mouse_present=mouse_start();mouse_stop();
  config_shortcut_changed=0;shortcut_refresh();
  x=(screen_cols-66)/2;y=(screen_rows-20)/2;
  for(;;){
    wait_vertical_retrace();if(redraw){draw_config_page(x,y,tab,focus,hover,redraw==2);redraw=0;}
    wait_input(&k,&mx,&my,&mb);hit=config_hit(x,y,tab,mx,my);
    if(mb&MOUSE_MOVED){if(hover!=hit){hover=hit;redraw=1;}continue;}
    if(mb&1){
      if(hit>=0&&hit<CONFIG_TAB_COUNT){tab=hit;focus=hit;hover=-1;redraw=2;continue;}
      if(hit>=CONFIG_CONTROL_BASE&&hit<20){
        focus=hit;item=hit-CONFIG_CONTROL_BASE;
        if(tab==0&&item==0){
          if(config_shortcut_active){
            press_button(x+45,y+5,"  Unload  ",10);shortcut_unload();
          } else {
            press_button(x+45,y+5,"  Activate  ",12);shortcut_activate();
          }
          shortcut_refresh();
        } else if(tab==0&&item==1){
          press_button(x+31,y+11,"  Choose  ",10);
          if(shortcut_set_dialog())config_shortcut_changed=1;
          shortcut_refresh();
        } else if(tab==3&&((appearance.screensaver==1&&item==3) ||
                           (appearance.screensaver!=1&&item==2))){
          press_button(x+25,appearance.screensaver==1?y+10:y+8,"  Preview   ",11);
          run_screensaver();screensaver_ran=0;
        } else change_config_value(tab,item,1);
        redraw=2;continue;
      }
      if(hit==20){
        press_button(x+5,y+17,"  Save  ",8);
        if(font_commit(appearance.font_id)&&save_appearance()){close_menu();return 1;}
        notice_box("Write Error","Could not save the configuration.");redraw=2;continue;
      }
      if(hit==21){press_button(x+16,y+17,"  Cancel  ",10);appearance=original;font_restore();close_menu();return 0;}
      continue;
    }
    if(k==27){appearance=original;font_restore();close_menu();return 0;}
    count=config_count(tab);
    if(k==9||k==0x5000){
      if(focus<CONFIG_TAB_COUNT-1)focus++;
      else if(focus==CONFIG_TAB_COUNT-1)
        focus=count?CONFIG_CONTROL_BASE:20;
      else if(focus>=CONFIG_CONTROL_BASE&&
              focus<CONFIG_CONTROL_BASE+count-1)focus++;
      else if(focus<20)focus=20;else if(focus==20)focus=21;else focus=0;
      redraw=1;continue;
    }
    if(k==0x4800){
      if(focus==0)focus=21;
      else if(focus<CONFIG_TAB_COUNT)focus--;
      else if(focus==20)
        focus=count?CONFIG_CONTROL_BASE+count-1:CONFIG_TAB_COUNT-1;
      else if(focus==21)focus=20;
      else if(focus==CONFIG_CONTROL_BASE)focus=CONFIG_TAB_COUNT-1;
      else focus--;
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
      if(tab==0&&item==0){
        if(k==13||k==' '){
          if(config_shortcut_active){
            press_button(x+45,y+5,"  Unload  ",10);shortcut_unload();
          } else {
            press_button(x+45,y+5,"  Activate  ",12);shortcut_activate();
          }
          shortcut_refresh();redraw=2;
        }
      } else if(tab==0&&item==1){
        if(k==13||k==' '){
          press_button(x+31,y+11,"  Choose  ",10);
          if(shortcut_set_dialog())config_shortcut_changed=1;
          shortcut_refresh();redraw=2;
        }
      } else if(tab==3&&((appearance.screensaver==1&&item==3) ||
                         (appearance.screensaver!=1&&item==2))){
        if(k==13||k==' '){run_screensaver();screensaver_ran=0;redraw=2;}
      } else if(k==0x4B00){change_config_value(tab,item,-1);redraw=2;}
      else if(k==0x4D00||k==' '){change_config_value(tab,item,1);redraw=2;}
      else if(k==13){focus=(item+1<count)?focus+1:20;redraw=1;}
      continue;
    }
    if(k==0x4B00||k==0x4D00){focus=focus==20?21:20;redraw=1;}
    else if(k==13&&focus==20){
      if(font_commit(appearance.font_id)&&save_appearance()){close_menu();return 1;}
      notice_box("Write Error","Could not save the configuration.");redraw=2;
    } else if(k==13){appearance=original;font_restore();close_menu();return 0;}
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
  int list[MAX_CHILD],press_enter=1,change_dir=0,prompt_params=0;
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
  if(!item_form(folder,name,exe,params,&press_enter,&change_dir,&prompt_params,0))return 0;
  if(folder && !stricmp(name,"More")){notice_box("Reserved Name","More is reserved for automatic overflow.");return 0;}
  if(folder && find_folder(name,parent)>=0){notice_box("Duplicate Folder","That folder name is already in this menu.");return 0;}
  if(folder) node=add_node(name,"",parent,1);
  else {strcpy(cmd,exe);if(*params){strcat(cmd," ");strncat(cmd,params,MAX_CMD-strlen(cmd)-1);}node=add_node(name,cmd,parent,0);}
  if(node<0){notice_box("Menu Full","The menu database is full.");return 0;}
  nodes[node].press_enter=(unsigned char)press_enter;nodes[node].change_dir=(unsigned char)change_dir;
  nodes[node].prompt_params=(unsigned char)prompt_params;
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
  int prompt_params=nodes[node].prompt_params;
  static char name[MAX_TITLE],exe[MAX_CMD],params[MAX_CMD],cmd[MAX_CMD];
  exe[0]=params[0]=cmd[0]=0;
  if(nodes[node].separator)return 0;
  strcpy(name,nodes[node].title);
  if(nodes[node].folder && !stricmp(name,"More")){notice_box("Automatic Folder","More is managed automatically.");return 0;}
  if(!nodes[node].folder)split_command(nodes[node].command,exe,params);
  if(!item_form(nodes[node].folder,name,exe,params,&press_enter,&change_dir,&prompt_params,1))return 0;
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
    nodes[node].prompt_params=(unsigned char)prompt_params;
  }
  return 1;
}

static void add_help_line(const char *text,int length)
{
  int i;
  if(help_line_count>=MAX_HELP_LINES)return;
  if(length>HELP_WIDTH)length=HELP_WIDTH;
  for(i=0;i<length;i++){
    unsigned char ch=(unsigned char)text[i];
    help_lines[help_line_count][i]=(ch>=32 && ch!=127)?(char)ch:' ';
  }
  help_lines[help_line_count][length]=0;help_line_count++;
}

static void wrap_help_text(char *line)
{
  int length,cut;char *p=line;
  while(*p){
    length=strlen(p);
    if(length<=HELP_WIDTH){add_help_line(p,length);return;}
    cut=HELP_WIDTH;
    while(cut>0 && p[cut]!=' ' && p[cut]!='\t')cut--;
    if(cut<HELP_WIDTH/2)cut=HELP_WIDTH;
    add_help_line(p,cut);p+=cut;
    while(*p==' ' || *p=='\t')p++;
  }
  if(!*line)add_help_line("",0);
}

static void read_help_output(void)
{
  FILE *f;static char line[256];char *end;
  help_line_count=0;f=fopen(help_file,"rt");
  if(!f){const char *message="No /? help output is available for this command.";
    add_help_line(message,strlen(message));return;}
  while(help_line_count<MAX_HELP_LINES && fgets(line,sizeof(line),f)){
    end=line+strlen(line);while(end>line && (end[-1]=='\r' || end[-1]=='\n'))*--end=0;
    wrap_help_text(line);
  }
  fclose(f);remove(help_file);
  if(!help_line_count){const char *message="The command did not produce any /? help output.";
    add_help_line(message,strlen(message));}
}

static void command_for_spawn(const char *source,char *destination)
{
  int length;
  strncpy(destination,source,MAX_CMD-1);destination[MAX_CMD-1]=0;
  length=strlen(destination);
  if(length>=2 && destination[0]=='\"' && destination[length-1]=='\"'){
    memmove(destination,destination+1,length-2);destination[length-2]=0;
  }
}

static void capture_command_help(const char *command)
{
  FILE *out;int saved_out,saved_err,result;
  char executable[MAX_CMD],parameters[MAX_CMD],spawn_name[MAX_CMD],shell_line[MAX_CMD];
  const char *comspec;
  split_command(command,executable,parameters);
  command_for_spawn(executable,spawn_name);remove(help_file);
  out=fopen(help_file,"wt");
  if(!out){const char *message="Unable to create the temporary help file.";
    help_line_count=0;add_help_line(message,strlen(message));return;}
  fflush(stdout);fflush(stderr);saved_out=_dup(1);saved_err=_dup(2);
  if(saved_out<0 || saved_err<0){
    if(saved_out>=0)_close(saved_out);
    if(saved_err>=0)_close(saved_err);
    fclose(out);remove(help_file);help_line_count=0;
    {const char *message="Unable to redirect the command's help output.";
      add_help_line(message,strlen(message));}return;
  }
  _dup2(_fileno(out),1);_dup2(_fileno(out),2);
  mouse_stop();result=spawnlp(P_WAIT,spawn_name,spawn_name,"/?",NULL);
  if(result==-1 && (comspec=getenv("COMSPEC"))!=0 &&
     (int)strlen(executable)<MAX_CMD-4){
    strcpy(shell_line,executable);
    strcat(shell_line," /?");
    result=spawnl(P_WAIT,comspec,comspec,"/C",shell_line,NULL);
  }
  fflush(stdout);fflush(stderr);_dup2(saved_out,1);_dup2(saved_err,2);
  _close(saved_out);_close(saved_err);fclose(out);
  read_help_output();
}

static void parameter_field(int x,int y,const char *value,int position,
                            int focused,int hovered,int width)
{
  int length=strlen(value),scroll=0,cursor;
  if(position>=width)scroll=position-width+1;
  textout(x,y,value+scroll,(focused||hovered)?C_SELECTED:C_INPUT_FIELD,width);
  if(focused){cursor=position-scroll;cell(x+cursor,y,
      position<length?value[position]:' ',C_INPUT_FIELD);}
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

static int parameter_help_dialog(const char *title,const char *command,
                                 char *executable,char *parameters)
{
  int x=(screen_cols-76)/2,y=(screen_rows-22)/2,k=0,mx=0,my=0;
  int focus=1,hover=-1,scroll=0,position,length,i,max_scroll;unsigned mb=0;
  char filename[19],section[48];
  position=strlen(parameters);
  command_filename(executable,filename);
  sprintf(section," Run %s with parameters... ",filename);
  capture_command_help(command);max_scroll=help_line_count>14?help_line_count-14:0;
  dialog_box(x,y,76,22,title);cell(x,y+18,195,C_BORDER);cell(x+75,y+18,180,C_BORDER);
  for(i=1;i<75;i++)cell(x+i,y+18,196,C_BORDER);
  textout(x+2,y+18,section,C_TITLE,strlen(section));
  for(;;){
    wait_vertical_retrace();
    for(i=0;i<14;i++)textout(x+2,y+2+i,
       scroll+i<help_line_count?help_lines[scroll+i]:"",C_INPUT_FIELD,HELP_WIDTH);
    cell(x+73,y+2,scroll>0?30:' ',C_BUTTON);
    cell(x+73,y+15,scroll<max_scroll?31:' ',C_BUTTON);
    textout(x+2,y+16,"/ and PgUp/PgDn scrolls command help.",(focus==0||hover==0)?C_SELECTED:C_INPUT_LABEL,52);
    parameter_field(x+2,y+19,parameters,position,focus==1,hover==1,44);
    draw_button(x+49,y+19,"  Run  ",7,focus==2||hover==2);
    draw_button(x+61,y+19,"  Cancel  ",10,focus==3||hover==3);
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      hover=-1;
      if(my>=y+2 && my<=y+15)hover=0;
      else if(my==y+19 && mx>=x+2 && mx<x+46)hover=1;
      else if(my==y+19 && mx>=x+49 && mx<x+56)hover=2;
      else if(my==y+19 && mx>=x+61 && mx<x+71)hover=3;
      continue;
    }
    if(mb&1){
      if(my>=y+2 && my<=y+15){
        focus=0;
        if(mx>=x+72){if(my<y+9 && scroll>0)scroll--;else if(my>=y+9 && scroll<max_scroll)scroll++;}
        continue;
      }
      if(my==y+19 && mx>=x+2 && mx<x+46){
        int shown=position>=44?position-43:0;focus=1;position=shown+mx-(x+2);
        length=strlen(parameters);if(position>length)position=length;continue;
      }
      if(my==y+19 && mx>=x+49 && mx<x+56){
        press_button(x+49,y+19,"  Run  ",7);
        if(compose_prompt_command(executable,parameters))return 1;
        notice_box("Parameters Too Long","Shorten the parameters before running.");continue;
      }
      if(my==y+19 && mx>=x+61 && mx<x+71){
        press_button(x+61,y+19,"  Cancel  ",10);return 0;
      }
      continue;
    }
    if(k==27)return 0;
    if(k==9){focus=(focus+1)%4;continue;}
    if(k==0x4800){if(scroll>0)scroll--;focus=0;continue;}
    if(k==0x5000){if(scroll<max_scroll)scroll++;focus=0;continue;}
    if(k==0x4900){scroll=scroll>13?scroll-14:0;focus=0;continue;}
    if(k==0x5100){scroll=scroll+14<max_scroll?scroll+14:max_scroll;focus=0;continue;}
    if(focus==0){if(k==13)focus=1;continue;}
    if(focus==1){
      length=strlen(parameters);
      if(k==0x4B00){if(position>0)position--;}
      else if(k==0x4D00){if(position<length)position++;}
      else if(k==8){if(position>0){memmove(parameters+position-1,parameters+position,length-position+1);position--;}}
      else if(k==0x5300){if(position<length)memmove(parameters+position,parameters+position+1,length-position);}
      else if(k==13)focus=2;
      else if(k>=32 && k<127 && length<MAX_CMD-1){
        memmove(parameters+position+1,parameters+position,length-position+1);
        parameters[position++]=(char)k;
      }
      continue;
    }
    if(k==0x4B00 || k==0x4D00)focus=focus==2?3:2;
    else if(k==13){
      if(focus==3)return 0;
      if(compose_prompt_command(executable,parameters))return 1;
      notice_box("Parameters Too Long","Shorten the parameters before running.");
    }
  }
}

static int parameter_prompt_command(const char *title,const char *command)
{
  int x=(screen_cols-54)/2,y=(screen_rows-9)/2,k=0,mx=0,my=0;
  int focus=0,hover=-1,position,length,shown;unsigned mb=0;
  static char executable[MAX_CMD],parameters[MAX_CMD];
  char filename[19],display_name[14],message[49];
  split_command(command,executable,parameters);position=strlen(parameters);
  command_filename(executable,filename);
  strncpy(display_name,filename,13);display_name[13]=0;
  sprintf(message,"Run %s with the following parameters:",display_name);
  dialog_box(x,y,54,9,"Run with parameters");
  textout(x+3,y+2,message,C_INPUT_LABEL,48);
  for(;;){
    wait_vertical_retrace();
    parameter_field(x+3,y+4,parameters,position,focus==0,hover==0,48);
    draw_button(x+3,y+6,"  Run  ",7,focus==1||hover==1);
    draw_button(x+13,y+6,"  /?  ",7,focus==2||hover==2);
    draw_button(x+23,y+6,"  Cancel  ",10,focus==3||hover==3);
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      hover=-1;
      if(my==y+4&&mx>=x+3&&mx<x+51)hover=0;
      else if(my==y+6&&mx>=x+3&&mx<x+10)hover=1;
      else if(my==y+6&&mx>=x+13&&mx<x+20)hover=2;
      else if(my==y+6&&mx>=x+23&&mx<x+33)hover=3;
      continue;
    }
    if(mb&1){
      if(my==y+4&&mx>=x+3&&mx<x+51){
        shown=position>=48?position-47:0;focus=0;
        position=shown+mx-(x+3);length=strlen(parameters);
        if(position>length)position=length;continue;
      }
      if(my==y+6&&mx>=x+3&&mx<x+10){
        press_button(x+3,y+6,"  Run  ",7);
        if(compose_prompt_command(executable,parameters))return 1;
        notice_box("Parameters Too Long","Shorten the parameters before running.");continue;
      }
      if(my==y+6&&mx>=x+13&&mx<x+20){
        press_button(x+13,y+6,"  /?  ",7);
        return parameter_help_dialog(title,command,executable,parameters);
      }
      if(my==y+6&&mx>=x+23&&mx<x+33){
        press_button(x+23,y+6,"  Cancel  ",10);return 0;
      }
      continue;
    }
    if(k==27)return 0;
    if(k==9){focus=(focus+1)%4;continue;}
    if(focus==0){
      length=strlen(parameters);
      if(k==0x4B00){if(position>0)position--;}
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
    if(k==0x4B00)focus=focus==1?3:focus-1;
    else if(k==0x4D00)focus=focus==3?1:focus+1;
    else if(k==13){
      if(focus==3)return 0;
      if(focus==2)return parameter_help_dialog(title,command,executable,parameters);
      if(compose_prompt_command(executable,parameters))return 1;
      notice_box("Parameters Too Long","Shorten the parameters before running.");
    }
  }
}

static int parameter_help_command(const char *title,const char *command)
{
  static char executable[MAX_CMD],parameters[MAX_CMD];
  split_command(command,executable,parameters);
  return parameter_help_dialog(title,command,executable,parameters);
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
static unsigned char explore_classify_drive(unsigned drive)
{
  union REGS r;struct SREGS s;unsigned char far *dpb;
  static unsigned char driver_name[2];
  unsigned offset,segment,word;
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
  int i;
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
  if(focus<4)return focus+1;
  return 0;
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

static int explore_help(char *path,int selected)
{
  char command[MAX_CMD];
  if(!explore_command(path,selected,command))return 0;
  return parameter_help_command(explore_entries[selected].name,command);
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

static void draw_explore_buttons(int x,int y,int focus,int hover)
{
  draw_button(x+4,y+18,"  Run  ",7,focus==1||hover==1);
  draw_button(x+14,y+18,"  Params  ",10,focus==2||hover==2);
  draw_button(x+27,y+18,"  /?  ",7,focus==3||hover==3);
  draw_button(x+61,y+18,"  Cancel  ",10,focus==4||hover==4);
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
    if(redraw){dialog_box(x,y,76,21,"Explore & Run");
    explore_selection_field(x+2,y+2,path,selected);
    cell(x,y+3,195,C_BORDER);cell(x+75,y+3,180,C_BORDER);
    for(i=1;i<75;i++)cell(x+i,y+3,196,C_BORDER);
    explore_drive_bar(x+2,y+4,focus>=5?focus-5:-1);
    cell(x,y+5,195,C_BORDER);cell(x+75,y+5,180,C_BORDER);
    for(i=1;i<75;i++)cell(x+i,y+5,196,C_BORDER);
    cell(x,y+17,195,C_BORDER);cell(x+75,y+17,180,C_BORDER);
    for(i=1;i<75;i++)cell(x+i,y+17,196,C_BORDER);
    for(column=0;column<EXPLORE_COLS;column++)for(row=0;row<EXPLORE_ROWS;row++){
      index=top+column*EXPLORE_ROWS+row;
      if(index<explore_count)textout(x+2+column*18,y+6+row,explore_entries[index].name,
        explore_entry_attribute(index,index==selected||index==hover_entry),16);
      else textout(x+2+column*18,y+6+row,"",C_MENU_BACKGROUND,16);
    }
    cell(x+73,y+6,top>0?30:' ',C_BUTTON);
    cell(x+73,y+16,top+page<explore_count?31:' ',C_BUTTON);
    if(!explore_count)textout(x+2,y+6,"No executable files or directories",C_EMPTY,38);
    draw_explore_buttons(x,y,focus,hover_control);
    redraw=0;}
    wait_input(&k,&mx,&my,&mb);
    if(mb&MOUSE_MOVED){
      int next_hover=-1,next_control=-1;
      if(my>=y+6 && my<y+17 && mx>=x+2 && mx<x+72){
        column=(mx-(x+2))/18;row=my-(y+6);
        index=top+column*EXPLORE_ROWS+row;
        if(index<explore_count)next_hover=index;
      }
      else if(my==y+18 && mx>=x+4 && mx<x+11)next_control=1;
      else if(my==y+18 && mx>=x+14 && mx<x+24)next_control=2;
      else if(my==y+18 && mx>=x+27 && mx<x+34)next_control=3;
      else if(my==y+18 && mx>=x+61 && mx<x+71)next_control=4;
      change_explore_hover(x,y,top,hover_entry,next_hover,selected);
      hover_entry=next_hover;
      if(next_control!=hover_control){hover_control=next_control;
        draw_explore_buttons(x,y,focus,hover_control);}
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
      if(my==y+18 && mx>=x+4 && mx<x+11){
        focus=1;press_button(x+4,y+18,"  Run  ",7);
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
      if(my==y+18 && mx>=x+27 && mx<x+34){
        focus=3;press_button(x+27,y+18,"  /?  ",7);
        if(explore_help(path,selected))return 1;
        redraw=1;continue;
      }
      if(my==y+18 && mx>=x+61 && mx<x+71){
        focus=4;press_button(x+61,y+18,"  Cancel  ",10);return 0;
      }
      continue;
    }
    if(k==27)return 0;
    if(k==9){
      int old_focus=focus;
      focus=explore_tab_focus(focus);wait_vertical_retrace();
      if(old_focus>=5 || focus>=5)
        explore_drive_bar(x+2,y+4,focus>=5?focus-5:-1);
      if((old_focus>=1 && old_focus<=4) || (focus>=1 && focus<=4))
        draw_explore_buttons(x,y,focus,hover_control);
      continue;
    }
    if(focus>=5){
      int old_drive=focus-5,new_drive=old_drive;
      if(k==0x4B00)new_drive=explore_adjacent_drive(old_drive,-1);
      else if(k==0x4D00)new_drive=explore_adjacent_drive(old_drive,1);
      else if(k==0x5000){focus=0;new_drive=-1;}
      else if(k==13){
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
      if(k==0x4B00)focus=focus==1?4:focus-1;
      else if(k==0x4D00)focus=focus==4?1:focus+1;
      else if(k==0x4800)focus=0;
      else if(k==13){
        if(focus==4)return 0;
        if(focus==2){if(explore_params(path,selected))return 1;redraw=1;continue;}
        if(focus==3){if(explore_help(path,selected))return 1;redraw=1;continue;}
        action=explore_activate(path,selected);
        if(action==1)return 1;
        if(action==2){if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");selected=-1;top=focus=0;hover_entry=-1;redraw=1;}
      }
      if(focus!=old_focus){wait_vertical_retrace();
        draw_explore_buttons(x,y,focus,hover_control);}
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
    else if(k==13){
      action=explore_activate(path,selected);
      if(action==1)return 1;
      if(action==2){if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");selected=-1;top=0;hover_entry=-1;redraw=1;}
    }
  }
}

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

static void change_menu_selection(int depth,int *list,int n,int *selection,
                                  int direction,int panel_y)
{
  int old=*selection,next,x=depth*MENU_WIDTH;
  if(!n)return;
  next=(old+n+direction)%n;if(next==old)return;
  if(x+MENU_WIDTH>screen_cols)x=screen_cols-MENU_WIDTH;
  wait_vertical_retrace();
  row_attribute(x+1,panel_y+1+old,18,menu_entry_attribute(list[old],0));
  *selection=next;
  row_attribute(x+1,panel_y+1+next,18,menu_entry_attribute(list[next],1));
}

static void set_menu_selection(int depth,int *list,int *selection,
                               int next,int panel_y)
{
  int old=*selection,x=depth*MENU_WIDTH;
  if(next==old)return;
  if(x+MENU_WIDTH>screen_cols)x=screen_cols-MENU_WIDTH;
  wait_vertical_retrace();
  row_attribute(x+1,panel_y+1+old,18,menu_entry_attribute(list[old],0));
  *selection=next;
  row_attribute(x+1,panel_y+1+next,18,menu_entry_attribute(list[next],1));
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

static int menu(const char *open_to)
{
  static int parent[MAX_DEPTH],sel[MAX_DEPTH],list[MAX_CHILD];
  static int panel_y[MAX_DEPTH],panel_h[MAX_DEPTH],panel_n[MAX_DEPTH];
  static int draw_list[MAX_CHILD],hitlist[MAX_CHILD];
  union REGS clock_regs;
  int depth=0,n,k,i,h,x,y,node,redraw=2,mx=0,my=0,hit,pos;
  unsigned mb,last_mouse_x,last_mouse_y;unsigned char last_second=255;
  unsigned long last_activity,now;
  parent[0]=-1; sel[0]=0;depth=menu_open_folder(open_to,parent,sel);
  video_init();if(!save_screen()){puts("Launch!: insufficient memory");return -1;}
  cursor_hide();mouse_present=mouse_start();mouse_stop();
  last_mouse_x=mouse_raw_x;last_mouse_y=mouse_raw_y;last_activity=bios_ticks();
  for(;;){
    n=menu_children(parent[depth],list);
    if(sel[depth]>=n) sel[depth]=n ? n-1 : 0;
    if(redraw){
      if(redraw==2) restore_screen();
      for(i=0;i<=depth;i++){
        int tn,j;
        tn=menu_children(parent[i],draw_list); h=(tn?tn+2:3)+(i==0?1:0);
        if(h>screen_rows) h=screen_rows;
        if(i==0) y=appearance.menu_top?0:screen_rows-h;
        else {
          y=panel_y[i-1]+1+sel[i-1];
          if(y+h>screen_rows) y=screen_rows-h;
          if(y<0) y=0;
        }
        panel_y[i]=y;panel_h[i]=h;panel_n[i]=tn;
        x=i*MENU_WIDTH;
        if(x+MENU_WIDTH>screen_cols) x=screen_cols-MENU_WIDTH;
        menu_box(x,y,MENU_WIDTH,h,(i==0)?"Launch!":nodes[parent[i]].title,
                 (i==0)?C_ROOT_TITLE:C_TITLE);
        for(j=0;j<tn && j<h-2;j++){
          node=draw_list[j];
          if(node==BUILTIN_EXPLORE){
	    textout(x+1,y+1+j,"Explore & Run",(j==sel[i])?C_SELECTED:C_ITEM,18);
          } else if(node==BUILTIN_POWER){
	    textout(x+1,y+1+j,"Shutdown...",(j==sel[i])?C_SELECTED:C_ITEM,18);
          } else if(nodes[node].separator){
            int a=(j==sel[i])?ATTR(appearance.selected_bg,appearance.border):C_BORDER;
            int sx;for(sx=0;sx<18;sx++)cell(x+1+sx,y+1+j,196,a);
          } else if(nodes[node].folder){
            textout(x+1,y+1+j,nodes[node].title,(j==sel[i])?C_SELECTED:C_FOLDER,16);
            cell(x+17,y+1+j,' ',(j==sel[i])?C_SELECTED:C_FOLDER);
            cell(x+18,y+1+j,16,(j==sel[i])?C_SELECTED:C_FOLDER);
          } else textout(x+1,y+1+j,nodes[node].title,(j==sel[i])?C_SELECTED:C_ITEM,18);
        }
        if(!tn)textout(x+1,y+1,"Empty",C_EMPTY,18);
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
             my<=panel_y[i]+panel_n[i]){
            hit=i;pos=my-panel_y[i]-1;break;
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
          hit=i;if(my>panel_y[i] && my<=panel_y[i]+panel_n[i])pos=my-panel_y[i]-1;break;
        }
      }
      if((mb&2) && hit==0 && my==panel_y[0] && mx>=3 && mx<10){
        close_menu();return BUILTIN_CONFIG;
      }
      if((mb&1) && hit<0){close_menu();return -1;}
      if(hit>=0 && pos>=0){
        int hn=menu_children(parent[hit],hitlist);
        if(pos<hn){
          depth=hit;sel[hit]=pos;node=hitlist[pos];
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
        if(nodes[node].separator)strcpy(question,"Remove this separator from the menu?");
        else sprintf(question,"Remove %s from the menu?",nodes[node].title);
        if(confirm_box("Remove Item",question)){
        delete_tree(node);normalize_order(parent[depth]);if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
        }
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
      if(node==BUILTIN_EXPLORE && k==13){
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

static void build_macro(int node,const char *command,char *text)
{
  char directory[MAX_CMD];text[0]=0;
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
  f=fopen(font_file,"rb");if(!f)return 0;
  offset=((long)id-1L)*4096L;
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
  if(!id){font_bios_standard();return 1;}
  if(_dos_allocmem(256,&segment)!=0)return 0;
  if(!font_read(id,segment)){_dos_freemem(segment);return 0;}
  font_bios_load(segment,0);_dos_freemem(segment);return 1;
}

static int font_commit(unsigned char id)
{
  unsigned resident,temporary;
  if(!font_is_vga())return 1;
  if(!id){
    if(!font_unload())return 0;
    font_bios_standard();return 1;
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
  font_bios_load(resident,FONT_ACTIVE_FONT_OFF);return 1;
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
  dialog_box(x,y,54,10,"Set Keyboard Shortcut");
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
    hit=setkey_stristr(p,"SHORTCUT.COM");
    if(!found && hit && strnicmp(p,"REM",3)!=0){
      key=setkey_stristr(hit,"/KEY=");
      if(key){end=key+5;while(*end && !isspace((unsigned char)*end))end++;
        memmove(key,end,strlen(end)+1);
      }
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
      hit=setkey_stristr(p,"SHORTCUT.COM");
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
    notice_box("Shortcut Error","No active SHORTCUT.COM entry exists in AUTOEXEC.BAT.");
    return 0;
  }
  if(result<0){
    notice_box("Shortcut Error","AUTOEXEC.BAT could not be updated.");return 0;
  }
  return 1;
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
      if(strnicmp(p,"REM",3)!=0&&setkey_stristr(p,"SHORTCUT.COM")){
        fclose(f);return 1;
      }
    }
    fclose(f);
  }
  f=fopen(autoexec,"a+b");if(!f)return 0;
  fseek(f,0L,SEEK_END);size=ftell(f);
  if(size>0){fseek(f,-1L,SEEK_END);last=fgetc(f);fseek(f,0L,SEEK_END);}
  if(size>0&&last!='\n'&&fputs("\r\n",f)==EOF){fclose(f);return 0;}
  if(fprintf(f,"LOADHIGH %sSHORTCUT.COM\r\n",program_dir)<0){fclose(f);return 0;}
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
  strcpy(shortcut,program_dir);strcat(shortcut,"SHORTCUT.COM");
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
  if(result==-1){notice_box("Shortcut Error","SHORTCUT.COM could not be started.");return 0;}
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

static void show_help(void)
{
  puts("Launch! 2.7 - a lightweight command menu for DOS\n");
  puts("Usage: ! [/CONFIG | /EXPLORE | /NOW | /USE=file.mnu | /OPENTO=folder | /?]\n");
  puts("Menu management shortcuts:");
  puts("  Ctrl+A        Add a folder, launcher or separator");
  puts("  Ctrl+D        Delete the selected item");
  puts("  Ctrl+E        Edit the selected item");
  puts("  Ctrl+Up/Down  Move the selected item");
  puts("  Ctrl+S        Sort the current menu\n");
  puts("Command-line parameters:");
  puts("  /CONFIG       Configure menu appearance and options");
  puts("  /EXPLORE      Open Explore & Run directly");
  puts("  /NOW          Start the selected screensaver immediately");
  puts("  /USE=file.mnu Use another menu file beside !.EXE (or a full path)");
  puts("  /OPENTO=name  Open directly to the first folder with this name");
  puts("  /?            Show this help");
}

int main(int argc,char **argv)
{
  int i,result,config_status,config_mode=0,explore_mode=0,now_mode=0;
  static char macro[MAX_MACRO],open_to[MAX_TITLE];
  config_path(argv[0]);
  if(!load_appearance())puts("Launch!: LAUNCH.CFG is invalid; using default appearance.");
  shortcut_idle_sync();
  for(i=1;i<argc;i++){
    if(!stricmp(argv[i],"/?") || !stricmp(argv[i],"-?")){show_help();return 0;}
    if(!stricmp(argv[i],"/CONFIG"))config_mode=1;
    else if(!stricmp(argv[i],"/EXPLORE"))explore_mode=1;
    else if(!stricmp(argv[i],"/NOW"))now_mode=1;
    else if(!strnicmp(argv[i],"/USE=",5)){
      if(!select_menu_file(argv[i]+5)){puts("Launch!: invalid /USE menu filename.");return 1;}
    }
    else if(!strnicmp(argv[i],"/OPENTO=",8)){
      strncpy(open_to,argv[i]+8,MAX_TITLE-1);open_to[MAX_TITLE-1]=0;
    }
    else {printf("Launch!: unknown option %s (use ! /?)\n",argv[i]);return 1;}
  }
  if(!font_commit(appearance.font_id)){
    font_commit(0);puts("Launch!: FONT.DAT could not be read; using the standard VGA font.");
  }
  if(config_mode){configure_appearance();return 0;}
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
  if(config_status==2)puts("Launch!: LAUNCH.MNU was missing or invalid; restored LAUNCH.BAK.");
  else if(config_status==3)puts("Launch!: no valid menu file was found; installed the initial DOS menu.");
  do {
    result=menu(open_to);open_to[0]=0;
    if(result==BUILTIN_CONFIG)configure_appearance();
  } while(result==BUILTIN_CONFIG);
  if(result>=0){
    build_macro(run_node,run_command,macro);
    if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
  }
  return 0;
}
