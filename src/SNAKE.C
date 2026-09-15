/* Launch! Snake accessory - Rattler Race inspired text-mode game. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "ACCLIB.H"

#define BW 58
#define BH 14
#define MAX_SNAKE 256
#define LEVELS 6
#define FRUITS 8

typedef struct { unsigned char x,y; } POINT;
static POINT snake[MAX_SNAKE];
static int slen,dir,nextdir,score,lives,level,fruit_left,timebar,running;
static unsigned char board[BH][BW],fruit[BH][BW];

static const char *patterns[LEVELS][BH]={
 {"                                                          ","                                                          ","              #                            #              ","              #                            #              ","              #                            #              ","              #                            #              ","              #                            #              ","              #                            #              ","              #                            #              ","              #                            #              ","              #                            #              ","                                                          ","                                                          ","                                                          "},
 {"                                                          ","      ##########                         ##########       ","                                                          ","                                                          ","                    ##################                    ","                                                          ","                                                          ","                                                          ","                    ##################                    ","                                                          ","                                                          ","      ##########                         ##########       ","                                                          ","                                                          "},
 {"                                                          ","        #        #        #        #        #             ","        #        #        #        #        #             ","        #        #        #        #        #             ","                 #                 #                      ","                                                          ","  #################                 #################     ","                                                          ","                       #                 #                ","              #        #        #        #        #       ","              #        #        #        #        #       ","              #        #        #        #        #       ","                                                          ","                                                          "},
 {"                                                          ","     ###########                            ###########   ","     #         #                            #         #   ","     #         #                            #         #   ","     #         ##############################         #   ","     #                                              #     ","     ##################          ####################     ","                      #          #                        ","     ##################          ####################     ","     #                                              #     ","     #         ##############################         #   ","     #         #                            #         #   ","     ###########                            ###########   ","                                                          "},
 {"                                                          ","       #      #      #      #      #      #               ","                                                          ","  #####   #####   #####   #####   #####   #####           ","                                                          ","       #      #      #      #      #      #               ","                                                          ","  #####   #####   #####   #####   #####   #####           ","                                                          ","       #      #      #      #      #      #               ","                                                          ","  #####   #####   #####   #####   #####   #####           ","                                                          ","                                                          "},
 {"                                                          ","   ####################################################   ","   #                                                  #   ","   #  #########  #########  #########  #########      #   ","   #  #       #  #       #  #       #  #       #      #   ","   #  #       #  #       #  #       #  #       #      #   ","   #      #####       #####       #####       #####    #  ","   #                                                  #   ","   #  #####       #####       #####       #####        #  ","   #      #       #       #       #       #            #  ","   #      #########       #########       #########     # ","   #                                                  #   ","   ####################################################   ","                                                          "}
};

static void board_load(void){int y,x;for(y=0;y<BH;y++)for(x=0;x<BW;x++)board[y][x]=(patterns[level][y][x]=='#');}
static int snake_at(int x,int y){int i;for(i=0;i<slen;i++)if((int)snake[i].x==x&&(int)snake[i].y==y)return 1;return 0;}
static void put_fruit(void){int x,y,tries=0;do{x=1+rand()%(BW-2);y=1+rand()%(BH-2);tries++;}while(tries<500&&(board[y][x]||snake_at(x,y)||fruit[y][x]));if(tries<500)fruit[y][x]=1;}
static void reset_round(void){int i;memset(fruit,0,sizeof(fruit));board_load();slen=6;for(i=0;i<slen;i++){snake[i].x=(unsigned char)(8-i);snake[i].y=(unsigned char)(BH/2);}dir=nextdir=1;fruit_left=FRUITS;timebar=100;running=0;for(i=0;i<FRUITS;i++)put_fruit();}
static int wall_char(int x,int y)
{
  int u=y>0&&board[y-1][x],d=y<BH-1&&board[y+1][x],l=x>0&&board[y][x-1],r=x<BW-1&&board[y][x+1];
  if(l&&r&&u&&d)return 197;if(l&&r&&d)return 194;if(l&&r&&u)return 193;
  if(u&&d&&r)return 195;if(u&&d&&l)return 180;if(r&&d)return 218;if(l&&d)return 191;
  if(r&&u)return 192;if(l&&u)return 217;if(l||r)return 196;if(u||d)return 179;return 254;
}
static int body_char(int i)
{
  int px,nx,py,ny,h,v;if(i==0)return 2;if(i==slen-1)return 250;
  px=(int)snake[i-1].x-(int)snake[i].x;py=(int)snake[i-1].y-(int)snake[i].y;
  nx=(int)snake[i+1].x-(int)snake[i].x;ny=(int)snake[i+1].y-(int)snake[i].y;
  h=(px!=0)||(nx!=0);v=(py!=0)||(ny!=0);if(!h)return 186;if(!v)return 205;
  if((px>0||nx>0)&&(py>0||ny>0))return 218;
  if((px<0||nx<0)&&(py>0||ny>0))return 191;
  if((px>0||nx>0)&&(py<0||ny<0))return 192;
  return 217;
}
static void draw_cell(int ox,int oy,int x,int y)
{
  int i,attr=ACC_ATTR(0,10),ch=' ';
  if(board[y][x]){ch=wall_char(x,y);attr=ACC_ATTR(0,9);}else if(fruit[y][x]){ch=3;attr=ACC_ATTR(0,12);}
  for(i=slen-1;i>=0;i--)if((int)snake[i].x==x&&(int)snake[i].y==y){ch=body_char(i);attr=ACC_ATTR(0,i?14:15);break;}
  acc_put(ox+x,oy+y,ch,attr);
}

static void draw_board(int ox,int oy){int x,y;acc_fill(ox-1,oy-1,BW+2,BH+2,' ',ACC_BG);acc_put(ox-1,oy-1,218,ACC_BORDER);acc_put(ox+BW,oy-1,191,ACC_BORDER);acc_put(ox-1,oy+BH,192,ACC_BORDER);acc_put(ox+BW,oy+BH,217,ACC_BORDER);for(x=0;x<BW;x++){acc_put(ox+x,oy-1,196,ACC_BORDER);acc_put(ox+x,oy+BH,196,ACC_BORDER);}for(y=0;y<BH;y++){acc_put(ox-1,oy+y,179,ACC_BORDER);acc_put(ox+BW,oy+y,179,ACC_BORDER);for(x=0;x<BW;x++)draw_cell(ox,oy,x,y);}}
static void status_draw(int x,int y){char s[32];int n=(timebar*28)/100;acc_text(x,y,"Time",ACC_LABEL,4);acc_fill(x+5,y,28,1,176,ACC_ATTR(acc_appearance.background,8));if(n>0)acc_fill(x+5,y,n,1,219,ACC_ATTR(acc_appearance.background,10));sprintf(s,"Lives %d",lives);acc_text(x+36,y,s,ACC_HEADING,8);sprintf(s,"Score %06d",score);acc_text(x+47,y,s,ACC_HEADING,12);sprintf(s,"Board %d/%d",level+1,LEVELS);acc_text(x,y+1,s,ACC_LABEL,12);sprintf(s,"Fruit %d",fruit_left);acc_text(x+48,y+1,s,ACC_LABEL,10);}
static void life_lost(void){lives--;if(lives<=0){acc_notice("Snake","Game over");score=0;lives=3;level=0;}reset_round();}
static int step_snake(int ox,int oy){int dx=0,dy=0,nx,ny,i,grow=0;POINT oldtail=snake[slen-1];if(nextdir+dir!=3)dir=nextdir;if(dir==0)dy=-1;else if(dir==1)dx=1;else if(dir==2)dy=1;else dx=-1;nx=snake[0].x+dx;ny=snake[0].y+dy;if(nx<0||ny<0||nx>=BW||ny>=BH||board[ny][nx]||snake_at(nx,ny)){life_lost();draw_board(ox,oy);return 0;}if(fruit[ny][nx]){fruit[ny][nx]=0;fruit_left--;score+=10;grow=1;}if(grow&&slen<MAX_SNAKE)slen++;for(i=slen-1;i>0;i--)snake[i]=snake[i-1];snake[0].x=(unsigned char)nx;snake[0].y=(unsigned char)ny;draw_cell(ox,oy,oldtail.x,oldtail.y);for(i=slen-1;i>=0&&i>slen-4;i--)draw_cell(ox,oy,snake[i].x,snake[i].y);draw_cell(ox,oy,nx,ny);if(fruit_left<=0){score+=timebar;level=(level+1)%LEVELS;acc_notice("Snake","Board complete!");reset_round();draw_board(ox,oy);}return 1;}

static void snake_buttons(int x,int y,int w,int h,int focus)
{
  acc_button(x+4,y+h-3," Restart ",focus==1);
  acc_button(x+16,y+h-3," Pattern ",focus==2);
  acc_button(x+w-11,y+h-3," Close ",focus==3);
}

int main(int argc,char **argv)
{
  int w=70,h=23,x,y,ox,oy,key=0,mx=0,my=0,focus=0,last_focus=-1,lastb=0;unsigned long last_tick,t;int tick_div=0;
  if(acc_help(argc,argv,"!SNAKE","A text-mode fruit-eating snake game inspired by Rattler Race."))return 0;
  if(!acc_begin(argv[0],"Snake",0))return 1;srand((unsigned)acc_ticks());x=(acc_cols-w)/2;y=(acc_rows-h)/2;ox=x+6;oy=y+5;score=0;lives=3;level=0;reset_round();acc_box(x,y,w,h,"Snake");draw_board(ox,oy);status_draw(x+4,y+2);last_tick=acc_ticks();
  while(key!=27){
    if(focus!=last_focus){snake_buttons(x,y,w,h,focus);last_focus=focus;}
    if(kbhit()){key=acc_key();if(key==9){focus=(focus+1)%4;key=0;}else if(key==256+72){nextdir=0;running=1;focus=0;key=0;}else if(key==256+77){nextdir=1;running=1;focus=0;key=0;}else if(key==256+80){nextdir=2;running=1;focus=0;key=0;}else if(key==256+75){nextdir=3;running=1;focus=0;key=0;}else if((key==13||key==' ')&&focus){if(focus==1){reset_round();draw_board(ox,oy);}else if(focus==2){level=(level+1)%LEVELS;reset_round();draw_board(ox,oy);}else if(focus==3)key=27;if(key!=27)key=0;}}
    if(acc_mouse_present){int b=0;acc_mouse(&mx,&my,&b);if((b&1)&&!lastb){if(my==y+h-3){if(mx>=x+4&&mx<x+13){focus=1;reset_round();draw_board(ox,oy);}else if(mx>=x+16&&mx<x+25){focus=2;level=(level+1)%LEVELS;reset_round();draw_board(ox,oy);}else if(mx>=x+w-11){key=27;}}}lastb=b&1;}
    t=acc_ticks();if(t!=last_tick){last_tick=t;tick_div++;if(running&&tick_div>=3){tick_div=0;if(timebar>0)timebar--;if(timebar==0){timebar=100;memset(fruit,0,sizeof(fruit));fruit_left=FRUITS;{int i;for(i=0;i<FRUITS;i++)put_fruit();}draw_board(ox,oy);}step_snake(ox,oy);status_draw(x+4,y+2);}}
  }
  acc_end();return 0;
}
