/* Launch! Snake accessory - Rattler Race inspired text-mode game. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include "ACCLIB.H"

#define BW 58
#define BH 13
#define MAX_SNAKE 256
#define LEVELS 50
#define FRUITS 8

/* 3.5 canonical Snake glyphs (logical IDs in LAUNCHUI). */
#define SG_FRUIT       75
#define SG_HEAD_UP     76
#define SG_HEAD_DOWN   77
#define SG_HEAD_LEFT   78 /* VGA 9th-column extension */
#define SG_HEAD_RIGHT  79
#define SG_BODY_VERT   80
#define SG_BODY_HORIZ  81 /* VGA 9th-column extension */
#define SG_CORNER_SW   82 /* VGA 9th-column extension */
#define SG_CORNER_SE   83
#define SG_CORNER_NW   84 /* VGA 9th-column extension */
#define SG_CORNER_NE   85

/* Runtime positions. 202-205 are extension-capable and unused by Snake's
   text-only toolbar; the other Snake glyphs deliberately use non-extension
   positions. Bricks reuse the shared game wall pair. */
#define SC_FRUIT       128
#define SC_HEAD_UP     129
#define SC_HEAD_DOWN   130
#define SC_HEAD_LEFT   202
#define SC_HEAD_RIGHT  131
#define SC_BODY_VERT   132
#define SC_BODY_HORIZ  218
#define SC_CORNER_SW   204
#define SC_CORNER_SE   133
#define SC_CORNER_NW   205
#define SC_CORNER_NE   134
#define SC_WALL_L      193
#define SC_WALL_R      195
#define SC_LOWER_HALF  221

typedef struct { unsigned char x,y; } POINT;
static POINT snake[MAX_SNAKE];
static int slen,dir,nextdir,score,lives,level,fruit_left,timebar,running,target_units;
static long fruit_time_left,fruit_time_total;static unsigned long fruit_last_tick;
static unsigned char board[BH][BW],fruit[BH][BW];
static unsigned char snake_old_glyph[14][32];

/* Board geometry follows the ten classic QBasic NIBBLES.BAS level designs,
   adapted from its 80x50 logical arena to Launch!'s 58x13 text-cell field. */
