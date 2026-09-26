/*
    __                           __    __
   / /   ____ ___  ______  _____/ /_  / /
  / /   / __ `/ / / / __ \/ ___/ __ \/ / 
 / /___/ /_/ / /_/ / / / /__/ / / /_/  
/_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
Launch! for DOS ---------------------
*/
/*
 * MAINTAINER NOTES - Launch! 3.73
 * File: BOXBLD.C
 * Role: Build copy of !BOXES
 * Build/ownership: Derived from BOXES.C.
 * Maintainer contract: Keep synchronized with BOXES.C.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* Launch! Boxes accessory: 250-puzzle Sokoban/box-pushing game.
   Puzzle set: selected Microban I/II levels by David W. Skinner.
   Levels 1-200 include precomputed Solve playback; 201-250 are challenge levels.
   Tile artwork is embedded from the canonical LaunchUI glyph library.
   Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "ACCLIB.H"

#define MAX_W 22
#define MAX_H 14
#define MAX_CELLS (MAX_W*MAX_H)
#define BUILTIN_LEVELS 250
#define MAX_LEVELS 750
#define MAX_LEVEL_FILES 32
#define SOLVE_LIMIT 200
#define SOLVE_MAX 2048
#define DLG_W 68
#define DLG_H 20

#define TILE_VOID 0
#define TILE_FLOOR 1
#define TILE_WALL 2
#define TILE_GOAL 3

#define GLYPH_WALL_L 193 /* both brick cells require VGA 9th-column extension */
#define GLYPH_WALL_R 195
#define GLYPH_BOX_L 206  /* left cell requires extension; right does not */
#define GLYPH_BOX_R 185
#define GLYPH_GOAL_L 202 /* left cell requires extension; right does not */
#define GLYPH_GOAL_R 183
#define GLYPH_MAN_L 204  /* left cell requires extension; right does not */
#define GLYPH_MAN_R 184
#define GLYPH_FIELD_A 221 /* logical glyph 88, rebased into VGA 9th-cell range */
#define GLYPH_FIELD_B 222 /* private blank right-half; logical 89 is Open in 3.7 */

#define COL_VOID ACC_BG
#define COL_FLOOR ACC_ATTR(6,6)
#define COL_WALL ACC_ATTR(4,12)
#define COL_BOX ACC_ATTR(6,14)
#define COL_BOX_GOAL ACC_ATTR(6,12)
#define COL_GOAL ACC_ATTR(6,10)
#define COL_MAN ACC_ATTR(6,13)

static const unsigned char glyph_code[8]={
  GLYPH_WALL_L,GLYPH_WALL_R,GLYPH_BOX_L,GLYPH_BOX_R,
  GLYPH_GOAL_L,GLYPH_GOAL_R,GLYPH_MAN_L,GLYPH_MAN_R
};

