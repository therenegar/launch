/* Shared Launch! 3.1 accessory runtime.  Microsoft C/C++ 7.0, small model. */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <bios.h>
#include <direct.h>
#include <io.h>
#include "ACCLIB.H"

ACC_APPEARANCE acc_appearance={1,11,15,7,12,14,15,10,15,3,0,7,7,0,1,1,1,1,10,0,1,1,1,0,0};
int acc_cols=80,acc_rows=25,acc_mouse_present=0;
char acc_directory[ACC_PATH];
static unsigned short saved_screen[80*50];
static int saved_mode,saved_page,saved_cursor_start,saved_cursor_end;
static int saved_cursor_x,saved_cursor_y;
static unsigned saved_video_segment=0xB800;
static int close_x=-1,close_y=-1,mouse_visible=0,mouse_last_buttons=0;
static unsigned mouse_raw_x=0,mouse_raw_y=0;
static unsigned char mouse_old_glyph[32];static int mouse_glyph_saved=0;
static unsigned char mouse_old_target[32];static int mouse_target_saved=0;
#define ACC_MAX_BUTTONS 16
typedef struct {int x,y,selected;const char *text;} ACC_BUTTON_REC;
static ACC_BUTTON_REC acc_buttons[ACC_MAX_BUTTONS];
static int acc_button_count=0;
static unsigned short notice_screen[81*10];
#define MAKE_FP(seg,off) ((void far *)((((unsigned long)(seg))<<16)|(unsigned short)(off)))

typedef struct {unsigned char seq2,seq4,gc4,gc5,gc6;} FONT_REGS;
static unsigned char indexed_read(unsigned port,unsigned char index){outp(port,index);return(unsigned char)inp(port+1);}
static void indexed_write(unsigned port,unsigned char index,unsigned char value){outp(port,index);outp(port+1,value);}
static void font_plane_open(FONT_REGS *old){old->seq2=indexed_read(0x3C4,2);old->seq4=indexed_read(0x3C4,4);old->gc4=indexed_read(0x3CE,4);old->gc5=indexed_read(0x3CE,5);old->gc6=indexed_read(0x3CE,6);indexed_write(0x3C4,2,4);indexed_write(0x3C4,4,7);indexed_write(0x3CE,4,2);indexed_write(0x3CE,5,0);indexed_write(0x3CE,6,0);}
static void font_plane_close(const FONT_REGS *old){indexed_write(0x3C4,2,old->seq2);indexed_write(0x3C4,4,old->seq4);indexed_write(0x3CE,4,old->gc4);indexed_write(0x3CE,5,old->gc5);indexed_write(0x3CE,6,old->gc6);}
static void mouse_glyph_write(const unsigned char *glyph){FONT_REGS old;unsigned char far *font;int i;font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,127*32);for(i=0;i<32;i++)font[i]=glyph[i];font_plane_close(&old);}
void acc_glyph_read(int code,unsigned char *glyph){FONT_REGS old;unsigned char far *font;int i;font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,code*32);for(i=0;i<32;i++)glyph[i]=font[i];font_plane_close(&old);}
void acc_glyph_write(int code,const unsigned char *glyph){FONT_REGS old;unsigned char far *font;int i;font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,code*32);for(i=0;i<32;i++)font[i]=glyph[i];font_plane_close(&old);}
static void mouse_pointer_restore(void){union REGS r;if(!mouse_glyph_saved&&!mouse_target_saved)return;if(mouse_glyph_saved)mouse_glyph_write(mouse_old_glyph);if(mouse_target_saved)acc_glyph_write(8,mouse_old_target);memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;r.x.cx=0xFFFF;r.x.dx=0x7700;int86(0x33,&r,&r);mouse_glyph_saved=0;mouse_target_saved=0;}
static void mouse_pointer_install(void)
{
  static const unsigned char arrow16[16]={0,0,0,0,0,0,0,0x10,0x18,0x1C,0x1E,0x1F,0x1E,0x12,3,1};
  static const unsigned char target16[32]={0,0,0,0x18,0x18,0x18,0x3C,0xE7,0xE7,0x3C,0x18,0x18,0x18,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  FONT_REGS old;unsigned char far *font,height_far;unsigned char arrow[32];union REGS r;int i,height,source;
  if(acc_appearance.mouse_cursor){if(acc_appearance.mouse_cursor==2){acc_glyph_read(8,mouse_old_target);mouse_target_saved=1;acc_glyph_write(8,target16);}memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;if(acc_appearance.mouse_cursor==1){r.x.cx=0xFFFF;r.x.dx=0x7700;}else{r.x.cx=0xF000;r.x.dx=0x0F08;}int86(0x33,&r,&r);return;}
  font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,127*32);for(i=0;i<32;i++)mouse_old_glyph[i]=font[i];font_plane_close(&old);mouse_glyph_saved=1;
  memset(arrow,0,sizeof(arrow));height_far=*(unsigned char far *)MAKE_FP(0x40,0x85);height=height_far;if(height<8||height>32)height=16;
  for(i=0;i<height;i++){source=i*16/height;if(source>15)source=15;arrow[i]=arrow16[source];}mouse_glyph_write(arrow);
  memset(&r,0,sizeof(r));r.x.ax=0x000A;r.x.bx=0;r.x.cx=0xF000;r.x.dx=0x0F7F;int86(0x33,&r,&r);
}