static void wall_v(int x,int y1,int y2){int y;if(x<0||x>=BW)return;if(y1<0)y1=0;if(y2>=BH)y2=BH-1;for(y=y1;y<=y2;y++)board[y][x]=1;}
static void wall_h(int y,int x1,int x2){int x;if(y<0||y>=BH)return;if(x1<0)x1=0;if(x2>=BW)x2=BW-1;for(x=x1;x<=x2;x++)board[y][x]=1;}
static unsigned level_seed[LEVELS];
static unsigned local_rand_state;
static unsigned local_rand(void){local_rand_state=local_rand_state*25173U+13849U;return local_rand_state;}
static int generated_connected(void)
{
 int qx[BW*BH],qy[BW*BH],head=0,tail=0,x,y,nx,ny,d,free_count=0;
 unsigned char seen[BH][BW];static const int dx4[4]={0,1,0,-1},dy4[4]={-1,0,1,0};
 memset(seen,0,sizeof(seen));if(board[BH/2][8])return 0;
 for(y=1;y<BH-1;y++)for(x=1;x<BW-1;x++)if(!board[y][x])free_count++;
 seen[BH/2][8]=1;qx[tail]=8;qy[tail++]=BH/2;
 while(head<tail){x=qx[head];y=qy[head++];for(d=0;d<4;d++){nx=x+dx4[d];ny=y+dy4[d];if(nx<1||nx>=BW-1||ny<1||ny>=BH-1||board[ny][nx]||seen[ny][nx])continue;seen[ny][nx]=1;qx[tail]=nx;qy[tail++]=ny;}}
 return tail==free_count;
}
static void generated_board(int lev)
{
 int attempt,s,x,y,len,i,vert,segments=5+(lev-10)/5;
 if(!level_seed[lev]){level_seed[lev]=(unsigned)(rand()^(unsigned)acc_ticks()^(unsigned)(lev*1237U));if(!level_seed[lev])level_seed[lev]=(unsigned)(lev+1);}
 for(attempt=0;attempt<96;attempt++){
  memset(board,0,sizeof(board));local_rand_state=level_seed[lev]^(unsigned)(attempt*4051U);
  for(s=0;s<segments;s++){vert=(int)(local_rand()&1U);len=2+(int)(local_rand()%6U);x=3+(int)(local_rand()%(BW-7));y=2+(int)(local_rand()%(BH-5));for(i=0;i<len;i++){int xx=x+(vert?0:i),yy=y+(vert?i:0);if(xx>0&&xx<BW-1&&yy>0&&yy<BH-1)board[yy][xx]=1;}}
  for(x=2;x<=10;x++)board[BH/2][x]=0;
  if(generated_connected())return;
 }
 memset(board,0,sizeof(board));
}
static void board_load(void)
{
 int i;memset(board,0,sizeof(board));if(level>=10){generated_board(level);return;}
 switch(level){
 case 0: break; /* Nibbles level 1: open arena */
 case 1: wall_h(6,14,43); break;
 case 2: wall_v(14,2,10);wall_v(43,2,10);break;
 case 3: wall_v(14,1,7);wall_v(43,6,12);wall_h(9,1,28);wall_h(3,29,56);break;
 case 4: wall_v(15,3,9);wall_v(42,3,9);wall_h(2,16,41);wall_h(10,16,41);break;
 case 5: for(i=7;i<=50;i+=7){wall_v(i,1,5);wall_v(i,8,12);}break;
 case 6: for(i=1;i<12;i+=2)board[i][29]=1;break;
 case 7: for(i=7;i<=50;i+=7){if(((i/7)&1)==0)wall_v(i,1,10);else wall_v(i,3,12);}break;
 case 8: for(i=2;i<12;i++){int xx=4+i*3;if(xx<BW-1)board[i][xx]=1;if(xx+20<BW-1)board[i][xx+20]=1;}break;
 default: for(i=1;i<12;i+=2){board[i][7]=1;board[i+1][14]=1;board[i][21]=1;board[i+1][28]=1;board[i][35]=1;board[i+1][42]=1;board[i][49]=1;}break;
 }
}
static int snake_at(int x,int y){int i;for(i=0;i<slen;i++)if((int)snake[i].x==x&&(int)snake[i].y==y)return 1;return 0;}
static void put_fruit(void){int x,y,tries=0;do{x=1+rand()%(BW-2);y=1+rand()%(BH-2);tries++;}while(tries<500&&(board[y][x]||snake_at(x,y)||fruit[y][x]));if(tries<500)fruit[y][x]=1;}
static long fruit_limit_ticks(void){int sec=30-(level*25)/(LEVELS-1);if(sec<5)sec=5;return(long)sec*18L;}
static void fruit_timer_reset(void){fruit_time_total=fruit_limit_ticks();fruit_time_left=fruit_time_total;fruit_last_tick=acc_ticks();timebar=100;}
static void snake_start(void){if(!running)fruit_last_tick=acc_ticks();running=1;}
static void reset_round(void){int i,x,y,sx=8,sy=BH/2,ok;memset(fruit,0,sizeof(fruit));board_load();slen=6;ok=1;for(i=0;i<slen;i++)if(board[sy][sx-i])ok=0;if(!ok){for(y=1;y<BH-1&&!ok;y++)for(x=6;x<BW-1&&!ok;x++){ok=1;for(i=0;i<slen;i++)if(board[y][x-i]){ok=0;break;}if(ok){sx=x;sy=y;}}}for(i=0;i<slen;i++){snake[i].x=(unsigned char)(sx-i);snake[i].y=(unsigned char)sy;}dir=nextdir=1;target_units=5;fruit_left=FRUITS;running=0;fruit_timer_reset();for(i=0;i<FRUITS;i++)put_fruit();}
static void snake_font(int install)
{
  static const unsigned char code[13]={SC_FRUIT,SC_HEAD_UP,SC_HEAD_DOWN,SC_HEAD_LEFT,
    SC_HEAD_RIGHT,SC_BODY_VERT,SC_BODY_HORIZ,SC_CORNER_SW,SC_CORNER_SE,
    SC_CORNER_NW,SC_CORNER_NE,SC_WALL_L,SC_WALL_R};
  static const unsigned char logical[13]={SG_FRUIT,SG_HEAD_UP,SG_HEAD_DOWN,SG_HEAD_LEFT,
    SG_HEAD_RIGHT,SG_BODY_VERT,SG_BODY_HORIZ,SG_CORNER_SW,SG_CORNER_SE,
    SG_CORNER_NW,SG_CORNER_NE,42,43};
  int i,h;unsigned char g[32];
  if(install){
    for(i=0;i<13;i++){acc_glyph_read(code[i],snake_old_glyph[i]);acc_glyph_library(logical[i],code[i]);}
    /* ACCLIB uses CP437 220 for its button shadow, so provide a private real
       lower-half block for the thicker Snake barriers. */
    acc_glyph_read(SC_LOWER_HALF,snake_old_glyph[13]);memset(g,0,sizeof(g));
    h=acc_font_height();for(i=h/2;i<h;i++)g[i]=0xFF;acc_glyph_write(SC_LOWER_HALF,g);
  }else{
    for(i=0;i<13;i++)acc_glyph_write(code[i],snake_old_glyph[i]);
    acc_glyph_write(SC_LOWER_HALF,snake_old_glyph[13]);
  }
}

