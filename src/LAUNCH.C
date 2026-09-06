/* Launch! 0.6 - modal command menu for DOS
 * Microsoft C/C++ 7.0, small model (.EXE), 286/EGA or later.
 */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NODES 96
#define MAX_TITLE 24
#define MAX_CMD 128
#define MAX_MACRO 384
#define MAX_DEPTH 4
#define MAX_CHILD 21
#define MENU_CAPACITY 20
#define MENU_WIDTH 20
#define MAKE_FP(seg,off) ((void far *)((((unsigned long)(seg))<<16) | \
                                      (unsigned short)(off)))

typedef struct {
  unsigned char background,border,main_title,titles,folders,launchers;
  unsigned char selected_fg,selected_bg,controls_fg,controls_bg,labels;
  unsigned char menu_top,show_time;
} APPEARANCE;

static const APPEARANCE default_appearance={1,11,12,14,15,10,15,3,0,7,7,0,1};
static APPEARANCE appearance={1,11,12,14,15,10,15,3,0,7,7,0,1};

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
#define C_FAUX_SHADOW     0x08  /* dark grey on black */
#define C_BLOCK_SHADOW_FG 0x00  /* black half-block foreground */

typedef struct {
  char title[MAX_TITLE];
  char command[MAX_CMD];
  int parent;
  unsigned char folder;
  unsigned char active;
  unsigned char order;
  unsigned char press_enter;
  unsigned char change_dir;
} NODE;

/* Preassembled 8086 keyboard helper.  The final 384 bytes are its queue. */
#define MACRO_BLOB_SIZE 559
#define MACRO_INT16_OFF 0x0000
#define MACRO_INT2F_OFF 0x0066
#define MACRO_OLD16_OFF 0x00A3
#define MACRO_OLD2F_OFF 0x00A7
static unsigned char macro_blob[MACRO_BLOB_SIZE]={
128,252,0,116,20,128,252,16,116,15,128,252,1,116,47,128,
252,17,116,42,46,255,46,163,0,83,46,139,30,171,0,46,
59,30,173,0,115,21,46,138,135,175,0,67,46,137,30,171,
0,48,228,60,13,117,2,180,28,91,207,91,235,214,85,137,
229,83,46,139,30,171,0,46,59,30,173,0,115,20,46,138,
135,175,0,48,228,60,13,117,2,180,28,131,102,6,191,91,
93,207,91,93,235,174,61,160,213,116,10,61,161,213,116,9,
46,255,46,167,0,187,72,76,207,80,81,86,87,6,14,7,
49,255,46,137,62,171,0,129,249,128,1,118,3,185,128,1,
46,137,14,173,0,191,175,0,252,243,164,7,95,94,89,88,
48,192,207
};

static NODE nodes[MAX_NODES];
static int node_count;
static int screen_cols, screen_rows;
static unsigned short far *video;
static unsigned short *saved;
static unsigned char cursor_start,cursor_end;
static int run_node;
static char config_file[MAX_CMD];
static char backup_file[MAX_CMD];
static char temp_file[MAX_CMD];
static char backup_temp_file[MAX_CMD];
static char bad_file[MAX_CMD];
static char appearance_file[MAX_CMD];
static char appearance_temp_file[MAX_CMD];
static char write_path[MAX_CMD];
static unsigned char copy_buffer[512];
static unsigned char key_shift;
static unsigned char key_scan;
static int mouse_present;
static unsigned mouse_last_buttons;
static int added_visible_node;

static const char *sample_config[] = {
  "; Launch! 0.6 menu definition\n",
  "; ITEM=title|command and parameters|press Enter (0/1)|change directory (0/1)\n",
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
  "ITEM=WordPerfect 5.1|C:\\WP51\\WP.EXE|1|1\n",
  "ITEM=Microsoft Word 5.5|C:\\WORD55\\WORD.EXE|1|1\n",
  "ITEM=Lotus 1-2-3|C:\\123R34\\123.EXE|1|1\n",
  "\n",
  "[Launcher\\Internet]\n",
  "ITEM=FTP2P|C:\\MTCP\\FTP2P.EXE|1|0\n",
  "ITEM=Telnet|C:\\MTCP\\TELNET.EXE|1|0\n",
  "ITEM=LYNX|C:\\LYNX\\LYNX.EXE|1|0\n",
  "ITEM=Arachne|C:\\ARACHNE\\ARACHNE.BAT|1|1\n",
  "FOLDER=Communications\n",
  "\n",
  "[Launcher\\Internet\\Communications]\n",
  "ITEM=Pegasus|C:\\COMM\\PEGASUS.EXE|1|0\n",
  "ITEM=Pine|C:\\COMM\\PINE.EXE|1|0\n",
  "ITEM=Compuserve|C:\\COMM\\CIS.BAT|1|0\n",
  "ITEM=Prodigy|C:\\COMM\\PRODIGY.BAT|1|0\n",
  "\n",
  "[Launcher\\Utilities]\n",
  "ITEM=Font Selector|C:\\UTILS\\FONTSEL.COM|1|0\n",
  "ITEM=File Manager|C:\\4DOS\\4START.BAT|1|0\n",
  "\n",
  "[Launcher\\System Tools]\n",
  "ITEM=Memory Report|MEM /C /P|1|0\n",
  "ITEM=System Information|C:\\UTILS\\NSSI.EXE|1|0\n",
  0
};

