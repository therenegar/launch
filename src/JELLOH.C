/*
    __                           __    __
   / /   ____ ___  ______  _____/ /_  / /
  / /   / __ `/ / / / __ \/ ___/ __ \/ /
 / /___/ /_/ / /_/ / / / / /__/ / / /_/
/_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)
Launch! for DOS ---------------------
*/
/*
 * MAINTAINER NOTES - Launch! 3.79
 * File: JELLOH.C
 * Role: !JELLOH side-view block-merging puzzle game.
 * Build/ownership: Canonical source; JELLOHBLD.C is the build copy.
 *
 * Game contract:
 * - Coloured jelly pieces move horizontally and fall under gravity.
 * - Orthogonally touching jellies of the same colour merge permanently.
 * - Black support jellies move and fall but NEVER merge with one another.
 * - Fixed coloured jellies participate in colour merging but cannot move;
 *   once another jelly merges with one, the complete merged piece is fixed.
 * - A hidden wall jelly is shown as a coloured mark in a brick.  Bringing a
 *   matching jelly alongside it pushes that jelly one square away, reveals
 *   the hidden jelly in the vacated square, and merges the two.
 * - A puzzle is complete when all visible/hidden jellies of each colour have
 *   become one connected piece.  Black supports are ignored for completion.
 * - Levels are data driven.  JELLOH.LVL is the base pack and additional
 *   JELLOH*.LVL files can be dropped beside it for future expansion packs.
 *
 * Level characters:
 *   # wall/brick    . or space empty    k (or legacy w) black support
 *   b r g p movable jelly               B R G P fixed jelly
 *   1 2 3 4 hidden wall jelly: blue/red/green/purple respectively
 *
 * DOS constraints: Microsoft C/C++ 7.0 small model.  Keep large scratch
 * arrays static; avoid putting board-sized work buffers on the runtime stack.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <time.h>
#include "ACCLIB.H"

#define MAX_W 24
#define MAX_H 13
#define MAX_CELLS (MAX_W*MAX_H)
#define MAX_LEVELS 160
#define MAX_LEVEL_FILES 16
#define DLG_W 68
#define DLG_H 20
#define WALL_L 193
#define WALL_R 195
#define FIELD_A 199
#define FIELD_B 32
#define JELLY_SINGLE_L 204
#define JELLY_SINGLE_R 205
#define JELLY_TL 206
#define JELLY_TR 207
#define JELLY_BR 208
#define JELLY_BL 209
#define JELLY_STAR 166
#define STAR_COUNT 12

static unsigned char board[MAX_CELLS];
static unsigned short piece_id[MAX_CELLS];
static unsigned char piece_fixed[MAX_CELLS+1];
static unsigned char piece_colour[MAX_CELLS+1];
static unsigned short next_piece_id=1;
static unsigned char star_mask[MAX_CELLS];
static unsigned char sel_mask[MAX_CELLS];
static unsigned char visit[MAX_CELLS];
static short qx[MAX_CELLS],qy[MAX_CELLS];
static short gx[MAX_CELLS],gy[MAX_CELLS];
static int board_w,board_h,sel_x=-1,sel_y=-1,moves=0;
static int current_level=0,level_count=0;
static long level_offset[MAX_LEVELS];
static unsigned char level_file_index[MAX_LEVELS];
static char level_files[MAX_LEVEL_FILES][13];
static int level_file_count=0;
static char level_title[32];
static unsigned char old_wall[2][32],old_field[2][32],old_jelly_glyph[6][32],old_star_glyph[32];
static int jelly_font_active=0,jelly_shapes_active=0;


/* Rounded Jelly and star artwork supplied for Release 3.74. Both VGA
   and EGA forms are embedded so !JELLY remains independent of the user's
   currently selected font. Jelly artwork is copied to private VGA line-graphics slots 204-209. These slots retain ninth-cell extension without overwriting UI glyphs 219-223:
   VGA/EGA ninth-column extension applies only to the C0-DF line-graphics
   range, and relocating these shapes to ordinary private slots creates a
   visible black seam. The star alone uses private slot 166. All replaced
   glyphs are restored on exit. */
