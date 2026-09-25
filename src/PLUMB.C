/* Launch! Plumb accessory - pipe-laying puzzle game.
   Pipe artwork comes from the canonical LAUNCHUI.FNT/LAUNCHUI.F14 designs,
   copied into private extension-capable runtime glyph slots.
   Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include "ACCLIB.H"

void acc_mouse_display(int show);

#define BW 14
#define BH 10
#define QUEUE 5
#define PIECES 7
#define DLG_W 50
#define DLG_H 21

#define C_N 1
#define C_E 2
#define C_S 4
#define C_W 8

#define P_H 0
#define P_V 1
#define P_NE 2
#define P_ES 3
#define P_SW 4
#define P_WN 5
#define P_X 6

/* Keep the game artwork away from Launch!'s shared toolbar/icon glyph slots. */
#define PLUMB_SEG_BASE 144
#define PLUMB_LOWER_HALF 222
#define WALL_L 193
#define WALL_R 195
#define PLUMB_JOIN 213
#define EMPTY 255

/* All runtime pipe cells use VGA line-graphics codes (C0h-DFh), so the
   ninth-column extension needed by the edited artwork remains active.
   Codes used by Plumb's own New/Close buttons, board walls and shadows are
   deliberately excluded. */
static const unsigned char pipe_code[PIECES*2]={
  194,197, 198,199, 202,221, 204,205, 206,207, 208,209, 210,212
};

static const unsigned char pipe_mask[PIECES]={
  C_E|C_W,C_N|C_S,C_N|C_E,C_E|C_S,C_S|C_W,C_W|C_N,C_N|C_E|C_S|C_W
};

static unsigned char board[BH][BW],filled[BH][BW],queue_piece[QUEUE];
static unsigned char old_pipe[PIECES*2][32],old_wall[2][32],old_join[32];
static unsigned char old_seg[11][32],old_lower[32];
static int cursor_x=2,cursor_y=BH/2,source_x,source_y,source_side;
static int level=1,lives=3,target=12,flow_tiles=0,flow_x=-1,flow_y=0,flow_dir=1;
static long score=0;
static int flow_started=0,round_started=0,paused=0,qualified=0;
static unsigned long round_start,last_step,pause_started;
static int focus=-1;
static int ui_x,ui_y,ui_bx,ui_by;

/* Direction indices are N,E,S,W. */
static const int dx[4]={0,1,0,-1};
static const int dy[4]={-1,0,1,0};
static const unsigned char dbit[4]={C_N,C_E,C_S,C_W};


static const unsigned char plumb_pipe16[14][16]={
  {0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,0x00},
  {0x0F,0x0F,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07},
  {0xF0,0xF0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0},
  {0x0F,0x0F,0x07,0x07,0x07,0x05,0x05,0x05,0x06,0x03,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xF0,0xF0,0xE0,0xE0,0xF1,0xFF,0xFF,0xFF,0x3F,0xFF,0x01,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x03,0x06,0x05,0x05,0x05,0x07,0x07,0x07,0x07,0x07,0x07},
  {0x00,0x00,0x00,0x00,0x01,0xFF,0x3F,0xFF,0xFF,0xFF,0xF1,0xE0,0xE0,0xE0,0xE0,0xE0},
  {0x00,0x00,0x00,0x00,0x00,0xFF,0xFC,0xFF,0xFF,0xFF,0x0F,0x07,0x07,0x07,0x07,0x07},
  {0x00,0x00,0x00,0x00,0x00,0xC0,0xE0,0x60,0x60,0x60,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0},
  {0x0F,0x0F,0x07,0x07,0x0F,0xFF,0xFF,0xFF,0xFC,0xFF,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xF0,0xF0,0xE0,0xE0,0xE0,0xA0,0xA0,0xA0,0x60,0xC0,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x0F,0x0F,0x07,0x05,0x0D,0xFD,0xC3,0xFF,0xFF,0xFF,0x0F,0x07,0x07,0x07,0x07,0x07},
  {0xF0,0xF0,0xE0,0xE0,0xF1,0xFF,0xFF,0xFF,0xC3,0xBF,0xB1,0xA0,0xE0,0xE0,0xE0,0xE0}
};

