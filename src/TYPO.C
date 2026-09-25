/* Launch! Typo accessory - timed typing accuracy and WPM practice.
   Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <dos.h>
#include "ACCLIB.H"

void acc_mouse_display(int show);
void acc_modal_begin(void);
void acc_modal_end(void);

#define DLG_W 73
#define DLG_H 22
#define LEVELS 100
#define MAX_TEXT 3200
#define MAX_NAME 31
#define SEG_BASE 128
#define LOWER_HALF 222
#define KEY_LEFT 193
#define KEY_RIGHT 139
#define RECORDS 10

#define F_RESET 1
#define F_PREV 2
#define F_NEXT 3
#define F_RECORDS 4
#define F_CLOSE 5

typedef struct {unsigned short year;unsigned char month,day;unsigned short mistakes,wpm;} TYPO_RECORD;

static long level_offset[LEVELS];
static int level_count=0,level_secs=60;
static char level_name[MAX_NAME+1],level_text[MAX_TEXT+1];
static int level_len=0;

static void level_file_path(char *p)
{
  size_t n;strcpy(p,acc_directory);n=strlen(p);if(n&&p[n-1]!='\\'&&p[n-1]!='/')strcat(p,"\\");strcat(p,"TYPO.LVL");
}

static void strip_eol(char *s)
{int n=(int)strlen(s);while(n&&((unsigned char)s[n-1]==13||(unsigned char)s[n-1]==10))s[--n]=0;}

static int scan_levels(void)
{
  char p[ACC_PATH],line[128];FILE *f;long pos;level_count=0;level_file_path(p);f=fopen(p,"rt");if(!f)return 0;
  for(;;){pos=ftell(f);if(!fgets(line,sizeof(line),f))break;if(line[0]=='@'&&level_count<LEVELS)level_offset[level_count++]=pos;}
  fclose(f);return level_count==LEVELS;
}

static int load_level_text(int which)
{
  char p[ACC_PATH],head[128],*sep;FILE *f;int n;
  if(which<0||which>=level_count)return 0;level_file_path(p);f=fopen(p,"rt");if(!f)return 0;
  if(fseek(f,level_offset[which],SEEK_SET)!=0||!fgets(head,sizeof(head),f)){fclose(f);return 0;}
  strip_eol(head);sep=strchr(head+1,'|');if(!sep){fclose(f);return 0;}*sep++=0;
  level_secs=atoi(head+1);if(level_secs<30)level_secs=60;
  strncpy(level_name,sep,MAX_NAME);level_name[MAX_NAME]=0;
  if(!fgets(level_text,sizeof(level_text),f)){fclose(f);return 0;}fclose(f);strip_eol(level_text);
  n=(int)strlen(level_text);if(n>MAX_TEXT)n=MAX_TEXT;level_text[n]=0;level_len=n;return n>0;
}

static unsigned char old_seg[11][32],old_lower[32],old_key[2][32];
static int level=0,focus=-1,running=0,finished=0,errors=0,last_bad=0,last_key=0;
static unsigned long start_tick=0,last_second=999;
static long correct_chars=0;
static TYPO_RECORD records[RECORDS];static int record_count=0;

static const unsigned char keycap16[2][16]={
  {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F},
  {0xF0,0xFC,0xFC,0xFE,0xFE,0xFE,0xFE,0xFA,0xFE,0xFA,0xFA,0xFA,0xF2,0xE6,0x1C,0xF0}
};

static const unsigned char keycap14[2][14]={
  {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F},
  {0xF0,0xFC,0xFE,0xFE,0xFE,0xFA,0xFE,0xFA,0xFA,0xFA,0xF2,0xE6,0x1C,0xF0}
};

static void typo_font(int install)
{
  int i,h;unsigned char g[32];
  if(install){
    for(i=0;i<11;i++){acc_glyph_read(SEG_BASE+i,old_seg[i]);acc_glyph_library(64+i,SEG_BASE+i);}
    acc_glyph_read(LOWER_HALF,old_lower);memset(g,0,sizeof(g));h=acc_font_height();for(i=h/2;i<h;i++)g[i]=0xFF;acc_glyph_write(LOWER_HALF,g);
    acc_glyph_read(KEY_LEFT,old_key[0]);acc_glyph_read(KEY_RIGHT,old_key[1]);
    memset(g,0,sizeof(g));if(h==14)memcpy(g,keycap14[0],14);else memcpy(g,keycap16[0],16);acc_glyph_write(KEY_LEFT,g);
    memset(g,0,sizeof(g));if(h==14)memcpy(g,keycap14[1],14);else memcpy(g,keycap16[1],16);acc_glyph_write(KEY_RIGHT,g);
  } else {
    for(i=0;i<11;i++)acc_glyph_write(SEG_BASE+i,old_seg[i]);acc_glyph_write(LOWER_HALF,old_lower);
    acc_glyph_write(KEY_LEFT,old_key[0]);acc_glyph_write(KEY_RIGHT,old_key[1]);
  }
}

static unsigned char seg_char(int c)
{if(c>='0'&&c<='9')return(unsigned char)(SEG_BASE+c-'0');return(unsigned char)c;}

static void records_path(char *p){acc_path(p,"DATA","TYPO.REC");}

static void load_records(void)
{
  char path[ACC_PATH];FILE *f;unsigned yy,mm,dd,ee,ww;record_count=0;records_path(path);f=fopen(path,"rt");if(!f)return;
  while(record_count<RECORDS&&fscanf(f,"%u %u %u %u %u",&yy,&mm,&dd,&ee,&ww)==5){
    records[record_count].year=(unsigned short)yy;records[record_count].month=(unsigned char)mm;
    records[record_count].day=(unsigned char)dd;records[record_count].mistakes=(unsigned short)ee;
    records[record_count].wpm=(unsigned short)ww;record_count++;
  }
  fclose(f);
}

static void save_records(void)
{
  char path[ACC_PATH];FILE *f;int i;records_path(path);f=fopen(path,"wt");if(!f)return;
  for(i=0;i<record_count;i++)fprintf(f,"%u %u %u %u %u\n",records[i].year,records[i].month,records[i].day,records[i].mistakes,records[i].wpm);
  fclose(f);
}

static int record_better(const TYPO_RECORD *a,const TYPO_RECORD *b)
{return a->wpm>b->wpm||(a->wpm==b->wpm&&a->mistakes<b->mistakes);}

static void add_record(int wpm)
{
  union REGS r;TYPO_RECORD n;int i,j;
  memset(&r,0,sizeof(r));r.h.ah=0x2A;int86(0x21,&r,&r);
  n.year=r.x.cx;n.month=r.h.dh;n.day=r.h.dl;n.mistakes=(unsigned short)errors;n.wpm=(unsigned short)wpm;
  for(i=0;i<record_count;i++)if(record_better(&n,&records[i]))break;
  if(i>=RECORDS)return;if(record_count<RECORDS)record_count++;
  for(j=record_count-1;j>i;j--)records[j]=records[j-1];records[i]=n;save_records();
}

static void show_records(void)
{
  int w=42,h=15,x=(acc_cols-w)/2,y=(acc_rows-h)/2+1,i,key=0,mx=0,my=0;unsigned mb=0;char s[40];
  acc_modal_begin();
  acc_subbox(x,y,w,h,"Typo Records",0);acc_text(x+4,y+2,"Date",ACC_HEADING,12);acc_text(x+20,y+2,"Mistakes",ACC_TITLE,10);acc_text(x+33,y+2,"WPM",ACC_TEXT,5);
  for(i=0;i<RECORDS;i++){
    if(i<record_count){sprintf(s,"%02u/%02u/%04u",records[i].day,records[i].month,records[i].year);acc_text(x+4,y+4+i,s,ACC_LABEL,12);sprintf(s,"%u",records[i].mistakes);acc_text(x+23,y+4+i,s,ACC_TITLE,6);sprintf(s,"%u",records[i].wpm);acc_text(x+34,y+4+i,s,ACC_TEXT,5);}
    else {acc_text(x+4,y+4+i,"--/--/----",ACC_LABEL,12);acc_text(x+23,y+4+i,"-",ACC_TITLE,6);acc_text(x+34,y+4+i,"-",ACC_TEXT,5);}
  }
  while(key!=27){acc_wait(&key,&mx,&my,&mb);if(key!=27)key=0;}
  acc_modal_end();
}

static void speaker_error(void)
{
  unsigned old=(unsigned)inp(0x61),div=(unsigned)(1193180UL/180UL);unsigned long t;
  outp(0x43,0xB6);outp(0x42,div&255);outp(0x42,(div>>8)&255);outp(0x61,old|3);
  t=acc_ticks();while(acc_ticks()==t);outp(0x61,old);
}

static char stream_char(long p)
{if(p<0)return ' ';if(p>=level_len)return 0;return level_text[p];
}

static unsigned long elapsed(void)
{
  unsigned long e;if(!running)return finished?(unsigned long)level_secs:0;e=(acc_ticks()-start_tick)/18UL;if(e>(unsigned long)level_secs)e=(unsigned long)level_secs;return e;
}

static int current_wpm(void)
{
  unsigned long sec=elapsed();if(!sec)return 0;return(int)((correct_chars*12L+(long)sec/2L)/(long)sec);
}

static void reset_level(void)
{running=0;finished=0;errors=0;last_bad=0;last_key=0;correct_chars=0;start_tick=0;last_second=999;}

static void finish_session(int completed)
{
  int wpm;unsigned long sec=elapsed();if(finished)return;finished=1;running=0;if(!sec)sec=1;wpm=(int)((correct_chars*12L+(long)sec/2L)/(long)sec);add_record(wpm);
  acc_notice("Typo",completed?"Level complete! Result recorded.":"Time! Result recorded.");
}

static void number_box(int x,int y,int w,const char *s,int fg)
{
  int i,start;
  for(i=0;i<w;i++){
    acc_put(x+i,y,LOWER_HALF,ACC_ATTR(acc_appearance.background,0));
    acc_put(x+i,y+1,' ',0x00);
    acc_put(x+i,y+2,223,ACC_ATTR(acc_appearance.background,0));
  }
  start=x+w-1-(int)strlen(s);
  for(i=0;s[i];i++)acc_put(start+i,y+1,seg_char((unsigned char)s[i]),ACC_ATTR(0,fg));
}

static void draw_metrics(int x,int y)
{
  char s[16];int wpm;
  sprintf(s,"%03d",errors>999?999:errors);
  number_box(x+52,y+1,9,s,acc_appearance.main_title);
  wpm=current_wpm();if(wpm>999)wpm=999;sprintf(s,"%d",wpm);
  number_box(x+62,y+1,8,s,acc_appearance.launchers);
}
static void draw_status(int x,int y)
{
  char s[16];int i,filled;unsigned long sec=elapsed(),left=(unsigned long)level_secs-sec;
  if(left>(unsigned long)level_secs)left=(unsigned long)level_secs;
  filled=(int)(sec*28UL/(unsigned long)level_secs);
  acc_text(x+3,y+2,"Time",ACC_LABEL,4);
  for(i=0;i<28;i++)
    acc_put(x+8+i,y+2,i<filled?219:176,
            ACC_ATTR(acc_appearance.background,i<filled?acc_appearance.launchers:acc_appearance.launchers));
  sprintf(s,"%02lu:%02lu",left/60UL,left%60UL);acc_text(x+40,y+2,s,ACC_HEADING,6);
  draw_metrics(x,y);
  acc_text(x+53,y+4,"Mistakes",ACC_LABEL,8);
  acc_text(x+67,y+4,"WPM",ACC_LABEL,3);
}

static void draw_console_content(int x,int y)
{
  int i,center=31,w=65;long base=correct_chars-center;char line[66];
  int inner=x+1;
  acc_fill(inner,y+1,w,3,' ',ACC_ATTR(0,0));
  for(i=0;i<w;i++){char c=stream_char(base+i);line[i]=c?c:' ';}line[w]=0;
  acc_put(inner+center,y+1,31,ACC_ATTR(0,acc_appearance.main_title));
  acc_text(inner,y+2,line,ACC_ATTR(0,acc_appearance.labels),w);
  for(i=0;i<center;i++)
    if(base+i>=0&&base+i<correct_chars)
      {char c=stream_char(base+i);if(c)acc_put(inner+i,y+3,c,ACC_ATTR(0,acc_appearance.launchers));}
  if(last_bad)acc_put(inner+center,y+3,last_bad,ACC_ATTR(0,acc_appearance.main_title));
}
static void draw_console(int x,int y)
{
  int i;
  acc_put(x,y,218,ACC_BORDER);for(i=1;i<66;i++)acc_put(x+i,y,196,ACC_BORDER);acc_put(x+66,y,191,ACC_BORDER);
  for(i=1;i<4;i++){acc_put(x,y+i,179,ACC_BORDER);acc_put(x+66,y+i,179,ACC_BORDER);}
  acc_put(x,y+4,192,ACC_BORDER);for(i=1;i<66;i++)acc_put(x+i,y+4,196,ACC_BORDER);acc_put(x+66,y+4,217,ACC_BORDER);
  draw_console_content(x,y);
}

static int base_key(int c)
{
  if(c>='a'&&c<='z')return toupper(c);if(c>='A'&&c<='Z')return c;
  switch(c){case '!':return'1';case '@':return'2';case '#':return'3';case '$':return'4';case '%':return'5';case '^':return'6';case '&':return'7';case '*':return'8';case '(':return'9';case ')':return'0';case '_':return'-';case '+':return'=';case '{':return'[';case '}':return']';case ':':return';';case '"':return'\'';case '<':return',';case '>':return'.';case '?':return'/';case '|':return'\\';}return c;
}

static void key_mark(int x,int y,int ch,int pressed)
{
  int a=pressed?ACC_SELECT:ACC_CONTROL;
  int capfg=pressed?acc_appearance.selected_bg:acc_appearance.controls_bg;
  int ca=ACC_ATTR(acc_appearance.background,capfg);
  if(!pressed&&(toupper((unsigned char)ch)=='F'||toupper((unsigned char)ch)=='J'))a=ACC_ATTR(acc_appearance.controls_bg,acc_appearance.main_title);
  acc_put(x,y,KEY_LEFT,ca);acc_put(x+1,y,ch,a);acc_put(x+2,y,KEY_RIGHT,ca);
}

static void space_mark(int x,int y,int pressed)
{
  int a=pressed?ACC_SELECT:ACC_CONTROL;
  int capfg=pressed?acc_appearance.selected_bg:acc_appearance.controls_bg;
  int ca=ACC_ATTR(acc_appearance.background,capfg);
  acc_put(x,y,KEY_LEFT,ca);acc_text(x+1,y,"  SPACE  ",a,9);acc_put(x+10,y,KEY_RIGHT,ca);
}

static int keyboard_flags(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.h.ah=2;int86(0x16,&r,&r);return r.h.al;
}

static char shifted_key(char c)
{
  switch(c){case '1':return '!';case '2':return '@';case '3':return '#';case '4':return '$';case '5':return '%';case '6':return '^';case '7':return '&';case '8':return '*';case '9':return '(';case '0':return ')';case '-':return '_';case '=':return '+';case '[':return '{';case ']':return '}';case '\\':return '|';case ';':return ':';case '\'':return '"';case ',':return '<';case '.':return '>';case '/':return '?';}return c;
}

static void wide_key(int x,int y,const char *label,int pressed,int width)
{
  int a=pressed?ACC_SELECT:ACC_CONTROL,capfg=pressed?acc_appearance.selected_bg:acc_appearance.controls_bg;
  int ca=ACC_ATTR(acc_appearance.background,capfg),n=(int)strlen(label),i,start;
  if(width<3)return;acc_put(x,y,KEY_LEFT,ca);for(i=1;i<width-1;i++)acc_put(x+i,y,' ',a);acc_put(x+width-1,y,KEY_RIGHT,ca);
  if(n>width-2)n=width-2;start=x+(width-n)/2;acc_text(start,y,label,a,n);
}

static void draw_keyboard(int x,int y)
{
  static const char *r1="1234567890-=";
  static const char *r2="QWERTYUIOP[]\\";
  static const char *r3="ASDFGHJKL;'";
  static const char *r4="ZXCVBNM,./";
  int i,k=base_key(last_key),fl=keyboard_flags(),shift=(fl&3)!=0,caps=(fl&0x40)!=0;
  int upper=shift^caps;char c;
  /* Keyboard legends remain ordinary CP437. */
  for(i=0;r1[i];i++){c=shift?shifted_key(r1[i]):r1[i];key_mark(x+6+i*4,y,c,k==r1[i]);}
  for(i=0;r2[i];i++){c=r2[i];if(isalpha((unsigned char)c))c=(char)(upper?toupper(c):tolower(c));else if(shift)c=shifted_key(c);key_mark(x+8+i*4,y+1,c,k==r2[i]);}
  wide_key(x+2,y+2,"CAPS",caps,7);
  for(i=0;r3[i];i++){c=r3[i];if(isalpha((unsigned char)c))c=(char)(upper?toupper(c):tolower(c));else if(shift)c=shifted_key(c);key_mark(x+10+i*4,y+2,c,k==r3[i]);}
  wide_key(x+3,y+3,"SHIFT",fl&2,8);
  for(i=0;r4[i];i++){c=r4[i];if(isalpha((unsigned char)c))c=(char)(upper?toupper(c):tolower(c));else if(shift)c=shifted_key(c);key_mark(x+12+i*4,y+3,c,k==r4[i]);}
  wide_key(x+52,y+3,"SHIFT",fl&1,8);
  wide_key(x+20,y+4,"SPACE",k==' ',21);
}

