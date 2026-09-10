/* Launch! 1.8 - modal command menu for DOS
 * Microsoft C/C++ 7.0, small model (.EXE), 286/EGA or later.
 */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <io.h>
#include <process.h>
#include "CLOCKDAT.H"

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
#define EXPLORE_ROWS 15
#define EXPLORE_COLS 4
#define MAKE_FP(seg,off) ((void far *)((((unsigned long)(seg))<<16) | \
                                      (unsigned short)(off)))

typedef struct {
  unsigned char background,border,main_title,titles,folders,launchers;
  unsigned char selected_fg,selected_bg,controls_fg,controls_bg,labels;
  unsigned char menu_top,show_explore,show_power,show_time;
  unsigned char screensaver,saver_color,hour_12;
} APPEARANCE;

static const APPEARANCE default_appearance={1,11,12,14,15,10,15,3,0,7,7,0,1,1,1,1,10,1};
static APPEARANCE appearance={1,11,12,14,15,10,15,3,0,7,7,0,1,1,1,1,10,1};

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
static unsigned short *saved;
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
static char write_path[MAX_CMD];
static unsigned char copy_buffer[512];
static unsigned char key_shift;
static unsigned char key_scan;
static int mouse_present;
static unsigned mouse_last_buttons;
static unsigned mouse_raw_x,mouse_raw_y;
static int added_visible_node;
static char help_lines[MAX_HELP_LINES][HELP_WIDTH+1];
static int help_line_count;

#define BUILTIN_EXPLORE (-3)
#define BUILTIN_POWER (-2)
#define SCREENSAVER_TICKS 1092UL

typedef struct {
  char name[13];
  unsigned char directory;
} EXPLORE_ENTRY;

static EXPLORE_ENTRY explore_entries[MAX_EXPLORE_ENTRIES];
static int explore_count;

static const char *sample_config[] = {
  "; Launch! 1.8 menu definition\n",
  "; ITEM=title|command and parameters|press Enter|change directory|prompt (0/1)\n",
  "; SEPARATOR= adds a movable horizontal separator\n",
  "\n",
  "[Launcher]\n",
  "FOLDER=Office\n",
  "FOLDER=Internet\n",
  "FOLDER=Multimedia\n",
  "FOLDER=Games\n",
  "FOLDER=Utilities\n",
  "FOLDER=System Tools\n",
  "\n",
  "[Launcher\\Office]\n",
  "ITEM=WordPerfect 5.1|C:\\WP51\\WP.EXE|1|1|0\n",
  "ITEM=Microsoft Word 5.5|C:\\WORD55\\WORD.EXE|1|1|0\n",
  "ITEM=Lotus 1-2-3|C:\\123R34\\123.EXE|1|1|0\n",
  "\n",
  "[Launcher\\Internet]\n",
  "ITEM=FTP2P|C:\\MTCP\\FTP2P.EXE|1|0|0\n",
  "ITEM=Telnet|C:\\MTCP\\TELNET.EXE|1|0|0\n",
  "ITEM=LYNX|C:\\LYNX\\LYNX.EXE|1|0|0\n",
  "ITEM=Arachne|C:\\ARACHNE\\ARACHNE.BAT|1|1|0\n",
  "FOLDER=Communications\n",
  "\n",
  "[Launcher\\Internet\\Communications]\n",
  "ITEM=Pegasus|C:\\COMM\\PEGASUS.EXE|1|0|0\n",
  "ITEM=Pine|C:\\COMM\\PINE.EXE|1|0|0\n",
  "ITEM=Compuserve|C:\\COMM\\CIS.BAT|1|0|0\n",
  "ITEM=Prodigy|C:\\COMM\\PRODIGY.BAT|1|0|0\n",
  "\n",
  "[Launcher\\Utilities]\n",
  "ITEM=Font Selector|C:\\UTILS\\FONTSEL.COM|1|0|0\n",
  "ITEM=File Manager|C:\\4DOS\\4START.BAT|1|0|0\n",
  "\n",
  "[Launcher\\System Tools]\n",
  "ITEM=Memory Report|MEM /C /P|1|0|0\n",
  "ITEM=System Information|C:\\UTILS\\NSSI.EXE|1|0|0\n",
  0
};