static const unsigned char plumb_pipe14[14][14]={
  {0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00},
  {0x0F,0x0F,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07},
  {0xF0,0xF0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0},
  {0x0F,0x0F,0x07,0x07,0x07,0x05,0x05,0x05,0x06,0x03,0x00,0x00,0x00,0x00},
  {0xF0,0xF0,0xE0,0xE0,0xF1,0xFF,0xFF,0xFF,0x3F,0xFF,0x01,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x03,0x06,0x05,0x05,0x05,0x07,0x07,0x07,0x07},
  {0x00,0x00,0x00,0x00,0x01,0xFF,0x3F,0xFF,0xFF,0xFF,0xF1,0xE0,0xE0,0xE0},
  {0x00,0x00,0x00,0x00,0x00,0xFF,0xFC,0xFF,0xFF,0xFF,0x0F,0x07,0x07,0x07},
  {0x00,0x00,0x00,0x00,0x00,0xC0,0xE0,0x60,0x60,0x60,0xE0,0xE0,0xE0,0xE0},
  {0x0F,0x0F,0x07,0x07,0x0F,0xFF,0xFF,0xFF,0xFC,0xFF,0x00,0x00,0x00,0x00},
  {0xF0,0xF0,0xE0,0xE0,0xE0,0xA0,0xA0,0xA0,0x60,0xC0,0x00,0x00,0x00,0x00},
  {0x0F,0x0F,0x07,0x05,0x0D,0xFD,0xC3,0xFF,0xFF,0xFF,0x0F,0x07,0x07,0x07},
  {0xF0,0xF0,0xE0,0xE0,0xF1,0xFF,0xFF,0xFF,0xC3,0xBF,0xB1,0xA0,0xE0,0xE0}
};

static void plumb_font(int install)
{
  int i,h;unsigned char g[32];
  if(install){
    h=acc_font_height();
    for(i=0;i<PIECES*2;i++)acc_glyph_read(pipe_code[i],old_pipe[i]);
    acc_glyph_read(WALL_L,old_wall[0]);acc_glyph_read(WALL_R,old_wall[1]);acc_glyph_read(PLUMB_JOIN,old_join);
    acc_glyph_read(195,g);acc_glyph_write(PLUMB_JOIN,g);
    for(i=0;i<11;i++){acc_glyph_read(PLUMB_SEG_BASE+i,old_seg[i]);acc_glyph_library(64+i,PLUMB_SEG_BASE+i);}
    acc_glyph_read(PLUMB_LOWER_HALF,old_lower);memset(g,0,sizeof(g));
    for(i=h/2;i<h;i++)g[i]=0xFF;acc_glyph_write(PLUMB_LOWER_HALF,g);
    for(i=0;i<PIECES*2;i++){
      memset(g,0,sizeof(g));
      if(h==14)memcpy(g,plumb_pipe14[i],14);
      else memcpy(g,plumb_pipe16[i],16);
      acc_glyph_write(pipe_code[i],g);
    }
    acc_glyph_library(42,WALL_L);acc_glyph_library(43,WALL_R);
  } else {
    for(i=0;i<PIECES*2;i++)acc_glyph_write(pipe_code[i],old_pipe[i]);
    for(i=0;i<11;i++)acc_glyph_write(PLUMB_SEG_BASE+i,old_seg[i]);
    acc_glyph_write(PLUMB_LOWER_HALF,old_lower);
    acc_glyph_write(WALL_L,old_wall[0]);acc_glyph_write(WALL_R,old_wall[1]);acc_glyph_write(PLUMB_JOIN,old_join);
  }
}

static unsigned char plumb_seg_char(int c)
{if(c>='0'&&c<='9')return(unsigned char)(PLUMB_SEG_BASE+c-'0');return(unsigned char)c;}

static void number_box(int x,int y,int w,const char *s)
{
  int i,start;
  for(i=0;i<w;i++){
    acc_put(x+i,y,PLUMB_LOWER_HALF,ACC_ATTR(acc_appearance.background,0));
    acc_put(x+i,y+1,' ',0x00);
    acc_put(x+i,y+2,223,ACC_ATTR(acc_appearance.background,0));
  }
  start=x+w-1-(int)strlen(s);
  for(i=0;s[i];i++)acc_put(start+i,y+1,plumb_seg_char((unsigned char)s[i]),0x0A);
}

