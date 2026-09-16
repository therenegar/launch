/* Launch! Pop accessory - timed SameGame-style bubble game.
   Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "ACCLIB.H"

#define BW 20
#define BH 10
#define DLG_W 52
#define DLG_H 21
#define MAX_START_TICKS 1092L   /* level 1: about 60 seconds */
#define TICKS_PER_SEC 18L
#define BAR_W 40
#define GLYPH_L 203
#define GLYPH_R 190
#define WALL_L 199
#define WALL_R 200
#define CLOSE_L 208
#define CLOSE_R 187

static unsigned char board[BH][BW];
static unsigned char mark[BH][BW];
static unsigned char old_glyph[6][32];
static int selx=-1,sely=-1,sel_count=0;
static long score=0,high_score=0,time_left=MAX_START_TICKS,start_ticks=MAX_START_TICKS;
static int level=1;
static unsigned long last_tick;
static int cursor_x=0,cursor_y=0,focus=0;

static const unsigned char pop16[2][32]={
  {0x00,0x00,0x03,0x0F,0x1C,0x1B,0x37,0x37,0x3F,0x3F,0x1F,0x1F,0x0F,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0xC0,0xF0,0xF8,0xF8,0xFC,0xFC,0xFC,0xFC,0xF8,0xF8,0xF0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};
/* Manually reviewed EGA Boxes target/circle artwork. */
static const unsigned char pop14[2][32]={
  {0x00,0x03,0x0F,0x1C,0x1B,0x37,0x37,0x3F,0x3F,0x1F,0x1F,0x0F,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0xC0,0xF0,0xF8,0xF8,0xFC,0xFC,0xFC,0xFC,0xF8,0xF8,0xF0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

static const unsigned char wall16[2][32]={
  {0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0xFF,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
static const unsigned char wall14[2][32]={
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x10,0x10,0x10,0x10,0x10,0x10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0x04,0x04,0x04,0x04,0x04,0x04,0x04,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static int bubble_fg(int v)
{
  static const int c[5]={0,12,10,11,13};
  return (v>=1&&v<=4)?c[v]:15;
}

static void pop_font(int install)
{
  int i;const unsigned char (*g)[32]=(acc_font_height()==14)?pop14:pop16;
  const unsigned char (*wg)[32]=(acc_font_height()==14)?wall14:wall16;
  if(install){
    acc_glyph_read(GLYPH_L,old_glyph[0]);acc_glyph_read(GLYPH_R,old_glyph[1]);
    acc_glyph_read(WALL_L,old_glyph[2]);acc_glyph_read(WALL_R,old_glyph[3]);
    acc_glyph_read(CLOSE_L,old_glyph[4]);acc_glyph_read(CLOSE_R,old_glyph[5]);
    /* Keep the normal Close icon available while 199/200 carry the brick pair. */
    acc_glyph_write(CLOSE_L,old_glyph[3]); /* original glyph 200 */
    {unsigned char close_right[32];acc_glyph_read(201,close_right);acc_glyph_write(CLOSE_R,close_right);}
    for(i=0;i<2;i++)acc_glyph_write(i?GLYPH_R:GLYPH_L,g[i]);
    for(i=0;i<2;i++)acc_glyph_write(i?WALL_R:WALL_L,wg[i]);
    acc_set_close_glyphs(CLOSE_L,CLOSE_R);
  } else {
    acc_set_close_glyphs(200,201);
    acc_glyph_write(GLYPH_L,old_glyph[0]);acc_glyph_write(GLYPH_R,old_glyph[1]);
    acc_glyph_write(WALL_L,old_glyph[2]);acc_glyph_write(WALL_R,old_glyph[3]);
    acc_glyph_write(CLOSE_L,old_glyph[4]);acc_glyph_write(CLOSE_R,old_glyph[5]);
  }
}

static int flood(int sx,int sy,int make_mark)
{
  int qx[BW*BH],qy[BW*BH],head=0,tail=0,x,y,nx,ny,i;unsigned char v;
  static const int dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};
  if(sx<0||sx>=BW||sy<0||sy>=BH||!board[sy][sx])return 0;
  memset(mark,0,sizeof(mark));v=board[sy][sx];qx[tail]=sx;qy[tail++]=sy;mark[sy][sx]=1;
  while(head<tail){x=qx[head];y=qy[head++];for(i=0;i<4;i++){nx=x+dx[i];ny=y+dy[i];if(nx>=0&&nx<BW&&ny>=0&&ny<BH&&!mark[ny][nx]&&board[ny][nx]==v){mark[ny][nx]=1;qx[tail]=nx;qy[tail++]=ny;}}}
  if(!make_mark)memset(mark,0,sizeof(mark));return tail;
}

static void clear_selection(void){selx=sely=-1;sel_count=0;memset(mark,0,sizeof(mark));}

static int any_moves(void)
{
  int x,y;for(y=0;y<BH;y++)for(x=0;x<BW;x++)if(board[y][x]){
    if(x+1<BW&&board[y][x]==board[y][x+1])return 1;
    if(y+1<BH&&board[y][x]==board[y+1][x])return 1;
  }return 0;
}

static int remaining(void){int x,y,n=0;for(y=0;y<BH;y++)for(x=0;x<BW;x++)if(board[y][x])n++;return n;}

static long level_ticks(void){return (long)(60-((level-1)*57)/9)*TICKS_PER_SEC;}
static void new_game(void)
{
  int x,y;unsigned seed=(unsigned)acc_ticks()+(unsigned)(level*977);srand(seed);
  for(y=0;y<BH;y++)for(x=0;x<BW;x++)board[y][x]=(unsigned char)(1+rand()%4);
  score=0;start_ticks=level_ticks();time_left=start_ticks;last_tick=acc_ticks();cursor_x=0;cursor_y=0;focus=0;clear_selection();
}

static void gravity(void)
{
  int x,y,dst,src,write=0,empty;
  for(x=0;x<BW;x++){dst=BH-1;for(y=BH-1;y>=0;y--)if(board[y][x]){board[dst][x]=board[y][x];if(dst!=y)board[y][x]=0;dst--;}while(dst>=0)board[dst--][x]=0;}
  for(src=0;src<BW;src++){empty=1;for(y=0;y<BH;y++)if(board[y][src]){empty=0;break;}if(!empty){if(write!=src)for(y=0;y<BH;y++){board[y][write]=board[y][src];board[y][src]=0;}write++;}}
  for(x=write;x<BW;x++)for(y=0;y<BH;y++)board[y][x]=0;
}

static void draw_cell(int bx,int by,int x,int y,int flash_bg)
{
  int v=board[y][x],bg=0,attr;if(!v){acc_put(bx+x*2,by+y,' ',0);acc_put(bx+x*2+1,by+y,' ',0);return;}
  if(flash_bg>=0)bg=flash_bg;else if(sel_count>=2&&mark[y][x])bg=15;
  attr=ACC_ATTR(bg,bubble_fg(v));acc_put(bx+x*2,by+y,GLYPH_L,attr);acc_put(bx+x*2+1,by+y,GLYPH_R,attr);
}

static void wall_pair(int x,int y)
{int a=ACC_ATTR(7,8);acc_put(x,y,WALL_L,a);acc_put(x+1,y,WALL_R,a);}
static void draw_board(int bx,int by)
{
  int x,y;
  /* Corners are full brick pairs too, giving one continuous surround. */
  wall_pair(bx-2,by-1);wall_pair(bx+BW*2,by-1);
  for(x=0;x<BW;x++){wall_pair(bx+x*2,by-1);wall_pair(bx+x*2,by+BH);}
  for(y=0;y<BH;y++){wall_pair(bx-2,by+y);wall_pair(bx+BW*2,by+y);}
  wall_pair(bx-2,by+BH);wall_pair(bx+BW*2,by+BH);
  for(y=0;y<BH;y++)for(x=0;x<BW;x++)draw_cell(bx,by,x,y,-1);
}
static void draw_level(int x,int y)
{char s[16];sprintf(s,"Level %d",level);acc_text(x+25,y+18,s,ACC_HEADING,8);}

static void draw_status(int x,int y)
{
  char s[32];int secs=(int)((time_left+TICKS_PER_SEC-1)/TICKS_PER_SEC),filled,i,barattr;
  if(secs<0)secs=0;sprintf(s,"Score %06ld",score);acc_text(x+6,y+2,s,ACC_HEADING,18);sprintf(s,"Time %02d:%02d",secs/60,secs%60);acc_text(x+DLG_W-16,y+2,s,ACC_HEADING,12);
  filled=(int)(time_left*BAR_W/start_ticks);if(filled<0)filled=0;if(filled>BAR_W)filled=BAR_W;barattr=ACC_ATTR(acc_appearance.background,filled<=4?acc_appearance.main_title:acc_appearance.launchers);
  for(i=0;i<BAR_W;i++)acc_put(x+6+i,y+3,i<filled?219:176,barattr);
}

static void pop_pause(unsigned long ticks){unsigned long t=acc_ticks();while((unsigned long)(acc_ticks()-t)<ticks);}

static void flash_group(int bx,int by)
{
  static const int bg[4]={15,12,14,15};int k,x,y;
  for(k=0;k<4;k++){for(y=0;y<BH;y++)for(x=0;x<BW;x++)if(mark[y][x])draw_cell(bx,by,x,y,bg[k]);pop_pause(1);}
}

static void pop_selected(int bx,int by)
{
  int x,y,n=sel_count;if(n<2)return;flash_group(bx,by);
  for(y=0;y<BH;y++)for(x=0;x<BW;x++)if(mark[y][x])board[y][x]=0;
  score+=(long)(n-2)*(long)(n-2);time_left+=(long)(n+2)*TICKS_PER_SEC;if(time_left>start_ticks)time_left=start_ticks;
  gravity();clear_selection();if(cursor_x>=BW)cursor_x=BW-1;if(cursor_y>=BH)cursor_y=BH-1;
}

static void select_at(int x,int y)
{
  int n=flood(x,y,1);if(n<2){clear_selection();return;}selx=x;sely=y;sel_count=n;
}

static void mouse_show(int show){union REGS r;if(!acc_mouse_present)return;memset(&r,0,sizeof(r));r.x.ax=show?1:2;int86(0x33,&r,&r);}

static void pop_result_notice(int is_error,long final_score)
{
  int w=is_error?44:40,h=10,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,bx=x+3;unsigned mb=0;char n[24];
  if(final_score>high_score)high_score=final_score;
  acc_box(x,y,w,h,"Pop");
  if(is_error){acc_put(x+3,y+2,219,ACC_ATTR(acc_appearance.background,4));acc_put(x+4,y+2,173,ACC_ATTR(4,15));acc_put(x+5,y+2,219,ACC_ATTR(acc_appearance.background,4));acc_text(x+7,y+2,"Time's up!",ACC_LABEL,20);}
  else acc_text(x+3,y+2,"Nothing left to pop!",ACC_LABEL,28);
  acc_text(x+3,y+4,"Your score:",ACC_LABEL,12);sprintf(n,"%ld",final_score);acc_text(x+15,y+4,n,ACC_HEADING,w-18);
  acc_text(x+3,y+5,"High score:",ACC_LABEL,12);sprintf(n,"%ld",high_score);acc_text(x+15,y+5,n,ACC_HEADING,w-18);
  acc_button(bx,y+h-3,"  OK  ",1);
  while(!k){acc_wait(&k,&mx,&my,&mb);if((mb&1)&&my==y+h-3&&mx>=bx&&mx<bx+6)k=13;}
}

int main(int argc,char **argv)
{
  int x,y,bx,by,key=0,mx=0,my=0,buttons=0,last_buttons=0,tx,ty,full=1,hover=-1,oldhover=-1;
  unsigned long now,elapsed;
  if(acc_help(argc,argv,"!POP","A timed SameGame-style bubble popping game."))return 0;
  if(!acc_begin(argv[0],"Pop",0))return 1;
  pop_font(1);x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;bx=x+6;by=y+6;acc_box(x,y,DLG_W,DLG_H,"Pop");new_game();mouse_show(1);
  while(key!=27){
    now=acc_ticks();elapsed=now-last_tick;if(elapsed){last_tick=now;if(time_left>(long)elapsed)time_left-=(long)elapsed;else time_left=0;draw_status(x,y);}
    if(full){acc_fill(x+1,y+1,DLG_W-2,DLG_H-2,' ',ACC_BG);draw_status(x,y);draw_board(bx,by);draw_level(x,y);acc_button(x+3,y+18,"  Retry  ",focus==1);acc_button(x+11,y+18,"  Prev  ",focus==2);acc_button(x+18,y+18,"  Next  ",focus==3);acc_button(x+DLG_W-9,y+18,"  Close  ",focus==4);full=0;}
    if(time_left<=0||!any_moves()){
      mouse_show(0);if(time_left<=0){pop_result_notice(1,score);new_game();}else{if(remaining()==0){score+=1000;score+=(long)((time_left>0?time_left:0)/TICKS_PER_SEC)*10L;}pop_result_notice(0,score);if(level<10)level++;new_game();}full=1;mouse_show(1);continue;
    }
    if(acc_key_ready())key=acc_key();else key=0;
    if(acc_mouse_present){acc_mouse(&mx,&my,&buttons);if((buttons&1)&&!(last_buttons&1)){
        if(my==y+18&&mx>=x+3&&mx<x+9){acc_press_button(x+3,y+18,"  Retry  ");new_game();full=1;}
        else if(my==y+18&&mx>=x+11&&mx<x+16){acc_press_button(x+11,y+18,"  Prev  ");if(--level<1)level=10;new_game();full=1;}
        else if(my==y+18&&mx>=x+18&&mx<x+23){acc_press_button(x+18,y+18,"  Next  ");if(++level>10)level=1;new_game();full=1;}
        else if(my==y+18&&mx>=x+DLG_W-9&&mx<x+DLG_W-3){acc_press_button(x+DLG_W-9,y+18,"  Close  ");key=27;}
        else if(mx>=bx&&mx<bx+BW*2&&my>=by&&my<by+BH){tx=(mx-bx)/2;ty=my-by;if(sel_count>=2&&mark[ty][tx]){pop_selected(bx,by);draw_board(bx,by);draw_status(x,y);}else{select_at(tx,ty);draw_board(bx,by);}focus=0;}
      }last_buttons=buttons;
      hover=-1;if(my==y+18&&mx>=x+3&&mx<x+9)hover=1;else if(my==y+18&&mx>=x+11&&mx<x+16)hover=2;else if(my==y+18&&mx>=x+18&&mx<x+23)hover=3;else if(my==y+18&&mx>=x+DLG_W-9&&mx<x+DLG_W-3)hover=4;if(hover!=oldhover){acc_button(x+3,y+18,"  Retry  ",focus==1||hover==1);acc_button(x+11,y+18,"  Prev  ",focus==2||hover==2);acc_button(x+18,y+18,"  Next  ",focus==3||hover==3);acc_button(x+DLG_W-9,y+18,"  Close  ",focus==4||hover==4);oldhover=hover;}
    }
    if(key==9||key==271){if(key==271){focus--;if(focus<0)focus=4;}else{focus++;if(focus>4)focus=0;}full=1;}
    else if((key==13||key==' ')&&focus==1){new_game();full=1;}
    else if((key==13||key==' ')&&focus==2){if(--level<1)level=10;new_game();full=1;}
    else if((key==13||key==' ')&&focus==3){if(++level>10)level=1;new_game();full=1;}
    else if((key==13||key==' ')&&focus==4)key=27;
    else if(focus==0&&(key==256+75||key==256+77||key==256+72||key==256+80)){if(key==256+75&&cursor_x>0)cursor_x--;if(key==256+77&&cursor_x<BW-1)cursor_x++;if(key==256+72&&cursor_y>0)cursor_y--;if(key==256+80&&cursor_y<BH-1)cursor_y++;select_at(cursor_x,cursor_y);draw_board(bx,by);}
    else if(focus==0&&(key==13||key==' ')){if(sel_count>=2&&mark[cursor_y][cursor_x]){pop_selected(bx,by);draw_board(bx,by);draw_status(x,y);}else{select_at(cursor_x,cursor_y);draw_board(bx,by);}}
    
  }
  mouse_show(0);pop_font(0);acc_end();return 0;
}