static void cursor_restore(void);
static void mouse_stop(void);

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
  else if(!stricmp(key,"SHOW_TIME")){field=&a->show_time;limit=1;}
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
  nodes[n].active=1; nodes[n].order=(unsigned char)ord;
  nodes[n].press_enter=1;nodes[n].change_dir=0;
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
      char *r,*s;int node,enter=1,cd=0;
      p=trim(p+5); q=strchr(p,'|');
      if(q && *trim(p)){
        *q++=0;r=strrchr(q,'|');
        if(r && (r[1]=='0' || r[1]=='1') && r[2]==0){
          cd=r[1]-'0';*r=0;s=strrchr(q,'|');
          if(s && (s[1]=='0' || s[1]=='1') && s[2]==0){enter=s[1]-'0';*s=0;}
        }
        p=trim(p);q=trim(q);
        if(!*p || !*q){valid=0;break;}
        node=add_node(p,q,parent,0);
        if(node<0){valid=0;break;}
        nodes[node].press_enter=(unsigned char)enter;nodes[node].change_dir=(unsigned char)cd;
      } else {valid=0;break;}
    }
    else if(!strnicmp(p,"FOLDER=",7)){
      p=trim(p+7);
      if(!*p){valid=0;break;}
      if(find_folder(p,parent)<0 && add_node(p,"",parent,1)<0){valid=0;break;}
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
  union REGS r; char value[9]; int x=MENU_WIDTH-12;
  if(last_second!=255 && tick==last_tick)return last_second;
  last_tick=tick;
  r.h.ah=0x2C;int86(0x21,&r,&r);
  if(r.h.dh!=last_second){
    sprintf(value,"%02u:%02u:%02u",r.h.ch,r.h.cl,r.h.dh);
    cell(x-1,y,180,C_BORDER);textout(x,y,value,C_TITLE,8);cell(x+8,y,195,C_BORDER);
  }
  return r.h.dh;
}