static void wall_pair(int x,int y)
{
  int a=ACC_ATTR(2,10);
  acc_put(x,y,WALL_L,a);acc_put(x+1,y,WALL_R,a);
}

static int pipe_attr(int x,int y,int cursor)
{
  int p=board[y][x];
  /* Launcher green is reserved for slime-filled pipe.  The piece under
     board focus uses the Title colour; laid, unfilled pipe uses Folder. */
  if(filled[y][x])return ACC_ATTR(0,acc_appearance.launchers);
  if(p==EMPTY)return ACC_ATTR(0,7);
  if(cursor)return ACC_ATTR(0,acc_appearance.titles);
  return ACC_ATTR(0,acc_appearance.folders);
}

static void draw_pipe_pair(int sx,int sy,int type,int attr)
{
  if(type<0||type>=PIECES){acc_put(sx,sy,' ',attr);acc_put(sx+1,sy,' ',attr);return;}
  acc_put(sx,sy,pipe_code[type*2],attr);
  acc_put(sx+1,sy,pipe_code[type*2+1],attr);
}

static void draw_cell(int bx,int by,int x,int y)
{
  int attr=pipe_attr(x,y,x==cursor_x&&y==cursor_y&&focus==0);
  if(board[y][x]==EMPTY){
    int pa=(x==cursor_x&&y==cursor_y&&focus==0)?ACC_ATTR(0,acc_appearance.launchers):ACC_ATTR(0,8);
    acc_put(bx+x*2,by+y,169,pa);acc_put(bx+x*2+1,by+y,170,pa);
  } else draw_pipe_pair(bx+x*2,by+y,board[y][x],attr);
}

static void draw_board(int bx,int by)
{
  int x,y;
  acc_fill(bx,by,BW*2,BH,' ',ACC_ATTR(0,7));
  wall_pair(bx-2,by-1);wall_pair(bx+BW*2,by-1);
  for(x=0;x<BW;x++){wall_pair(bx+x*2,by-1);wall_pair(bx+x*2,by+BH);}
  for(y=0;y<BH;y++){wall_pair(bx-2,by+y);wall_pair(bx+BW*2,by+y);}
  wall_pair(bx-2,by+BH);wall_pair(bx+BW*2,by+BH);
  /* Supply pipe may enter from any edge. */
  if(source_side==0)draw_pipe_pair(bx+source_x*2,by-1,P_V,ACC_ATTR(0,acc_appearance.launchers));
  else if(source_side==1)draw_pipe_pair(bx+BW*2,by+source_y,P_H,ACC_ATTR(0,acc_appearance.launchers));
  else if(source_side==2)draw_pipe_pair(bx+source_x*2,by+BH,P_V,ACC_ATTR(0,acc_appearance.launchers));
  else draw_pipe_pair(bx-2,by+source_y,P_H,ACC_ATTR(0,acc_appearance.launchers));
  for(y=0;y<BH;y++)for(x=0;x<BW;x++)draw_cell(bx,by,x,y);
}

static void draw_target(int bx,int by)
{
  char s[24];sprintf(s,"Connect %d pipes",target);acc_text(bx-2,by+BH+1,s,ACC_HEADING,20);
}
static void spill_animation(int bx,int by)
{
  unsigned short tile_cell[BW*BH];
  unsigned long start,elapsed;
  int n=0,i,j,x,y,target_count,done=0,tmp;
  int attr=ACC_ATTR(0,10);
  /* Animate whole Plumb tiles, not individual character cells.  Each board
     square is two text cells wide, so painting the pair together keeps the
     splash visually aligned with the pipe grid.  Empty tiles are shuffled
     once, then progressively revealed so every non-pipe tile is bright green
     by the end of the four-second sequence. */
  for(y=0;y<BH;y++)for(x=0;x<BW;x++)if(board[y][x]==EMPTY)
    tile_cell[n++]=(unsigned short)((y<<8)|x);
  for(i=n-1;i>0;i--){j=rand()%(i+1);tmp=tile_cell[i];tile_cell[i]=tile_cell[j];tile_cell[j]=(unsigned short)tmp;}
  acc_mouse_display(0);start=acc_ticks();
  while(done<n){
    elapsed=(unsigned long)(acc_ticks()-start);if(elapsed>72UL)elapsed=72UL;
    target_count=(int)((elapsed*(unsigned long)n)/72UL);
    if(target_count>n)target_count=n;
    while(done<target_count){
      y=(tile_cell[done]>>8)&255;x=tile_cell[done]&255;
      acc_put(bx+x*2,by+y,219,attr);
      acc_put(bx+x*2+1,by+y,219,attr);
      done++;
    }
  }
  acc_mouse_display(1);
}