static const unsigned char jelly_vga_glyph[6][32]={
  {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xF8,0xFE,0xFE,0xFF,0x6F,0x6F,0x6F,0xFF,0xFF,0xFD,0xFD,0xFD,0xFB,0xE6,0x1E,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x07,0x1F,0x1F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xF8,0xFE,0xFE,0xFF,0xFF,0x6F,0x6F,0x6F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFD,0xFD,0xFD,0xFD,0xFB,0xFB,0xE6,0x1E,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x1F,0x1F,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};
static const unsigned char jelly_ega_glyph[6][32]={
  {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xF8,0xFE,0x6E,0x6F,0x6F,0xFF,0xFF,0xFD,0xFD,0xFD,0xFB,0xE6,0x1E,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x07,0x1F,0x1F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xF8,0xFE,0xFE,0xFF,0xFF,0x6F,0x6F,0x6F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0xFF,0xFF,0xFF,0xFF,0xFF,0xFD,0xFD,0xFD,0xFD,0xFB,0xFB,0xE6,0x1E,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x1F,0x1F,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};
static const unsigned char jelly_star_vga[32]={0x00,0x00,0x00,0x00,0x00,0x18,0x18,0xFF,0xFF,0x3C,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char jelly_star_ega[32]={0x00,0x00,0x00,0x00,0x00,0x18,0x18,0xFF,0xFF,0x3C,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

static int inside(int x,int y)
{return x>=0&&x<board_w&&y>=0&&y<board_h;}

static int at(int x,int y)
{return y*board_w+x;}

static int jelly_cell(unsigned char c)
{return c=='b'||c=='r'||c=='g'||c=='p'||c=='y';}

static int jelly_symbol(unsigned char c)
{return jelly_cell((unsigned char)(c|0x20));}

static unsigned char jelly_colour(unsigned char c)
{return jelly_symbol(c)?(unsigned char)(c|0x20):0;}

static int hidden_cell(unsigned char c)
{return c>='1'&&c<='5';}

static unsigned char hidden_colour(unsigned char c)
{static const unsigned char hc[5]={'b','r','g','p','y'};return hidden_cell(c)?hc[c-'1']:0;}

static int support_cell(unsigned char c)
{return c=='k';}

static int piece_cell(unsigned char c)
{return jelly_cell(c)||support_cell(c);}

static int logical_colour_present(unsigned char c)
{
  int i; unsigned char b,h;
  c=(unsigned char)(c|0x20);
  for(i=0;i<board_w*board_h;++i){
    b=board[i];
    if(jelly_colour(b)==c)return 1;
    h=hidden_colour(b);if(h==c)return 1;
  }
  return 0;
}

static int colour_fg(unsigned char c,int bright)
{
  static const unsigned char logical[5]={'b','r','g','p','y'};
  static const int palette[5]={3,5,2,1,4}; /* Cyan, Purple, Green, Blue, Red */
  int i,rank=0,v=3;
  c=(unsigned char)(c|0x20);
  /* Compress whichever logical colours this level actually uses onto the
     palette from the beginning. Thus every one-colour level is Cyan, every
     two-colour level is Cyan/Purple, etc., regardless of its source symbols. */
  for(i=0;i<5;++i){
    if(logical[i]==c){v=palette[rank];break;}
    if(logical_colour_present(logical[i]))++rank;
  }
  return bright?v+8:v;
}

static void jelly_level_path(char *out,int fi)
{
  size_t n;
  strcpy(out,acc_directory);n=strlen(out);
  if(n&&out[n-1]!='\\'&&out[n-1]!='/')strcat(out,"\\");
  strcat(out,"GAMERES\\");strcat(out,level_files[fi]);
}

static void strip_eol(char *s)
{
  size_t n=strlen(s);
  while(n&&(s[n-1]=='\r'||s[n-1]=='\n'))s[--n]=0;
}

static int scan_levels(void)
{
  struct find_t ff;
  FILE *f;
  char mask[ACC_PATH],path[ACC_PATH],line[96],tmp[13];
  long pos;
  unsigned err;
  int i,j,fi;

  level_count=0;level_file_count=0;
  strcpy(mask,acc_directory);
  if(mask[0]&&mask[strlen(mask)-1]!='\\'&&mask[strlen(mask)-1]!='/')strcat(mask,"\\");
  strcat(mask,"GAMERES\\JELLOH*.LVL");
  err=_dos_findfirst(mask,_A_NORMAL,&ff);
  while(!err&&level_file_count<MAX_LEVEL_FILES){
    strncpy(level_files[level_file_count],ff.name,12);
    level_files[level_file_count][12]=0;
    level_file_count++;
    err=_dos_findnext(&ff);
  }
  for(i=0;i<level_file_count-1;i++)for(j=i+1;j<level_file_count;j++){
    if(stricmp(level_files[i],level_files[j])>0){
      strcpy(tmp,level_files[i]);strcpy(level_files[i],level_files[j]);strcpy(level_files[j],tmp);
    }
  }
  for(fi=0;fi<level_file_count&&level_count<MAX_LEVELS;fi++){
    jelly_level_path(path,fi);f=fopen(path,"rb");if(!f)continue;
    for(;;){
      pos=ftell(f);if(!fgets(line,sizeof(line),f))break;
      if(line[0]=='@'&&line[1]=='L'&&line[2]=='|'&&level_count<MAX_LEVELS){
        level_offset[level_count]=pos;
        level_file_index[level_count]=(unsigned char)fi;
        level_count++;
      }
    }
    fclose(f);
  }
  return level_count>0;
}

static int collect_piece(int sx,int sy)
{
  int i,n=0;unsigned short id;
  if(!inside(sx,sy))return 0;
  id=piece_id[at(sx,sy)];if(!id)return 0;
  for(i=0;i<board_w*board_h;i++)if(piece_id[i]==id){gx[n]=(short)(i%board_w);gy[n]=(short)(i/board_w);n++;}
  return n;
}

static int group_has_xy(int n,int x,int y)
{int i;for(i=0;i<n;i++)if(gx[i]==x&&gy[i]==y)return 1;return 0;}

static int can_shift_piece(int n,int dx,int dy)
{
  int i,nx,ny,ni;unsigned short id;
  if(n<=0)return 0;id=piece_id[at(gx[0],gy[0])];
  for(i=0;i<n;i++){
    nx=gx[i]+dx;ny=gy[i]+dy;if(!inside(nx,ny))return 0;ni=at(nx,ny);
    if(piece_id[ni]==id)continue;
    if(board[ni]!='.')return 0;
  }
  return 1;
}

static void shift_piece(int n,int dx,int dy)
{
  int i,selection=0;unsigned short id;unsigned char c;
  if(n<=0)return;id=piece_id[at(gx[0],gy[0])];c=board[at(gx[0],gy[0])];
  if(sel_x>=0&&piece_id[at(sel_x,sel_y)]==id)selection=1;
  for(i=0;i<n;i++){board[at(gx[i],gy[i])]='.';piece_id[at(gx[i],gy[i])]=0;}
  for(i=0;i<n;i++){board[at(gx[i]+dx,gy[i]+dy)]=c;piece_id[at(gx[i]+dx,gy[i]+dy)]=id;}
  if(selection){sel_x+=dx;sel_y+=dy;}
}

static int merge_ids(unsigned short keep,unsigned short lose)
{
  int i;if(!keep||!lose||keep==lose)return 0;
  if(piece_colour[keep]!=piece_colour[lose]||!jelly_cell(piece_colour[keep]))return 0;
  for(i=0;i<board_w*board_h;i++)if(piece_id[i]==lose)piece_id[i]=keep;
  if(piece_fixed[lose])piece_fixed[keep]=1;
  piece_colour[lose]=0;piece_fixed[lose]=0;return 1;
}

static int merge_touching(void)
{
  int changed=0,x,y,i,j;unsigned short a,b;
  do{
    changed=0;
    for(y=0;y<board_h;y++)for(x=0;x<board_w;x++){
      i=at(x,y);if(!jelly_cell(board[i])||!piece_id[i])continue;
      if(x+1<board_w){j=at(x+1,y);a=piece_id[i];b=piece_id[j];if(a!=b&&b&&board[j]==board[i]&&merge_ids(a,b)){changed=1;break;}}
      if(y+1<board_h){j=at(x,y+1);a=piece_id[i];b=piece_id[j];if(a!=b&&b&&board[j]==board[i]&&merge_ids(a,b)){changed=1;break;}}
    }
  }while(changed);
  return 0;
}

static int hidden_open_dir(int x,int y,int *dx,int *dy)
{
  static const int ddx[4]={1,-1,0,0},ddy[4]={0,0,1,-1};int d,nx,ny;
  for(d=0;d<4;d++){
    nx=x+ddx[d];ny=y+ddy[d];if(!inside(nx,ny))continue;
    if(board[at(nx,ny)]!='#'&&!hidden_cell(board[at(nx,ny)])){*dx=ddx[d];*dy=ddy[d];return 1;}
  }
  return 0;
}

static int activate_hidden_once(void)
{
  int x,y,dx,dy,n,ax,ay,ni;unsigned char hc;unsigned short id;
  for(y=0;y<board_h;y++)for(x=0;x<board_w;x++)if(hidden_cell(board[at(x,y)])){
    hc=hidden_colour(board[at(x,y)]);if(!hidden_open_dir(x,y,&dx,&dy))continue;
    ax=x+dx;ay=y+dy;if(!inside(ax,ay)||board[at(ax,ay)]!=hc)continue;
    id=piece_id[at(ax,ay)];if(!id||piece_fixed[id])continue;
    n=collect_piece(ax,ay);if(!can_shift_piece(n,dx,dy))continue;
    shift_piece(n,dx,dy);
    board[at(x,y)]='#';piece_id[at(x,y)]=0;
    board[at(ax,ay)]=hc;piece_id[at(ax,ay)]=next_piece_id;
    piece_colour[next_piece_id]=hc;piece_fixed[next_piece_id]=0;next_piece_id++;
    merge_touching();return 1;
  }
  return 0;
}

static int selected_is_movable(void)
{
  unsigned short id;if(!inside(sel_x,sel_y))return 0;id=piece_id[at(sel_x,sel_y)];return id&&!piece_fixed[id];
}

static void clear_selection_if_fixed(void)
{if(sel_x>=0&&!selected_is_movable()){sel_x=-1;sel_y=-1;}}

static void settle_gravity(void)
{
  int changed=1,x,y,n,i;unsigned short id;static unsigned char seen[MAX_CELLS+1];
  while(changed){
    changed=0;memset(seen,0,sizeof(seen));
    for(y=board_h-2;y>=0;y--)for(x=0;x<board_w;x++){
      id=piece_id[at(x,y)];if(!id||seen[id]||piece_fixed[id])continue;seen[id]=1;
      n=collect_piece(x,y);if(can_shift_piece(n,0,1)){shift_piece(n,0,1);changed=1;merge_touching();}
    }
  }
  while(activate_hidden_once()){merge_touching();}
  merge_touching();clear_selection_if_fixed();
}

static int move_selected(int dx)
{
  unsigned short id;int n;
  if(!inside(sel_x,sel_y))return 0;id=piece_id[at(sel_x,sel_y)];if(!id||piece_fixed[id])return 0;
  n=collect_piece(sel_x,sel_y);if(!n||!can_shift_piece(n,dx,0))return 0;
  shift_piece(n,dx,0);moves++;merge_touching();settle_gravity();return 1;
}

static void update_selection_mask(void)
{
  int i;unsigned short id;memset(sel_mask,0,sizeof(sel_mask));if(!inside(sel_x,sel_y))return;
  id=piece_id[at(sel_x,sel_y)];if(!id)return;for(i=0;i<board_w*board_h;i++)if(piece_id[i]==id)sel_mask[i]=1;
}

static int select_cell(int x,int y)
{
  unsigned short id;if(!inside(x,y))return 0;id=piece_id[at(x,y)];if(!id||piece_fixed[id])return 0;sel_x=x;sel_y=y;return 1;
}

static void cycle_selection(int backwards)
{
  int i,start,total=board_w*board_h,idx;start=(sel_x>=0)?at(sel_x,sel_y):(backwards?0:total-1);
  for(i=1;i<=total;i++){idx=backwards?(start-i+total)%total:(start+i)%total;if(select_cell(idx%board_w,idx/board_w))return;}sel_x=sel_y=-1;
}

static int solved(void)
{
  static const unsigned char colours[5]={'b','r','g','p','y'};int ci,i;unsigned short id=0;unsigned char c;
  for(ci=0;ci<5;ci++){
    c=colours[ci];id=0;
    for(i=0;i<board_w*board_h;i++){
      if(hidden_cell(board[i])&&hidden_colour(board[i])==c)return 0;
      if(board[i]==c){if(!id)id=piece_id[i];else if(piece_id[i]!=id)return 0;}
    }
  }
  return 1;
}

static void jelly_shapes(int install)
{
  static const unsigned char jc[6]={JELLY_SINGLE_L,JELLY_SINGLE_R,JELLY_TL,JELLY_TR,JELLY_BR,JELLY_BL};
  const unsigned char (*shapes)[32];
  const unsigned char *star;
  int i;
  shapes=acc_font_height()==14?jelly_ega_glyph:jelly_vga_glyph;
  star=acc_font_height()==14?jelly_star_ega:jelly_star_vga;
  if(install){
    if(jelly_shapes_active)return;
    for(i=0;i<6;i++){acc_glyph_read(jc[i],old_jelly_glyph[i]);acc_glyph_write(jc[i],shapes[i]);}
    acc_glyph_read(JELLY_STAR,old_star_glyph);acc_glyph_write(JELLY_STAR,star);
    jelly_shapes_active=1;
  }else if(jelly_shapes_active){
    for(i=0;i<6;i++)acc_glyph_write(jc[i],old_jelly_glyph[i]);
    acc_glyph_write(JELLY_STAR,old_star_glyph);
    jelly_shapes_active=0;
  }
}

static void jelly_font(int install)
{
  unsigned char blank[32];
  if(install){
    if(jelly_font_active)return;
    acc_glyph_read(WALL_L,old_wall[0]);acc_glyph_read(WALL_R,old_wall[1]);
    acc_glyph_library(42,WALL_L);acc_glyph_library(43,WALL_R);
    acc_glyph_read(FIELD_A,old_field[0]);
    acc_glyph_library(88,FIELD_A);
    jelly_font_active=1;
  }else if(jelly_font_active){
    jelly_shapes(0);
    acc_glyph_write(WALL_L,old_wall[0]);acc_glyph_write(WALL_R,old_wall[1]);
    acc_glyph_write(FIELD_A,old_field[0]);
    jelly_font_active=0;
  }
}

static void draw_pair(int sx,int sy,int a,int b,int attr)
{acc_put(sx,sy,a,attr);acc_put(sx+1,sy,b,attr);}

static int same_piece(int x,int y,unsigned short id)
{return inside(x,y)&&piece_id[at(x,y)]==id;}

static void draw_jelly_shape_cell(int sx,int sy,int x,int y,unsigned char c,int bright,int fixed)
{
  unsigned short id=piece_id[at(x,y)];
  int up=same_piece(x,y-1,id),down=same_piece(x,y+1,id),left=same_piece(x-1,y,id),right=same_piece(x+1,y,id);
  int fg=colour_fg(c,bright),solid=ACC_ATTR(fg,0),edge=ACC_ATTR(0,fg),lc,rc;
  if(fixed){draw_pair(sx,sy,' ',' ',solid);return;}
  if(!up&&!down){lc=left?' ':JELLY_SINGLE_L;rc=right?' ':JELLY_SINGLE_R;acc_put(sx,sy,lc,left?solid:edge);acc_put(sx+1,sy,rc,right?solid:edge);}
  else if(!up){lc=left?' ':JELLY_TL;rc=right?' ':JELLY_TR;acc_put(sx,sy,lc,left?solid:edge);acc_put(sx+1,sy,rc,right?solid:edge);}
  else if(!down){lc=left?' ':JELLY_BL;rc=right?' ':JELLY_BR;acc_put(sx,sy,lc,left?solid:edge);acc_put(sx+1,sy,rc,right?solid:edge);}
  else draw_pair(sx,sy,' ',' ',solid);
}

static void draw_hidden_wall_cell(int sx,int sy,int x,int y,unsigned char c)
{
  int dx=0,dy=0,low=colour_fg(hidden_colour(c),0),hi=low+8;
  draw_pair(sx,sy,WALL_L,WALL_R,ACC_ATTR(4,12));
  if(hidden_open_dir(x,y,&dx,&dy)){
    if(dx>0)acc_put(sx+1,sy,WALL_R,ACC_ATTR(low,hi));
    else if(dx<0)acc_put(sx,sy,WALL_L,ACC_ATTR(low,hi));
    else {acc_put(sx,sy,WALL_L,ACC_ATTR(low,hi));acc_put(sx+1,sy,WALL_R,ACC_ATTR(low,hi));}
  }
}

static void make_stars(void)
{
  int placed=0,tries=0,x,y,i;
  memset(star_mask,0,sizeof(star_mask));
  while(placed<STAR_COUNT&&tries<800){
    ++tries;x=rand()%board_w;y=rand()%board_h;i=at(x,y);
    if(board[i]=='.'&&!star_mask[i]){star_mask[i]=(unsigned char)(1+(rand()&1));placed++;}
  }
}

static void draw_board(int bx,int by)
{
  int x,y,sx,i,attr;int fy=by-(14-board_h)/2;int dlgx=bx-(DLG_W-board_w*2)/2;
  int field_attr=ACC_ATTR(acc_appearance.background,(acc_appearance.background&7)|8);unsigned char c;unsigned short id;
  update_selection_mask();
  for(y=0;y<14;y++)for(x=0;x<(DLG_W-2)/2;x++)draw_pair(dlgx+1+x*2,fy+y,FIELD_A,FIELD_B,field_attr);
  for(y=0;y<board_h;y++)for(x=0;x<board_w;x++){
    i=at(x,y);c=board[i];sx=bx+x*2;id=piece_id[i];
    if(c=='#')draw_pair(sx,by+y,WALL_L,WALL_R,ACC_ATTR(4,12));
    else if(hidden_cell(c))draw_hidden_wall_cell(sx,by+y,x,y,c);
    else if(support_cell(c)){attr=ACC_ATTR(sel_mask[i]?15:8,0);draw_pair(sx,by+y,' ',' ',attr);}
    else if(jelly_cell(c))draw_jelly_shape_cell(sx,by+y,x,y,c,sel_mask[i]?1:0,id?piece_fixed[id]:0);
    else if(star_mask[i]){if(star_mask[i]==1){acc_put(sx,by+y,JELLY_STAR,ACC_ATTR(0,8));acc_put(sx+1,by+y,' ',ACC_ATTR(0,8));}else{acc_put(sx,by+y,' ',ACC_ATTR(0,8));acc_put(sx+1,by+y,JELLY_STAR,ACC_ATTR(0,8));}}
    else draw_pair(sx,by+y,' ',' ',ACC_ATTR(0,7));
  }
}

static void init_pieces(void)
{
  int x,y,i,nx,ny,head,tail,d;unsigned char c,col;unsigned short id;static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
  memset(piece_id,0,sizeof(piece_id));memset(piece_fixed,0,sizeof(piece_fixed));memset(piece_colour,0,sizeof(piece_colour));next_piece_id=1;
  for(i=0;i<board_w*board_h;i++)if(board[i]=='w')board[i]='k';
  for(y=0;y<board_h;y++)for(x=0;x<board_w;x++){
    i=at(x,y);if(piece_id[i])continue;c=board[i];
    if(!(jelly_symbol(c)||c=='k'))continue;
    col=jelly_colour(c);id=next_piece_id++;head=tail=0;qx[tail]=(short)x;qy[tail]=(short)y;tail++;piece_id[i]=id;
    if(c>='A'&&c<='Z')piece_fixed[id]=1;piece_colour[id]=col?col:'k';
    while(head<tail){int px=qx[head],py=qy[head];head++;for(d=0;d<4;d++){nx=px+dx[d];ny=py+dy[d];if(!inside(nx,ny))continue;i=at(nx,ny);if(piece_id[i])continue;c=board[i];if(piece_colour[id]=='k'){if(c!='k')continue;}else if(jelly_colour(c)!=piece_colour[id])continue;piece_id[i]=id;if(c>='A'&&c<='Z')piece_fixed[id]=1;qx[tail]=(short)nx;qy[tail]=(short)ny;tail++;}}
  }
  for(i=0;i<board_w*board_h;i++)if(jelly_symbol(board[i]))board[i]=jelly_colour(board[i]);
  merge_touching();
}

static int load_level(int n)
{
  FILE *f;char path[ACC_PATH],line[96],row[MAX_W+8],*p,*q;int w,h,x,y;
  if(n<0||n>=level_count)return 0;jelly_level_path(path,level_file_index[n]);f=fopen(path,"rb");if(!f)return 0;
  if(fseek(f,level_offset[n],SEEK_SET)!=0||!fgets(line,sizeof(line),f)){fclose(f);return 0;}strip_eol(line);p=line+3;q=strchr(p,'|');if(!q){fclose(f);return 0;}*q=0;w=atoi(p);p=q+1;q=strchr(p,'|');if(!q){fclose(f);return 0;}*q=0;h=atoi(p);p=q+1;strncpy(level_title,p,sizeof(level_title)-1);level_title[sizeof(level_title)-1]=0;
  if(w<4||w>MAX_W||h<4||h>MAX_H){fclose(f);return 0;}board_w=w;board_h=h;memset(board,'.',sizeof(board));
  for(y=0;y<h;y++){if(!fgets(row,sizeof(row),f)){fclose(f);return 0;}strip_eol(row);if((int)strlen(row)<w){fclose(f);return 0;}for(x=0;x<w;x++){unsigned char c=(unsigned char)row[x];if(c==' ')c='.';if(c!='.'&&c!='#'&&c!='k'&&c!='w'&&!jelly_symbol(c)&&!hidden_cell(c)){fclose(f);return 0;}board[at(x,y)]=c;}}
  fclose(f);moves=0;sel_x=sel_y=-1;init_pieces();settle_gravity();make_stars();return 1;
}


static void save_level(void)
{
  char path[ACC_PATH];FILE *f;unsigned short n=(unsigned short)current_level;
  acc_path(path,"DATA","JELLY.DAT");f=fopen(path,"wb");if(f){fwrite(&n,sizeof(n),1,f);fclose(f);}
}

static void load_saved_level(void)
{
  char path[ACC_PATH];FILE *f;unsigned short n=0;
  current_level=0;acc_path(path,"DATA","JELLY.DAT");f=fopen(path,"rb");
  if(f){if(fread(&n,sizeof(n),1,f)==1&&n<(unsigned)level_count)current_level=(int)n;fclose(f);}
}

static void change_level(int delta)
{
  current_level+=delta;if(current_level<0)current_level=level_count-1;if(current_level>=level_count)current_level=0;
  load_level(current_level);save_level();
}

static void draw_status(int x,int y)
{
  char s[48],field[16];
  int len,start,i;
  sprintf(s,"Puzzle %03d of %d",current_level+1,level_count);
  acc_text(x+3,y+2,s,ACC_HEADING,20);

  sprintf(s,"Moves %d",moves);
  len=(int)strlen(s);if(len>11)len=11;
  for(i=0;i<11;i++)field[i]=' ';
  start=11-len;memcpy(field+start,s,len);field[11]=0;
  acc_text(x+DLG_W-14,y+2,field,ACC_LABEL,11);
}

static int goto_dialog(void)
{
  int w=38,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=0,focus=0,v,result=-1;
  unsigned mb=0;char s[5];s[0]=0;
  acc_modal_begin();acc_mouse_display(1);
  for(;;){
    acc_subbox(x,y,w,h,"Go To",1);acc_text(x+3,y+2,"Level number:",ACC_LABEL,13);
    acc_fill(x+17,y+2,5,1,' ',focus==0?ACC_SELECT:ACC_CONTROL);acc_text(x+17,y+2,s,focus==0?ACC_SELECT:ACC_CONTROL,4);
    acc_button(x+3,y+6,"  Go  ",focus==1);acc_button(x+11,y+6,"  Cancel  ",focus==2);
    acc_wait(&k,&mx,&my,&mb);
    if((mb&1)&&my==y+2&&mx>=x+17&&mx<x+22){focus=0;k=0;continue;}
    if((mb&1)&&my==y+6){if(mx>=x+3&&mx<x+9){focus=1;k=13;}else if(mx>=x+11&&mx<x+21){focus=2;k=13;}}
    if(k==27){result=-1;break;}
    if(k==9||k==271){focus=(k==271)?(focus+2)%3:(focus+1)%3;k=0;continue;}
    if(k==13&&focus==2){result=-1;break;}
    if(k==13&&focus==1){v=atoi(s);if(v>=1&&v<=level_count){result=v-1;break;}k=0;continue;}
    if(focus==0){if(k==8&&pos){s[--pos]=0;}else if(k>='0'&&k<='9'&&pos<3){s[pos++]=(char)k;s[pos]=0;}}k=0;
  }
  acc_modal_end();acc_mouse_display(1);return result;
}

int main(int argc,char **argv)
{
  int x,y,bx,by,key=0,mx=0,my=0,full=1,focus=0,tx,ty,g;
  unsigned mb=0;
  char msg[96];
  if(acc_help(argc,argv,"!JELLOH","A gravity block puzzle: merge every jelly of each colour together."))return 0;
  if(!acc_begin(argv[0],"Jell-Oh",0))return 1;
  srand((unsigned)time(NULL));
  jelly_font(1);jelly_shapes(1);
  if(!scan_levels()){
    acc_notice("Jelly Error","No valid JELLOH*.LVL level files were found.");
    jelly_shapes(0);jelly_font(0);acc_end_screen();acc_end();return 1;
  }
  load_saved_level();if(!load_level(current_level)){current_level=0;if(!load_level(0)){jelly_shapes(0);jelly_font(0);acc_end();return 1;}}
  x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;acc_box(x,y,DLG_W,DLG_H,"Jell-Oh");

  while(key!=27){
    bx=x+(DLG_W-board_w*2)/2;by=y+3+(14-board_h)/2;
    if(full){
      acc_fill(x+1,y+1,DLG_W-2,DLG_H-2,' ',ACC_BG);draw_status(x,y);draw_board(bx,by);
      acc_button(x+3,y+17," Retry ",focus==1);
      acc_button(x+12,y+17," Prev ",focus==2);
      acc_button(x+19,y+17," Next ",focus==3);
      acc_button(x+26,y+17,"  Go To  ",focus==4);
      acc_button(x+DLG_W-10,y+17," Exit ",focus==5);full=0;
    }
    acc_wait(&key,&mx,&my,&mb);

    if(key==256+0x3F){load_level(current_level);focus=0;full=1;key=0;} /* F5 */
    else if(key==256+0x73){change_level(-1);focus=0;full=1;key=0;} /* Ctrl+Left */
    else if(key==256+0x74){change_level(1);focus=0;full=1;key=0;} /* Ctrl+Right */
    else if(key==7){g=goto_dialog();if(g>=0){current_level=g;load_level(current_level);save_level();}acc_box(x,y,DLG_W,DLG_H,"Jell-Oh");focus=0;full=1;key=0;} /* Ctrl+G */

    if((mb&1)&&!(mb&ACC_MOUSE_MOVED)){
      if(my==y+17){
        if(mx>=x+3&&mx<x+10){load_level(current_level);focus=1;full=1;key=0;}
        else if(mx>=x+12&&mx<x+18){change_level(-1);focus=2;full=1;key=0;}
        else if(mx>=x+19&&mx<x+25){change_level(1);focus=3;full=1;key=0;}
        else if(mx>=x+26&&mx<x+36){g=goto_dialog();if(g>=0){current_level=g;load_level(current_level);save_level();}acc_box(x,y,DLG_W,DLG_H,"Jell-Oh");focus=0;full=1;key=0;}
        else if(mx>=x+DLG_W-10&&mx<x+DLG_W-4){focus=5;key=27;}
      }else if(mx>=bx&&mx<bx+board_w*2&&my>=by&&my<by+board_h){
        tx=(mx-bx)/2;ty=my-by;if(select_cell(tx,ty)){focus=0;draw_board(bx,by);}key=0;
      }
    }else if(key==9||key==271){
      /* Tab cycles movable jelly/support pieces; toolbar remains mouse/shortcut driven. */
      cycle_selection(key==271);focus=0;draw_board(bx,by);key=0;
    }else if(key==256+75){if(move_selected(-1)){draw_status(x,y);draw_board(bx,by);}key=0;}
    else if(key==256+77){if(move_selected(1)){draw_status(x,y);draw_board(bx,by);}key=0;}
    else if(key==13&&focus>0){
      if(focus==1){load_level(current_level);full=1;}
      else if(focus==2){change_level(-1);full=1;}
      else if(focus==3){change_level(1);full=1;}
      else if(focus==4){g=goto_dialog();if(g>=0){current_level=g;load_level(current_level);save_level();}acc_box(x,y,DLG_W,DLG_H,"Jell-Oh");focus=0;full=1;}
      else if(focus==5)key=27;
      if(key!=27)key=0;
    }else if(key!=27)key=0;

    if(solved()){
      sprintf(msg,"Puzzle solved in %d moves!",moves);acc_notice("Jell-Oh",msg);
      current_level++;if(current_level>=level_count)current_level=0;save_level();load_level(current_level);
      acc_box(x,y,DLG_W,DLG_H,"Jell-Oh");focus=0;full=1;key=0;
    }
  }
  save_level();jelly_shapes(0);jelly_font(0);acc_end();return 0;
}