static void draw_keyboard_key(int x,int y,int key,int pressed)
{
  static const char *r1="1234567890-=";
  static const char *r2="QWERTYUIOP[]\\";
  static const char *r3="ASDFGHJKL;'";
  static const char *r4="ZXCVBNM,./";
  int i,k=base_key(key),fl=keyboard_flags(),shift=(fl&3)!=0,caps=(fl&0x40)!=0;
  int upper=shift^caps;char c;
  if(!key)return;
  if(k==' '){wide_key(x+20,y+4,"SPACE",pressed,21);return;}
  for(i=0;r1[i];i++)if(k==r1[i]){c=shift?shifted_key(r1[i]):r1[i];key_mark(x+6+i*4,y,c,pressed);return;}
  for(i=0;r2[i];i++)if(k==r2[i]){c=r2[i];if(isalpha((unsigned char)c))c=(char)(upper?toupper(c):tolower(c));else if(shift)c=shifted_key(c);key_mark(x+8+i*4,y+1,c,pressed);return;}
  for(i=0;r3[i];i++)if(k==r3[i]){c=r3[i];if(isalpha((unsigned char)c))c=(char)(upper?toupper(c):tolower(c));else if(shift)c=shifted_key(c);key_mark(x+10+i*4,y+2,c,pressed);return;}
  for(i=0;r4[i];i++)if(k==r4[i]){c=r4[i];if(isalpha((unsigned char)c))c=(char)(upper?toupper(c):tolower(c));else if(shift)c=shifted_key(c);key_mark(x+12+i*4,y+3,c,pressed);return;}
}