static void queue_fill(void)
{
  int i;for(i=0;i<QUEUE;i++)queue_piece[i]=(unsigned char)(rand()%PIECES);
}

static void queue_advance(void)
{
  int i;for(i=0;i<QUEUE-1;i++)queue_piece[i]=queue_piece[i+1];
  queue_piece[QUEUE-1]=(unsigned char)(rand()%PIECES);
}

static int level_target(void)
{
  int n=10+level*2;if(n>30)n=30;return n;
}

static unsigned long step_delay(void)
{
  int n=18-(level-1);if(n<5)n=5;return (unsigned long)n;
}

static void new_round(int reset_score)
{
  if(reset_score)score=0;
  memset(board,EMPTY,sizeof(board));memset(filled,0,sizeof(filled));
  source_side=rand()%4;source_x=1+rand()%(BW-2);source_y=1+rand()%(BH-2);
  if(source_side==0){cursor_x=source_x;cursor_y=0;flow_x=source_x;flow_y=-1;flow_dir=2;}
  else if(source_side==1){cursor_x=BW-1;cursor_y=source_y;flow_x=BW;flow_y=source_y;flow_dir=3;}
  else if(source_side==2){cursor_x=source_x;cursor_y=BH-1;flow_x=source_x;flow_y=BH;flow_dir=0;}
  else {cursor_x=0;cursor_y=source_y;flow_x=-1;flow_y=source_y;flow_dir=1;}
  queue_fill();target=level_target();flow_tiles=0;
  flow_started=0;round_started=0;paused=0;qualified=0;
  round_start=last_step=0;focus=0;
}

static void draw_queue(int x,int y)
{
  int i,a=ACC_BORDER,frame=ACC_ATTR(acc_appearance.background,0),piece=ACC_ATTR(0,acc_appearance.folders);
  acc_put(x,y,218,a);acc_put(x+7,y,191,a);
  acc_put(x+1,y,196,a);acc_text(x+2,y,"Next",ACC_HEADING,4);acc_put(x+6,y,196,a);
  for(i=1;i<12;i++){acc_put(x,y+i,179,a);acc_put(x+7,y+i,179,a);}
  acc_put(x,y+12,192,a);for(i=1;i<7;i++)acc_put(x+i,y+12,196,a);acc_put(x+7,y+12,217,a);

  /* Current piece: the same recessed half-block treatment as the countdown
     display, with a literal black centre and Folders-colour pipe artwork. */
  for(i=0;i<6;i++){
    acc_put(x+1+i,y+1,PLUMB_LOWER_HALF,frame);
    acc_put(x+1+i,y+2,' ',0x00);
    acc_put(x+1+i,y+3,223,frame);
  }
  draw_pipe_pair(x+3,y+2,queue_piece[0],piece);

  /* Connector and arrow through the right side of the Next box. */
  acc_put(x+7,y+2,PLUMB_JOIN,a);acc_put(x+8,y+2,196,a);acc_put(x+9,y+2,16,ACC_HEADING);

  for(i=1;i<QUEUE;i++)
    draw_pipe_pair(x+3,y+2+i*2,queue_piece[i],piece);
}

static int seconds_to_flow(void)
{
  unsigned long e,now;
  if(flow_started)return 0;
  if(!round_started)return 15;
  now=paused?pause_started:acc_ticks();
  e=(now-round_start)/18UL;
  if(e>=15UL)return 0;
  return 15-(int)e;
}