static int body_char(int i)
{
  int n,s,e,w;
  if(i==0){if(dir==0)return SC_HEAD_UP;if(dir==1)return SC_HEAD_RIGHT;if(dir==2)return SC_HEAD_DOWN;return SC_HEAD_LEFT;}
  if(i==slen-1){
    /* Square-ended tail: use the straight body glyph matching its neighbour. */
    return snake[i-1].x==snake[i].x?SC_BODY_VERT:SC_BODY_HORIZ;
  }
  n=(snake[i-1].y<snake[i].y)||(snake[i+1].y<snake[i].y);
  s=(snake[i-1].y>snake[i].y)||(snake[i+1].y>snake[i].y);
  e=(snake[i-1].x>snake[i].x)||(snake[i+1].x>snake[i].x);
  w=(snake[i-1].x<snake[i].x)||(snake[i+1].x<snake[i].x);
  if(n&&s)return SC_BODY_VERT;if(e&&w)return SC_BODY_HORIZ;
  if(s&&w)return SC_CORNER_NE;if(s&&e)return SC_CORNER_NW;
  if(n&&w)return SC_CORNER_SE;return SC_CORNER_SW;
}
static int barrier_char(int x,int y)
{
  int u=y>0&&board[y-1][x],d=y<BH-1&&board[y+1][x];
  int l=x>0&&board[y][x-1],r=x<BW-1&&board[y][x+1];
  /* Full blocks make verticals, corners and junctions substantial. Pure
     horizontal runs use a half block, avoiding the old thin line-art look. */
  if((l||r)&&!(u||d))return SC_LOWER_HALF;
  return 219;
}
static void draw_under_cell(int ox,int oy,int x,int y)
{
  int attr=ACC_ATTR(0,7),ch=' ';
  if(board[y][x]){ch=barrier_char(x,y);attr=ACC_ATTR(0,9);}
  else if(fruit[y][x]){ch=SC_FRUIT;attr=ACC_ATTR(0,12);}
  acc_put(ox+x,oy+y,ch,attr);
}
static void draw_cell(int ox,int oy,int x,int y)
{
  int i,attr=ACC_ATTR(0,10),ch=' ';
  if(board[y][x]){ch=barrier_char(x,y);attr=ACC_ATTR(0,9);}
  else if(fruit[y][x]){ch=SC_FRUIT;attr=ACC_ATTR(0,12);}
  for(i=slen-1;i>=0;i--)if((int)snake[i].x==x&&(int)snake[i].y==y){ch=body_char(i);attr=ACC_ATTR(0,14);break;}
  acc_put(ox+x,oy+y,ch,attr);
}
static void brick_pair(int x,int y)
{int a=ACC_ATTR(7,8);acc_put(x,y,SC_WALL_L,a);acc_put(x+1,y,SC_WALL_R,a);}
static void draw_board(int ox,int oy)
{
  int x,y;
  acc_fill(ox,oy,BW,BH,' ',ACC_ATTR(0,7));
  /* Same two-cell brick surround used by !POP. */
  brick_pair(ox-2,oy-1);brick_pair(ox+BW,oy-1);
  for(x=0;x<BW;x+=2){brick_pair(ox+x,oy-1);brick_pair(ox+x,oy+BH);}
  for(y=0;y<BH;y++){brick_pair(ox-2,oy+y);brick_pair(ox+BW,oy+y);}
  brick_pair(ox-2,oy+BH);brick_pair(ox+BW,oy+BH);
  for(y=0;y<BH;y++)for(x=0;x<BW;x++)draw_cell(ox,oy,x,y);
}
static void status_draw(int x,int y){char s[32];int i,n=(timebar*22)/100;acc_text(x,y,"Time",ACC_LABEL,4);acc_fill(x+5,y,22,1,176,ACC_ATTR(acc_appearance.background,8));if(n>0)acc_fill(x+5,y,n,1,219,ACC_ATTR(acc_appearance.background,10));acc_fill(x+33,y,8,1,' ',ACC_BG);for(i=0;i<lives&&i<5;i++)acc_put(x+33+i,y,3,ACC_TITLE);sprintf(s,"Score %06d",score);acc_text(x+47,y,s,ACC_HEADING,12);}
static void snake_notice(const char *title,const char *msg){acc_notice(title,msg);}
static void life_lost(void){lives--;if(lives<=0){snake_notice("Snake","Game over");score=0;lives=3;level=0;}reset_round();}
static int edge_units(POINT a,POINT b){return a.y==b.y?1:2;}
static int step_snake(int ox,int oy){int dx=0,dy=0,nx,ny,i,total,grow=0,oldlen=slen;POINT oldsnake[MAX_SNAKE];for(i=0;i<oldlen;i++)oldsnake[i]=snake[i];if(nextdir!=((dir+2)&3))dir=nextdir;if(dir==0)dy=-1;else if(dir==1)dx=1;else if(dir==2)dy=1;else dx=-1;nx=snake[0].x+dx;ny=snake[0].y+dy;if(nx<0||ny<0||nx>=BW||ny>=BH||board[ny][nx]||snake_at(nx,ny)){life_lost();draw_board(ox,oy);return 0;}if(fruit[ny][nx]){fruit[ny][nx]=0;fruit_left--;score+=10;fruit_time_left+=fruit_time_total/5L;if(fruit_time_left>fruit_time_total)fruit_time_left=fruit_time_total;timebar=(int)(fruit_time_left*100L/fruit_time_total);grow=1;target_units+=2;}if(slen<MAX_SNAKE){for(i=slen;i>0;i--)snake[i]=snake[i-1];slen++;}else for(i=slen-1;i>0;i--)snake[i]=snake[i-1];snake[0].x=(unsigned char)nx;snake[0].y=(unsigned char)ny;total=0;for(i=0;i<slen-1;i++){int u=edge_units(snake[i],snake[i+1]);if(total+u>target_units){slen=i+1;break;}total+=u;}for(i=0;i<oldlen;i++)draw_under_cell(ox,oy,oldsnake[i].x,oldsnake[i].y);for(i=0;i<slen;i++)draw_cell(ox,oy,snake[i].x,snake[i].y);if(fruit_left<=0){score+=timebar;level=(level+1)%LEVELS;snake_notice("Snake","Board complete!");reset_round();draw_board(ox,oy);}return 1;}

