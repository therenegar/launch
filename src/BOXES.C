/* Launch! Boxes accessory: 100-puzzle Sokoban/box-pushing game.
   Puzzle set: Microban levels 1-100 by David W. Skinner.
   Tile artwork extracted from the user-supplied BOXES.FNT (8x16 glyphs).
   Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ACCLIB.H"

#define MAX_W 22
#define MAX_H 14
#define MAX_CELLS (MAX_W*MAX_H)
#define LEVELS 100
#define DLG_W 68
#define DLG_H 20

#define TILE_VOID 0
#define TILE_FLOOR 1
#define TILE_WALL 2
#define TILE_GOAL 3

#define GLYPH_WALL_L 199
#define GLYPH_WALL_R 200
#define GLYPH_BOX_L 201
#define GLYPH_BOX_R 224
#define GLYPH_GOAL_L 203
#define GLYPH_GOAL_R 190
#define GLYPH_MAN_L 202
#define GLYPH_MAN_R 191

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

static unsigned char old_glyph[8][32];

typedef struct {
  unsigned char w,h;
  unsigned short best;
  const char *data;
} BOX_LEVEL;

static const BOX_LEVEL level_data[LEVELS]={
  {6,7,33,"####~~# .#~~#  ####*@  ##  $ ##  #######~~"}, /* 001: Microban 1 */
  {6,7,16,"#######    ## #@ ## $* ## .* ##    #######"}, /* 002: Microban 2 */
  {9,6,41,"~~####~~~###  #####     $ ## #  #$ ## . .#@ ##########"}, /* 003: Microban 3 */
  {8,6,23,"#########      ## .**$@##      ######  #~~~~####"}, /* 004: Microban 4 */
  {8,7,25,"~#######~#     #~# .$. ### $@$ ##  .$. ##      #########"}, /* 005: Microban 5 */
  {12,6,107,"######~######    ###   ## $$     #@## $ #...   ##   #############~~~~~~~"}, /* 006: Microban 6 */
  {7,8,26,"########     ## .$. ## $.$ ## .$. ## $.$ ##  @  ########"}, /* 007: Microban 7 */
  {8,12,97,"~~######~~# ..@#~~# $$ #~~## ###~~~# #~~~~~# #~~#### #~~#    ##~# #   #~#   # #~###   #~~~#####~"}, /* 008: Microban 8 */
  {6,7,30,"#####~#.  ###@$$ ###   #~##  #~~##.#~~~###"}, /* 009: Microban 9 */
  {11,8,89,"~~~~~~#####~~~~~~#.  #~~~~~~#.# ########.# ## @ $ $ $ ## # # # ####       #~~#########~~"}, /* 010: Microban 10 */
  {9,8,78,"~~######~~~#    #~~~# ##@##### # $ ## ..# $ ##       ##  ##########~~~~~"}, /* 011: Microban 11 */
  {9,8,49,"#####~~~~#   ##~~~# $  #~~~## $ ####~###@.  #~~#  .# #~~#     #~~#######"}, /* 012: Microban 12 */
  {7,9,52,"####~~~#. ##~~#.@ #~~#. $#~~##$ ###~# $  #~#    #~#  ###~####~~"}, /* 013: Microban 13 */
  {7,6,51,"########     ## # # ##. $*@##   ########~~"}, /* 014: Microban 14 */
  {9,7,37,"~~~~~###~######@###    .* ##   #   ######$# #~~~~#   #~~~~#####"}, /* 015: Microban 15 */
  {10,8,100,"~####~~~~~~#  ####~~~#     ##~## ##   #~#. .# @$###   # $$ ##  .#    ###########"}, /* 016: Microban 16 */
  {6,7,25,"#####~# @ #~#...#~#$$$###    ##    #######"}, /* 017: Microban 17 */
  {7,9,71,"########     ##. .  ## ## ###  $ #~###$ #~~~#@ #~~~#  #~~~####~"}, /* 018: Microban 18 */
  {8,8,41,"#########   .. ##  @$$ ###### ##~~~#  #~~~~#  #~~~~#  #~~~~####~"}, /* 019: Microban 19 */
  {9,8,50,"#######~~#     ####  @$$..##### ## #~~#     #~~#  ####~~#  #~~~~~####~~~"}, /* 020: Microban 20 */
  {7,6,17,"####~~~#  ##### . . ## $$#@###    #~######"}, /* 021: Microban 21 */
  {7,9,47,"#####~~#   ####. .  ##   # ### #  #~#@$$ #~#    #~#  ###~####~~"}, /* 022: Microban 22 */
  {7,7,56,"########  *  ##     ### # ##~#$@.#~~#   #~~#####~"}, /* 023: Microban 23 */
  {7,7,35,"#~#####~~#   ####$$@##   ####     ## . . ########"}, /* 024: Microban 24 */
  {7,7,29,"~####~~~#  ###~# $$ ###... ##  @$ ##   ########~~"}, /* 025: Microban 25 */
  {6,8,41,"~#####~# @ #~#   ####$ ## ...## $$ ####  #~~####"}, /* 026: Microban 26 */
  {7,7,50,"######~#   .#~# ## ###  $$@## #   ##.  ########~~"}, /* 027: Microban 27 */
  {7,7,33,"#####~~#   #~~# @ #~~# $$#####. . #~#    #~######"}, /* 028: Microban 28 */
  {11,9,104,"~~~~~#####~~~~~~#   ##~~~~~#    #~######   ###     #. ## $ $ @  ### ######.#~#        #~##########~"}, /* 029: Microban 29 */
  {6,7,21,"####~~#  #### $$ ##... ## @$ ##   #######~"}, /* 030: Microban 30 */
  {7,7,17,"~~####~~##  #~##@$.### $$  ## . . ####   #~~#####"}, /* 031: Microban 31 */
  {7,7,35,"~####~~##  ####     ##.**$@##   #####  #~~~####~~"}, /* 032: Microban 32 */
  {7,7,41,"########. #  ##  $  ##. $#@##  $  ##. #  ########"}, /* 033: Microban 33 */
  {9,6,30,"~~####~~~###  #####       ##@$***. ##       ##########"}, /* 034: Microban 34 */
  {7,10,77,"~~####~~##  #~~#. $#~~#.$ #~~#.$ #~~#.$ #~~#. $##~#   @#~##   #~~#####"}, /* 035: Microban 35 */
  {15,5,156,"####~~~~~~~~~~~#  ############# $ $ $ $ $ @ ## .....       ################"}, /* 036: Microban 36 */
  {9,8,71,"~~~~~~########~#.##   ###.##   $ #.## $  $  ######@# #~~~~#   #~~~~#####"}, /* 037: Microban 37 */
  {10,7,37,"###########        ## ##.### ## # $$ . ## . @$## ######    #~~~~######"}, /* 038: Microban 38 */
  {10,9,85,"#####~~~~~#   ####~~# # # .#~~#    $ ###### #$.  ##   #@   ## # #######   #~~~~~#####~~~~~"}, /* 039: Microban 39 */
  {7,6,20,"~#####~~#   #~##   ### $$$ ## .+. ########"}, /* 040: Microban 40 */
  {8,6,50,"#######~#     #~#@$$$ ###  #...###    ##~######~"}, /* 041: Microban 41 */
  {7,8,47,"~~~####~~~#  #~~~#@ #####$.##   $.## # $.##    ########~"}, /* 042: Microban 42 */
  {9,9,61,"~~~~~####~~~~~# @#~~~~~#  ####### .##   $  .##  $$# .##    #######  #~~~~~####~~~"}, /* 043: Microban 43 */
  {5,3,1,"######@$.######"}, /* 044: Microban 44 */
  {6,7,45,"#######... ##  $ ## #$###  $ ##  @ #######"}, /* 045: Microban 45 */
  {7,8,47,"~########    ##  ## ## # $ ##  * .### #@##~#   #~~#####~"}, /* 046: Microban 46 */
  {11,7,83,"~~#######~~###     #~~# $ $   #~~# ### ###### @ . .   ##   ###   ######~#####"}, /* 047: Microban 47 */
  {8,8,64,"######~~#  @ #~~#  # ##~# .#  ### .$$$ ## .#   #####   #~~~#####"}, /* 048: Microban 48 */
  {8,10,82,"######~~# @  #~~# $# #~~# $  #~~# $ ##~~### ####~#  #  #~#...  #~#     #~#######"}, /* 049: Microban 49 */
  {10,7,76,"~~####~~~~###  ######  $  @..## $    # #### #### #~~#      #~~########"}, /* 050: Microban 50 */
  {8,7,34,"####~~~~#  ###~~#    ####  $*@ #### .# #~~#    #~~######"}, /* 051: Microban 51 */
  {6,8,26,"~~####### @##  $ ##  *.##  *.##  $ ####  #~~####"}, /* 052: Microban 52 */
  {7,7,37,"~#####~##. .### * * ##  #  ## $ $ ### @ ##~#####~"}, /* 053: Microban 53 */
  {12,8,82,"~~~~~~######~~~~~~#    #~~##### .  ####  ###.  ## $  $  . ### @$$ # . #~##    #####~~######~~~~~"}, /* 054: Microban 54 */
  {10,8,64,"########~~# @ #  #~~#      #~~#####$ #~~~~~~#  ###~##~#$ ..#~##~#  ###~~~~####~~"}, /* 055: Microban 55 */
  {7,6,23,"#####~~#   ####  $  ###* . #~#   @#~######"}, /* 056: Microban 56 */
  {8,9,60,"~~####~~~~#  #~~~~#@ #~~~~#  #~~### #####    * ##  $   ######. #~~~~####"}, /* 057: Microban 57 */
  {7,7,44,"####~~~#  #####.*$  ## .$# ### @  #~#   ##~#####~"}, /* 058: Microban 58 */
  {13,9,178,"############~#          #~# ####### @### #         ## #  $   #  ## $$ #####  ####  #~# ...#~~####~#    #~~~~~~~######"}, /* 059: Microban 59 */
  {10,10,169,"~#########~#       ###@##### ##  #   # ##  #   $.##  ##$##.###$##  #.##   $  #.##   #  ###########~~"}, /* 060: Microban 60 */
  {9,10,100,"########~#      #~# #### #~# #...@#~# ###$#### #     ##  $$ $ #####   ##~~~#.###~~~~###~~~"}, /* 061: Microban 61 */
  {13,6,64,"~~~##############    ##  ##  $$$....$@##      ###  ##   ####~#########~~~~~~~~"}, /* 062: Microban 62 */
  {19,6,101,"#####~~~####~~~~~~~#   ##### .#~~~~~~~#       $  ###########  #### .$    @ #~~#  #~~#  ####   #~~####~~####~~#####"}, /* 063: Microban 63 */
  {10,9,95,"~######~~~##    #~~~#   $ #~~~#  $$ #~~~### .#####~~##.# @ #~~~#.  $ #~~~#. ####~~~####~~~"}, /* 064: Microban 64 */
  {9,9,138,"~~######~~~#    #~~~#  $ #~~####$ #~## $ $ #~#....# ###     @ ###  #   #~########"}, /* 065: Microban 65 */
  {9,14,69,"~~~###~~~~~~#@#~~~~###$###~##  .  ###  # #  ## #   # ## #   # ## #   # ##  # #  ### $ $ ##~##. .##~~~#   #~~~~#   #~~~~#####~~"}, /* 066: Microban 66 */
  {7,8,37,"#####~~#   ##~# #  #~#@$*.####  . #~# $# #~##   #~~#####"}, /* 067: Microban 67 */
  {10,7,98,"~####~~~~~~#  ########    $  ## .# $   ## .#$###### .@ #~~~~######~~~~"}, /* 068: Microban 68 */
  {11,8,125,"####~~####~#  ####  #~#  #  #  #~#  #    $###  . .#$  ##@ ## # $ ##   . #   ############"}, /* 069: Microban 69 */
  {8,10,78,"#####~~~# @ #####      ## $ $$ ###$##  ##   ##### ..  #~##..  #~~###  #~~~~####~"}, /* 070: Microban 70 */
  {13,9,120,"###########~~#     #   #### $@$ # .  .## ## ### ## ## #       # ## #   #   # ## ######### ##           ##############"}, /* 071: Microban 71 */
  {10,11,105,"~~####~~~~~##  #####~#  $  @ #~#  $#   ##### ######  #   #~~#    $ #~~# ..#  #~~#  .####~~#  ##~~~~~####~~~~~~"}, /* 072: Microban 72 */
  {8,10,102,"####~~~~#  ###### $$ $ ##      ### ## ###...#@#~# ### ###      ##  #   #########"}, /* 073: Microban 73 */
  {11,8,117,"~####~~~~~~~#  #######~#$ @#   .### #$$   .##  $  ##..##   # ########   #~~~~~~#####~~~~"}, /* 074: Microban 74 */
  {10,7,92,"~#######~~## ....##~#   #######   $ $ @####  $ $ #~~###    #~~~~######"}, /* 075: Microban 75 */
  {10,11,181,"~#####~~~~##   #~~~~#    ######  #.#   ##@ #.# $ ##  #.#  ###    #  #~##  ##$$#~~##     #~~~#  ####~~~####~~~~"}, /* 076: Microban 76 */
  {11,7,189,"##########~# @ .... #~#   ####$#### #  $ $ #~# $      #~#   ######~#####~~~~~"}, /* 077: Microban 77 */
  {11,8,135,"~#######~~~##     ##~~#  $ $  #~~# $ $ $ #~~## ### ####~#@  .....#~##     ###~~#######~~"}, /* 078: Microban 78 */
  {10,6,48,"~#########~#    #  ### $#$#  ##  .$.@  ##  .#    ###########"}, /* 079: Microban 79 */
  {10,11,131,"####~~~~~~#  ########  . ## .## $#    .### ## # .#~#    #  #~#### #  #~~# @$ ###~~# $$ #~~~~#    #~~~~######~~"}, /* 080: Microban 80 */
  {6,9,46,"~#####~#   #~# . ### * ##  *###  @#### $ #~#   #~#####"}, /* 081: Microban 81 */
  {8,8,52,"#####~~~#   ###~# .   ####*#$  ## .# $ ## @## ###     #~#######~"}, /* 082: Microban 82 */
  {8,10,164,"######~~#    ##~# $ $ #### $$  #~# #   #~# ## ##~#  . .#~# @. .#~#  ####~####~~~"}, /* 083: Microban 83 */
  {12,10,201,"########~~~~#  ... #~~~~#  ### ##~~~#  # $  #~~~## #@$  #~~~~# # $  #~~~~# ### #####~#         #~#   ###   #~#####~#####"}, /* 084: Microban 84 */
  {11,11,155,"~~~~~~~####~#######  #~# $      #~#   $ $  #~# ########## # .  #~~#  # #  #~~#  @ . ##~~## # # #~~~~#   . #~~~~#######~~~"}, /* 085: Microban 85 */
  {9,8,105,"~~~~####~~~###  ##~## $   ### $  # ## @#$$  ## ..  #### ..###~~#####~~~~"}, /* 086: Microban 86 */
  {9,10,149,"~~~~~##########  ##       ##  ... .###$####### $  #~~~#   $###~##  $  #~~## @  #~~~######~"}, /* 087: Microban 87 */
  {11,12,195,"~~~~~####~~~#~###  #~~~#~#    #~~~#~#  # #~~~#~#$ #.#~~~#~#  # #~#~#~#$ #.#~#~~~#  # #~#####$ #.#~## @     #~##   #  ##~#########~~~"}, /* 088: Microban 88 */
  {10,9,146,"###########   ##   ## $  $@# ##### # $ #~~~#.#  ##~#~#.# $#~~#~#.   #~~#~#.   #~~~~######~"}, /* 089: Microban 89 */
  {10,7,64,"~########~~#  @   #~~# $  $ #~### ## ####  $..$  ##   ..   ###########"}, /* 090: Microban 90 */
  {11,5,45,"############    .##  ## $$@..$$ ##   ##.   ############"}, /* 091: Microban 91 */
  {15,10,126,"~~####~~~~~~~~~~~#  #~~~~#####~~#  #~~~~#   #~~#  ######.# #####  $    .  ##   $$# ###.# ##   #   #~#   ##########~#@ ##~~~~~~~~~~#  #~~~~~~~~~~~####~"}, /* 092: Microban 92 */
  {11,11,91,"~#########~##   #   ###    #    ##  $ # $  ##   *.*   #####.@.#####   *.*   ##  $ # $  ##    #    ###   #   ##~#########~"}, /* 093: Microban 93 */
  {9,8,83,"########## @ #   ## $ $   ###$### ###  ...  ##   #   #######  #~~~~~####"}, /* 094: Microban 94 */
  {8,8,25,"#########@     ## .$$. ## $..$ ## $..$ ## .$$. ##      #########"}, /* 095: Microban 95 */
  {11,11,92,"~~######~~~~~#    #~~~~~#    #~~~#####  #~~~#   #.######   $@$   ######.#   #~~~## ## ##~~~#   $.#~~~~#   ###~~~~#####~~~"}, /* 096: Microban 96 */
  {14,10,164,"~~~####~~~~~~~~~~#  ############ $ $.....##   $   #######@### ###~~~~~#  $  #~~~~~~~# $ # #~~~~~~~## #  #~~~~~~~~#    #~~~~~~~~######~~~~~~~"}, /* 097: Microban 97 */
  {16,10,269,"#####~~~~~~~~~~~#   ##~####~~~~~#  $ ### .#~~~~~# $   $  .#~~~~~## $#####.#~##### $  #~# .###  ##    #~# .#  @ ####  #~#       #~~####~##     ##~~~~~~~~#######~"}, /* 098: Microban 98 */
  {22,10,349,"~~~~~~~~~~~~~~~#####~~~~~~~~~~~~~~~~~#   #~~#######~~####### # #~~#     #~~#  #      #~~#  @  ####  #     #####  #    ....## ####  ##    ##### ## $$ $ $ #######~~~#           #~~~~~~~~~#  ##########~~~~~~~~~####~~~~~~~~~"}, /* 099: Microban 99 */
  {8,8,155,"#######~# @#  #~#.$   #~#. # $###.$#   ##. # $ ##  #   #########"}, /* 100: Microban 100 */
};

