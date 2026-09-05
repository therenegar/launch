/* Launch! 0.41 - modal application menu for DOS
 * Microsoft C/C++ 7.0, small model (.EXE), 286/EGA or later.
 */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <process.h>

#define MAX_NODES 96
#define MAX_TITLE 24
#define MAX_CMD 128
#define MAX_DEPTH 4
#define MAX_CHILD 21
#define MENU_CAPACITY 20
#define MENU_WIDTH 20
#define MAKE_FP(seg,off) ((void far *)((((unsigned long)(seg))<<16) | \
                                      (unsigned short)(off)))

/* Colour scheme: high nibble = background, low nibble = foreground. */
#define C_MENU_BACKGROUND 0x11  /* blue on blue */
#define C_BORDER          0x1B  /* bright cyan on blue */
#define C_TITLE           0x1E  /* yellow on blue */
#define C_ROOT_TITLE      0x1C  /* bright red on blue */
#define C_FOLDER          0x1F  /* bright white on blue */
#define C_ITEM            0x1A  /* bright green on blue */
#define C_EMPTY           0x13  /* teal on blue */
#define C_SELECTED        0x3F  /* bright white on teal */
#define C_INPUT_FIELD     0x70  /* black on light grey */
#define C_BUTTON          0x70  /* black on light grey */
#define C_INPUT_LABEL     0x17  /* light grey on blue */
#define C_FAUX_SHADOW     0x08  /* dark grey on black */
#define C_BLOCK_SHADOW_FG 0x00  /* black half-block foreground */

typedef struct {
  char title[MAX_TITLE];
  char command[MAX_CMD];
  int parent;
  unsigned char folder;
  unsigned char active;
  unsigned char order;
} NODE;

static NODE nodes[MAX_NODES];
static int node_count;
static int screen_cols, screen_rows;
static unsigned short far *video;
static unsigned short *saved;
static unsigned char cursor_start,cursor_end;
static char run_command[MAX_CMD];
static char config_file[MAX_CMD];
static unsigned char key_shift;
static unsigned char key_scan;
static int menu_at_top;
static int mouse_present;
static unsigned mouse_last_buttons;
static int added_visible_node;

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
  FILE *f; char line[256],*p,*q; int parent=-1;
  node_count=0;
  f=fopen(name,"rt"); if(!f) return 0;
  while(fgets(line,sizeof(line),f)){
    p=trim(line); if(!*p || *p==';' || *p=='#') continue;
    if(*p=='[' && (q=strchr(p,']'))!=0){ *q=0; parent=section_parent(trim(p+1)); continue; }
    if(!strnicmp(p,"ITEM=",5)){
      p=trim(p+5); q=strchr(p,'|');
      if(q){ *q++=0; add_node(trim(p),trim(q),parent,0); }
    }
    else if(!strnicmp(p,"FOLDER=",7)){
      p=trim(p+7); if(find_folder(p,parent)<0) add_node(p,"",parent,1);
    }
  }
  fclose(f); return 1;
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

static void write_section(FILE *f,int parent,const char *path)
{
  int list[MAX_CHILD],n,i; char child_path[128];
  n=children(parent,list);
  fprintf(f,"[%s]\n",path);
  for(i=0;i<n;i++){
    if(nodes[list[i]].folder) fprintf(f,"FOLDER=%s\n",nodes[list[i]].title);
    else fprintf(f,"ITEM=%s|%s\n",nodes[list[i]].title,nodes[list[i]].command);
  }
  fputc('\n',f);
  for(i=0;i<n;i++) if(nodes[list[i]].folder){
    strcpy(child_path,path); strcat(child_path,"\\"); strcat(child_path,nodes[list[i]].title);
    write_section(f,list[i],child_path);
  }
}

