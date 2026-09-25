/* Launch! FreeCell accessory - Release 3.5 development. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ACCLIB.H"
#define CARDS 52
#define COLS 8
#define BOARD_BG 2
#define BOARD_FG 10
#define CARD_BG 7
#define MAXCOL 52
typedef struct{unsigned char rank,suit;} CARD;
static CARD card[CARDS];
static int tab[COLS][MAXCOL],tn[COLS],freec[4],foundation[4];
static int cursor[COLS],selected,stype,scol,sidx,score,reveal[COLS];
static unsigned long game_start_tick=0;
static unsigned game_last_sec=0xFFFFU;
static int game_timer_running=0;
static int game_timer_base_x=0,game_timer_y=0,game_timer_col=0;
static unsigned game_elapsed_seconds(void){unsigned long now,ticks,sec;if(!game_timer_running)return 0;now=acc_ticks();if(now>=game_start_tick)ticks=now-game_start_tick;else ticks=now+0x1800B0UL-game_start_tick;sec=ticks/18UL;if(sec>5999UL)sec=5999UL;return(unsigned)sec;}
static void game_timer_start(void){if(!game_timer_running){game_timer_running=1;game_start_tick=acc_ticks();game_last_sec=0xFFFFU;}}
static void game_timer_tick(void){unsigned sec=game_elapsed_seconds();char s[12];if(sec==game_last_sec)return;game_last_sec=sec;sprintf(s,"Time %02u:%02u",sec/60U,sec%60U);acc_text(game_timer_col,game_timer_y,s,ACC_HEADING,10);}
static void game_draw_status(void){char ss[24],ts[12];unsigned sec=game_elapsed_seconds();acc_fill(game_timer_base_x,game_timer_y,40,1,' ',ACC_BG);sprintf(ss,"Score %d",score);acc_text(game_timer_base_x,game_timer_y,ss,ACC_HEADING,(int)strlen(ss));game_timer_col=game_timer_base_x+(int)strlen(ss)+3;sprintf(ts,"Time %02u:%02u",sec/60U,sec%60U);acc_text(game_timer_col,game_timer_y,ts,ACC_HEADING,10);game_last_sec=sec;}
static unsigned seedadd;
#define CARD_A 128
#define CARD_K 129
#define CARD_Q 130
#define CARD_J 131
#define CARD_HEART 132
#define CARD_DIAMOND 133
#define CARD_CLUB 134
#define CARD_SPADE 135
static const unsigned char suit_glyph[4]={CARD_HEART,CARD_DIAMOND,CARD_CLUB,CARD_SPADE};
static unsigned char card_old_glyph[8][32];
static int card_font_installed=0;
static void card_font(int install)
{
  static const unsigned char code[8]={CARD_A,CARD_K,CARD_Q,CARD_J,CARD_HEART,CARD_DIAMOND,CARD_CLUB,CARD_SPADE};
  static const unsigned char logical[8]={96,97,98,99,100,101,102,103};
  int i;if(install){if(card_font_installed)return;for(i=0;i<8;i++){acc_glyph_read(code[i],card_old_glyph[i]);acc_glyph_library(logical[i],code[i]);}card_font_installed=1;}
  else{if(!card_font_installed)return;for(i=0;i<8;i++)acc_glyph_write(code[i],card_old_glyph[i]);card_font_installed=0;}
}
static int card_rank_glyph(int rank){if(rank==1)return CARD_A;if(rank==11)return CARD_J;if(rank==12)return CARD_Q;if(rank==13)return CARD_K;return 0;}
static int red(int s){return s<2;}
static void rank_chars(int rank,int suit,char *s){int g;s[0]='|';if(rank==10){s[1]='1';s[2]='0';}else if((g=card_rank_glyph(rank))!=0){s[1]=(char)g;s[2]=' ';}else{s[1]=(char)('0'+rank);s[2]=' ';}s[3]=(char)suit_glyph[suit];s[4]='|';s[5]=0;}
static void draw_face(int x,int y,int id,int mark){char s[6];int a=ACC_ATTR(CARD_BG,red(card[id].suit)?12:0);rank_chars(card[id].rank,card[id].suit,s);if(mark){s[0]='[';s[4]=']';}acc_text(x,y,s,a,5);}
static void draw_empty(int x,int y,int mark){acc_text(x,y,mark?"[   ]":"|   |",mark?ACC_SELECT:ACC_BORDER,5);}
static void card_selected(int x,int y){int a=ACC_ATTR(BOARD_BG,15);acc_put(x-1,y,16,a);acc_put(x+5,y,17,a);}
static void card_shadow(int x,int y,int background){int i;for(i=1;i<5;i++)acc_put(x+i,y+1,220,ACC_ATTR(background,0));acc_put(x+5,y,245,ACC_ATTR(background,0));acc_put(x+5,y+1,244,ACC_ATTR(background,0));}
static void new_game(void){int o[CARDS],i,j,c,t;for(i=0;i<CARDS;i++){card[i].rank=(unsigned char)(i%13+1);card[i].suit=(unsigned char)(i/13);o[i]=i;}seedadd+=0x9e37U;srand((unsigned)acc_ticks()^seedadd);for(i=51;i>0;i--){j=rand()%(i+1);t=o[i];o[i]=o[j];o[j]=t;}memset(tn,0,sizeof(tn));for(i=0;i<4;i++){freec[i]=-1;foundation[i]=0;}for(i=0;i<CARDS;i++){c=i%COLS;tab[c][tn[c]++]=o[i];}for(c=0;c<COLS;c++){cursor[c]=tn[c]-1;reveal[c]=0;}selected=stype=scol=sidx=0;score=0;game_timer_running=0;game_start_tick=0;game_last_sec=0xFFFFU;}
static int seq_ok(int c,int idx){int i,a,b;if(idx<0||idx>=tn[c])return 0;for(i=idx;i<tn[c]-1;i++){a=tab[c][i];b=tab[c][i+1];if((int)card[a].rank!=(int)card[b].rank+1||red((int)card[a].suit)==red((int)card[b].suit))return 0;}return 1;}
static int free_slots(void){int i,n=0;for(i=0;i<4;i++)if(freec[i]<0)n++;return n;}
static int empty_cols(int except){int i,n=0;for(i=0;i<COLS;i++)if(i!=except&&!tn[i])n++;return n;}
static int movable_count(int dest){int n=free_slots()+1,e=empty_cols(dest);while(e-->0&&n<32)n*=2;return n;}
static int source_id(void){if(!selected)return-1;if(stype==1)return freec[scol];if(stype==2&&sidx<tn[scol])return tab[scol][sidx];if(stype==3&&foundation[scol])return scol*13+foundation[scol]-1;return-1;}
static int source_count(void){return stype==2?tn[scol]-sidx:1;}
static void remove_source(void){if(stype==1)freec[scol]=-1;else if(stype==2){tn[scol]=sidx;if(tn[scol])cursor[scol]=tn[scol]-1;}else if(stype==3){foundation[scol]--;if(score>=10)score-=10;}selected=0;}
static int can_tableau(int id,int d){int top;if(!tn[d])return 1;top=tab[d][tn[d]-1];return (int)card[top].rank==(int)card[id].rank+1&&red((int)card[top].suit)!=red((int)card[id].suit);}
static int move_tableau(int d){int id=source_id(),n,i;if(id<0||(stype==2&&scol==d)||!can_tableau(id,d))return 0;n=source_count();if(stype==2){if(!seq_ok(scol,sidx)||n>movable_count(d))return 0;for(i=sidx;i<tn[scol];i++)tab[d][tn[d]++]=tab[scol][i];}else tab[d][tn[d]++]=id;remove_source();cursor[d]=tn[d]-1;return 1;}
static int move_free(int d){int id=source_id();if(id<0||freec[d]>=0||source_count()!=1||stype==1)return 0;freec[d]=id;remove_source();return 1;}
static int move_found(int s){int id=source_id();if(id<0||source_count()!=1||stype==3||(int)card[id].suit!=s||(int)card[id].rank!=foundation[s]+1)return 0;foundation[s]++;score+=10;remove_source();return 1;}
static int won(void){return foundation[0]==13&&foundation[1]==13&&foundation[2]==13&&foundation[3]==13;}
static void destination_mark(int x,int y,int focused){int a=ACC_ATTR(2,15);acc_text(x,y,"|   |",a,5);if(focused)card_selected(x,y);}
static int valid_free_dest(int d){return selected&&freec[d]<0&&source_count()==1&&stype!=1;}
static int valid_found_dest(int s){int id=source_id();return id>=0&&source_count()==1&&stype!=3&&(int)card[id].suit==s&&(int)card[id].rank==foundation[s]+1;}
static int valid_tableau_dest(int d){int id=source_id(),n;if(id<0||(stype==2&&scol==d)||!can_tableau(id,d))return 0;n=source_count();if(stype==2&&(!seq_ok(scol,sidx)||n>movable_count(d)))return 0;return 1;}
static int view_start(int c){if(tn[c]<=11)return 0;return reveal[c]?0:tn[c]-10;}
static int view_end(int c){if(tn[c]<=11)return tn[c];return reveal[c]?10:tn[c];}
static int row_index(int c,int row){int st=view_start(c),en=view_end(c),n=en-st,off=0;if(tn[c]>11){if(!reveal[c]){if(row==0)return-2;off=1;}else if(row==10)return-2;}if(row==off+n)return-3;if(row<off||row>=off+n)return-1;return st+(row-off);}
static void hidden_mark(int x,int y){acc_text(x,y,"|...|",ACC_ATTR(BOARD_BG,1),5);}
static void render(int x,int y,int focus,int full,int kbd_nav,int kbd_dest){int i,c,r,id,st,en,row,sy;char q[6],ss[24];/* Clear transient focus-arrow cells only; avoid a full playfield redraw. */
for(i=0;i<4;i++){acc_put(x+2+i*8,y+2,' ',ACC_BG);acc_put(x+8+i*8,y+2,' ',ACC_BG);acc_put(x+42+i*8,y+2,' ',ACC_BG);acc_put(x+48+i*8,y+2,' ',ACC_BG);}
for(c=0;c<COLS;c++){
  /* Rebuild the complete tableau footprint from game state on every dirty
     render.  ACCLIB suppresses unchanged video writes, so this is still
     flicker-free, but vacated card cells cannot survive as visual ghosts. */
  acc_fill(x+4+c*9,y+5,7,12,178,ACC_ATTR(BOARD_BG,BOARD_FG));
}if(full){acc_fill(x+1,y+1,75,19,' ',ACC_BG);acc_fill(x+1,y+4,75,14,178,ACC_ATTR(BOARD_BG,BOARD_FG));}for(i=0;i<4;i++){if(freec[i]>=0)draw_face(x+3+i*8,y+2,freec[i],0);else{draw_empty(x+3+i*8,y+2,0);acc_put(x+5+i*8,y+2,'o',ACC_BORDER);}if(kbd_nav&&focus==i&&!selected)card_selected(x+3+i*8,y+2);if(selected&&stype==1&&scol==i){card_selected(x+3+i*8,y+2);if(kbd_dest)card_shadow(x+3+i*8,y+2,BOARD_BG);}if(selected&&valid_free_dest(i))destination_mark(x+3+i*8,y+2,focus==i);}for(i=0;i<4;i++){if(foundation[i]){rank_chars(foundation[i],i,q);acc_text(x+43+i*8,y+2,q,ACC_ATTR(CARD_BG,red(i)?12:0),5);}else{q[0]='|';q[1]=' ';q[2]=(char)suit_glyph[i];q[3]=' ';q[4]='|';q[5]=0;acc_text(x+43+i*8,y+2,q,ACC_BORDER,5);}if(kbd_nav&&focus==4+i&&!selected)card_selected(x+43+i*8,y+2);if(selected&&stype==3&&scol==i){card_selected(x+43+i*8,y+2);if(kbd_dest)card_shadow(x+43+i*8,y+2,BOARD_BG);}if(selected&&valid_found_dest(i))destination_mark(x+43+i*8,y+2,focus==4+i);}for(c=0;c<COLS;c++){acc_fill(x+4+c*9,y+5,1,12,178,ACC_ATTR(BOARD_BG,BOARD_FG));acc_fill(x+10+c*9,y+5,1,12,178,ACC_ATTR(BOARD_BG,BOARD_FG));st=view_start(c);en=view_end(c);row=0;if(!tn[c]){draw_empty(x+5+c*9,y+5,0);if(kbd_nav&&!selected&&focus==8+c)card_selected(x+5+c*9,y+5);}if(tn[c]>11&&!reveal[c]){hidden_mark(x+5+c*9,y+5);row=1;}for(r=st;r<en;r++,row++){id=tab[c][r];draw_face(x+5+c*9,y+5+row,id,0);}if(tn[c]>11&&reveal[c])hidden_mark(x+5+c*9,y+15);if(selected&&stype==2&&scol==c&&sidx>=st&&sidx<en){sy=(tn[c]>11&&!reveal[c]?1:0)+(sidx-st);if(sy<=11){card_selected(x+5+c*9,y+5+sy);if(kbd_dest)card_shadow(x+5+c*9,y+5+sy,BOARD_BG);}}else if(kbd_nav&&!selected&&focus==8+c&&cursor[c]>=st&&cursor[c]<en){sy=(tn[c]>11&&!reveal[c]?1:0)+(cursor[c]-st);if(sy<=11){card_selected(x+5+c*9,y+5+sy);}}if(selected&&valid_tableau_dest(c)){int dr=(tn[c]>11)?11:tn[c];destination_mark(x+5+c*9,y+5+dr,focus==8+c);}}game_draw_status();}