static unsigned char tile[MAX_CELLS],box_at[MAX_CELLS];
static int board_w,board_h,player_x,player_y,current_level,moves,pushes;

static int inside(int x,int y){return x>=0&&x<board_w&&y>=0&&y<board_h;}
static int at(int x,int y){return y*board_w+x;}

static void boxes_font(int install)
{
  int i;
  for(i=0;i<8;i++){
    if(install)acc_glyph_read(glyph_code[i],old_glyph[i]);
    acc_glyph_write(glyph_code[i],install?boxes_glyph[i]:old_glyph[i]);
  }
}

static void load_level(int n)
{
  const char *p=level_data[n].data;
  int i,x,y,cells;
  board_w=level_data[n].w;board_h=level_data[n].h;cells=board_w*board_h;
  memset(tile,TILE_VOID,sizeof(tile));
  memset(box_at,0,sizeof(box_at));
  player_x=player_y=0;
  for(i=0;i<cells;i++){
    x=i%board_w;y=i/board_w;
    switch(p[i]){
      case '~':tile[i]=TILE_VOID;break;
      case '#':tile[i]=TILE_WALL;break;
      case ' ':tile[i]=TILE_FLOOR;break;
      case '.':tile[i]=TILE_GOAL;break;
      case '$':tile[i]=TILE_FLOOR;box_at[i]=1;break;
      case '*':tile[i]=TILE_GOAL;box_at[i]=1;break;
      case '@':tile[i]=TILE_FLOOR;player_x=x;player_y=y;break;
      case '+':tile[i]=TILE_GOAL;player_x=x;player_y=y;break;
      default:tile[i]=TILE_VOID;break;
    }
  }
  moves=pushes=0;
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
  if(tile[i]==TILE_VOID)draw_pair(bx+x*2,by+y,' ',' ',ACC_BG);
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
  int x,y;
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
  unsigned char n=(unsigned char)current_level;
  acc_path(p,"DATA","BOXES.DAT");
  f=fopen(p,"wb");
  if(f){fwrite(&n,1,1,f);fclose(f);}
}