static void acc_source_dir(const char *program)
{
  char exe[ACC_PATH],candidate[ACC_PATH];const char *path,*end,*slash,*other;int n;
  strncpy(exe,program,ACC_PATH-1);exe[ACC_PATH-1]=0;
  if(!strchr(exe,'\\')&&!strchr(exe,'/')&&!strchr(exe,':')){
    path=getenv("PATH");
    while(path&&*path){end=strchr(path,';');n=end?(int)(end-path):(int)strlen(path);
      if(n>0&&n<ACC_PATH-(int)strlen(exe)-2){strncpy(candidate,path,n);candidate[n]=0;
        if(candidate[n-1]!='\\'&&candidate[n-1]!='/')strcat(candidate,"\\");strcat(candidate,exe);
        if(acc_exists(candidate)){strcpy(exe,candidate);break;}}
      if(!end)break;path=end+1;}
  }
  slash=strrchr(exe,'\\');other=strrchr(exe,'/');if(!slash||(other&&other>slash))slash=other;
  n=slash?(int)(slash-exe)+1:((exe[0]&&exe[1]==':')?2:0);
  strncpy(acc_directory,exe,n);acc_directory[n]=0;
}

int acc_exists(const char *path){FILE *f=fopen(path,"rb");if(!f)return 0;fclose(f);return 1;}