static void snake_buttons(int x,int y,int w,int h,int focus)
{
  acc_button(x+3,y+h-3," Refresh ",focus==1);
  acc_button(x+10,y+h-3," Next ",focus==2);
  acc_button(x+w-10,y+h-3," Exit ",focus==3);
}

int main(int argc,char **argv)
{
  int w=70,h=22,x,y,ox,oy,key=0,mx=0,my=0,focus=-1,last_focus=-2,lastb=0;unsigned long last_tick,t;int tick_div=0,grow_ticks=0;
  if(acc_help(argc,argv,"!SNAKE","A text-mode fruit-eating snake game inspired by Rattler Race."))return 0;
  if(!acc_begin(argv[0],"Snake",0))return 1;snake_font(1);if(acc_mouse_present){union REGS mr;memset(&mr,0,sizeof(mr));mr.x.ax=1;int86(0x33,&mr,&mr);}srand((unsigned)acc_ticks());x=(acc_cols-w)/2;y=(acc_rows-h)/2;ox=x+6;oy=y+4;score=0;lives=3;level=0;reset_round();acc_box(x,y,w,h,"Snake");draw_board(ox,oy);status_draw(x+4,y+2);last_tick=acc_ticks();
  while(key!=27){
    if(focus!=last_focus){snake_buttons(x,y,w,h,focus);last_focus=focus;}
    if(kbhit()){key=acc_key();if(key==9||key==271){if(focus<0)focus=(key==271)?3:0;else focus=(key==271)?(focus+3)%4:(focus+1)%4;key=0;}else if(key==256+72||key==0x4800){focus=0;nextdir=0;snake_start();key=0;}else if(key==256+77||key==0x4D00){focus=0;nextdir=1;snake_start();key=0;}else if(key==256+80||key==0x5000){focus=0;nextdir=2;snake_start();key=0;}else if(key==256+75||key==0x4B00){focus=0;nextdir=3;snake_start();key=0;}else if(key==13&&focus>0){if(focus==1){reset_round();draw_board(ox,oy);}else if(focus==2){level=(level+1)%LEVELS;reset_round();draw_board(ox,oy);}else if(focus==3)key=27;if(key!=27)key=0;}}
    if(acc_mouse_present){int b=0;acc_mouse(&mx,&my,&b);if((b&1)&&!lastb){if((b&1)&&my==y&&(mx==x+w-5||mx==x+w-4)){key=27;}else if(my==y+h-3){if(mx>=x+4&&mx<x+14){focus=1;reset_round();draw_board(ox,oy);}else if(mx>=x+12&&mx<x+18){focus=2;level=(level+1)%LEVELS;reset_round();draw_board(ox,oy);}else if(mx>=x+w-11){key=27;}}}lastb=b&1;}
    t=acc_ticks();if(t!=last_tick){last_tick=t;tick_div++;if(running){grow_ticks++;if(grow_ticks>=36){grow_ticks=0;if(target_units<MAX_SNAKE-2)target_units++;}}if(running){unsigned long elapsed=t-fruit_last_tick;if(elapsed){fruit_last_tick=t;if(fruit_time_left>(long)elapsed)fruit_time_left-=(long)elapsed;else fruit_time_left=0;timebar=(int)(fruit_time_left*100L/fruit_time_total);if(timebar<0)timebar=0;if(fruit_time_left<=0){memset(fruit,0,sizeof(fruit));fruit_left=FRUITS;{int i;for(i=0;i<FRUITS;i++)put_fruit();}fruit_timer_reset();draw_board(ox,oy);}status_draw(x+4,y+2);}}if(running&&tick_div>=((dir==0||dir==2)?6:3)){tick_div=0;step_snake(ox,oy);status_draw(x+4,y+2);}}
  }
  acc_end_screen();snake_font(0);acc_end();return 0;
}