static const unsigned char boxes_glyph[8][32]={
  {0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xFF,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xDF,0xC0,0x00,0x98,0x98,0x86,0x86,0x81,0x81,0x86,0x86,0x98,0x98,0x00,0xC0,0xDF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xFB,0x03,0x00,0x19,0x19,0x61,0x61,0x81,0x81,0x61,0x61,0x19,0x19,0x00,0x03,0xFB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x03,0x0F,0x1C,0x1B,0x37,0x37,0x3F,0x3F,0x1F,0x1F,0x0F,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0xC0,0xF0,0xF8,0xF8,0xFC,0xFC,0xFC,0xFC,0xF8,0xF8,0xF0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x01,0x1F,0x20,0x44,0x44,0x41,0x20,0x1F,0x7F,0xFF,0x0F,0x1F,0x3C,0x3C,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x80,0xF0,0x08,0x24,0x24,0x84,0x08,0xF0,0xFE,0xFF,0xF8,0xF8,0x3C,0x3C,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

static const unsigned char boxes_glyph14[8][32]={
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x04,0x04,0x04,0x04,0x04,0x04,0x04,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xBF,0x00,0x98,0x98,0x86,0x86,0x81,0x81,0x86,0x86,0x98,0x98,0x00,0xBF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xFD,0x00,0x19,0x19,0x61,0x61,0x81,0x81,0x61,0x61,0x19,0x19,0x00,0xFD,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x03,0x0F,0x1C,0x1B,0x37,0x37,0x3F,0x3F,0x1F,0x1F,0x0F,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0xC0,0xF0,0xF8,0xF8,0xFC,0xFC,0xFC,0xFC,0xF8,0xF8,0xF0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1F,0x20,0x44,0x44,0x41,0x20,0x1F,0x7F,0xFF,0x0F,0x1F,0x3C,0x3C,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xF0,0x08,0x24,0x24,0x84,0x08,0xF0,0xFE,0xFF,0xF8,0xF8,0x3C,0x3C,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

static unsigned char old_glyph[8][32],old_field[2][32];

static long level_offset[MAX_LEVELS];
static unsigned char level_file_index[MAX_LEVELS];
static char level_files[MAX_LEVEL_FILES][13];static int level_file_count=0,level_count=0;
static unsigned short current_best=0;
static char current_solution[SOLVE_MAX];
static char level_line[SOLVE_MAX+64];

static void boxes_level_path(char *p,int fi)
{
  size_t n;strcpy(p,acc_directory);n=strlen(p);if(n&&p[n-1]!='\\'&&p[n-1]!='/')strcat(p,"\\");strcat(p,level_files[fi]);
}
static void boxes_strip_eol(char *s)
{size_t n=strlen(s);while(n&&(s[n-1]=='\n'||s[n-1]=='\r'))s[--n]=0;}
static int boxes_scan_levels(void)
{
  struct find_t ff;FILE *f;char mask[ACC_PATH],p[ACC_PATH],line[96],tmp[13];long pos;unsigned e;int i,j,fi;
  level_count=0;level_file_count=0;strcpy(mask,acc_directory);if(mask[0]&&mask[strlen(mask)-1]!='\\'&&mask[strlen(mask)-1]!='/')strcat(mask,"\\");strcat(mask,"BOXES*.LVL");
  e=_dos_findfirst(mask,_A_NORMAL,&ff);while(!e&&level_file_count<MAX_LEVEL_FILES){strncpy(level_files[level_file_count],ff.name,12);level_files[level_file_count][12]=0;level_file_count++;e=_dos_findnext(&ff);}
  for(i=0;i<level_file_count-1;i++)for(j=i+1;j<level_file_count;j++)if(stricmp(level_files[i],level_files[j])>0){strcpy(tmp,level_files[i]);strcpy(level_files[i],level_files[j]);strcpy(level_files[j],tmp);}
  for(fi=0;fi<level_file_count&&level_count<MAX_LEVELS;fi++){boxes_level_path(p,fi);f=fopen(p,"rb");if(!f)continue;for(;;){pos=ftell(f);if(!fgets(line,sizeof(line),f))break;if(line[0]=='@'&&line[1]=='L'&&line[2]=='|'&&level_count<MAX_LEVELS){level_offset[level_count]=pos;level_file_index[level_count]=(unsigned char)fi;level_count++;}}fclose(f);}
  return level_count>0;
}

static unsigned char tile[MAX_CELLS],box_at[MAX_CELLS];
static int board_w,board_h,player_x,player_y,current_level,moves,pushes;

static int inside(int x,int y){return x>=0&&x<board_w&&y>=0&&y<board_h;}
static int at(int x,int y){return y*board_w+x;}

static void boxes_font(int install)
{
  int i;unsigned char blank[32];
  if(install){
    for(i=0;i<8;i++){acc_glyph_read(glyph_code[i],old_glyph[i]);acc_glyph_library(42+i,glyph_code[i]);}
    acc_glyph_read(GLYPH_FIELD_A,old_field[0]);acc_glyph_read(GLYPH_FIELD_B,old_field[1]);
    acc_glyph_library(88,GLYPH_FIELD_A);memset(blank,0,sizeof(blank));acc_glyph_write(GLYPH_FIELD_B,blank);
  } else {
    for(i=0;i<8;i++)acc_glyph_write(glyph_code[i],old_glyph[i]);
    acc_glyph_write(GLYPH_FIELD_A,old_field[0]);acc_glyph_write(GLYPH_FIELD_B,old_field[1]);
  }
}

static void boxes_ui_glyphs(int ui){(void)ui;}
static int load_level(int n)
{
  FILE *f;char pth[ACC_PATH],row[MAX_W+4],*p,*q;int i,x,y,cells,w,h;
  boxes_level_path(pth,level_file_index[n]);f=fopen(pth,"rb");if(!f)return 0;
  if(n<0||n>=level_count||fseek(f,level_offset[n],SEEK_SET)!=0||!fgets(level_line,sizeof(level_line),f)){fclose(f);return 0;}
  boxes_strip_eol(level_line);if(level_line[0]!='@'||level_line[1]!='L'||level_line[2]!='|'){fclose(f);return 0;}
  p=level_line+3;q=strchr(p,'|');if(!q){fclose(f);return 0;}*q=0;w=atoi(p);
  p=q+1;q=strchr(p,'|');if(!q){fclose(f);return 0;}*q=0;h=atoi(p);
  p=q+1;q=strchr(p,'|');if(!q){fclose(f);return 0;}*q=0;current_best=(unsigned short)atoi(p);
  p=q+1;strncpy(current_solution,p,SOLVE_MAX-1);current_solution[SOLVE_MAX-1]=0;
  if(w<1||w>MAX_W||h<1||h>MAX_H){fclose(f);return 0;}
  board_w=w;board_h=h;cells=w*h;
  memset(tile,TILE_VOID,sizeof(tile));memset(box_at,0,sizeof(box_at));player_x=player_y=0;
  for(y=0;y<h;y++){
    if(!fgets(row,sizeof(row),f)){fclose(f);return 0;}boxes_strip_eol(row);
    if((int)strlen(row)<w){fclose(f);return 0;}
    for(x=0;x<w;x++){
      i=y*w+x;
      switch(row[x]){
        case '~':tile[i]=TILE_VOID;break;
        case '#':tile[i]=TILE_WALL;break;
        case ' ':tile[i]=TILE_FLOOR;break;
        case '.':tile[i]=TILE_GOAL;break;
        case '$':tile[i]=TILE_FLOOR;box_at[i]=1;break;
        case '*':tile[i]=TILE_GOAL;box_at[i]=1;break;
        case '@':tile[i]=TILE_FLOOR;player_x=x;player_y=y;break;
        case '+':tile[i]=TILE_GOAL;player_x=x;player_y=y;break;
        default:fclose(f);return 0;
      }
    }
  }
  fclose(f);moves=pushes=0;return 1;
}

static int solved(void)
{
  int i,cells=board_w*board_h,boxes=0,goals=0;
  for(i=0;i<cells;i++){
    if(box_at[i]){boxes++;if(tile[i]==TILE_GOAL)goals++;}
  }
  return boxes&&boxes==goals;
}

static int move_player(int dx,int dy)
{
  int nx=player_x+dx,ny=player_y+dy,bi,nnx,nny,ni;
  if(!inside(nx,ny))return 0;
  bi=at(nx,ny);
  if(tile[bi]==TILE_VOID||tile[bi]==TILE_WALL)return 0;
  if(box_at[bi]){
    nnx=nx+dx;nny=ny+dy;
    if(!inside(nnx,nny))return 0;
    ni=at(nnx,nny);
    if(tile[ni]==TILE_VOID||tile[ni]==TILE_WALL||box_at[ni])return 0;
    box_at[bi]=0;box_at[ni]=1;pushes++;
  }
  player_x=nx;player_y=ny;moves++;
  return 1;
}

static int walk_to(int tx,int ty)
{
  int q[MAX_CELLS],prev[MAX_CELLS],step[MAX_CELLS];
  int head=0,tail=0,i,p,nx,ny,ni,pi,dir;
  static const int dx[4]={0,0,-1,1};
  static const int dy[4]={-1,1,0,0};
  int target;
  if(!inside(tx,ty))return 0;
  target=at(tx,ty);
  if(tile[target]==TILE_VOID||tile[target]==TILE_WALL||box_at[target])return 0;
  for(i=0;i<MAX_CELLS;i++){prev[i]=-1;step[i]=-1;}
  pi=at(player_x,player_y);prev[pi]=pi;q[tail++]=pi;
  while(head<tail&&prev[target]<0){
    p=q[head++];
    for(dir=0;dir<4;dir++){
      nx=(p%board_w)+dx[dir];ny=(p/board_w)+dy[dir];
      if(!inside(nx,ny))continue;
      ni=at(nx,ny);
      if(prev[ni]>=0||tile[ni]==TILE_VOID||tile[ni]==TILE_WALL||box_at[ni])continue;
      prev[ni]=p;step[ni]=dir;q[tail++]=ni;
    }
  }
  if(prev[target]<0)return 0;
  tail=0;p=target;
  while(p!=pi&&tail<MAX_CELLS){q[tail++]=step[p];p=prev[p];}
  while(tail>0){
    dir=q[--tail];
    move_player(dx[dir],dy[dir]);
  }
  return target!=pi;
}

static unsigned char prev_box[MAX_CELLS];
static int prev_player_x=-1,prev_player_y=-1;

static void draw_pair(int x,int y,int left,int right,int attr)
{
  acc_put(x,y,left,attr);acc_put(x+1,y,right,attr);
}

static void draw_cell(int bx,int by,int x,int y)
{
  int i=at(x,y);
  if(tile[i]==TILE_VOID)return;
  else if(tile[i]==TILE_WALL)draw_pair(bx+x*2,by+y,GLYPH_WALL_L,GLYPH_WALL_R,COL_WALL);
  else if(x==player_x&&y==player_y)draw_pair(bx+x*2,by+y,GLYPH_MAN_L,GLYPH_MAN_R,COL_MAN);
  else if(box_at[i])draw_pair(bx+x*2,by+y,GLYPH_BOX_L,GLYPH_BOX_R,
                               tile[i]==TILE_GOAL?COL_BOX_GOAL:COL_BOX);
  else if(tile[i]==TILE_GOAL)draw_pair(bx+x*2,by+y,GLYPH_GOAL_L,GLYPH_GOAL_R,COL_GOAL);
  else draw_pair(bx+x*2,by+y,' ',' ',COL_FLOOR);
}

static void remember_board(void)
{
  memcpy(prev_box,box_at,sizeof(prev_box));
  prev_player_x=player_x;prev_player_y=player_y;
}

static void render_board(int bx,int by)
{
  int x,y,fy=by-(MAX_H-board_h)/2;
  int dlgx=bx-(DLG_W-board_w*2)/2;
  int pa=ACC_ATTR(acc_appearance.background,(acc_appearance.background&7)|8);
  /* Fixed playfield texture: logical glyphs 88 + 89 are always drawn as
     the same left/right pair. Every row begins 88,89,88,89... with no
     stagger, and the field extends from the dialog's left inner border to
     its right inner border. */
  for(y=0;y<MAX_H;y++)for(x=0;x<(DLG_W-2)/2;x++)
    draw_pair(dlgx+1+x*2,fy+y,GLYPH_FIELD_A,GLYPH_FIELD_B,pa);
  for(y=0;y<board_h;y++)for(x=0;x<board_w;x++)draw_cell(bx,by,x,y);
  remember_board();
}

static void render_changed(int bx,int by)
{
  int x,y,i,was_player,is_player;
  for(y=0;y<board_h;y++)for(x=0;x<board_w;x++){
    i=at(x,y);
    was_player=(x==prev_player_x&&y==prev_player_y);
    is_player=(x==player_x&&y==player_y);
    if(prev_box[i]!=box_at[i]||was_player!=is_player)draw_cell(bx,by,x,y);
  }
  remember_board();
}

static void save_level(void)
{
  char p[ACC_PATH];
  FILE *f;
  unsigned short n=(unsigned short)current_level;
  acc_path(p,"DATA","BOXES.DAT");
  f=fopen(p,"wb");
  if(f){fwrite(&n,sizeof(n),1,f);fclose(f);}
}

static void load_saved_level(void)
{
  char p[ACC_PATH];
  FILE *f;
  unsigned short n=0;
  current_level=0;
  acc_path(p,"DATA","BOXES.DAT");
  f=fopen(p,"rb");
  if(f){if(fread(&n,sizeof(n),1,f)==1&&n<(unsigned)level_count)current_level=(int)n;else {rewind(f);{unsigned char oldn=0;if(fread(&oldn,1,1,f)==1&&oldn<level_count)current_level=oldn;}}fclose(f);}
}

static void change_level(int delta)
{
  current_level+=delta;
  if(current_level<0)current_level=level_count-1;
  if(current_level>=level_count)current_level=0;
  load_level(current_level);
  save_level();
}

static void draw_counts(int x,int y)
{
  char s[32],field[24];
  int len,start,i;
  sprintf(s,"Moves %d  Pushes %d",moves,pushes);
  len=(int)strlen(s);
  if(len>23)len=23;
  for(i=0;i<23;i++)field[i]=' ';
  start=23-len;
  memcpy(field+start,s,len);
  field[23]=0;
  acc_text(x+DLG_W-26,y+2,field,ACC_LABEL,23);
}

static void draw_status(int x,int y)
{
  char s[32];
  sprintf(s,"Puzzle %03d of %d",current_level+1,level_count);
  acc_text(x+3,y+2,s,ACC_HEADING,20);
  draw_counts(x,y);
}

static unsigned char solve_path[SOLVE_MAX];static int solve_len;static int solve_abort;
static int boxes_find_solution(void)
{
  int i;
  solve_abort=0;if(current_level>=SOLVE_LIMIT||!current_solution[0])return 0;
  solve_len=(int)strlen(current_solution);if(solve_len>=SOLVE_MAX)return 0;
  for(i=0;i<solve_len;i++){
    if(current_solution[i]<'0'||current_solution[i]>'3')return 0;
    solve_path[i]=(unsigned char)(current_solution[i]-'0');
  }
  return 1;
}
static unsigned long boxes_play_ticks(void)
{
 /* Read the BIOS timer through INT 1Ah during Solve playback.  This makes the
    animation clock completely independent of mouse polling, hover processing
    and the accessory idle loop. */
 union REGS r;
 memset(&r,0,sizeof(r));r.h.ah=0x00;int86(0x1A,&r,&r);
 return ((unsigned long)r.x.cx<<16)|(unsigned long)r.x.dx;
}
static int boxes_play_solution(int bx,int by,int x,int y)
{
 int i;static const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
 unsigned long next_tick;
 load_level(current_level);render_board(bx,by);draw_counts(x,y);
 next_tick=boxes_play_ticks();
 for(i=0;i<solve_len;i++){
   /* One BIOS timer tick (~55 ms) between moves.  No mouse state is read or
      required for the animation to advance. */
   while((unsigned long)(boxes_play_ticks()-next_tick)<1UL){
     if(kbhit()){int k=acc_key();if(k==27){solve_abort=1;return 0;}}
   }
   next_tick=boxes_play_ticks();
   if(kbhit()){int k=acc_key();if(k==27){solve_abort=1;return 0;}}
   move_player(dx[solve_path[i]],dy[solve_path[i]]);
   render_changed(bx,by);draw_counts(x,y);
 }
 return solved();
}

static void boxes_solve_action(int x,int y)
{
 int bx,by;
 if(!boxes_find_solution()){
   boxes_ui_glyphs(1);
   acc_notice("Game Over",solve_abort==2?"Solver time limit reached.":"There are no more legal moves.");
   boxes_ui_glyphs(0);
   return;
 }
 /* Release 3.6 adds persistent hover/tool-tip state around accessory buttons.
    Solve playback is a self-contained animation, so suspend that UI state
    while the recorded moves are played.  This also prevents a stale toolbar
    click from being interpreted after the animation returns. */
 acc_modal_begin();
 acc_mouse_display(0);
 /* Replace the Solve button itself with the transient status so the toolbar
    never has two controls occupying the same visual space. */
 acc_fill(x+11,y+17,10,2,' ',ACC_BG);
 acc_text(x+12,y+17,"Solving",ACC_HEADING,7);
 bx=x+(DLG_W-board_w*2)/2;by=y+3+(14-board_h)/2;
 boxes_play_solution(bx,by,x,y);
 acc_mouse_display(0);
 acc_modal_end();
}

static int boxes_goto_dialog(void)
{
  int w=38,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=0,focus=-1,v,result=-1;unsigned mb=0;char s[5];s[0]=0;
  acc_modal_begin();acc_mouse_display(1);
  for(;;){acc_subbox(x,y,w,h,"Go To",1);acc_text(x+3,y+2,"Level number:",ACC_LABEL,13);acc_fill(x+17,y+2,5,1,' ',focus==0?ACC_SELECT:ACC_CONTROL);acc_text(x+17,y+2,s,focus==0?ACC_SELECT:ACC_CONTROL,4);acc_button(x+3,y+6,"  Go  ",focus==1);acc_button(x+11,y+6,"  Cancel  ",focus==2);acc_wait(&k,&mx,&my,&mb);
    if((mb&1)&&my==y+2&&mx>=x+17&&mx<x+22){focus=0;k=0;continue;}if((mb&1)&&my==y+6){if(mx>=x+3&&mx<x+9){focus=1;k=13;}else if(mx>=x+11&&mx<x+21){focus=2;k=13;}}
    if(k==27){result=-1;break;}if(k==9||k==271){if(focus<0)focus=(k==271)?2:0;else focus=(k==271)?(focus+2)%3:(focus+1)%3;k=0;continue;}if(k==13&&focus==2){result=-1;break;}if(k==13&&focus==1){v=atoi(s);if(v>=1&&v<=level_count){result=v-1;break;}k=0;continue;}
    if(focus==0){if(k==8&&pos){s[--pos]=0;}else if(k>='0'&&k<='9'&&pos<3){s[pos++]=(char)k;s[pos]=0;}}k=0;}
  acc_modal_end();acc_mouse_display(1);return result;
}
int main(int argc,char **argv)
{
  int x,y,bx,by,key=0,mx=0,my=0,dirty=1,full=1,focus=-1;
  int tx,ty,won_moves,won_best;
  unsigned mb=0;
  char msg[96];
  if(acc_help(argc,argv,"!BOXES","A 250-puzzle box-pushing game using custom text-mode tile glyphs."))return 0;
  if(!acc_begin(argv[0],"Boxes",0))return 1;
  boxes_font(1);acc_glyph_library(5,169);acc_glyph_library(6,170);
  if(!boxes_scan_levels()){acc_notice("Boxes Error","No valid BOXES*.LVL level files were found.");boxes_font(0);acc_end_screen();acc_end();return 1;}
  x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;
  acc_box(x,y,DLG_W,DLG_H,"Boxes");
  load_saved_level();
  if(!load_level(current_level)){current_level=0;if(!load_level(0)){boxes_font(0);acc_end_screen();acc_end();return 1;}}

  while(key!=27){
    bx=x+(DLG_W-board_w*2)/2;
    by=y+3+(14-board_h)/2;
    if(full){
      acc_fill(x+1,y+1,DLG_W-2,DLG_H-2,' ',ACC_BG);
      draw_status(x,y);
      render_board(bx,by);
      acc_button(x+3,y+17," Retry ",focus==1);
      if(current_level<SOLVE_LIMIT)acc_button(x+11,y+17,"  Solve  ",focus==2);else acc_button_disabled(x+11,y+17,"  Solve  ");
      acc_button(x+22,y+17," Prev ",focus==3);
      acc_button(x+29,y+17," Next ",focus==4);
      acc_button(x+36,y+17,"  Go To  ",focus==5);
      acc_button(x+DLG_W-10,y+17," Exit ",focus==6);
      full=0;dirty=0;
    }else if(dirty){
      render_changed(bx,by);
      draw_counts(x,y);
      dirty=0;
    }
    acc_wait(&key,&mx,&my,&mb);

    /* Consistent game shortcuts. */
    if(key==256+0x3F){focus=0;load_level(current_level);full=1;key=0;} /* F5 retry */
    else if(key==256+0x73){focus=0;change_level(-1);full=1;key=0;} /* Ctrl+Left */
    else if(key==256+0x74){focus=0;change_level(1);full=1;key=0;}  /* Ctrl+Right */
    else if(key==7){int g;g=boxes_goto_dialog();if(g>=0){current_level=g;save_level();load_level(current_level);}acc_box(x,y,DLG_W,DLG_H,"Boxes");focus=0;full=1;key=0;} /* Ctrl+G */

    if((mb&1)&&!(mb&ACC_MOUSE_MOVED)){
      if(my==y+17){
        if(mx>=x+3&&mx<x+10){focus=1;load_level(current_level);full=1;key=0;}
        else if(current_level<SOLVE_LIMIT&&mx>=x+11&&mx<x+20){focus=0;boxes_solve_action(x,y);full=1;dirty=0;key=0;mb=0;}
        else if(mx>=x+23&&mx<x+28){focus=3;change_level(-1);full=1;key=0;}
        else if(mx>=x+30&&mx<x+35){focus=4;change_level(1);full=1;key=0;}
        else if(mx>=x+36&&mx<x+45){int g;focus=5;g=boxes_goto_dialog();if(g>=0){current_level=g;save_level();load_level(current_level);}acc_box(x,y,DLG_W,DLG_H,"Boxes");focus=0;full=1;key=0;}
        else if(mx>=x+DLG_W-10&&mx<x+DLG_W-4){focus=6;key=27;}
      }else if(mx>=bx&&mx<bx+board_w*2&&my>=by&&my<by+board_h){
        focus=0;tx=(mx-bx)/2;ty=my-by;
        if(walk_to(tx,ty))dirty=1;
        key=0;
      }
    }else if(key==9||key==271){
      if(focus<0)focus=(key==271)?6:0;
      else do{focus=(key==271)?(focus+6)%7:(focus+1)%7;}while(current_level>=SOLVE_LIMIT&&focus==2);
      acc_button(x+3,y+17," Retry ",focus==1);
      if(current_level<SOLVE_LIMIT)acc_button(x+11,y+17,"  Solve  ",focus==2);else acc_button_disabled(x+11,y+17,"  Solve  ");
      acc_button(x+22,y+17," Prev ",focus==3);
      acc_button(x+29,y+17," Next ",focus==4);
      acc_button(x+36,y+17,"  Go To  ",focus==5);
      acc_button(x+DLG_W-10,y+17," Exit ",focus==6);
      key=0;
    }else if(key==13&&focus>0){
      if(focus==1){load_level(current_level);full=1;}
      else if(focus==2&&current_level<SOLVE_LIMIT){focus=0;boxes_solve_action(x,y);full=1;dirty=0;mb=0;}
      else if(focus==3){change_level(-1);full=1;}
      else if(focus==4){change_level(1);full=1;}
      else if(focus==5){int g;g=boxes_goto_dialog();if(g>=0){current_level=g;save_level();load_level(current_level);}acc_box(x,y,DLG_W,DLG_H,"Boxes");focus=0;full=1;}
      else if(focus==6)key=27;
      if(key!=27)key=0;
    }else if(key==256+72){focus=0;dirty=move_player(0,-1);key=0;}
    else if(key==256+80){focus=0;dirty=move_player(0,1);key=0;}
    else if(key==256+75){focus=0;dirty=move_player(-1,0);key=0;}
    else if(key==256+77){focus=0;dirty=move_player(1,0);key=0;}
    else if(key=='r'||key=='R'){focus=0;load_level(current_level);full=1;key=0;}
    else if(key==256+73){focus=0;change_level(-1);full=1;key=0;}
    else if(key==256+81){focus=0;change_level(1);full=1;key=0;}
    else if(key!=27)key=0;

    if((dirty||full)&&solved()){
      won_moves=moves;won_best=current_best;
      if(dirty){render_changed(bx,by);draw_counts(x,y);dirty=0;}
      if(won_best)sprintf(msg,"Puzzle solved!\nYour moves %d\nBest %d",won_moves,won_best);
      else sprintf(msg,"Puzzle solved!\nYour moves %d",won_moves);
      boxes_ui_glyphs(1);acc_notice("Boxes",msg);boxes_ui_glyphs(0);
      if(current_level<level_count-1)current_level++;
      else current_level=0;
      save_level();
      load_level(current_level);
      focus=-1;
      full=1;
    }
  }
  save_level();
  boxes_font(0);
  acc_end();
  return 0;
}
