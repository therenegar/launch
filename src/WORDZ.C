/* Launch! Wordz accessory - straight-line word search puzzle game.
   Release 3.6 V1h uses an external long-play puzzle library with an 8x7 board,
   horizontal, vertical and diagonal words, click/drag selection and up to 12 answers.
   Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <dos.h>
#include "ACCLIB.H"

void acc_mouse_display(int show);

#define GW 8
#define GH 7
#define WORDS 12
#define MAX_PUZZLES 64
#define MAX_HINT 79
#define WZ_SEG_BASE 128
#define WZ_COLON 139
#define WZ_LOWER_HALF 222
#define WZ_KEY_LEFT 193
#define WZ_KEY_RIGHT 194
#define DLG_W 70
#define DLG_H 23
#define MAX_PATH 8

#define BTN_RESET 1
#define BTN_PREV 2
#define BTN_NEXT 3
#define BTN_HINT 4
#define BTN_CLOSE 5

typedef struct {signed char x,y,dx,dy;} WORD_PLACE;
static long puzzle_offset[MAX_PUZZLES];
static int puzzle_total=0,puzzle_words=0;

static unsigned char wz_old_seg[11][32],wz_old_colon[32],wz_old_lower[32],wz_old_key[2][32];
static const unsigned char wz_keycap16[2][16]={
 {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F},
 {0xF0,0xFC,0xFC,0xFE,0xFE,0xFE,0xFE,0xFA,0xFE,0xFA,0xFA,0xFA,0xF2,0xE6,0x1C,0xF0}
};
static const unsigned char wz_keycap14[2][14]={
 {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F},
 {0xF0,0xFC,0xFE,0xFE,0xFE,0xFA,0xFE,0xFA,0xFA,0xFA,0xF2,0xE6,0x1C,0xF0}
};
static void wordz_font(int install)
{
 int i,h;unsigned char g[32];
 if(install){
  h=acc_font_height();
  for(i=0;i<11;i++){acc_glyph_read(WZ_SEG_BASE+i,wz_old_seg[i]);acc_glyph_library(64+i,WZ_SEG_BASE+i);}
  acc_glyph_read(WZ_COLON,wz_old_colon);acc_glyph_library(58,WZ_COLON);
  acc_glyph_read(WZ_LOWER_HALF,wz_old_lower);memset(g,0,sizeof(g));for(i=h/2;i<h;i++)g[i]=0xFF;acc_glyph_write(WZ_LOWER_HALF,g);
  acc_glyph_read(WZ_KEY_LEFT,wz_old_key[0]);acc_glyph_read(WZ_KEY_RIGHT,wz_old_key[1]);
  memset(g,0,sizeof(g));if(h==14)memcpy(g,wz_keycap14[0],14);else memcpy(g,wz_keycap16[0],16);acc_glyph_write(WZ_KEY_LEFT,g);
  memset(g,0,sizeof(g));if(h==14)memcpy(g,wz_keycap14[1],14);else memcpy(g,wz_keycap16[1],16);acc_glyph_write(WZ_KEY_RIGHT,g);
 } else {
  for(i=0;i<11;i++)acc_glyph_write(WZ_SEG_BASE+i,wz_old_seg[i]);
  acc_glyph_write(WZ_COLON,wz_old_colon);acc_glyph_write(WZ_LOWER_HALF,wz_old_lower);
  acc_glyph_write(WZ_KEY_LEFT,wz_old_key[0]);acc_glyph_write(WZ_KEY_RIGHT,wz_old_key[1]);
 }
}
static unsigned char wordz_seg_char(int c)
{
 if(c>='0'&&c<='9')return(unsigned char)(WZ_SEG_BASE+c-'0');
 if(c==':')return WZ_COLON;
 return(unsigned char)c;
}
static void wordz_time_box(int x,int y,int width,unsigned long sec)
{
 char s[8];int i,start;if(sec>5999UL)sec=5999UL;
 sprintf(s,"%02lu:%02lu",sec/60UL,sec%60UL);
 for(i=0;i<width;i++){
  acc_put(x+i,y,WZ_LOWER_HALF,ACC_ATTR(acc_appearance.background,0));
  acc_put(x+i,y+1,' ',0x00);
  acc_put(x+i,y+2,223,ACC_ATTR(acc_appearance.background,0));
 }
 start=x+(width-(int)strlen(s))/2;
 for(i=0;s[i];i++)acc_put(start+i,y+1,wordz_seg_char((unsigned char)s[i]),ACC_ATTR(0,acc_appearance.launchers));
}

static char puzzle_title[16],puzzle_word[WORDS][9],puzzle_hint[WORDS][MAX_HINT+1];
static WORD_PLACE puzzle_place[WORDS];

static void puzzle_file_path(char *p)
{
  size_t n;strcpy(p,acc_directory);n=strlen(p);if(n&&p[n-1]!='\\'&&p[n-1]!='/')strcat(p,"\\");strcat(p,"WORDZ.LVL");
}
static void puzzle_strip(char *s)
{int n=(int)strlen(s);while(n&&(s[n-1]=='\r'||s[n-1]=='\n'))s[--n]=0;}
static int scan_puzzles(void)
{
  char p[ACC_PATH],line[192];FILE *f;long pos;puzzle_total=0;puzzle_file_path(p);f=fopen(p,"rt");if(!f)return 0;
  for(;;){pos=ftell(f);if(!fgets(line,sizeof(line),f))break;if(line[0]=='@'&&puzzle_total<MAX_PUZZLES)puzzle_offset[puzzle_total++]=pos;}
  fclose(f);return puzzle_total>0;
}
static int load_puzzle(int which)
{
  char p[ACC_PATH],line[192],*part[6],*q;FILE *f;int i,j;
  if(which<0||which>=puzzle_total)return 0;puzzle_file_path(p);f=fopen(p,"rt");if(!f)return 0;
  if(fseek(f,puzzle_offset[which],SEEK_SET)!=0||!fgets(line,sizeof(line),f)){fclose(f);return 0;}
  puzzle_strip(line);if(line[0]!='@'){fclose(f);return 0;}strncpy(puzzle_title,line+1,15);puzzle_title[15]=0;puzzle_words=0;
  while(puzzle_words<WORDS&&fgets(line,sizeof(line),f)){
    puzzle_strip(line);if(!line[0]||line[0]=='@')break;part[0]=line;q=line;
    for(i=1;i<6;i++){q=strchr(q,'|');if(!q){fclose(f);return 0;}*q++=0;part[i]=q;}
    if(strlen(part[0])<3||strlen(part[0])>8){fclose(f);return 0;}
    strncpy(puzzle_word[puzzle_words],part[0],8);puzzle_word[puzzle_words][8]=0;
    strncpy(puzzle_hint[puzzle_words],part[1],MAX_HINT);puzzle_hint[puzzle_words][MAX_HINT]=0;
    puzzle_place[puzzle_words].x=(signed char)atoi(part[2]);puzzle_place[puzzle_words].y=(signed char)atoi(part[3]);
    puzzle_place[puzzle_words].dx=(signed char)atoi(part[4]);puzzle_place[puzzle_words].dy=(signed char)atoi(part[5]);
    for(j=0;puzzle_word[puzzle_words][j];j++)puzzle_word[puzzle_words][j]=(char)toupper((unsigned char)puzzle_word[puzzle_words][j]);
    puzzle_words++;
  }
  fclose(f);return puzzle_words>=7;
}

static char grid[GH][GW];
static unsigned char played[GH][GW],solved[WORDS];
static signed char pathx[MAX_PATH],pathy[MAX_PATH];
static unsigned char solx[WORDS][MAX_PATH],soly[WORDS][MAX_PATH],sollen[WORDS];
static int path_len=0,cursor_x=0,cursor_y=0,puzzle_no=0,focus=-1,hint_word=0;
static unsigned long start_tick=0,pause_tick=0;
static int timer_running=0,finished=0,mistakes=0;

static int puzzle_word_count(void)
{return puzzle_words;}

static int puzzle_data_valid(void)
{
  int i,j,n,x,y;if(puzzle_words<1||puzzle_words>WORDS)return 0;
  for(i=0;i<puzzle_words;i++){
    n=(int)strlen(puzzle_word[i]);if(n<3||n>MAX_PATH||!puzzle_hint[i][0])return 0;
    if(puzzle_place[i].dx==0&&puzzle_place[i].dy==0)return 0;
    for(j=0;j<n;j++){x=puzzle_place[i].x+puzzle_place[i].dx*j;y=puzzle_place[i].y+puzzle_place[i].dy*j;if(x<0||x>=GW||y<0||y>=GH)return 0;}
  }
  return 1;
}

static void build_grid(void)
{
  int x,y,i,j,sx,sy,dx,dy,n,count=puzzle_word_count();
  for(y=0;y<GH;y++)for(x=0;x<GW;x++)grid[y][x]=(char)('A'+((puzzle_no*7+x*11+y*5)%26));
  memset(played,0,sizeof(played));memset(solved,0,sizeof(solved));memset(sollen,0,sizeof(sollen));
  for(i=0;i<count;i++){
    sx=puzzle_place[i].x;sy=puzzle_place[i].y;dx=puzzle_place[i].dx;dy=puzzle_place[i].dy;
    n=(int)strlen(puzzle_word[i]);if(n>MAX_PATH)n=MAX_PATH;sollen[i]=(unsigned char)n;
    for(j=0;j<n;j++){x=sx+dx*j;y=sy+dy*j;grid[y][x]=puzzle_word[i][j];solx[i][j]=(unsigned char)x;soly[i][j]=(unsigned char)y;}
  }
}

static void reset_puzzle(void)
{
  build_grid();path_len=0;cursor_x=0;cursor_y=0;hint_word=0;
  start_tick=0;pause_tick=0;timer_running=0;finished=0;mistakes=0;
}

static int adjacent(int ax,int ay,int bx,int by)
{
  int dx=abs(ax-bx),dy=abs(ay-by);return(dx<=1&&dy<=1&&(dx||dy));
}

static int path_contains(int x,int y)
{
  int i;for(i=0;i<path_len;i++)if(pathx[i]==x&&pathy[i]==y)return i;return -1;
}

static void start_timer(void)
{if(!timer_running&&!finished){timer_running=1;start_tick=acc_ticks();}}

static void path_remove_at(int at)
{
  int i;if(at<0||at>=path_len)return;
  for(i=at;i<path_len-1;i++){pathx[i]=pathx[i+1];pathy[i]=pathy[i+1];}path_len--;
}

static void path_toggle(int x,int y)
{
  int at=path_contains(x,y);start_timer();
  if(at>=0){path_remove_at(at);return;}
  if(path_len>=MAX_PATH)return;
  if(path_len&&!adjacent(pathx[path_len-1],pathy[path_len-1],x,y))path_len=0;
  pathx[path_len]=(signed char)x;pathy[path_len]=(signed char)y;path_len++;
}

static void path_drag_add(int x,int y)
{
  if(path_contains(x,y)>=0||path_len>=MAX_PATH)return;
  if(path_len&&!adjacent(pathx[path_len-1],pathy[path_len-1],x,y))return;
  start_timer();pathx[path_len]=(signed char)x;pathy[path_len]=(signed char)y;path_len++;
}

static void selected_word(char *out)
{
  int i;for(i=0;i<path_len;i++)out[i]=grid[pathy[i]][pathx[i]];out[path_len]=0;
}

static int reverse_equal(const char *a,const char *b)
{
  int i,n=(int)strlen(a);if((int)strlen(b)!=n)return 0;
  for(i=0;i<n;i++)if(toupper(a[i])!=toupper(b[n-1-i]))return 0;return 1;
}

static int word_index(const char *s)
{
  int i,count=puzzle_word_count();for(i=0;i<count;i++)if(!stricmp(s,puzzle_word[i])||reverse_equal(s,puzzle_word[i]))return i;return -1;
}

static int path_is_solution(int wi)
{
  int i,n;if(wi<0)return 0;n=sollen[wi];if(path_len!=n)return 0;
  for(i=0;i<n;i++)if(pathx[i]!=solx[wi][i]||pathy[i]!=soly[wi][i])break;
  if(i==n)return 1;
  for(i=0;i<n;i++)if(pathx[i]!=solx[wi][n-1-i]||pathy[i]!=soly[wi][n-1-i])return 0;
  return 1;
}

static int solved_count(void)
{int i,n=0,count=puzzle_word_count();for(i=0;i<count;i++)if(solved[i])n++;return n;}

static void submit_path(void)
{
  char s[MAX_PATH+1];int wi,i,count=puzzle_word_count();if(path_len<2)return;
  selected_word(s);wi=word_index(s);
  if(wi>=0&&path_is_solution(wi)&&!solved[wi]){
    solved[wi]=1;
    for(i=0;i<path_len;i++)played[pathy[i]][pathx[i]]=1;
    path_len=0;hint_word=wi;
    if(solved_count()==count){
      pause_tick=(acc_ticks()-start_tick)/18UL;finished=1;timer_running=0;
      acc_notice("Wordz","Puzzle complete!");
    }
  } else {
    mistakes++;
    if(wi>=0)acc_notice("Wordz","Correct word, but wrong path.");
    else acc_notice("Wordz","That selection is not one of the words.");
    path_len=0;
  }
}

static int cell_attr(int x,int y)
{
  if(path_contains(x,y)>=0)return ACC_SELECT;
  if(played[y][x])return ACC_CONTROL;
  if(focus==0&&x==cursor_x&&y==cursor_y)return ACC_HEADING;
  return ACC_TEXT;
}

static void draw_grid_frame(int x,int y)
{
  int i;
  acc_put(x,y,218,ACC_BORDER);for(i=1;i<34;i++)acc_put(x+i,y,196,ACC_BORDER);acc_put(x+34,y,191,ACC_BORDER);
  for(i=1;i<14;i++){acc_put(x,y+i,179,ACC_BORDER);acc_put(x+34,y+i,179,ACC_BORDER);}
  acc_put(x,y+14,192,ACC_BORDER);for(i=1;i<34;i++)acc_put(x+i,y+14,196,ACC_BORDER);acc_put(x+34,y+14,217,ACC_BORDER);
}

static void draw_grid(int bx,int by)
{
  int x,y,a,sel,done,cap;
  for(y=0;y<GH;y++)for(x=0;x<GW;x++){
    sel=path_contains(x,y)>=0;done=played[y][x]!=0;a=cell_attr(x,y);
    if(sel){
      cap=ACC_ATTR(acc_appearance.background,acc_appearance.selected_bg);
      acc_put(bx+x*4-1,by+y*2,WZ_KEY_LEFT,cap);
      acc_put(bx+x*4,by+y*2,grid[y][x],ACC_SELECT);
      acc_put(bx+x*4+1,by+y*2,WZ_KEY_RIGHT,cap);
    } else if(done){
      cap=ACC_ATTR(acc_appearance.background,acc_appearance.controls_bg);
      acc_put(bx+x*4-1,by+y*2,WZ_KEY_LEFT,cap);
      acc_put(bx+x*4,by+y*2,grid[y][x],ACC_CONTROL);
      acc_put(bx+x*4+1,by+y*2,WZ_KEY_RIGHT,cap);
    } else {
      acc_text(bx+x*4-1,by+y*2,"   ",ACC_BG,3);
      acc_put(bx+x*4,by+y*2,grid[y][x],a);
    }
  }
}

static void draw_grid_cell(int bx,int by,int x,int y)
{
  int a,sel,done,cap;
  if(x<0||x>=GW||y<0||y>=GH)return;
  sel=path_contains(x,y)>=0;done=played[y][x]!=0;a=cell_attr(x,y);
  if(sel){
    cap=ACC_ATTR(acc_appearance.background,acc_appearance.selected_bg);
    acc_put(bx+x*4-1,by+y*2,WZ_KEY_LEFT,cap);
    acc_put(bx+x*4,by+y*2,grid[y][x],ACC_SELECT);
    acc_put(bx+x*4+1,by+y*2,WZ_KEY_RIGHT,cap);
  } else if(done){
    cap=ACC_ATTR(acc_appearance.background,acc_appearance.controls_bg);
    acc_put(bx+x*4-1,by+y*2,WZ_KEY_LEFT,cap);
    acc_put(bx+x*4,by+y*2,grid[y][x],ACC_CONTROL);
    acc_put(bx+x*4+1,by+y*2,WZ_KEY_RIGHT,cap);
  } else {
    acc_text(bx+x*4-1,by+y*2,"   ",ACC_BG,3);
    acc_put(bx+x*4,by+y*2,grid[y][x],a);
  }
}

static void draw_word_slot(int x,int y,int number,const char *word,int done)
{
  char slot[9],s[16];int i,n=(int)strlen(word);
  if(n>8)n=8;
  if(done){strncpy(slot,word,n);slot[n]=0;}
  else {for(i=0;i<n;i++)slot[i]='_';slot[n]=0;}
  if(number<10)sprintf(s,"%d  %-8s",number,slot);
  else sprintf(s,"%d %-8s",number,slot);
  acc_text(x,y,s,ACC_CONTROL,11);
}

static void draw_words(int x,int y)
{
  int i,count=puzzle_word_count();char title[16];int title_attr=ACC_ATTR(acc_appearance.controls_bg,acc_appearance.titles);
  acc_fill(x,y,27,9,' ',ACC_CONTROL);sprintf(title,"%s",puzzle_title);acc_text(x+2,y+1,title,title_attr,13);
  for(i=0;i<WORDS;i++){
    int px=x+2+(i>=6?14:0),py=y+2+(i%6);
    if(i<count)draw_word_slot(px,py,i+1,puzzle_word[i],solved[i]);else acc_text(px,py,"",ACC_CONTROL,11);
  }
}

static void draw_plain_button(int x,int y,const char *text,int selected)
{
  int i,w=(int)strlen(text),a=ACC_CONTROL,sa=ACC_ATTR(acc_appearance.background,0);
  int fa=ACC_ATTR(acc_appearance.controls_bg,acc_appearance.controls_fg);
  acc_fill(x,y,w,1,' ',a);acc_text(x,y,text,a,w);
  for(i=1;i<=w;i++)acc_put(x+i,y+1,220,sa);
  acc_put(x+w,y,245,sa);acc_put(x+w,y+1,244,sa);
  if(selected){acc_put(x,y,169,fa);acc_put(x+w-1,y,170,fa);}
}

static void wrap_hint(int x,int y,const char *s)
{
  char line[26];const char *p=s,*q;int row=0,len=0,n,i;
  for(i=0;i<4;i++)acc_text(x,y+i,"",ACC_LABEL,25);line[0]=0;
  while(*p&&row<4){
    while(*p==' ')p++;if(!*p)break;q=p;while(*q&&*q!=' ')q++;n=(int)(q-p);
    if(len&&len+1+n>25){acc_text(x,y+row,line,ACC_LABEL,25);row++;len=0;line[0]=0;if(row>=4)break;}
    if(len)line[len++]=' ';
    if(n>25-len)n=25-len;memcpy(line+len,p,n);len+=n;line[len]=0;p=q;
  }
  if(row<4&&len)acc_text(x,y+row,line,ACC_LABEL,25);
}

static void draw_hint(int x,int y)
{
  char h[12];int count=puzzle_word_count();if(hint_word>=count)hint_word=0;
  sprintf(h,"Hint #%d",hint_word+1);acc_text(x,y,h,ACC_HEADING,8);draw_plain_button(x+11,y," Next ",focus==BTN_HINT);
  wrap_hint(x,y+2,puzzle_hint[hint_word]);
}

static unsigned long elapsed_seconds(void)
{
  if(!timer_running)return start_tick?pause_tick:0;return(acc_ticks()-start_tick)/18UL;
}

static void draw_status(int x,int y)
{
  char s[32];unsigned long sec=elapsed_seconds();int found=solved_count(),count=puzzle_word_count();
  wordz_time_box(x+3,y+1,14,sec);
  sprintf(s,"Found: %d/%d",found,count);acc_text(x+20,y+2,s,ACC_HEADING,12);
}

static void draw_buttons(int x,int y)
{
  char s[24];
  acc_button(x+3,y+DLG_H-3," Refresh ",focus==BTN_RESET);
  acc_button(x+12,y+DLG_H-3," Prev ",focus==BTN_PREV);
  acc_button(x+20,y+DLG_H-3," Next ",focus==BTN_NEXT);
  sprintf(s,"Puzzle %d/%d",puzzle_no+1,puzzle_total);acc_text(x+28,y+DLG_H-3,s,ACC_HEADING,16);
  acc_button(x+DLG_W-10,y+DLG_H-3," Close ",focus==BTN_CLOSE);
}

static void draw_all(int x,int y,int bx,int by)
{
  acc_fill(x+1,y+1,DLG_W-2,DLG_H-2,' ',ACC_BG);
  draw_status(x,y);draw_grid_frame(x+3,y+4);draw_grid(bx,by);draw_words(x+40,y+2);draw_hint(x+42,y+12);draw_buttons(x,y);
}

static int mouse_cell(int bx,int by,int mx,int my,int *cx,int *cy)
{
  int rx;if(my<by||my>by+(GH-1)*2||((my-by)&1))return 0;
  if(mx<bx-1||mx>bx+(GW-1)*4+1)return 0;
  rx=mx-(bx-1);*cx=rx/4;if(*cx<0||*cx>=GW)return 0;
  if(mx<bx+(*cx)*4-1||mx>bx+(*cx)*4+1)return 0;
  *cy=(my-by)/2;return 1;
}

static void change_puzzle(int delta)
{
  int old=puzzle_no;puzzle_no+=delta;if(puzzle_no<0)puzzle_no=puzzle_total-1;if(puzzle_no>=puzzle_total)puzzle_no=0;
  if(!load_puzzle(puzzle_no)){puzzle_no=old;load_puzzle(puzzle_no);return;}reset_puzzle();
}

int main(int argc,char **argv)
{
  int x,y,bx,by,key=0,mx=0,my=0,buttons=0,last_buttons=0,cx,cy,need=1,last_sec=-1;
  int mouse_selecting=0,mouse_moved=0,mouse_start_x=-1,mouse_start_y=-1,mouse_start_selected=0,hint_hover=0,last_hint_hover=-1;
  unsigned long sec;
  if(acc_help(argc,argv,"!WORDZ","Find hinted words horizontally, vertically or diagonally."))return 0;
  if(!acc_begin(argv[0],"Wordz",0))return 1;wordz_font(1);acc_mouse_display(1);if(!scan_puzzles()){acc_notice("Wordz Error","WORDZ.LVL is missing or invalid.");acc_end_screen();acc_end();return 1;}srand((unsigned)acc_ticks());puzzle_no=puzzle_total>1?rand()%puzzle_total:0;if(!load_puzzle(puzzle_no)){acc_notice("Wordz Error","WORDZ.LVL is missing or invalid.");acc_end_screen();acc_end();return 1;}if(!puzzle_data_valid()){acc_notice("Wordz Error","Puzzle data is invalid.");acc_end_screen();acc_end();return 1;}
  x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;bx=x+6;by=y+5;
  acc_box(x,y,DLG_W,DLG_H,"Wordz");reset_puzzle();

  while(key!=27){
    sec=elapsed_seconds();if((int)sec!=last_sec){last_sec=(int)sec;draw_status(x,y);}
    if(need){draw_all(x,y,bx,by);last_hint_hover=-1;need=0;}

    if(kbhit()){
      key=acc_key();
      if(key==9||key==271){if(focus<0)focus=(key==271)?BTN_CLOSE:0;else if(key==271)focus=focus?focus-1:BTN_CLOSE;else {focus++;if(focus>BTN_CLOSE)focus=0;}need=1;key=0;}
      else if(focus==0&&(key==256+72||key==0x4800)){if(cursor_y>0)cursor_y--;need=1;key=0;}
      else if(focus==0&&(key==256+80||key==0x5000)){if(cursor_y<GH-1)cursor_y++;need=1;key=0;}
      else if(focus==0&&(key==256+75||key==0x4B00)){if(cursor_x>0)cursor_x--;need=1;key=0;}
      else if(focus==0&&(key==256+77||key==0x4D00)){if(cursor_x<GW-1)cursor_x++;need=1;key=0;}
      else if(key==8){if(path_len>0){int px=pathx[path_len-1],py=pathy[path_len-1];path_len--;draw_grid_cell(bx,by,px,py);}key=0;}
      else if(key==' '&&!focus){path_toggle(cursor_x,cursor_y);need=1;key=0;}
      else if(key==13&&!focus){submit_path();need=1;key=0;}
      else if(key==13&&focus>0){
        if(focus==BTN_RESET)reset_puzzle();
        else if(focus==BTN_PREV)change_puzzle(-1);
        else if(focus==BTN_NEXT)change_puzzle(1);
        else if(focus==BTN_HINT){int count=puzzle_word_count();if(count)hint_word=(hint_word+1)%count;}
        else if(focus==BTN_CLOSE)key=27;
        if(key!=27){need=1;key=0;}
      }
    }

    if(acc_mouse_present){
      acc_mouse(&mx,&my,&buttons);if(buttons&ACC_MOUSE_OUTSIDE){key=27;break;}
      hint_hover=(my==y+12&&mx>=x+53&&mx<x+59);
      if(hint_hover!=last_hint_hover){draw_plain_button(x+53,y+12," Next ",focus==BTN_HINT||hint_hover);last_hint_hover=hint_hover;}
      if((buttons&1)&&!(last_buttons&1)){
        mouse_selecting=0;mouse_moved=0;
        if((buttons&1)&&my==y&&(mx==x+DLG_W-5||mx==x+DLG_W-4)){key=27;}
        else if(mouse_cell(bx,by,mx,my,&cx,&cy)){
          mouse_selecting=1;mouse_start_x=cx;mouse_start_y=cy;mouse_start_selected=(path_contains(cx,cy)>=0);
          cursor_x=cx;cursor_y=cy;focus=0;if(!mouse_start_selected)path_toggle(cx,cy);need=1;
        } else if(my==y+DLG_H-3){
          if(mx>=x+3&&mx<x+9){focus=BTN_RESET;reset_puzzle();need=1;}
          else if(mx>=x+12&&mx<x+17){focus=BTN_PREV;change_puzzle(-1);need=1;}
          else if(mx>=x+20&&mx<x+25){focus=BTN_NEXT;change_puzzle(1);need=1;}
          else if(mx>=x+DLG_W-10&&mx<x+DLG_W-4)key=27;
        } else if(my==y+12&&mx>=x+53&&mx<x+59){
          focus=BTN_HINT;{int count=puzzle_word_count();if(count)hint_word=(hint_word+1)%count;}need=1;
        }
      }
      if((buttons&1)&&mouse_selecting&&mouse_cell(bx,by,mx,my,&cx,&cy)){
        if(cx!=mouse_start_x||cy!=mouse_start_y)mouse_moved=1;
        if(path_len==0||cx!=pathx[path_len-1]||cy!=pathy[path_len-1]){
          {int ox=cursor_x,oy=cursor_y,old_len=path_len;path_drag_add(cx,cy);cursor_x=cx;cursor_y=cy;draw_grid_cell(bx,by,ox,oy);if(path_len!=old_len||ox!=cx||oy!=cy)draw_grid_cell(bx,by,cx,cy);}
        }
      }
      if(!(buttons&1)&&(last_buttons&1)&&mouse_selecting){
        if(!mouse_moved&&mouse_start_selected){path_toggle(mouse_start_x,mouse_start_y);draw_grid_cell(bx,by,mouse_start_x,mouse_start_y);}
        mouse_selecting=0;
      }
      last_buttons=buttons;
    }
  }
  if(timer_running){pause_tick=elapsed_seconds();timer_running=0;}
  acc_end_screen();wordz_font(0);acc_end();return 0;
}