static int mouse_start(void)
{
  union REGS r;
  r.x.ax=0;int86(0x33,&r,&r);if(r.x.ax==0)return 0;
  r.x.ax=4;r.x.cx=1;r.x.dx=1;int86(0x33,&r,&r);
  r.x.ax=1;int86(0x33,&r,&r);
  r.x.ax=3;int86(0x33,&r,&r);mouse_last_buttons=r.x.bx;
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
    if(nodes[node].folder){if(fprintf(f,"FOLDER=%s\n",nodes[node].title)<0)return 0;}
    else if(fprintf(f,"ITEM=%s|%s|%u|%u\n",nodes[node].title,nodes[node].command,
                    nodes[node].press_enter,nodes[node].change_dir)<0)return 0;
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
  ok=fputs("; Launch! 0.6 menu definition\n; ITEM=title|command and parameters|press Enter (0/1)|change directory (0/1)\n\n",f)!=EOF;
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
  if(fputs("; Launch! 0.6 appearance settings\n",f)==EOF)ok=0;
  if(ok && fprintf(f,"BACKGROUND=%u\nBORDER=%u\nMAIN_TITLE=%u\nTITLES=%u\n"
      "FOLDERS=%u\nLAUNCHERS=%u\nSELECTED_FG=%u\nSELECTED_BG=%u\n"
      "CONTROLS_FG=%u\nCONTROLS_BG=%u\nLABELS=%u\nMENU_TOP=%u\nSHOW_TIME=%u\n",
      appearance.background,appearance.border,appearance.main_title,appearance.titles,
      appearance.folders,appearance.launchers,appearance.selected_fg,appearance.selected_bg,
      appearance.controls_fg,appearance.controls_bg,appearance.labels,
      appearance.menu_top,appearance.show_time)<0)ok=0;
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
    if(!stricmp(nodes[list[i]].title,"More") ||
       (stricmp(nodes[list[j]].title,"More") && stricmp(nodes[list[i]].title,nodes[list[j]].title)>0))
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
  for(;;){
    dialog_box(x,y,48,7,title);
    textout(x+3,y+2,line1,C_FOLDER,42);
    textout(x+3,y+3,line2,C_FOLDER,42);
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

static int choose_type(void)
{
  int x=(screen_cols-44)/2,y=(screen_rows-7)/2,k,folder=1,mx=0,my=0;unsigned mb;
  for(;;){
    dialog_box(x,y,44,7,"Add Item"); textout(x+3,y+2,"Choose the type of item to add:",C_FOLDER,36);
    draw_button(x+9,y+4,"  Folder  ",10,folder);
    draw_button(x+24,y+4,"  Launcher  ",12,!folder);
    wait_input(&k,&mx,&my,&mb);
    if((mb&1) && my==y+4){
      if(mx>=x+9 && mx<x+19)return 1;
      if(mx>=x+24 && mx<x+36)return 0;
    }
    if(k==27) return -1;
    if(k==0x4B00 || k==0x4D00 || k==9) folder=!folder;
    else if(k==13) return folder;
  }
}

static void field_line(int x,int y,const char *label,const char *value,int selected,int pos)
{
  int len=strlen(label),i,vlen=strlen(value),at=selected?C_SELECTED:C_INPUT_FIELD,scroll=0;
  if(pos>41)scroll=pos-41;
  textout(x,y,label,C_INPUT_LABEL,len); textout(x+13,y,value+scroll,at,42);
  if(selected){i=pos-scroll;cell(x+13+i,y,(pos<vlen)?value[pos]:' ',C_INPUT_FIELD);}
}

static void check_line(int x,int y,const char *label,int checked,int focused)
{
  char mark[4];
  sprintf(mark,"[%c]",checked?'X':' ');
  textout(x,y,mark,C_BUTTON,3);
  textout(x+4,y,label,focused?C_SELECTED:C_INPUT_LABEL,34);
}

static int item_form(int folder,char *name,char *exe,char *params,
                     int *press_enter,int *change_dir,int editing)
{
  char *fields[3]; int limits[3],pos[3],count,controls,focus=0,k,x,y,i,len,mx=0,my=0,scroll;
  unsigned mb;
  fields[0]=name;fields[1]=exe;fields[2]=params;
  limits[0]=folder?16:18;limits[1]=MAX_CMD-2;limits[2]=MAX_CMD-1;
  name[limits[0]]=0;
  pos[0]=pos[1]=pos[2]=0; count=folder?1:3;
  controls=folder?count:count+2;
  x=(screen_cols-64)/2;y=(screen_rows-(folder?10:14))/2;
  for(;;){
    dialog_box(x,y,64,folder?10:14,
        editing?(folder?"Edit Folder":"Edit Launcher"):
                (folder?"Add Folder":"Add Launcher"));
    field_line(x+3,y+2,"Name:",name,focus==0,pos[0]);
    if(!folder){field_line(x+3,y+4,"Executable:",exe,focus==1,pos[1]);field_line(x+3,y+6,"Parameters:",params,focus==2,pos[2]);}
    if(!folder){
      check_line(x+16,y+8,"Press \021\331 after launcher command",*press_enter,focus==3);
      check_line(x+16,y+9,"Change directory first",*change_dir,focus==4);
    }
    draw_button(x+19,y+(folder?7:11),"  OK  ",6,focus==controls);
    draw_button(x+33,y+(folder?7:11),"  Cancel  ",10,focus==controls+1);
    wait_input(&k,&mx,&my,&mb);
    if(mb&1){
      if(my==y+2 && mx>=x+16 && mx<x+58){focus=0;i=0;}
      else if(!folder && my==y+4 && mx>=x+16 && mx<x+58){focus=1;i=1;}
      else if(!folder && my==y+6 && mx>=x+16 && mx<x+58){focus=2;i=2;}
      else if(!folder && my==y+8 && mx>=x+16 && mx<x+54){*press_enter=!*press_enter;focus=3;continue;}
      else if(!folder && my==y+9 && mx>=x+16 && mx<x+54){*change_dir=!*change_dir;focus=4;continue;}
      else if(my==y+(folder?7:11) && mx>=x+19 && mx<x+25){
        if(*name && (folder || *exe))return 1;
        focus=controls;continue;
      }
      else if(my==y+(folder?7:11) && mx>=x+33 && mx<x+43)return 0;
      else continue;
      len=strlen(fields[i]);scroll=pos[i]>41?pos[i]-41:0;
      pos[i]=scroll+mx-(x+16);if(pos[i]>len)pos[i]=len;
      continue;
    }
    if(k==27) return 0;
    if(k==9 || k==0x5000){focus=(focus+1)%(controls+2);continue;}
    if(k==0x4800){focus=(focus+controls+1)%(controls+2);continue;}
    if(!folder && (focus==3 || focus==4)){
      if(k==' ' || k==13){if(focus==3)*press_enter=!*press_enter;else *change_dir=!*change_dir;}
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
  }
  return 0;
}

static void change_appearance_value(int focus,int direction)
{
  unsigned char *field;int limit,value;
  if(focus==12){appearance.show_time=!appearance.show_time;return;}
  field=appearance_field(focus,&limit);if(!field)return;
  value=(int)*field+direction;
  if(value<0)value=limit;
  if(value>limit)value=0;
  *field=(unsigned char)value;
}

static int configure_appearance(void)
{
  APPEARANCE original=appearance;int x,y,k=0,mx=0,my=0,focus=0,row=-1;
  unsigned mb=0;static const int rows[13]={2,3,4,5,6,7,8,8,9,9,10,12,14};
  video_init();if(!save_screen()){puts("Launch!: insufficient memory");return 0;}
  cursor_hide();mouse_present=mouse_start();
  x=(screen_cols-64)/2;y=(screen_rows-20)/2;
  for(;;){
    dialog_box(x,y,64,20,"Configure Appearance");
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
    check_line(x+22,y+14,"Show the time",appearance.show_time,focus==12);
    draw_button(x+19,y+17,"  OK  ",6,focus==13);
    draw_button(x+34,y+17,"  Cancel  ",10,focus==14);
    wait_input(&k,&mx,&my,&mb);
    if(mb){
      row=-1;
      if(mx>=x+22 && mx<x+37){
        int i;for(i=0;i<13;i++)if(my==y+rows[i]){row=i;break;}
      }
      if(mx>=x+41 && mx<x+56 && my==y+8)row=7;
      if(mx>=x+41 && mx<x+56 && my==y+9)row=9;
      if(row>=0){focus=row;change_appearance_value(focus,(mb&2)?-1:1);continue;}
      if(my==y+17 && (mb&1)){
        if(mx>=x+19 && mx<x+25){
          if(save_appearance()){close_menu();return 1;}
          notice_box("Write Error","Could not update LAUNCH.CFG.");continue;
        }
        if(mx>=x+34 && mx<x+44){appearance=original;close_menu();return 0;}
      }
      continue;
    }
    if(k==27){appearance=original;close_menu();return 0;}
    if(k==9 || k==0x5000){focus=(focus+1)%15;continue;}
    if(k==0x4800){focus=(focus+14)%15;continue;}
    if(k==0x4B00){change_appearance_value(focus,-1);continue;}
    if(k==0x4D00 || k==' '){change_appearance_value(focus,1);continue;}
    if(k==13){
      if(focus<13){focus++;continue;}
      if(focus==13){
        if(save_appearance()){close_menu();return 1;}
        notice_box("Write Error","Could not update LAUNCH.CFG.");continue;
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
  int list[MAX_CHILD],press_enter=1,change_dir=0;
  name[0]=exe[0]=params[0]=cmd[0]=0;
  if(children(parent,list)>=MENU_CAPACITY && depth>=MAX_DEPTH-1){notice_box("Menu Full","No further menu level is available.");return 0;}
  folder=choose_type(); if(folder<0)return 0;
  if(!item_form(folder,name,exe,params,&press_enter,&change_dir,0))return 0;
  if(folder && !stricmp(name,"More")){notice_box("Reserved Name","More is reserved for automatic overflow.");return 0;}
  if(folder && find_folder(name,parent)>=0){notice_box("Duplicate Folder","That folder name is already in this menu.");return 0;}
  if(folder) node=add_node(name,"",parent,1);
  else {strcpy(cmd,exe);if(*params){strcat(cmd," ");strncat(cmd,params,MAX_CMD-strlen(cmd)-1);}node=add_node(name,cmd,parent,0);}
  if(node<0){notice_box("Menu Full","The menu database is full.");return 0;}
  nodes[node].press_enter=(unsigned char)press_enter;nodes[node].change_dir=(unsigned char)change_dir;
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
  static char name[MAX_TITLE],exe[MAX_CMD],params[MAX_CMD],cmd[MAX_CMD];
  exe[0]=params[0]=cmd[0]=0;
  strcpy(name,nodes[node].title);
  if(nodes[node].folder && !stricmp(name,"More")){notice_box("Automatic Folder","More is managed automatically.");return 0;}
  if(!nodes[node].folder)split_command(nodes[node].command,exe,params);
  if(!item_form(nodes[node].folder,name,exe,params,&press_enter,&change_dir,1))return 0;
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
  }
  return 1;
}

static int menu(void)
{
  static int parent[MAX_DEPTH],sel[MAX_DEPTH],list[MAX_CHILD];
  static int panel_y[MAX_DEPTH],panel_h[MAX_DEPTH],panel_n[MAX_DEPTH];
  static int draw_list[MAX_CHILD],hitlist[MAX_CHILD];
  int depth=0,n,k,i,h,x,y,node,redraw=2,mx=0,my=0,hit,pos;
  unsigned mb; unsigned char last_second=255;
  parent[0]=-1; sel[0]=0;
  video_init();if(!save_screen()){puts("Launch!: insufficient memory");return -1;}
  cursor_hide();mouse_present=mouse_start();
  for(;;){
    n=children(parent[depth],list);
    if(sel[depth]>=n) sel[depth]=n ? n-1 : 0;
    if(redraw){
      if(redraw==2) restore_screen();
      for(i=0;i<=depth;i++){
        int tn,j;
        tn=children(parent[i],draw_list); h=(tn?tn+2:3)+(i==0?1:0);
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
          if(nodes[node].folder){
            textout(x+1,y+1+j,nodes[node].title,(j==sel[i])?C_SELECTED:C_FOLDER,16);
            cell(x+17,y+1+j,' ',(j==sel[i])?C_SELECTED:C_FOLDER);
            cell(x+18,y+1+j,16,(j==sel[i])?C_SELECTED:C_FOLDER);
          } else textout(x+1,y+1+j,nodes[node].title,(j==sel[i])?C_SELECTED:C_ITEM,18);
        }
        if(!tn)textout(x+1,y+1,"Empty",C_EMPTY,18);
      }
      if(appearance.show_time)last_second=draw_clock(panel_y[0]+panel_h[0]-1,255);
      else last_second=255;
      redraw=0;
    }
    k=0;mb=0;
    do {
      if(key_waiting()){k=keyread();break;}
      mb=mouse_poll(&mx,&my);
      if(appearance.show_time)
        last_second=draw_clock(panel_y[0]+panel_h[0]-1,last_second);
    } while(!mb);

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
        int hn=children(parent[hit],hitlist);
        if(pos<hn){
          depth=hit;sel[hit]=pos;node=hitlist[pos];
          if(mb&2){
            if(edit_dialog(node) && !save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
            redraw=2;continue;
          }
          if(mb&1){
            if(nodes[node].folder && depth<MAX_DEPTH-1){depth++;parent[depth]=node;sel[depth]=0;redraw=2;continue;}
            if(!nodes[node].folder){run_node=node;close_menu();return node;}
          }
        }
      }
      continue;
    }

    if(k==27){close_menu();return -1;}
    if((key_shift&0x04) && key_scan==0x1E){
      if(add_dialog(parent[depth],depth)){
        if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
        n=children(parent[depth],list);for(i=0;i<n;i++)if(list[i]==added_visible_node)sel[depth]=i;
      }
      redraw=2;
    }
    else if((key_shift&0x04) && key_scan==0x20 && n>0){
      node=list[sel[depth]];
      {
        char question[64];
        sprintf(question,"Remove %s from the menu?",nodes[node].title);
        if(confirm_box("Remove Item",question)){
        delete_tree(node);normalize_order(parent[depth]);if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
        }
      }
      redraw=2;
    }
    else if((key_shift&0x04) && key_scan==0x12 && n>0){
      node=list[sel[depth]];
      if(edit_dialog(node) && !save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
      redraw=2;
    }
    else if((key_shift&0x04) && (key_scan==0x48 || key_scan==0x8D) && n>0){
      sel[depth]=move_item(parent[depth],sel[depth],-1);if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");redraw=1;
    }
    else if((key_shift&0x04) && (key_scan==0x50 || key_scan==0x91) && n>0){
      sel[depth]=move_item(parent[depth],sel[depth],1);if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");redraw=1;
    }
    else if((key_shift&0x04) && key_scan==0x1F){
      if(n>0){node=list[sel[depth]];sort_menu(parent[depth]);n=children(parent[depth],list);for(i=0;i<n;i++)if(list[i]==node)sel[depth]=i;if(!save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");}
      redraw=1;
    }
    else if(k==0x4800 && n>0){sel[depth]=(sel[depth]+n-1)%n;redraw=1;}
    else if(k==0x5000 && n>0){sel[depth]=(sel[depth]+1)%n;redraw=1;}
    else if(k==0x4B00){if(depth){depth--;redraw=2;}}
    else if((k==13 || k==0x4D00) && n>0){
      node=list[sel[depth]];
      if(nodes[node].folder && depth<MAX_DEPTH-1){depth++;parent[depth]=node;sel[depth]=0;redraw=1;}
      else if(k==13 && !nodes[node].folder){run_node=node;close_menu();return node;}
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

static void build_macro(int node,char *text)
{
  char directory[MAX_CMD];text[0]=0;
  if(nodes[node].change_dir){
    command_directory(nodes[node].command,directory);
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
  macro_append(text,nodes[node].command);
  if(nodes[node].press_enter)macro_append(text,"\r");
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

static int macro_helper(void)
{
  union REGS r;unsigned seg,paragraphs;
  struct SREGS s;
  unsigned long old16,old2f;
  r.x.ax=0xD5A0;r.x.bx=0;int86(0x2F,&r,&r);
  if(r.x.bx==0x4C48)return 1;
  paragraphs=(MACRO_BLOB_SIZE+15)/16;
  if(_dos_allocmem(paragraphs,&seg)!=0)return 0;
  segread(&s);movedata(s.ds,(unsigned)macro_blob,seg,0,MACRO_BLOB_SIZE);
  old16=dos_get_vector(0x16);old2f=dos_get_vector(0x2F);
  far_write_long(seg,MACRO_OLD16_OFF,old16);
  far_write_long(seg,MACRO_OLD2F_OFF,old2f);
  *(unsigned far *)MAKE_FP(seg-1,1)=8; /* DOS-owned: survive LAUNCH.EXE */
  dos_set_vector(0x16,seg,MACRO_INT16_OFF);
  dos_set_vector(0x2F,seg,MACRO_INT2F_OFF);
  return 1;
}

static int queue_macro(const char *text)
{
  union REGS r;struct SREGS s;
  if(!macro_helper())return 0;
  segread(&s);r.x.ax=0xD5A1;r.x.si=(unsigned)text;r.x.cx=(unsigned)strlen(text);
  int86x(0x2F,&r,&r,&s);return r.h.al==0;
}

static void show_help(void)
{
  puts("Launch! 0.6 - lightweight command menu for DOS\n");
  puts("Usage: LAUNCH [/CONFIG | /?]\n");
  puts("Menu management shortcuts:");
  puts("  Ctrl+A        Add a folder or launcher");
  puts("  Ctrl+D        Delete the selected item");
  puts("  Ctrl+E        Edit the selected item");
  puts("  Ctrl+Up/Down  Move the selected item");
  puts("  Ctrl+S        Sort the current menu\n");
  puts("Command-line parameters:");
  puts("  /CONFIG       Configure colours, position and clock");
  puts("  /?            Show this help");
}

int main(int argc,char **argv)
{
  int i,config_status,config_mode=0;static char macro[MAX_MACRO];
  config_path(argv[0]);
  if(!load_appearance())puts("Launch!: LAUNCH.CFG is invalid; using default appearance.");
  for(i=1;i<argc;i++){
    if(!stricmp(argv[i],"/?") || !stricmp(argv[i],"-?")){show_help();return 0;}
    if(!stricmp(argv[i],"/CONFIG"))config_mode=1;
    else {printf("Launch!: unknown option %s (use LAUNCH /?)\n",argv[i]);return 1;}
  }
  if(config_mode){configure_appearance();return 0;}
  config_status=prepare_config();
  if(!config_status){printf("Launch!: cannot recover %s\n",config_file);return 1;}
  if(config_status==2)puts("Launch!: LAUNCH.MNU was missing or invalid; restored LAUNCH.BAK.");
  else if(config_status==3)puts("Launch!: no valid menu file was found; installed the sample menu.");
  if(menu()>=0){
    build_macro(run_node,macro);
    if(!queue_macro(macro)){puts("Launch!: cannot install keyboard macro helper");return 1;}
  }
  return 0;
}