static void choose(int focus){int c;if(focus<0)return;if(focus<4){if(selected&&stype==1&&scol==focus){selected=0;return;}if(selected){move_free(focus);}else if(freec[focus]>=0){game_timer_start();selected=1;stype=1;scol=focus;}}else if(focus<8){c=focus-4;if(selected&&stype==3&&scol==c){selected=0;return;}if(selected)move_found(c);else if(foundation[c]){game_timer_start();selected=1;stype=3;scol=c;}}else if(focus<16){c=focus-8;if(selected&&stype==2&&scol==c&&sidx==cursor[c]){selected=0;return;}if(selected)move_tableau(c);else if(tn[c]){game_timer_start();selected=1;stype=2;scol=c;sidx=cursor[c];}}}
static int first_tableau_focus(void){int c;for(c=0;c<COLS;c++)if(tn[c])return 8+c;return 8;}
static int tab_focus(int f,int back){int pf=first_tableau_focus();if(f<0)return back?17:0;if(!back){if(f<7)return f+1;if(f==7)return pf;if(f<16)return 16;if(f==16)return 17;return 0;}else{if(f==0)return 17;if(f<=7)return f-1;if(f<16)return 7;if(f==16)return pf;if(f==17)return 16;}return 0;}


static int dest_valid_focus(int f){if(f<4)return valid_free_dest(f);if(f<8)return valid_found_dest(f-4);if(f<16)return valid_tableau_dest(f-8);return 0;}
static void dest_pos(int f,int *px,int *py){if(f<4){*px=3+f*8;*py=2;}else if(f<8){*px=43+(f-4)*8;*py=2;}else{int c=f-8;*px=5+c*9;*py=5+(tn[c]>11?11:tn[c]);}}
static int dest_move(int cur,int dx,int dy){int f,best=cur,cx,cy,xp,yp,bp=32767,bs=32767,p,s;dest_pos(cur,&cx,&cy);for(f=0;f<16;f++)if(f!=cur&&dest_valid_focus(f)){dest_pos(f,&xp,&yp);if((dx<0&&xp>=cx)||(dx>0&&xp<=cx)||(dy<0&&yp>=cy)||(dy>0&&yp<=cy))continue;p=dx?abs(xp-cx):abs(yp-cy);s=dx?abs(yp-cy):abs(xp-cx);if(p<bp||(p==bp&&s<bs)){bp=p;bs=s;best=f;}}return best;}
int main(int argc,char **argv){int x=1,y=1,focus=-1,key=0,mx=0,my=0,c,r,dirty=1,full=1,quit=0,kbd_dest=0,kbd_nav=0;unsigned mb=0;if(acc_help(argc,argv,"!FCELL","A FreeCell card game for Launch!."))return 0;if(!acc_begin(argv[0],"FreeCell",0))return 1;card_font(1);acc_box(x,y,77,22,"FreeCell");game_timer_base_x=x+14;game_timer_y=y+19;acc_set_idle_hook(game_timer_tick);new_game();while(key!=27&&!quit){if(dirty){render(x,y,focus,full,kbd_nav,kbd_dest);full=0;acc_button(x+3,y+19," Refresh ",focus==16);acc_button(x+67,y+19," Exit ",focus==17);dirty=0;}acc_wait(&key,&mx,&my,&mb);if(mb&ACC_MOUSE_MOVED)continue;if((mb&1)){kbd_dest=0;kbd_nav=0;if(my==y+19){if(mx>=x+3&&mx<x+13){new_game();focus=16;dirty=1;full=1;}else if(mx>=x+67){focus=17;quit=1;}}else if(my==y+2){for(c=0;c<4;c++)if(mx>=x+3+c*8&&mx<x+8+c*8){focus=c;choose(focus);dirty=1;break;}for(c=0;c<4;c++)if(mx>=x+43+c*8&&mx<x+48+c*8){focus=4+c;choose(focus);dirty=1;break;}}else if(my>=y+5&&my<=y+17){for(c=0;c<COLS;c++)if(mx>=x+5+c*9&&mx<x+10+c*9){focus=8+c;r=row_index(c,my-(y+5));if(r==-2){reveal[c]=!reveal[c];if(reveal[c])cursor[c]=0;else cursor[c]=tn[c]-1;}else if(r==-3){if(selected&&valid_tableau_dest(c))move_tableau(c);}else if(r>=0){cursor[c]=r;if(selected&&stype==2&&scol==c&&sidx==r)selected=0;else choose(focus);}dirty=1;break;}}key=0;}else if(key==9||key==271){kbd_nav=1;kbd_dest=0;if(selected)selected=0;focus=tab_focus(focus,key==271);if(focus>=8&&focus<16){c=focus-8;if(tn[c])cursor[c]=tn[c]-1;}dirty=1;key=0;}else if((key==256+72||key==256+80)&&focus>=8&&focus<16&&!kbd_dest){kbd_nav=1;c=focus-8;if(key==256+72){if(tn[c]>11&&!reveal[c]&&cursor[c]<=view_start(c)){reveal[c]=1;cursor[c]=0;}else if(cursor[c]>0)cursor[c]--;}else{if(tn[c]>11&&reveal[c]&&cursor[c]>=view_end(c)-1){reveal[c]=0;cursor[c]=tn[c]-1;}else if(cursor[c]<tn[c]-1)cursor[c]++;}dirty=1;key=0;}else if(kbd_dest&&(key==256+75||key==256+77||key==256+72||key==256+80)){focus=dest_move(focus,key==256+75?-1:key==256+77?1:0,key==256+72?-1:key==256+80?1:0);kbd_nav=1;dirty=1;key=0;}else if(key==256+75){kbd_nav=1;if(focus>=8&&focus<16){focus=8+((focus-8+7)%8);c=focus-8;if(tn[c])cursor[c]=tn[c]-1;}dirty=1;key=0;}else if(key==256+77){kbd_nav=1;if(focus>=8&&focus<16){focus=8+((focus-8+1)%8);c=focus-8;if(tn[c])cursor[c]=tn[c]-1;}dirty=1;key=0;}else if(key==' '&&selected&&kbd_dest){selected=0;kbd_dest=0;dirty=1;key=0;}else if(key==' '&&!selected){kbd_nav=1;if(focus>=0&&focus<4&&freec[focus]>=0){game_timer_start();selected=1;stype=1;scol=focus;kbd_dest=1;}else if(focus>=4&&focus<8&&foundation[focus-4]){game_timer_start();selected=1;stype=3;scol=focus-4;kbd_dest=1;}else if(focus>=8&&focus<16){c=focus-8;if(tn[c]){game_timer_start();selected=1;stype=2;scol=c;sidx=cursor[c];kbd_dest=1;}}dirty=1;key=0;}else if(key==13&&selected&&kbd_dest){if(focus<4)move_free(focus);else if(focus<8)move_found(focus-4);else move_tableau(focus-8);if(!selected)kbd_dest=0;dirty=1;key=0;}else if(key==13){if(focus==16){new_game();focus=16;dirty=1;full=1;kbd_dest=0;}else if(focus==17)key=27;else{choose(focus);dirty=1;}if(key!=27)key=0;}else if(key!=27)key=0;if(won()){acc_notice("FreeCell","Congratulations - you won!");new_game();focus=-1;kbd_nav=0;kbd_dest=0;dirty=1;full=1;}}acc_set_idle_hook(0);card_font(0);acc_end();return 0;}