static void acc_load_config(void)
{
  char name[ACC_PATH],line[128],*eq,*key;int value,title_fg_set=0,title_bg_set=0;FILE *f;unsigned char *field=0;
  sprintf(name,"%sLAUNCH.CFG",acc_directory);f=fopen(name,"r");if(!f)return;
  while(fgets(line,sizeof(line),f)){
    key=line;while(*key&&isspace(*key))key++;if(!*key||*key==';'||*key=='#')continue;
    eq=strchr(key,'=');if(!eq)continue;*eq++=0;value=atoi(eq);field=0;
    if(!stricmp(key,"BACKGROUND"))field=&acc_appearance.background;
    else if(!stricmp(key,"BORDER"))field=&acc_appearance.border;
    else if(!stricmp(key,"TITLEBAR_FG"))field=&acc_appearance.titlebar_fg;
    else if(!stricmp(key,"TITLEBAR_BG"))field=&acc_appearance.titlebar_bg;
    else if(!stricmp(key,"MAIN_TITLE"))field=&acc_appearance.main_title;
    else if(!stricmp(key,"TITLES"))field=&acc_appearance.titles;
    else if(!stricmp(key,"FOLDERS"))field=&acc_appearance.folders;
    else if(!stricmp(key,"LAUNCHERS"))field=&acc_appearance.launchers;
    else if(!stricmp(key,"SELECTED_FG"))field=&acc_appearance.selected_fg;
    else if(!stricmp(key,"SELECTED_BG"))field=&acc_appearance.selected_bg;
    else if(!stricmp(key,"CONTROLS_FG"))field=&acc_appearance.controls_fg;
    else if(!stricmp(key,"CONTROLS_BG"))field=&acc_appearance.controls_bg;
    else if(!stricmp(key,"LABELS"))field=&acc_appearance.labels;
    if(field&&value>=0&&value<=15){*field=(unsigned char)value;if(field==&acc_appearance.titlebar_fg)title_fg_set=1;if(field==&acc_appearance.titlebar_bg)title_bg_set=1;}
  }fclose(f);if(!title_fg_set)acc_appearance.titlebar_fg=acc_appearance.border;if(!title_bg_set)acc_appearance.titlebar_bg=acc_appearance.controls_bg;
}

void acc_put(int x,int y,int ch,int attr)
{
  union REGS r;if(x<0||x>=acc_cols||y<0||y>=acc_rows)return;
  memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=0;r.h.dh=(unsigned char)y;r.h.dl=(unsigned char)x;int86(0x10,&r,&r);
  r.h.ah=9;r.h.al=(unsigned char)ch;r.h.bh=0;r.h.bl=(unsigned char)attr;r.x.cx=1;int86(0x10,&r,&r);
}
void acc_text(int x,int y,const char *s,int attr,int width)
{int i,ended=0;for(i=0;i<width;i++){if(!s||(!ended&&!s[i]))ended=1;acc_put(x+i,y,ended?' ':s[i],attr);}}
void acc_fill(int x,int y,int w,int h,int ch,int attr)
{int i,j;for(j=0;j<h;j++)for(i=0;i<w;i++)acc_put(x+i,y+j,ch,attr);}
void acc_clear(int attr){acc_fill(0,0,acc_cols,acc_rows,' ',attr);}