static int save_config(void)
{
  FILE *f=fopen(config_file,"wt");
  if(!f) return 0;
  fputs("; Launch! 0.41 menu definition\n; ITEM=display title|command and parameters\n\n",f);
  write_section(f,-1,"Launcher");
  if(fclose(f)!=0) return 0;
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

static int item_form(int folder,char *name,char *exe,char *params,int editing)
{
  char *fields[3]; int limits[3],pos[3],count,focus=0,k,x,y,i,len,mx=0,my=0,scroll;
  unsigned mb;
  fields[0]=name;fields[1]=exe;fields[2]=params;
  limits[0]=folder?16:18;limits[1]=MAX_CMD-2;limits[2]=MAX_CMD-1;
  name[limits[0]]=0;
  pos[0]=pos[1]=pos[2]=0; count=folder?1:3;
  x=(screen_cols-64)/2;y=(screen_rows-(folder?10:14))/2;
  for(;;){
    dialog_box(x,y,64,folder?10:14,
        editing?(folder?"Edit Folder":"Edit Launcher"):
                (folder?"Add Folder":"Add Launcher"));
    field_line(x+3,y+2,"Name:",name,focus==0,pos[0]);
    if(!folder){field_line(x+3,y+4,"Executable:",exe,focus==1,pos[1]);field_line(x+3,y+6,"Parameters:",params,focus==2,pos[2]);}
    draw_button(x+19,y+(folder?7:11),"  OK  ",6,focus==count);
    draw_button(x+33,y+(folder?7:11),"  Cancel  ",10,focus==count+1);
    wait_input(&k,&mx,&my,&mb);
    if(mb&1){
      if(my==y+2 && mx>=x+16 && mx<x+58){focus=0;i=0;}
      else if(!folder && my==y+4 && mx>=x+16 && mx<x+58){focus=1;i=1;}
      else if(!folder && my==y+6 && mx>=x+16 && mx<x+58){focus=2;i=2;}
      else if(my==y+(folder?7:11) && mx>=x+19 && mx<x+25){
        if(*name && (folder || *exe))return 1;
        focus=count;continue;
      }
      else if(my==y+(folder?7:11) && mx>=x+33 && mx<x+43)return 0;
      else continue;
      len=strlen(fields[i]);scroll=pos[i]>41?pos[i]-41:0;
      pos[i]=scroll+mx-(x+16);if(pos[i]>len)pos[i]=len;
      continue;
    }
    if(k==27) return 0;
    if(k==9 || k==0x5000){focus=(focus+1)%(count+2);continue;}
    if(k==0x4800){focus=(focus+count+1)%(count+2);continue;}
    if(focus>=count){
      if(k==0x4B00 || k==0x4D00) focus=(focus==count)?count+1:count;
      else if(k==13){if(focus==count && *name && (folder || *exe))return 1;if(focus==count+1)return 0;}
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
  int folder,node; char name[MAX_TITLE]="",exe[MAX_CMD]="",params[MAX_CMD]="",cmd[MAX_CMD];
  int list[MAX_CHILD];
  if(children(parent,list)>=MENU_CAPACITY && depth>=MAX_DEPTH-1){notice_box("Menu Full","No further menu level is available.");return 0;}
  folder=choose_type(); if(folder<0)return 0;
  if(!item_form(folder,name,exe,params,0))return 0;
  if(folder && !stricmp(name,"More")){notice_box("Reserved Name","More is reserved for automatic overflow.");return 0;}
  if(folder && find_folder(name,parent)>=0){notice_box("Duplicate Folder","That folder name is already in this menu.");return 0;}
  if(folder) node=add_node(name,"",parent,1);
  else {strcpy(cmd,exe);if(*params){strcat(cmd," ");strncat(cmd,params,MAX_CMD-strlen(cmd)-1);}node=add_node(name,cmd,parent,0);}
  if(node<0){notice_box("Menu Full","The menu database is full.");return 0;}
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
  int duplicate; char name[MAX_TITLE],exe[MAX_CMD]="",params[MAX_CMD]="",cmd[MAX_CMD];
  strcpy(name,nodes[node].title);
  if(nodes[node].folder && !stricmp(name,"More")){notice_box("Automatic Folder","More is managed automatically.");return 0;}
  if(!nodes[node].folder)split_command(nodes[node].command,exe,params);
  if(!item_form(nodes[node].folder,name,exe,params,1))return 0;
  duplicate=find_folder(name,nodes[node].parent);
  if(nodes[node].folder && !stricmp(name,"More") && stricmp(nodes[node].title,"More")){notice_box("Reserved Name","More is reserved for automatic overflow.");return 0;}
  if(nodes[node].folder && duplicate>=0 && duplicate!=node){notice_box("Duplicate Folder","That folder name is already in this menu.");return 0;}
  strcpy(nodes[node].title,name);
  if(!nodes[node].folder){
    strcpy(cmd,exe);
    if(*params && strlen(cmd)<MAX_CMD-1){strcat(cmd," ");strncat(cmd,params,MAX_CMD-strlen(cmd)-1);}
    strcpy(nodes[node].command,cmd);
  }
  return 1;
}

static int menu(void)
{
  int parent[MAX_DEPTH],sel[MAX_DEPTH],list[MAX_CHILD];
  int panel_y[MAX_DEPTH],panel_h[MAX_DEPTH],panel_n[MAX_DEPTH];
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
        int tmp[MAX_CHILD],tn,j;
        tn=children(parent[i],tmp); h=(tn?tn+2:3)+(i==0?1:0);
        if(h>screen_rows) h=screen_rows;
        if(i==0) y=menu_at_top?0:screen_rows-h;
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
          node=tmp[j];
          if(nodes[node].folder){
            textout(x+1,y+1+j,nodes[node].title,(j==sel[i])?C_SELECTED:C_FOLDER,16);
            cell(x+17,y+1+j,' ',(j==sel[i])?C_SELECTED:C_FOLDER);
            cell(x+18,y+1+j,16,(j==sel[i])?C_SELECTED:C_FOLDER);
          } else textout(x+1,y+1+j,nodes[node].title,(j==sel[i])?C_SELECTED:C_ITEM,18);
        }
        if(!tn)textout(x+1,y+1,"Empty",C_EMPTY,18);
      }
      last_second=draw_clock(panel_y[0]+panel_h[0]-1,255);
      redraw=0;
    }
    k=0;mb=0;
    do {
      if(key_waiting()){k=keyread();break;}
      mb=mouse_poll(&mx,&my);
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
        int hitlist[MAX_CHILD],hn=children(parent[hit],hitlist);
        if(pos<hn){
          depth=hit;sel[hit]=pos;node=hitlist[pos];
          if(mb&2){
            if(edit_dialog(node) && !save_config())notice_box("Write Error","Could not update LAUNCH.MNU.");
            redraw=2;continue;
          }
          if(mb&1){
            if(nodes[node].folder && depth<MAX_DEPTH-1){depth++;parent[depth]=node;sel[depth]=0;redraw=2;continue;}
            if(!nodes[node].folder){strcpy(run_command,nodes[node].command);close_menu();return node;}
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
      else if(k==13 && !nodes[node].folder){strcpy(run_command,nodes[node].command);close_menu();return node;}
    }
  }
}

static int command_args(char *line,char **args,int maximum)
{
  char *p=line,*out,*next; int count=0,quoted;
  while(*p && count<maximum-1){
    while(*p==' ' || *p=='\t')p++;
    if(!*p)break;
    args[count++]=out=p;quoted=0;
    while(*p){
      if(*p=='\"'){quoted=!quoted;p++;continue;}
      if(!quoted && (*p==' ' || *p=='\t'))break;
      *out++=*p++;
    }
    next=p;while(*next==' ' || *next=='\t')next++;
    *out=0;p=next;
  }
  args[count]=0;return count;
}

static int batch_command(const char *name)
{
  const char *dot=strrchr(name,'.');
  return dot && (!stricmp(dot,".BAT") || !stricmp(dot,".BTM"));
}

static void execute_command(const char *cmd)
{
  char line[MAX_CMD],*args[20]; const char *shell; int count;
  strcpy(line,cmd);count=command_args(line,args,20);if(!count)return;
  if(batch_command(args[0])){
    shell=getenv("COMSPEC");if(!shell || !*shell)shell="COMMAND.COM";
    spawnlp(P_OVERLAY,shell,shell,"/C",cmd,(char *)0);
  } else spawnvp(P_OVERLAY,args[0],args);
  perror("Launch!");
}

static void show_help(void)
{
  puts("Launch! 0.41 - lightweight application menu for DOS\n");
  puts("Usage: LAUNCH [/POS=TOP|BOTTOM]");
  puts("       LAUNCH /?\n");
  puts("Navigation:");
  puts("  Up/Down       Select an item");
  puts("  Right/Enter   Open a folder");
  puts("  Enter         Run a launcher");
  puts("  Left          Close a folder");
  puts("  Esc           Close Launch!\n");
  puts("Menu management:");
  puts("  Ctrl+A        Add a folder or launcher");
  puts("  Ctrl+D        Delete the selected item");
  puts("  Ctrl+E        Edit the selected item");
  puts("  Ctrl+Up/Down  Move the selected item");
  puts("  Ctrl+S        Sort the current menu\n");
  puts("Mouse:");
  puts("  Left click    Open or launch; outside closes menu");
  puts("  Right click   Edit an item\n");
  puts("Options:");
  puts("  /POS=TOP      Open at the upper-left");
  puts("  /POS=BOTTOM   Open at the lower-left (default)");
  puts("  /?            Show this help");
}

int main(int argc,char **argv)
{
  int i;
  menu_at_top=0;config_path(argv[0]);
  for(i=1;i<argc;i++){
    if(!stricmp(argv[i],"/?") || !stricmp(argv[i],"-?")){show_help();return 0;}
    if(!stricmp(argv[i],"/POS=TOP"))menu_at_top=1;
    else if(!stricmp(argv[i],"/POS=BOTTOM"))menu_at_top=0;
    else {printf("Launch!: unknown option %s (use LAUNCH /?)\n",argv[i]);return 1;}
  }
  if(!load_config(config_file)){printf("LAUNCH: cannot read %s\n",config_file);return 1;}
  if(menu()>=0) execute_command(run_command);
  return 0;
}