static void draw_status(int x,int y)
{
  char s[24];int i,sec=seconds_to_flow();
  if(flow_started)strcpy(s,"00");else sprintf(s,"%02d",sec);
  number_box(x+3,y+1,8,s);
  sprintf(s,"Level %02d",level);acc_text(x+15,y+2,s,ACC_HEADING,10);
  for(i=0;i<3;i++)acc_put(x+29+i,y+2,i<lives?3:' ',ACC_TITLE);
  sprintf(s,"Score %06ld",score);acc_text(x+35,y+2,s,ACC_HEADING,12);
}

static void toggle_pause(void)
{
  unsigned long now=acc_ticks(),held;
  if(!paused){paused=1;pause_started=now;}
  else {
    held=now-pause_started;if(round_started)round_start+=held;if(flow_started)last_step+=held;paused=0;
  }
}

static void draw_buttons(int x,int y,int focus_now)
{
  acc_button(x+3,y+DLG_H-3," Retry ",focus_now==1);
  acc_button(x+12,y+DLG_H-3,"  Flow!  ",focus_now==2);
  acc_button(x+23,y+DLG_H-3,paused?"  Resume  ":"  Pause  ",focus_now==3);
  acc_button(x+DLG_W-10,y+DLG_H-3," Exit ",focus_now==4);
}

static void draw_all(int x,int y,int bx,int by)
{
  acc_fill(x+1,y+1,DLG_W-2,DLG_H-2,' ',ACC_BG);
  draw_status(x,y);draw_queue(x+3,y+4);draw_board(bx,by);draw_target(bx,by);draw_buttons(x,y,focus);
}
static void redraw_main_window(void)
{acc_box(ui_x,ui_y,DLG_W,DLG_H,"Plumb");draw_all(ui_x,ui_y,ui_bx,ui_by);}

static void redraw_board_action(int x,int y,int bx,int by,int oldx,int oldy)
{
  draw_cell(bx,by,oldx,oldy);
  if(oldx!=cursor_x||oldy!=cursor_y)draw_cell(bx,by,cursor_x,cursor_y);
  draw_queue(x+3,y+4);
  draw_status(x,y);
}

static int outgoing_dir(int piece,int incoming_dir)
{
  unsigned char m=pipe_mask[piece],incoming=dbit[(incoming_dir+2)&3];
  int i,straight=incoming_dir,count=0,only=-1;
  if(!(m&incoming))return -1;
  if(piece==P_X && (m&dbit[straight]))return straight;
  for(i=0;i<4;i++)if(i!=((incoming_dir+2)&3) && (m&dbit[i])){only=i;count++;}
  if(count==1)return only;
  if(m&dbit[straight])return straight;
  return -1;
}

static void plumb_next_level_dialog(const char *msg)
{
  int w=48,h=8,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0;
  int bw=12,bx;unsigned mb=0;
  bx=x+(w-bw)/2;
  acc_subbox(x,y,w,h,"Plumb",1);
  /* Information icon used by normal notice dialogs.  Avoid the OK/tick
     icon here because Plumb temporarily owns those glyph slots. */
  acc_put(x+3,y+2,219,ACC_ATTR(acc_appearance.background,7));
  acc_put(x+4,y+2,174,ACC_ATTR(7,9));
  acc_put(x+5,y+2,219,ACC_ATTR(acc_appearance.background,7));
  acc_text(x+8,y+2,msg,ACC_LABEL,w-11);
  { int focus=-1;
    for(;;){
      acc_button(bx,y+h-3," Next level ",focus==0);
      acc_wait(&k,&mx,&my,&mb);
      if(k==27)return;
      if((mb&1)&&my==y+h-3&&mx>=bx&&mx<bx+bw)return;
      if(k==9||k==271){focus=0;k=0;continue;}
      if(k==13&&focus==0)return;
      k=0;
    }
  }
}