static void cursor_restore(void);
static void mouse_stop(void);
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
  saved=(unsigned short *)malloc(n*sizeof(unsigned short));
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

static void close_menu(void)
{
  mouse_stop();restore_screen();cursor_restore();free(saved);saved=0;
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
  strncpy(config_file,exe,n);config_file[n]=0;strcat(config_file,"LAUNCH.MNU");
  strncpy(backup_file,exe,n);backup_file[n]=0;strcat(backup_file,"LAUNCH.BAK");
  strncpy(temp_file,exe,n);temp_file[n]=0;strcat(temp_file,"LAUNCH.$$$");
  strncpy(backup_temp_file,exe,n);backup_temp_file[n]=0;strcat(backup_temp_file,"LAUNCH.BK$");
  strncpy(bad_file,exe,n);bad_file[n]=0;strcat(bad_file,"LAUNCH.BAD");
  strncpy(appearance_file,exe,n);appearance_file[n]=0;strcat(appearance_file,"LAUNCH.CFG");
  strncpy(appearance_temp_file,exe,n);appearance_temp_file[n]=0;strcat(appearance_temp_file,"LAUNCH.CF$");
  strncpy(logos_file,exe,n);logos_file[n]=0;strcat(logos_file,"PWROFF.BMP");
  strncpy(help_file,exe,n);help_file[n]=0;strcat(help_file,"LAUNCH.HLP");
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
  else if(!stricmp(key,"SCREENSAVER")){field=&a->screensaver;limit=1;}
  else if(!stricmp(key,"SAVER_COLOR"))field=&a->saver_color;
  else if(!stricmp(key,"HOUR_12")){field=&a->hour_12;limit=1;}
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

/* Exact 640x350 raster spans generated from the supplied VFD SVG artwork. */
static const unsigned char segment_mask[10]={
  0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F
};

static unsigned char far *ega_memory=(unsigned char far *)MAKE_FP(0xA000,0);

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

static void draw_clock_shape(int x,int y,const CLOCK_SHAPE *shape,
                             unsigned char colour)
{
  int row;const CLOCK_SPAN *span=shape->rows;
  for(row=0;row<shape->count;row++,span++)if(span->left!=255)
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

static void set_ega_clock_mode(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x0010;int86(0x10,&r,&r);
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
                      CLOCK_AM_STRIDE,!is_pm?colour:8);
    draw_clock_bitmap(16,191,clock_pm_bits,CLOCK_PM_WIDTH,CLOCK_PM_HEIGHT,
                      CLOCK_PM_STRIDE,is_pm?colour:8);
  }
  return r.h.dh;
}

static void mouse_show(void)
{
  union REGS r;if(!mouse_present)return;
  memset(&r,0,sizeof(r));r.x.ax=1;int86(0x33,&r,&r);
}

static void clock_screensaver(void)
{
  union REGS r;int mx=0,my=0,old_mode,old_rows=screen_rows;
  unsigned buttons,start_x,start_y;
  unsigned char second=255;signed char previous_digits[4]={-2,-2,-2,-2};
  mouse_stop();
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);old_mode=r.h.al;
  set_ega_clock_mode();
  /* A mode change can rescale or reset the mouse driver's coordinates.  Take
   * the inactivity baseline afterward so that this is not mistaken for real
   * mouse movement and used to dismiss the clock immediately. */
  (void)mouse_poll(&mx,&my);start_x=mouse_raw_x;start_y=mouse_raw_y;
  for(;;){
    second=draw_graphics_time(second,previous_digits);
    if(key_waiting()){keyread();break;}
    buttons=mouse_poll(&mx,&my);
    if(buttons || mouse_raw_x!=start_x || mouse_raw_y!=start_y)break;
  }
  memset(&r,0,sizeof(r));r.h.al=(unsigned char)old_mode;int86(0x10,&r,&r);
  if(old_rows>25){memset(&r,0,sizeof(r));r.x.ax=0x1112;r.h.bl=0;int86(0x10,&r,&r);}
  video_init();
  memset(&r,0,sizeof(r));r.h.ah=1;r.h.ch=0x20;r.h.cl=0;int86(0x10,&r,&r);
  mouse_show();
}