static void load_saved_level(void)
{
  char p[ACC_PATH];
  FILE *f;
  unsigned char n=0;
  current_level=0;
  acc_path(p,"DATA","BOXES.DAT");
  f=fopen(p,"rb");
  if(f){if(fread(&n,1,1,f)==1&&n<LEVELS)current_level=n;fclose(f);}
}

static void change_level(int delta)
{
  current_level+=delta;
  if(current_level<0)current_level=LEVELS-1;
  if(current_level>=LEVELS)current_level=0;
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
  sprintf(s,"Puzzle %03d of 100",current_level+1);
  acc_text(x+3,y+2,s,ACC_HEADING,20);
  draw_counts(x,y);
}

int main(int argc,char **argv)
{
  int x,y,bx,by,key=0,mx=0,my=0,dirty=1,full=1,focus=0;
  int tx,ty,won_moves,won_best;
  unsigned mb=0;
  char msg[96];
  if(acc_help(argc,argv,"!BOXES","A 100-puzzle box-pushing game using custom text-mode tile glyphs."))return 0;
  if(!acc_begin(argv[0],"Boxes",0))return 1;
  boxes_font(1);
  x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;
  acc_box(x,y,DLG_W,DLG_H,"Boxes");
  load_saved_level();
  load_level(current_level);

  while(key!=27){
    bx=x+(DLG_W-board_w*2)/2;
    by=y+3+(14-board_h)/2;
    if(full){
      acc_fill(x+1,y+1,DLG_W-2,DLG_H-2,' ',ACC_BG);
      draw_status(x,y);
      render_board(bx,by);
      acc_button(x+3,y+17," Retry ",focus==1);
      acc_button(x+13,y+17," < Prev ",focus==2);
      acc_button(x+23,y+17," Next > ",focus==3);
      acc_button(x+DLG_W-11,y+17," Close ",focus==4);
      full=0;dirty=0;
    }else if(dirty){
      render_changed(bx,by);
      draw_counts(x,y);
      dirty=0;
    }
    acc_wait(&key,&mx,&my,&mb);

    if((mb&1)&&!(mb&ACC_MOUSE_MOVED)){
      if(my==y+17){
        if(mx>=x+3&&mx<x+10){focus=1;load_level(current_level);full=1;key=0;}
        else if(mx>=x+13&&mx<x+21){focus=2;change_level(-1);full=1;key=0;}
        else if(mx>=x+23&&mx<x+31){focus=3;change_level(1);full=1;key=0;}
        else if(mx>=x+DLG_W-11&&mx<x+DLG_W-4){focus=4;key=27;}
      }else if(mx>=bx&&mx<bx+board_w*2&&my>=by&&my<by+board_h){
        focus=0;tx=(mx-bx)/2;ty=my-by;
        if(walk_to(tx,ty))dirty=1;
        key=0;
      }
    }else if(key==9){
      focus=(focus+1)%5;
      acc_button(x+3,y+17," Retry ",focus==1);
      acc_button(x+13,y+17," < Prev ",focus==2);
      acc_button(x+23,y+17," Next > ",focus==3);
      acc_button(x+DLG_W-11,y+17," Close ",focus==4);
      key=0;
    }else if((key==13||key==' ')&&focus){
      if(focus==1){load_level(current_level);full=1;}
      else if(focus==2){change_level(-1);full=1;}
      else if(focus==3){change_level(1);full=1;}
      else if(focus==4)key=27;
      if(key!=27)key=0;
    }else if(key==256+72&&focus==0){dirty=move_player(0,-1);key=0;}
    else if(key==256+80&&focus==0){dirty=move_player(0,1);key=0;}
    else if(key==256+75&&focus==0){dirty=move_player(-1,0);key=0;}
    else if(key==256+77&&focus==0){dirty=move_player(1,0);key=0;}
    else if(key=='r'||key=='R'){focus=0;load_level(current_level);full=1;key=0;}
    else if(key==256+73){focus=0;change_level(-1);full=1;key=0;}
    else if(key==256+81){focus=0;change_level(1);full=1;key=0;}
    else if(key!=27)key=0;

    if((dirty||full)&&solved()){
      won_moves=moves;won_best=level_data[current_level].best;
      if(dirty){render_changed(bx,by);draw_counts(x,y);dirty=0;}
      sprintf(msg,"Puzzle solved!\nYour moves %d\nBest %d",won_moves,won_best);
      acc_notice("Boxes",msg);
      if(current_level<LEVELS-1)current_level++;
      else current_level=0;
      save_level();
      load_level(current_level);
      focus=0;
      full=1;
    }
  }
  save_level();
  boxes_font(0);
  acc_end();
  return 0;
}