static void draw_buttons(int x,int y)
{
  acc_button(x+3,y+DLG_H-3," Refresh ",focus==F_RESET);
  if(level>0)acc_button(x+12,y+DLG_H-3," Prev ",focus==F_PREV);else acc_button_disabled(x+12,y+DLG_H-3," Prev ");
  if(level<LEVELS-1)acc_button(x+20,y+DLG_H-3," Next ",focus==F_NEXT);else acc_button_disabled(x+20,y+DLG_H-3," Next ");
  acc_button(x+28,y+DLG_H-3," Records ",focus==F_RECORDS);
  acc_button(x+DLG_W-10,y+DLG_H-3," Exit ",focus==F_CLOSE);
}

static void draw_all(int x,int y)
{
  char s[48];
  acc_fill(x+1,y+1,DLG_W-2,DLG_H-2,' ',ACC_BG);
  draw_status(x,y);
  sprintf(s,"Level %02d/%02d : %s",level+1,LEVELS,level_name);
  acc_text(x+3,y+4,s,ACC_HEADING,46);
  draw_console(x+3,y+6);
  draw_keyboard(x+5,y+12);
  draw_buttons(x,y);
}

static void level_change(int d)
{int n=level+d;if(n<0||n>=LEVELS)return;level=n;load_level_text(level);reset_level();}