static int mouse_start(void)
{
  union REGS r;
  r.x.ax=0;int86(0x33,&r,&r);if(r.x.ax==0)return 0;
  r.x.ax=4;r.x.cx=1;r.x.dx=1;int86(0x33,&r,&r);
  r.x.ax=1;int86(0x33,&r,&r);
  r.x.ax=3;int86(0x33,&r,&r);mouse_last_buttons=r.x.bx;
  mouse_raw_x=r.x.cx;mouse_raw_y=r.x.dx;
  return 1;
}

static void mouse_stop(void)
{
  union REGS r;if(!mouse_present)return;r.x.ax=2;int86(0x33,&r,&r);
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
  *key=0;*buttons=0;
  do {
    if(key_waiting()){*key=keyread();return;}
    *buttons=mouse_poll(column,row);
  } while(!*buttons);
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
  ok=fputs("; Launch! 1.8 menu definition\n; ITEM=title|command and parameters|press Enter|change directory|prompt (0/1)\n; SEPARATOR= adds a movable horizontal separator\n\n",f)!=EOF;
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
  if(fputs("; Launch! 1.8 appearance settings\n",f)==EOF)ok=0;
  if(ok && fprintf(f,"BACKGROUND=%u\nBORDER=%u\nMAIN_TITLE=%u\nTITLES=%u\n"
      "FOLDERS=%u\nLAUNCHERS=%u\nSELECTED_FG=%u\nSELECTED_BG=%u\n"
      "CONTROLS_FG=%u\nCONTROLS_BG=%u\nLABELS=%u\nMENU_TOP=%u\nSCREENSAVER=%u\nSAVER_COLOR=%u\nHOUR_12=%u\nSHOW_EXPLORE=%u\nSHOW_POWER=%u\nSHOW_TIME=%u\n",
      appearance.background,appearance.border,appearance.main_title,appearance.titles,
      appearance.folders,appearance.launchers,appearance.selected_fg,appearance.selected_bg,
      appearance.controls_fg,appearance.controls_bg,appearance.labels,
      appearance.menu_top,appearance.screensaver,appearance.saver_color,appearance.hour_12,
      appearance.show_explore,appearance.show_power,
      appearance.show_time)<0)ok=0;
  if(fclose(f)!=0)ok=0;
  if(!ok){remove(appearance_temp_file);return 0;}
  remove(appearance_file);
  if(rename(appearance_temp_file,appearance_file)!=0)return 0;
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
    if((mb&1) && my==y+4){
      if(mx>=x+14 && mx<x+21)return 1;
      if(mx>=x+28 && mx<x+34)return 0;
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
  int x=(screen_cols-48)/2,y=(screen_rows-7)/2,k,mx,my;unsigned mb;
  dialog_box(x,y,48,7,title); textout(x+3,y+2,message,C_FOLDER,42);
  draw_button(x+21,y+4,"  OK  ",6,1);wait_input(&k,&mx,&my,&mb);
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
  wait_for_escape();restore_screen();cursor_restore();free(saved);saved=0;
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
  free(saved);saved=0;return !failed;
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
    if((mb&1) && my==y+4){
      if(mx>=x+3 && mx<x+16)return 1;
      if(mx>=x+20 && mx<x+30)return 2;
      if(mx>=x+39 && mx<x+49)return 0;
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
    if((mb&1) && my==y+4){
      if(mx>=x+5 && mx<x+15)return 1;
      if(mx>=x+20 && mx<x+32)return 0;
      if(mx>=x+37 && mx<x+50)return 2;
    }
    if(k==27) return -1;
    if(k==0x4B00){choice=(choice+2)%3;}
    else if(k==0x4D00 || k==9){choice=(choice+1)%3;}
    else if(k==13)return choice==0?1:(choice==1?0:2);
  }
}

static void field_line(int x,int y,const char *label,const char *value,int selected,int pos,int width)
{
  int len=strlen(label),i,vlen=strlen(value),at=selected?C_SELECTED:C_INPUT_FIELD,scroll=0;
  if(pos>=width)scroll=pos-width+1;
  textout(x,y,label,C_INPUT_LABEL,len); textout(x+13,y,value+scroll,at,width);
  if(selected){i=pos-scroll;cell(x+13+i,y,(pos<vlen)?value[pos]:' ',C_INPUT_FIELD);}
}

static void check_line(int x,int y,const char *label,int checked,int focused)
{
  char mark[4];
  sprintf(mark,"[%c]",checked?'þ':' ');
  textout(x,y,mark,C_BUTTON,3);
  textout(x+4,y,label,focused?C_SELECTED:C_INPUT_LABEL,34);
}

static int item_form(int folder,char *name,char *exe,char *params,
                     int *press_enter,int *change_dir,int *prompt_params,int editing)
{
  char *fields[3]; int limits[3],pos[3],count,controls,focus=0,k,x,y,i,len,mx=0,my=0,scroll;
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
    field_line(x+3,y+2,"Name:",name,focus==0,pos[0],42);
    if(!folder){
      field_line(x+3,y+4,"Command:",exe,focus==1,pos[1],42);
      field_line(x+3,y+6,"Parameters:",params,focus==2,pos[2],28);
      textout(x+47,y+6,"Prompt?",focus==3?C_SELECTED:C_INPUT_LABEL,7);
      textout(x+55,y+6,*prompt_params?"[þ]":"[ ]",C_BUTTON,3);
    }
    if(!folder){
      check_line(x+16,y+8,"Provide \021\331 after launcher command",*press_enter,focus==4);
      check_line(x+16,y+9,"Change directory first",*change_dir,focus==5);
    }
    draw_button(x+16,y+(folder?7:11),"  Save  ",8,focus==controls);
    draw_button(x+28,y+(folder?7:11),"  Cancel  ",10,focus==controls+1);
    wait_input(&k,&mx,&my,&mb);
    if(mb&1){
      if(my==y+2 && mx>=x+16 && mx<x+58){focus=0;i=0;}
      else if(!folder && my==y+4 && mx>=x+16 && mx<x+58){focus=1;i=1;}
      else if(!folder && my==y+6 && mx>=x+16 && mx<x+44){focus=2;i=2;}
      else if(!folder && my==y+6 && mx>=x+47 && mx<x+58){*prompt_params=!*prompt_params;focus=3;continue;}
      else if(!folder && my==y+8 && mx>=x+16 && mx<x+54){*press_enter=!*press_enter;focus=4;continue;}
      else if(!folder && my==y+9 && mx>=x+16 && mx<x+54){*change_dir=!*change_dir;focus=5;continue;}
      else if(my==y+(folder?7:11) && mx>=x+16 && mx<x+24){
        if(*name && (folder || *exe))return 1;
        focus=controls;continue;
      }
      else if(my==y+(folder?7:11) && mx>=x+28 && mx<x+38)return 0;
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

static void cycle_control(int x,int y,const char *value,int focused)
{
  char field[16];
  sprintf(field,"[ %-11s ]",value);
  textout(x,y,field,focused?C_SELECTED:C_BUTTON,15);
}

static unsigned char *appearance_field(int focus,int *limit)
{
  *limit=15;
  switch(focus){
    case 0:*limit=7;return &appearance.background;
    case 1:return &appearance.border;
    case 2:return &appearance.main_title;
    case 3:return &appearance.titles;
    case 4:return &appearance.folders;
    case 5:return &appearance.launchers;
    case 6:return &appearance.selected_fg;
    case 7:*limit=7;return &appearance.selected_bg;
    case 8:return &appearance.controls_fg;
    case 9:*limit=7;return &appearance.controls_bg;
    case 10:return &appearance.labels;
    case 11:*limit=1;return &appearance.menu_top;
    case 12:*limit=1;return &appearance.screensaver;
    case 13:return &appearance.saver_color;
    case 17:*limit=1;return &appearance.hour_12;
  }
  return 0;
}

static void change_appearance_value(int focus,int direction)
{
  unsigned char *field;int limit,value;
  if(focus==14){appearance.show_explore=!appearance.show_explore;return;}
  if(focus==15){appearance.show_power=!appearance.show_power;return;}
  if(focus==16){appearance.show_time=!appearance.show_time;return;}
  field=appearance_field(focus,&limit);if(!field)return;
  value=(int)*field+direction;
  if(value<0)value=limit;
  if(value>limit)value=0;
  *field=(unsigned char)value;
}

static int configure_appearance(void)
{
  APPEARANCE original=appearance;int x,y,k=0,mx=0,my=0,focus=0,row=-1,redraw=1;
  unsigned mb=0;static const int rows[18]={2,3,4,5,6,7,8,8,9,9,10,12,13,13,15,16,17,17};
  video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 0;}
  cursor_hide();mouse_present=mouse_start();
  x=(screen_cols-64)/2;y=(screen_rows-22)/2;
  for(;;){
    wait_vertical_retrace();
    if(redraw){dialog_box(x,y,64,22,"Configure Appearance");redraw=0;}
    textout(x+3,y+2,"Background",C_INPUT_LABEL,18);
    textout(x+3,y+3,"Border",C_INPUT_LABEL,18);
    textout(x+3,y+4,"Main Title",C_INPUT_LABEL,18);
    textout(x+3,y+5,"Titles",C_INPUT_LABEL,18);
    textout(x+3,y+6,"Folders",C_INPUT_LABEL,18);
    textout(x+3,y+7,"Launchers",C_INPUT_LABEL,18);
    textout(x+3,y+8,"Selected items",C_INPUT_LABEL,18);
    textout(x+3,y+9,"Controls",C_INPUT_LABEL,18);
    textout(x+3,y+10,"Labels",C_INPUT_LABEL,18);
    cycle_control(x+22,y+2,colour_names[appearance.background],focus==0);
    cycle_control(x+22,y+3,colour_names[appearance.border],focus==1);
    cycle_control(x+22,y+4,colour_names[appearance.main_title],focus==2);
    cycle_control(x+22,y+5,colour_names[appearance.titles],focus==3);
    cycle_control(x+22,y+6,colour_names[appearance.folders],focus==4);
    cycle_control(x+22,y+7,colour_names[appearance.launchers],focus==5);
    cycle_control(x+22,y+8,colour_names[appearance.selected_fg],focus==6);
    cycle_control(x+41,y+8,colour_names[appearance.selected_bg],focus==7);
    cycle_control(x+22,y+9,colour_names[appearance.controls_fg],focus==8);
    cycle_control(x+41,y+9,colour_names[appearance.controls_bg],focus==9);
    cycle_control(x+22,y+10,colour_names[appearance.labels],focus==10);
    textout(x+3,y+12,"Menu position",C_INPUT_LABEL,18);
    cycle_control(x+22,y+12,appearance.menu_top?"Top":"Bottom",focus==11);
    textout(x+3,y+13,"Screensaver",C_INPUT_LABEL,18);
    cycle_control(x+22,y+13,appearance.screensaver?"Clock":"None",focus==12);
    cycle_control(x+41,y+13,colour_names[appearance.saver_color],focus==13);
    check_line(x+22,y+15,"Show 'Explore & Run' menu item",appearance.show_explore,focus==14);
    check_line(x+22,y+16,"Show 'Shutdown...' menu item",appearance.show_power,focus==15);
    check_line(x+22,y+17,"Show the time",appearance.show_time,focus==16);
    cycle_control(x+41,y+17,appearance.hour_12?"12-hour":"24-hour",focus==17);
    draw_button(x+19,y+19,"  Save  ",8,focus==18);
    draw_button(x+34,y+19,"  Cancel  ",10,focus==19);
    wait_input(&k,&mx,&my,&mb);
    if(mb){
      row=-1;
      if(mx>=x+22 && mx<x+37){
        int i;for(i=0;i<18;i++)if(my==y+rows[i]){row=i;break;}
      }
      if(mx>=x+41 && mx<x+56 && my==y+8)row=7;
      if(mx>=x+41 && mx<x+56 && my==y+9)row=9;
      if(mx>=x+41 && mx<x+56 && my==y+13)row=13;
      if(mx>=x+41 && mx<x+56 && my==y+17)row=17;
      if(row>=0){focus=row;change_appearance_value(focus,(mb&2)?-1:1);redraw=1;continue;}
      if(my==y+19 && (mb&1)){
        if(mx>=x+19 && mx<x+27){
          if(save_appearance()){close_menu();return 1;}
          notice_box("Write Error","Could not update LAUNCH.CFG.");redraw=1;continue;
        }
        if(mx>=x+34 && mx<x+44){appearance=original;close_menu();return 0;}
      }
      continue;
    }
    if(k==27){appearance=original;close_menu();return 0;}
    if(k==9 || k==0x5000){focus=(focus+1)%20;continue;}
    if(k==0x4800){focus=(focus+19)%20;continue;}
    if(k==0x4B00){change_appearance_value(focus,-1);redraw=1;continue;}
    if(k==0x4D00 || k==' '){change_appearance_value(focus,1);redraw=1;continue;}
    if(k==13){
      if(focus<18){focus++;continue;}
      if(focus==18){
        if(save_appearance()){close_menu();return 1;}
        notice_box("Write Error","Could not update LAUNCH.CFG.");redraw=1;continue;
      }
      appearance=original;close_menu();return 0;
    }
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
  FILE *out;int saved_out,saved_err,result;union REGS r;
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
  if(mouse_present){memset(&r,0,sizeof(r));r.x.ax=1;int86(0x33,&r,&r);}
  read_help_output();
}

static void parameter_field(int x,int y,const char *value,int position,int focused,int width)
{
  int length=strlen(value),scroll=0,cursor;
  if(position>=width)scroll=position-width+1;
  textout(x,y,value+scroll,focused?C_SELECTED:C_INPUT_FIELD,width);
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

static int parameter_prompt_command(const char *title,const char *command)
{
  int x=(screen_cols-76)/2,y=(screen_rows-22)/2,k=0,mx=0,my=0;
  int focus=1,scroll=0,position,length,i,max_scroll;unsigned mb=0;
  static char executable[MAX_CMD],parameters[MAX_CMD];char filename[19],section[48];
  split_command(command,executable,parameters);position=strlen(parameters);
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
    textout(x+2,y+16,"/ and PgUp/PgDn scrolls command help.",focus==0?C_SELECTED:C_INPUT_LABEL,52);
    parameter_field(x+2,y+19,parameters,position,focus==1,44);
    draw_button(x+49,y+19,"  Run  ",7,focus==2);
    draw_button(x+61,y+19,"  Cancel  ",10,focus==3);
    wait_input(&k,&mx,&my,&mb);
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
        if(compose_prompt_command(executable,parameters))return 1;
        notice_box("Parameters Too Long","Shorten the parameters before running.");continue;
      }
      if(my==y+19 && mx>=x+61 && mx<x+71)return 0;
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

static int explore_help(char *path,int selected)
{
  char command[MAX_CMD];int needed;
  if(selected<0 || selected>=explore_count || explore_entries[selected].directory){notice_box("Parameter Help","Select an executable file first.");return 0;}
  needed=strlen(path)+strlen(explore_entries[selected].name);
  if(needed>=MAX_CMD){notice_box("Path Too Long","That executable path is too long.");return 0;}
  strcpy(command,path);strcat(command,explore_entries[selected].name);
  return parameter_prompt_command(explore_entries[selected].name,command);
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
  row_attribute(x+2+column*18,y+4+row,16,
                explore_entry_attribute(index,selected));
}

static void select_explore_entry(int x,int y,int next,int *selected,int *top,
                                 int page,int *redraw)
{
  int old=*selected,new_top=(next/page)*page;
  if(next<0 || next>=explore_count || next==old)return;
  if(new_top!=*top){*selected=next;*top=new_top;*redraw=1;return;}
  wait_vertical_retrace();
  highlight_explore_entry(x,y,*top,old,0);*selected=next;
  highlight_explore_entry(x,y,*top,next,1);
}

static int explore_dialog(void)
{
  int x=(screen_cols-76)/2,y=(screen_rows-23)/2,k,mx=0,my=0;
  int selected=0,top=0,page=EXPLORE_ROWS*EXPLORE_COLS,focus=0,redraw=1;
  int i,row,column,index,action;unsigned mb;
  int last_click=-1;unsigned long last_click_tick=0,tick;
  static char path[MAX_CMD]="C:\\";
  strcpy(path,"C:\\");
  if(!explore_load(path)){notice_box("Explore Error","Unable to read drive C:.");return 0;}
  for(;;){
    if(selected>=explore_count)selected=explore_count?explore_count-1:0;
    if(selected<top)top=(selected/page)*page;
    if(selected>=top+page)top=(selected/page)*page;
    if(redraw){dialog_box(x,y,76,23,"Explore & Run");
    explore_path_field(x+2,y+2,path);
    cell(x,y+3,195,C_BORDER);cell(x+75,y+3,180,C_BORDER);
    cell(x,y+19,195,C_BORDER);cell(x+75,y+19,180,C_BORDER);
    for(i=1;i<75;i++){cell(x+i,y+3,196,C_BORDER);cell(x+i,y+19,196,C_BORDER);}
    for(column=0;column<EXPLORE_COLS;column++)for(row=0;row<EXPLORE_ROWS;row++){
      index=top+column*EXPLORE_ROWS+row;
      if(index<explore_count)textout(x+2+column*18,y+4+row,explore_entries[index].name,
        explore_entry_attribute(index,index==selected),16);
      else textout(x+2+column*18,y+4+row,"",C_MENU_BACKGROUND,16);
    }
    cell(x+73,y+4,top>0?30:' ',C_BUTTON);
    cell(x+73,y+18,top+page<explore_count?31:' ',C_BUTTON);
    if(!explore_count)textout(x+2,y+4,"No executable files or directories",C_EMPTY,38);
    draw_button(x+4,y+20,"  Run  ",7,focus==1);
    draw_button(x+14,y+20,"  /?  ",7,focus==2);
    draw_button(x+61,y+20,"  Cancel  ",10,focus==3);
    redraw=0;}
    wait_input(&k,&mx,&my,&mb);
    if(mb&1){
      if(my>=y+4 && my<y+19 && mx>=x+2 && mx<x+74){
        focus=0;
        if(mx>=x+72){
          if(my<y+11 && top>0){top-=page;selected=top;redraw=1;}
          else if(my>=y+11 && top+page<explore_count){top+=page;selected=top;redraw=1;}
          last_click=-1;
        } else {
          column=(mx-(x+2))/18;row=my-(y+4);index=top+column*EXPLORE_ROWS+row;
          if(index<explore_count){
            tick=*(unsigned long far *)MAKE_FP(0x40,0x6C);
            if(index==last_click && tick>=last_click_tick && tick-last_click_tick<=9UL){
              action=explore_activate(path,index);last_click=-1;
              if(action==1)return 1;
              if(action==2){
                if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");
                selected=top=0;redraw=1;
              }
            } else {
              select_explore_entry(x,y,index,&selected,&top,page,&redraw);
              last_click=index;last_click_tick=tick;
            }
          }
        }
        continue;
      }
      if(my==y+20 && mx>=x+4 && mx<x+11){
        focus=1;
        action=explore_activate(path,selected);
        last_click=-1;
        if(action==1)return 1;
        if(action==2){if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");selected=top=focus=0;redraw=1;}
        continue;
      }
      if(my==y+20 && mx>=x+14 && mx<x+21){focus=2;if(explore_help(path,selected))return 1;redraw=1;continue;}
      if(my==y+20 && mx>=x+61 && mx<x+71){focus=3;return 0;}
      continue;
    }
    if(k==27)return 0;
    if(k==9){focus=(focus+1)%4;redraw=1;continue;}
    if(focus){
      if(k==0x4B00)focus=focus==1?3:focus-1;
      else if(k==0x4D00)focus=focus==3?1:focus+1;
      else if(k==0x4800)focus=0;
      else if(k==13){
        if(focus==3)return 0;
        if(focus==2){if(explore_help(path,selected))return 1;redraw=1;continue;}
        action=explore_activate(path,selected);
        if(action==1)return 1;
        if(action==2){if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");selected=top=focus=0;redraw=1;}
      }
      redraw=1;
      continue;
    }
    if(k==0x4800 && selected>0)select_explore_entry(x,y,selected-1,&selected,&top,page,&redraw);
    else if(k==0x5000 && selected+1<explore_count)select_explore_entry(x,y,selected+1,&selected,&top,page,&redraw);
    else if(k==0x4B00){if(selected>=EXPLORE_ROWS)select_explore_entry(x,y,selected-EXPLORE_ROWS,&selected,&top,page,&redraw);else if(!explore_is_root(path)){explore_parent(path);explore_load(path);selected=top=0;redraw=1;}}
    else if(k==0x4D00 && selected+EXPLORE_ROWS<explore_count)select_explore_entry(x,y,selected+EXPLORE_ROWS,&selected,&top,page,&redraw);
    else if(k==0x4900){top=top>=page?top-page:0;selected=top;redraw=1;}
    else if(k==0x5100 && top+page<explore_count){top+=page;selected=top;redraw=1;}
    else if(k==8 && !explore_is_root(path)){explore_parent(path);explore_load(path);selected=top=0;redraw=1;}
    else if(k==13){
      action=explore_activate(path,selected);
      if(action==1)return 1;
      if(action==2){if(!explore_load(path))notice_box("Explore Error","Unable to read that directory.");selected=top=0;redraw=1;}
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

static int menu(void)
{
  static int parent[MAX_DEPTH],sel[MAX_DEPTH],list[MAX_CHILD];
  static int panel_y[MAX_DEPTH],panel_h[MAX_DEPTH],panel_n[MAX_DEPTH];
  static int draw_list[MAX_CHILD],hitlist[MAX_CHILD];
  int depth=0,n,k,i,h,x,y,node,redraw=2,mx=0,my=0,hit,pos;
  unsigned mb,last_mouse_x,last_mouse_y;unsigned char last_second=255;
  unsigned long last_activity,now;
  parent[0]=-1; sel[0]=0;
  video_init();if(!save_screen()){puts("Launch!: insufficient memory");return -1;}
  cursor_hide();mouse_present=mouse_start();
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
    k=0;mb=0;
    do {
      if(key_waiting()){k=keyread();break;}
      mb=mouse_poll(&mx,&my);
      if(mouse_raw_x!=last_mouse_x || mouse_raw_y!=last_mouse_y){
        last_mouse_x=mouse_raw_x;last_mouse_y=mouse_raw_y;last_activity=bios_ticks();
      }
      if(appearance.show_time)
        last_second=draw_clock(panel_y[0]+panel_h[0]-1,last_second);
      now=bios_ticks();
      if(appearance.screensaver && elapsed_ticks(last_activity,now)>=SCREENSAVER_TICKS){
        clock_screensaver();
        last_mouse_x=mouse_raw_x;last_mouse_y=mouse_raw_y;
        last_activity=bios_ticks();redraw=2;break;
      }
    } while(!mb);

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

static void show_help(void)
{
  puts("Launch! 1.8 - a lightweight command menu for DOS\n");
  puts("Usage: ! [/CONFIG | /?]\n");
  puts("Menu management shortcuts:");
  puts("  Ctrl+A        Add a folder, launcher or separator");
  puts("  Ctrl+D        Delete the selected item");
  puts("  Ctrl+E        Edit the selected item");
  puts("  Ctrl+Up/Down  Move the selected item");
  puts("  Ctrl+S        Sort the current menu\n");
  puts("Command-line parameters:");
  puts("  /CONFIG       Configure menu appearance and options");
  puts("  /?            Show this help");
}

int main(int argc,char **argv)
{
  int i,config_status,config_mode=0,now_mode=0;static char macro[MAX_MACRO];
  config_path(argv[0]);
  if(!load_appearance())puts("Launch!: LAUNCH.CFG is invalid; using default appearance.");
  for(i=1;i<argc;i++){
    if(!stricmp(argv[i],"/?") || !stricmp(argv[i],"-?")){show_help();return 0;}
    if(!stricmp(argv[i],"/CONFIG"))config_mode=1;
    else if(!stricmp(argv[i],"/NOW"))now_mode=1;
    else {printf("Launch!: unknown option %s (use ! /?)\n",argv[i]);return 1;}
  }
  if(config_mode){configure_appearance();return 0;}
  if(now_mode){
    video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 1;}
    cursor_hide();mouse_present=mouse_start();clock_screensaver();close_menu();return 0;
  }
  config_status=prepare_config();
  if(!config_status){printf("Launch!: cannot recover %s\n",config_file);return 1;}
  if(config_status==2)puts("Launch!: LAUNCH.MNU was missing or invalid; restored LAUNCH.BAK.");
  else if(config_status==3)puts("Launch!: no valid menu file was found; installed the sample menu.");
  if(menu()>=0){
    build_macro(run_node,run_command,macro);
    if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
  }
  return 0;
}