static void plumb_retry_dialog(const char *msg)
{
  int w=48,h=10,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,bx=x+3,n;unsigned mb=0;char line1[48],line2[48];const char *nl;
  line1[0]=line2[0]=0;nl=strchr(msg,'\n');
  if(nl){n=(int)(nl-msg);if(n>47)n=47;memcpy(line1,msg,n);line1[n]=0;strncpy(line2,nl+1,47);line2[47]=0;}
  else {strncpy(line1,msg,47);line1[47]=0;}
  acc_subbox(x,y,w,h,"Plumb",1);
  acc_put(x+3,y+2,219,ACC_ATTR(acc_appearance.background,4));
  acc_put(x+4,y+2,173,ACC_ATTR(4,15));
  acc_put(x+5,y+2,219,ACC_ATTR(acc_appearance.background,4));
  acc_text(x+8,y+2,line1,ACC_LABEL,w-11);if(line2[0])acc_text(x+8,y+3,line2,ACC_LABEL,w-11);
  { int focus=-1;
    for(;;){acc_button(bx,y+h-3,"  Try again  ",focus==0);acc_wait(&k,&mx,&my,&mb);if(k==27)return;if((mb&1)&&my==y+h-3&&mx>=bx&&mx<bx+13)return;if(k==9||k==271){focus=0;k=0;continue;}if(k==13&&focus==0)return;k=0;}
  }
}

static void round_result(int success)
{
  char msg[64];
  spill_animation(ui_bx,ui_by);
  if(success){
    score+=500L+(long)(flow_tiles-target)*25L;
    sprintf(msg,"Level %d complete! %d pipes filled.",level,flow_tiles);
    plumb_next_level_dialog(msg);
    level++;if(level>20)level=20;
    new_round(0);redraw_main_window();
  } else {
    lives--;
    if(lives<=0){
      sprintf(msg,"Game over. Score %ld.",score);plumb_retry_dialog(msg);
      level=1;lives=3;new_round(1);redraw_main_window();
    } else {
      sprintf(msg,"Oh no, you have a leak!\nYou plumbed %d of %d pipes.",flow_tiles,target);plumb_retry_dialog(msg);
      new_round(0);redraw_main_window();
    }
  }
}

static void flow_step(void)
{
  int nx=flow_x+dx[flow_dir],ny=flow_y+dy[flow_dir],p,out;
  if(nx<0||nx>=BW||ny<0||ny>=BH){round_result(qualified);return;}
  if(filled[ny][nx]||board[ny][nx]==EMPTY){round_result(qualified);return;}
  p=board[ny][nx];out=outgoing_dir(p,flow_dir);
  if(out<0){round_result(qualified);return;}
  filled[ny][nx]=1;flow_x=nx;flow_y=ny;flow_dir=out;flow_tiles++;
  score+=50L;if(p==P_X)score+=25L;
  if(flow_tiles>=target)qualified=1;
}

static void place_piece(void)
{
  if(filled[cursor_y][cursor_x])return;
  if(!round_started){round_started=1;round_start=last_step=acc_ticks();}
  if(board[cursor_y][cursor_x]!=EMPTY){
    score-=10L;if(score<0)score=0;
  }
  board[cursor_y][cursor_x]=queue_piece[0];queue_advance();
}

static int mouse_cell(int bx,int by,int mx,int my,int *cx,int *cy)
{
  if(mx<bx||my<by||mx>=bx+BW*2||my>=by+BH)return 0;
  *cx=(mx-bx)/2;*cy=my-by;return 1;
}