static int button_hit(int x,int y,int mx,int my)
{
  if(my!=y+DLG_H-3)return 0;
  if(mx>=x+3&&mx<x+9)return F_RESET;
  if(level>0&&mx>=x+12&&mx<x+17)return F_PREV;
  if(level<LEVELS-1&&mx>=x+20&&mx<x+25)return F_NEXT;
  if(mx>=x+28&&mx<x+37)return F_RECORDS;
  if(mx>=x+DLG_W-10&&mx<x+DLG_W-4)return F_CLOSE;
  return 0;
}

static void process_char(int ch)
{
  char want=stream_char(correct_chars);last_key=ch;if(!running&&!finished){running=1;start_tick=acc_ticks();}
  if(ch==(unsigned char)want){correct_chars++;last_bad=0;if(correct_chars>=level_len)finish_session(1);}
  else {errors++;last_bad=ch;speaker_error();}
}

int main(int argc,char **argv)
{
  int x,y,key=0,mx=0,my=0,buttons=0,last_buttons=0,hit,need=1,quit=0,last_kflags=-1,kflags,old_key;unsigned long sec;
  if(acc_help(argc,argv,"!TYPO","Timed typing practice for accuracy and words per minute."))return 0;
  if(!acc_begin(argv[0],"Typo",0))return 1;acc_mouse_display(1);typo_font(1);load_records();if(!scan_levels()||!load_level_text(0)){acc_notice("Typo Error","TYPO.LVL is missing or invalid.");typo_font(0);acc_end_screen();acc_end();return 1;}x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;acc_box(x,y,DLG_W,DLG_H,"Typo");reset_level();
  while(!quit){
    sec=elapsed();if(running&&sec>=(unsigned long)level_secs){finish_session(0);acc_box(x,y,DLG_W,DLG_H,"Typo");need=1;}
    if(sec!=last_second){last_second=sec;draw_status(x,y);}
    kflags=keyboard_flags();
    if(kflags!=last_kflags){
      last_kflags=kflags;
      if(!need)draw_keyboard(x+5,y+12);
    }
    if(need){draw_all(x,y);need=0;}
    if(kbhit()){
      key=acc_key();
      if(key==27){if(running){reset_level();need=1;key=0;}else quit=1;}
      else if(key==9||key==271){if(focus<0)focus=(key==271)?F_CLOSE:0;else if(key==271){focus--;if(level==LEVELS-1&&focus==F_NEXT)focus--;if(level==0&&focus==F_PREV)focus--;if(focus<0)focus=F_CLOSE;}else{focus++;if(level==0&&focus==F_PREV)focus++;if(level==LEVELS-1&&focus==F_NEXT)focus++;if(focus>F_CLOSE)focus=0;}last_key=0;need=1;key=0;}
      else if(key==13&&focus>0){
        if(focus==F_RESET)reset_level();
        else if(focus==F_PREV&&level>0)level_change(-1);
        else if(focus==F_RECORDS){show_records();acc_box(x,y,DLG_W,DLG_H,"Typo");}
        else if(focus==F_NEXT&&level<LEVELS-1)level_change(1);
        else if(focus==F_CLOSE)quit=1;
        if(!quit){need=1;key=0;}
      } else if(key==8&&!finished){
        focus=0;old_key=last_key;last_key=0;
        if(last_bad)last_bad=0;else if(correct_chars>0)correct_chars--;
        draw_console_content(x+3,y+6);
        draw_metrics(x,y);
        draw_keyboard_key(x+5,y+12,old_key,0);
        key=0;
      } else if(key>=32&&key<127&&!finished){
        focus=0;old_key=last_key;process_char(key);
        if(finished){
          acc_box(x,y,DLG_W,DLG_H,"Typo");
          need=1;
        }else{
          draw_console_content(x+3,y+6);
          draw_metrics(x,y);
          if(old_key!=last_key)draw_keyboard_key(x+5,y+12,old_key,0);
          draw_keyboard_key(x+5,y+12,last_key,1);
        }
        key=0;
      }
      else key=0;
    }
    if(acc_mouse_present){
      acc_mouse(&mx,&my,&buttons);if(buttons&ACC_MOUSE_OUTSIDE){key=27;break;}
      if((buttons&1)&&!(last_buttons&1)){
        if((buttons&1)&&my==y&&(mx==x+DLG_W-5||mx==x+DLG_W-4)){quit=1;need=1;}
        else {hit=button_hit(x,y,mx,my);if(hit){focus=hit;last_key=0;
          if(hit==F_RESET)reset_level();
          else if(hit==F_PREV)level_change(-1);
          else if(hit==F_RECORDS){show_records();acc_box(x,y,DLG_W,DLG_H,"Typo");}
          else if(hit==F_NEXT)level_change(1);
          else if(hit==F_CLOSE)quit=1;need=1;
        }}
      }
      last_buttons=buttons;
    }
  }
  typo_font(0);acc_end_screen();acc_end();return 0;
}