void acc_box(int x,int y,int w,int h,const char *title)
{
  int i,len,launch_len=7,title_len=0;unsigned short v;int top_border=ACC_ATTR(acc_appearance.titlebar_bg,acc_appearance.titlebar_fg);int launch_attr=ACC_ATTR(acc_appearance.titlebar_bg,acc_appearance.main_title);int heading_attr=ACC_ATTR(acc_appearance.titlebar_bg,acc_appearance.titles);int close_attr=ACC_ATTR(acc_appearance.titlebar_bg,acc_appearance.main_title);acc_fill(x,y,w,h,' ',ACC_BG);acc_fill(x,y,w,1,' ',ACC_ATTR(acc_appearance.titlebar_bg,acc_appearance.titlebar_bg));
  acc_put(x,y,213,top_border);acc_put(x+w-1,y,184,top_border);
  acc_put(x,y+h-1,192,ACC_BORDER);acc_put(x+w-1,y+h-1,217,ACC_BORDER);
  for(i=1;i<w-1;i++){acc_put(x+i,y,205,top_border);acc_put(x+i,y+h-1,196,ACC_BORDER);}
  for(i=1;i<h-1;i++){acc_put(x,y+i,179,ACC_BORDER);acc_put(x+w-1,y+i,179,ACC_BORDER);}
  if(title&&*title){len=w-18;if(len<0)len=0;title_len=(int)strlen(title);if(title_len>len)title_len=len;acc_put(x+2,y,181,top_border);acc_text(x+3,y,"Launch!",launch_attr,launch_len);acc_put(x+10,y,' ',heading_attr);if(title_len)acc_text(x+11,y,title,heading_attr,title_len);acc_put(x+11+title_len,y,198,top_border);}
  acc_put(x+w-5,y,181,top_border);acc_put(x+w-4,y,'X',close_attr);acc_put(x+w-3,y,198,top_border);close_x=x+w-4;close_y=y;
  if(x+w<acc_cols)for(i=1;i<=h;i++){v=*(unsigned short far *)MAKE_FP(0xB800,((y+i)*acc_cols+x+w)*2);acc_put(x+w,y+i,v&255,0x08);}
  if(y+h<acc_rows)for(i=1;i<=w;i++){v=*(unsigned short far *)MAKE_FP(0xB800,((y+h)*acc_cols+x+i)*2);acc_put(x+i,y+h,v&255,0x08);}
}
static void acc_button_draw(int x,int y,const char *text,int selected)
{int i,w=(int)strlen(text);unsigned short v;for(i=1;i<=w;i++){v=*(unsigned short far *)MAKE_FP(0xB800,((y+1)*acc_cols+x+i)*2);acc_put(x+i,y+1,223,((v>>8)&0xF0));}v=*(unsigned short far *)MAKE_FP(0xB800,(y*acc_cols+x+w)*2);acc_put(x+w,y,220,((v>>8)&0xF0));acc_text(x,y,text,ACC_CONTROL,w);if(selected){acc_put(x,y,16,ACC_CONTROL);acc_put(x+w-1,y,17,ACC_CONTROL);}}
void acc_button(int x,int y,const char *text,int selected)
{acc_button_draw(x,y,text,selected);if(acc_button_count<ACC_MAX_BUTTONS){acc_buttons[acc_button_count].x=x;acc_buttons[acc_button_count].y=y;acc_buttons[acc_button_count].text=text;acc_buttons[acc_button_count].selected=selected;acc_button_count++;}}
void acc_press_button(int x,int y,const char *text){union REGS r;int i,w=(int)strlen(text),mx,my;acc_put(x+w,y,' ',ACC_BG);for(i=1;i<=w;i++)acc_put(x+i,y+1,' ',ACC_BG);acc_text(x,y,text,ACC_CONTROL,w);if(acc_mouse_present){r.x.ax=1;int86(0x33,&r,&r);do{r.x.ax=3;int86(0x33,&r,&r);mx=r.x.cx/8;my=r.x.dx/8;(void)mx;(void)my;}while(r.x.bx&1);r.x.ax=2;int86(0x33,&r,&r);}}
void acc_scrollbar(int x,int top,int height,int position,int total,int page){int i,track=height-2,thumb=top+1;if(total>page&&track>1)thumb=top+1+position*(track-1)/(total-page);acc_put(x,top,30,ACC_CONTROL);for(i=1;i<height-1;i++)acc_put(x,top+i,i+top==thumb?219:176,ACC_CONTROL);acc_put(x,top+height-1,31,ACC_CONTROL);}
void acc_shadow(int x,int y,int w,int h){int i,j;unsigned short v;for(j=1;j<h;j++){v=*(unsigned short far *)MAKE_FP(saved_video_segment,((y+j)*acc_cols+x+w)*2);acc_put(x+w,y+j,219,(v>>8)&0xF0);}for(i=1;i<=w;i++){v=*(unsigned short far *)MAKE_FP(saved_video_segment,((y+h)*acc_cols+x+i)*2);acc_put(x+i,y+h,223,(v>>8)&0xF0);}}