int main(int argc,char **argv)
{
  int x,y,bx,by,key=0,mx=0,my=0,buttons=0,last_buttons=0,cx,cy,oldx,oldy,need_redraw=1;
  int last_focus=-1,last_sec=-1;unsigned long now;
  if(acc_help(argc,argv,"!PLUMB","Lay a continuous pipe before the flow catches you."))return 0;
  if(!acc_begin(argv[0],"Plumb",0))return 1;
  srand((unsigned)acc_ticks());x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;bx=x+17;by=y+5;ui_x=x;ui_y=y;ui_bx=bx;ui_by=by;
  acc_box(x,y,DLG_W,DLG_H,"Plumb");plumb_font(1);new_round(1);acc_mouse_display(1);
  while(key!=27){
    now=acc_ticks();
    if(!paused){
      if(!flow_started && seconds_to_flow()==0){round_started=1;flow_started=1;last_step=now;need_redraw=1;}
      if(flow_started && (unsigned long)(now-last_step)>=step_delay()){
        last_step=now;flow_step();need_redraw=1;
      }
    }
    if(seconds_to_flow()!=last_sec){last_sec=seconds_to_flow();draw_status(x,y);}
    if(need_redraw){draw_all(x,y,bx,by);need_redraw=0;last_focus=focus;}
    if(focus!=last_focus){draw_buttons(x,y,focus);last_focus=focus;}
    if(kbhit()){
      key=acc_key();
      if(key==9||key==271){if(focus<0)focus=(key==271)?4:0;else focus=(key==271)?(focus+4)%5:(focus+1)%5;need_redraw=1;key=0;}
      else if(focus==0&&(key==256+72||key==0x4800)){oldx=cursor_x;oldy=cursor_y;if(cursor_y>0)cursor_y--;draw_cell(bx,by,oldx,oldy);if(oldx!=cursor_x||oldy!=cursor_y)draw_cell(bx,by,cursor_x,cursor_y);key=0;}
      else if(focus==0&&(key==256+80||key==0x5000)){oldx=cursor_x;oldy=cursor_y;if(cursor_y<BH-1)cursor_y++;draw_cell(bx,by,oldx,oldy);if(oldx!=cursor_x||oldy!=cursor_y)draw_cell(bx,by,cursor_x,cursor_y);key=0;}
      else if(focus==0&&(key==256+75||key==0x4B00)){oldx=cursor_x;oldy=cursor_y;if(cursor_x>0)cursor_x--;draw_cell(bx,by,oldx,oldy);if(oldx!=cursor_x||oldy!=cursor_y)draw_cell(bx,by,cursor_x,cursor_y);key=0;}
      else if(focus==0&&(key==256+77||key==0x4D00)){oldx=cursor_x;oldy=cursor_y;if(cursor_x<BW-1)cursor_x++;draw_cell(bx,by,oldx,oldy);if(oldx!=cursor_x||oldy!=cursor_y)draw_cell(bx,by,cursor_x,cursor_y);key=0;}
      else if(key==13&&!focus){oldx=cursor_x;oldy=cursor_y;place_piece();redraw_board_action(x,y,bx,by,oldx,oldy);last_sec=seconds_to_flow();key=0;}
      else if(key==' '&&!focus){round_started=1;flow_started=1;last_step=now;need_redraw=1;key=0;}
      else if(key=='p'||key=='P'){toggle_pause();need_redraw=1;key=0;}
      else if(key==13){
        if(focus==1){level=1;lives=3;new_round(1);need_redraw=1;}
        else if(focus==2){round_started=1;flow_started=1;last_step=now;need_redraw=1;}
        else if(focus==3){toggle_pause();need_redraw=1;}
        else if(focus==4)key=27;
        if(key!=27)key=0;
      }
    }
    if(acc_mouse_present){
      acc_mouse(&mx,&my,&buttons);if(buttons&ACC_MOUSE_OUTSIDE){key=27;break;}
      if((buttons&1)&&!(last_buttons&1)){
        if((buttons&1)&&my==y&&(mx==x+DLG_W-5||mx==x+DLG_W-4)){key=27;}
        else if(mouse_cell(bx,by,mx,my,&cx,&cy)){
          oldx=cursor_x;oldy=cursor_y;cursor_x=cx;cursor_y=cy;focus=0;place_piece();
          redraw_board_action(x,y,bx,by,oldx,oldy);last_sec=seconds_to_flow();
        } else if(my==y+DLG_H-3){
          if(mx>=x+3&&mx<x+10){focus=1;level=1;lives=3;new_round(1);need_redraw=1;}
          else if(mx>=x+12&&mx<x+21){focus=2;round_started=1;flow_started=1;last_step=now;need_redraw=1;}
          else if(mx>=x+23&&mx<x+33){focus=3;toggle_pause();need_redraw=1;}
          else if(mx>=x+DLG_W-10&&mx<x+DLG_W-4){key=27;}
        }
      }
      last_buttons=buttons;
    }
  }
  plumb_font(0);acc_end_screen();acc_end();return 0;
}