int acc_mouse(int *x,int *y,int *buttons)
{
  union REGS r;if(!acc_mouse_present){*buttons=0;return 0;}
  r.x.ax=3;int86(0x33,&r,&r);*x=r.x.cx/8;*y=r.x.dx/8;*buttons=r.x.bx;return 1;
}
int acc_key(void){unsigned w=_bios_keybrd(_KEYBRD_READ);int c=w&255,scan=(w>>8)&255;if(!scan&&c)return 512+c;if(!c)return 256+scan;return c;}
void acc_wait(int *key,int *x,int *y,unsigned *buttons){union REGS r;int sx=(int)(mouse_raw_x/8),sy=(int)(mouse_raw_y/8),hover=-1,last_hover=-1,i,w;*key=0;*buttons=0;if(acc_mouse_present){r.x.ax=1;int86(0x33,&r,&r);mouse_visible=1;}for(;;){if(_bios_keybrd(_KEYBRD_READY)){*key=acc_key();break;}if(acc_mouse_present){r.x.ax=3;int86(0x33,&r,&r);mouse_raw_x=r.x.cx;mouse_raw_y=r.x.dx;*x=r.x.cx/8;*y=r.x.dx/8;*buttons=(unsigned)(r.x.bx&~mouse_last_buttons);mouse_last_buttons=r.x.bx;if(*buttons)break;if(*x!=sx||*y!=sy){hover=-1;for(i=0;i<acc_button_count;i++){w=(int)strlen(acc_buttons[i].text);if(*y==acc_buttons[i].y&&*x>=acc_buttons[i].x&&*x<acc_buttons[i].x+w){hover=i;break;}}if(hover!=last_hover){for(i=0;i<acc_button_count;i++)acc_button_draw(acc_buttons[i].x,acc_buttons[i].y,acc_buttons[i].text,acc_buttons[i].selected||i==hover);last_hover=hover;}sx=*x;sy=*y;if(r.x.bx&3){*buttons=ACC_MOUSE_MOVED|(unsigned)(r.x.bx&3);break;}}}}if(acc_mouse_present&&mouse_visible){r.x.ax=2;int86(0x33,&r,&r);mouse_visible=0;}acc_button_count=0;if((*buttons&1)&&*x==close_x&&*y==close_y){*buttons=0;*key=27;}}
unsigned long acc_ticks(void){return *(unsigned long far *)(((unsigned long)0x40<<16)|0x6C);}
int acc_help(int argc,char **argv,const char *name,const char *description){if(argc>1&&(!stricmp(argv[1],"/?")||!stricmp(argv[1],"-?"))){printf("%s - Launch! 3.1 accessory\n\n%s\n\nThis accessory requires !.EXE in the same directory.\n",name,description);return 1;}return 0;}

int acc_make_dir(const char *path){_mkdir(path);return _access(path,0)==0;}
void acc_path(char *out,const char *sub,const char *name)
{
  strcpy(out,acc_directory);if(sub&&*sub){strcat(out,sub);acc_make_dir(out);strcat(out,"\\");}
  if(name)strcat(out,name);
}

void acc_notice(const char *title,const char *message)
{
  const char *nl=strchr(message,'\n');int len1=nl?(int)(nl-message):(int)strlen(message),len2=nl?(int)strlen(nl+1):0,w=(len1>len2?len1:len2)+6,h=nl?8:7,x,y,k=0,mx=0,my=0,i,j,bx,old_close_x=close_x,old_close_y=close_y;unsigned mb=0;if(w<54)w=54;if(w>acc_cols-4)w=acc_cols-4;x=(acc_cols-w)/2;y=(acc_rows-h)/2;for(j=0;j<=h;j++)for(i=0;i<=w;i++)notice_screen[j*(w+1)+i]=*(unsigned short far *)MAKE_FP(saved_video_segment,((y+j)*acc_cols+x+i)*2);acc_box(x,y,w,h,title);
  acc_text(x+3,y+2,message,ACC_LABEL,len1);if(nl)acc_text(x+3,y+3,nl+1,ACC_LABEL,len2);
  bx=x+(w-6)/2;while(!k){acc_button(bx,y+h-3,"  OK  ",1);acc_wait(&k,&mx,&my,&mb);if((mb&1)&&my==y+h-3&&mx>=bx&&mx<bx+6)k=13;}
  for(j=0;j<=h;j++)for(i=0;i<=w;i++)*(unsigned short far *)MAKE_FP(saved_video_segment,((y+j)*acc_cols+x+i)*2)=notice_screen[j*(w+1)+i];close_x=old_close_x;close_y=old_close_y;
}

int acc_begin(const char *argv0,const char *title,int graphics)
{
  union REGS r;char launch[ACC_PATH];int i;
  (void)title;(void)graphics;acc_source_dir(argv0);sprintf(launch,"%s!.EXE",acc_directory);
  if(!acc_exists(launch)){puts("This accessory requires Launch!");return 0;}
  acc_load_config();memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);
  saved_mode=r.h.al;saved_page=r.h.bh;acc_cols=r.h.ah;if(acc_cols>80)acc_cols=80;
  acc_rows=*(unsigned char far *)(((unsigned long)0x40<<16)|0x84)+1;
  if(acc_rows<25||acc_rows>50)acc_rows=25;
  saved_video_segment=(saved_mode==7)?0xB000:0xB800;
  for(i=0;i<acc_cols*acc_rows;i++)saved_screen[i]=*(unsigned short far *)(((unsigned long)saved_video_segment<<16)|(i*2));
  r.h.ah=3;r.h.bh=0;int86(0x10,&r,&r);saved_cursor_start=r.h.ch;saved_cursor_end=r.h.cl;
  saved_cursor_y=r.h.dh;saved_cursor_x=r.h.dl;
  r.h.ah=1;r.h.ch=0x20;r.h.cl=0;int86(0x10,&r,&r);
  r.x.ax=0;int86(0x33,&r,&r);acc_mouse_present=r.x.ax!=0;
  if(acc_mouse_present){r.x.ax=4;r.x.cx=1;r.x.dx=1;int86(0x33,&r,&r);mouse_pointer_install();r.x.ax=1;int86(0x33,&r,&r);mouse_visible=1;r.x.ax=3;int86(0x33,&r,&r);mouse_last_buttons=r.x.bx;mouse_raw_x=r.x.cx;mouse_raw_y=r.x.dx;}
  return 1;
}

void acc_restore_screen(void)
{
  union REGS r;int i;memset(&r,0,sizeof(r));r.h.ah=0;r.h.al=(unsigned char)saved_mode;int86(0x10,&r,&r);
  /* A video mode set restores VGA/EGA attribute bit 7 to blink.  Launch! uses
     it as the bright-background bit, so explicitly select intensity again. */
  memset(&r,0,sizeof(r));r.x.ax=0x1003;r.x.bx=0;int86(0x10,&r,&r);
  for(i=0;i<acc_cols*acc_rows;i++)*(unsigned short far *)(((unsigned long)saved_video_segment<<16)|(i*2))=saved_screen[i];r.h.ah=1;r.h.ch=0x20;r.h.cl=0;int86(0x10,&r,&r);if(acc_mouse_present){r.x.ax=0;int86(0x33,&r,&r);mouse_pointer_install();}
}

void acc_end(void)
{
  union REGS r;int i;if(acc_mouse_present){r.x.ax=2;int86(0x33,&r,&r);mouse_pointer_restore();}
  if(saved_mode==3||saved_mode==2||saved_mode==7)
    for(i=0;i<acc_cols*acc_rows;i++)*(unsigned short far *)(((unsigned long)saved_video_segment<<16)|(i*2))=saved_screen[i];
  memset(&r,0,sizeof(r));r.h.ah=1;r.h.ch=(unsigned char)saved_cursor_start;r.h.cl=(unsigned char)saved_cursor_end;int86(0x10,&r,&r);
  r.h.ah=2;r.h.bh=(unsigned char)saved_page;r.h.dh=(unsigned char)saved_cursor_y;r.h.dl=(unsigned char)saved_cursor_x;int86(0x10,&r,&r);
}
