/* Launch! Solitaire accessory: compact Klondike for 80-column DOS. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ACCLIB.H"
#define COLS 7
#define CARDS 52
#define SOL_BOARD_BG 2
#define SOL_BOARD_FG 10
#define SOL_BACK_BG 7
#define SOL_BACK_FG 1
#define SOL_CARD_BG 7
#define SOL_CARD_DIM_BG 7
typedef struct{unsigned char rank,suit,face;} CARD;
static CARD card[CARDS];
static int tab[COLS][CARDS],tn[COLS],stock[CARDS],ns,waste[CARDS],nw,foundation[4];
static int selected=0,selcol=0,selidx=0,cursor[COLS],reveal[COLS];static unsigned deal_seed=0;
static int score=0;
static unsigned long game_start_tick=0;
static unsigned game_last_sec=0xFFFFU;
static int game_timer_running=0;
static int game_timer_base_x=0,game_timer_y=0,game_timer_col=0;
static unsigned game_elapsed_seconds(void){unsigned long now,ticks,sec;if(!game_timer_running)return 0;now=acc_ticks();if(now>=game_start_tick)ticks=now-game_start_tick;else ticks=now+0x1800B0UL-game_start_tick;sec=ticks/18UL;if(sec>5999UL)sec=5999UL;return(unsigned)sec;}
static void game_timer_start(void){if(!game_timer_running){game_timer_running=1;game_start_tick=acc_ticks();game_last_sec=0xFFFFU;}}
static void game_timer_tick(void){unsigned sec=game_elapsed_seconds();char s[12];if(sec==game_last_sec)return;game_last_sec=sec;sprintf(s,"Time %02u:%02u",sec/60U,sec%60U);acc_text(game_timer_col,game_timer_y,s,ACC_HEADING,10);}
static void game_draw_status(void){char ss[24],ts[12];unsigned sec=game_elapsed_seconds();acc_fill(game_timer_base_x,game_timer_y,40,1,' ',ACC_BG);sprintf(ss,"Score %d",score);acc_text(game_timer_base_x,game_timer_y,ss,ACC_HEADING,(int)strlen(ss));game_timer_col=game_timer_base_x+(int)strlen(ss)+3;sprintf(ts,"Time %02u:%02u",sec/60U,sec%60U);acc_text(game_timer_col,game_timer_y,ts,ACC_HEADING,10);game_last_sec=sec;}
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
static int red(int suit){return suit<2;}
static int first_face(int c){int i;for(i=0;i<tn[c];i++)if(card[tab[c][i]].face)return i;return tn[c];}
static void cursor_reset(int c){cursor[c]=first_face(c);if(cursor[c]>=tn[c]&&tn[c])cursor[c]=tn[c]-1;}
static void new_game(void){int order[CARDS],i,j,c,r,p=0,t;for(i=0;i<CARDS;i++){card[i].rank=(unsigned char)(i%13+1);card[i].suit=(unsigned char)(i/13);card[i].face=0;order[i]=i;}deal_seed+=0x9E37U;srand((unsigned)acc_ticks()^deal_seed);for(i=CARDS-1;i>0;i--){j=rand()%(i+1);t=order[i];order[i]=order[j];order[j]=t;}memset(tn,0,sizeof(tn));memset(foundation,0,sizeof(foundation));nw=ns=0;for(c=0;c<COLS;c++)for(r=0;r<=c;r++){t=order[p++];card[t].face=(unsigned char)(r==c);tab[c][tn[c]++]=t;}while(p<CARDS)stock[ns++]=order[p++];for(c=0;c<COLS;c++){cursor_reset(c);reveal[c]=0;}selected=0;score=0;game_timer_running=0;game_start_tick=0;game_last_sec=0xFFFFU;}
static void rank_chars(int rank,int suit,char *s){int g;s[0]='|';if(rank==10){s[1]='1';s[2]='0';}else if((g=card_rank_glyph(rank))!=0){s[1]=(char)g;s[2]=' ';}else{s[1]=(char)('0'+rank);s[2]=' ';}s[3]=(char)suit_glyph[suit];s[4]='|';s[5]=0;}
static void card_chars(int id,char *s){rank_chars(card[id].rank,card[id].suit,s);}
static void draw_empty(int x,int y,int focus){char s[6]={'|',' ',' ',' ','|',0};if(focus){s[0]='[';s[4]=']';}acc_text(x,y,s,ACC_ATTR(SOL_BOARD_BG,15),5);}
static void draw_back(int x,int y,int focus,int behind){char s[6]={'|',' ',(char)21,' ','|',0};(void)behind;if(focus){s[0]='[';s[4]=']';}acc_text(x,y,s,ACC_ATTR(SOL_BACK_BG,SOL_BACK_FG),5);}
static void card_shadow(int x,int y,int background){int i;for(i=1;i<5;i++)acc_put(x+i,y+1,220,ACC_ATTR(background,0));acc_put(x+5,y,245,ACC_ATTR(background,0));acc_put(x+5,y+1,244,ACC_ATTR(background,0));}
static void card_selected(int x,int y,int attr,int background){int a=ACC_ATTR(2,15);(void)attr;(void)background;acc_put(x-1,y,16,a);acc_put(x+5,y,17,a);}
static void draw_face(int x,int y,int id,int mark,int behind){char s[6];int colour=red(card[id].suit)?12:0;int attr=behind?ACC_ATTR(SOL_CARD_DIM_BG,colour):ACC_ATTR(SOL_CARD_BG,colour);card_chars(id,s);if(mark){s[0]='[';s[4]=']';}acc_text(x,y,s,attr,5);}
static void draw_waste_under(int x,int y,int id){char s[6];int colour=red(card[id].suit)?12:0;card_chars(id,s);s[4]=0;acc_text(x,y,s,ACC_ATTR(SOL_CARD_DIM_BG,colour),4);}
static void draw_rank(int x,int y,int rank,int suit,int mark){char s[6];int colour=red(suit)?12:0;rank_chars(rank,suit,s);if(mark){s[0]='[';s[4]=']';}acc_text(x,y,s,ACC_ATTR(SOL_CARD_BG,colour),5);}
static int source_id(void){if(selected==1)return nw?waste[nw-1]:-1;if(selected==2&&selidx<tn[selcol])return tab[selcol][selidx];if(selected==3&&foundation[selcol])return selcol*13+foundation[selcol]-1;return-1;}
static void remove_source(void){int i;if(selected==1)nw--;else if(selected==2){tn[selcol]=selidx;if(tn[selcol]){if(!card[tab[selcol][tn[selcol]-1]].face)score+=5;card[tab[selcol][tn[selcol]-1]].face=1;}cursor_reset(selcol);}else if(selected==3){foundation[selcol]--;if(score>=10)score-=10;}for(i=0;i<COLS;i++)cursor_reset(i);selected=0;}
static int can_place(int id,int dest){int top;if(!tn[dest])return (int)card[id].rank==13;top=tab[dest][tn[dest]-1];return card[top].face&&(int)card[top].rank==(int)card[id].rank+1&&red(card[top].suit)!=red(card[id].suit);}
static int move_tableau(int dest){int id=source_id(),i,n;if(id<0||!can_place(id,dest)||(selected==2&&selcol==dest))return 0;if(selected==2)n=tn[selcol]-selidx;else n=1;if(tn[dest]+n>CARDS)return 0;if(selected==2)for(i=selidx;i<tn[selcol];i++)tab[dest][tn[dest]++]=tab[selcol][i];else tab[dest][tn[dest]++]=id;remove_source();cursor_reset(dest);return 1;}
static int move_foundation(int suit){int id=source_id();if(id<0||(int)card[id].suit!=suit||(int)card[id].rank!=(int)foundation[suit]+1)return 0;if(selected==2&&selidx!=tn[selcol]-1)return 0;if(selected==3)return 0;foundation[suit]++;score+=10;remove_source();return 1;}
static int won(void){return foundation[0]==13&&foundation[1]==13&&foundation[2]==13&&foundation[3]==13;}
static void deal_stock(void){int id,n=0;if(ns){while(ns&&n<3){id=stock[--ns];card[id].face=1;waste[nw++]=id;n++;}}else while(nw){id=waste[--nw];card[id].face=0;stock[ns++]=id;}selected=0;}
static int sol_cap(int c){int n=12-first_face(c);if(n<1)n=1;return n;}
static int sol_vstart(int c){int ff=first_face(c),cap=sol_cap(c);if(tn[c]-ff<=cap)return ff;return reveal[c]?ff:tn[c]-cap;}
static int sol_vend(int c){int ff=first_face(c),cap=sol_cap(c);if(tn[c]-ff<=cap)return tn[c];return reveal[c]?ff+cap:tn[c];}
static int row_index(int c,int row){int ff=first_face(c),st=sol_vstart(c),en=sol_vend(c),n=en-st,off=4+ff;if(ff>0&&row>=4&&row<4+ff)return-4;if(tn[c]-ff>sol_cap(c)&&row==3)return-2;if(row==off+n)return-3;if(row<off||row>=off+n)return-1;return st+(row-off);}
static void hidden_mark(int x,int y){acc_text(x,y,"|...|",ACC_ATTR(SOL_BOARD_BG,1),5);}
static void destination_mark(int x,int y,int focused)
{int a=ACC_ATTR(2,15);acc_text(x,y,"|   |",a,5);if(focused)card_selected(x,y,a,2);}
static int valid_foundation_dest(int suit){int id=source_id();return id>=0&&selected!=3&&(int)card[id].suit==suit&&(int)card[id].rank==(int)foundation[suit]+1&&(selected!=2||selidx==tn[selcol]-1);}
static int valid_tableau_dest(int c){int id=source_id();return id>=0&&!(selected==2&&selcol==c)&&can_place(id,c);}
static void render(int x,int y,int focus,int erase,int kbd_nav,int kbd_dest){char q[6];int c,i,row,id,wshow,st,en,sy;/* Clear all transient focus-arrow cells before repainting them. */
for(i=0;i<4;i++){acc_put(x+37+i*8,y+2,' ',ACC_BG);acc_put(x+43+i*8,y+2,' ',ACC_BG);}
acc_put(x+2,y+2,' ',ACC_BG);acc_put(x+8,y+2,' ',ACC_BG);acc_put(x+18,y+2,' ',ACC_BG);acc_put(x+24,y+2,' ',ACC_BG);
for(c=0;c<COLS;c++){for(i=4;i<=15;i++){acc_put(x+2+c*10,y+i,178,ACC_ATTR(SOL_BOARD_BG,SOL_BOARD_FG));acc_put(x+8+c*10,y+i,178,ACC_ATTR(SOL_BOARD_BG,SOL_BOARD_FG));}}if(erase){acc_fill(x+1,y+1,71,19,' ',ACC_BG);acc_fill(x+1,y+4,71,14,178,ACC_ATTR(SOL_BOARD_BG,SOL_BOARD_FG));acc_fill(x+1,y+18,71,1,' ',ACC_BG);}if(ns)draw_back(x+3,y+2,0,0);else draw_empty(x+3,y+2,0);if(kbd_nav&&focus==0)card_selected(x+3,y+2,0,0);if(nw){wshow=nw<3?nw:3;for(i=wshow-1;i>0;i--)draw_waste_under(x+19-i*4,y+2,waste[nw-1-i]);draw_face(x+19,y+2,waste[nw-1],0,0);if(kbd_nav&&focus==1&&!selected)card_selected(x+19,y+2,0,0);if(selected==1){card_selected(x+19,y+2,0,0);if(kbd_dest)card_shadow(x+19,y+2,SOL_BOARD_BG);}}else{draw_empty(x+19,y+2,0);if(kbd_nav&&focus==1)card_selected(x+19,y+2,0,0);}for(i=0;i<4;i++){if(foundation[i]){draw_rank(x+38+i*8,y+2,foundation[i],i,0);if(kbd_nav&&focus==2+i&&!selected)card_selected(x+38+i*8,y+2,0,0);if(selected==3&&selcol==i){card_selected(x+38+i*8,y+2,0,0);if(kbd_dest)card_shadow(x+38+i*8,y+2,SOL_BOARD_BG);}}else{q[0]='|';q[1]=' ';q[2]=(char)suit_glyph[i];q[3]=' ';q[4]='|';q[5]=0;acc_text(x+38+i*8,y+2,q,ACC_BORDER,5);if(kbd_nav&&focus==2+i&&!selected)card_selected(x+38+i*8,y+2,0,0);}}for(c=0;c<COLS;c++){acc_fill(x+2+c*10,y+4,1,12,178,ACC_ATTR(SOL_BOARD_BG,SOL_BOARD_FG));acc_fill(x+8+c*10,y+4,1,12,178,ACC_ATTR(SOL_BOARD_BG,SOL_BOARD_FG));st=sol_vstart(c);en=sol_vend(c);row=4;if(!tn[c]){draw_empty(x+3+c*10,y+4,0);if(kbd_nav&&!selected&&focus==6+c)card_selected(x+3+c*10,y+4,0,0);}if(tn[c]-first_face(c)>11&&!reveal[c]){hidden_mark(x+3+c*10,y+4);row=5;}for(i=st;i<en;i++,row++){id=tab[c][i];draw_face(x+3+c*10,y+row,id,0,i<tn[c]-1);}if(tn[c]-first_face(c)>11&&reveal[c])hidden_mark(x+3+c*10,y+14);if(selected==2&&selcol==c&&selidx>=st&&selidx<en){sy=4+(tn[c]-first_face(c)>11&&!reveal[c]?1:0)+(selidx-st);if(sy<=15){card_selected(x+3+c*10,y+sy,0,0);if(kbd_dest)card_shadow(x+3+c*10,y+sy,SOL_BOARD_BG);}}else if(kbd_nav&&!selected&&focus==6+c&&cursor[c]>=st&&cursor[c]<en){sy=4+(tn[c]-first_face(c)>11&&!reveal[c]?1:0)+(cursor[c]-st);if(sy<=15)card_selected(x+3+c*10,y+sy,0,0);}if(selected&&valid_tableau_dest(c)){int dr=(tn[c]-first_face(c)>11)?15:4+(tn[c]-first_face(c));destination_mark(x+3+c*10,y+dr,focus==6+c);}}if(selected)for(i=0;i<4;i++)if(valid_foundation_dest(i))destination_mark(x+38+i*8,y+2,focus==2+i);game_draw_status();}
static void select_tableau(int col,int idx){if(idx>=first_face(col)&&idx<tn[col]){game_timer_start();selected=2;selcol=col;selidx=idx;cursor[col]=idx;}}
static int activate(int focus){int col,moved;if(focus==0){if(ns||nw)game_timer_start();deal_stock();return 1;}if(focus==1){if(nw){game_timer_start();if(selected==1)selected=0;else selected=1;}return 0;}if(focus>=2&&focus<=5){col=focus-2;if(selected){moved=move_foundation(col);if(!moved&&selected==3&&selcol==col)selected=0;return moved;}if(foundation[col]){game_timer_start();selected=3;selcol=col;}return 0;}if(focus>=6&&focus<=12){col=focus-6;if(selected){moved=move_tableau(col);if(!moved&&selected==2&&selcol==col)selected=0;return moved;}select_tableau(col,cursor[col]);}return 0;}
static int first_play_focus(void){int c;for(c=0;c<COLS;c++)if(tn[c])return 6+c;return 6;}
static int tab_focus(int f,int back){int pf=first_play_focus();if(f<0)return back?14:0;if(!back){if(f<5)return f+1;if(f==5)return pf;if(f<13)return 13;if(f==13)return 14;return 0;}else{if(f==0)return 14;if(f<=5)return f-1;if(f<13)return 5;if(f==13)return pf;if(f==14)return 13;}return 0;}
static int enter_foundation(void){int id=source_id();if(id<0)return 0;return move_foundation((int)card[id].suit);}


static int dest_valid_focus(int f){if(f>=2&&f<=5)return valid_foundation_dest(f-2);if(f>=6&&f<=12)return valid_tableau_dest(f-6);return 0;}
static void dest_pos(int f,int *px,int *py){if(f==1){*px=19;*py=2;}else if(f>=2&&f<=5){*px=38+(f-2)*8;*py=2;}else{int c=f-6;*px=3+c*10;*py=(tn[c]-first_face(c)>11)?15:4+(tn[c]-first_face(c));}}
static int dest_move(int cur,int dx,int dy){int f,best=cur,cx,cy,xp,yp,bp=32767,bs=32767,p,s;dest_pos(cur,&cx,&cy);for(f=2;f<=12;f++)if(f!=cur&&dest_valid_focus(f)){dest_pos(f,&xp,&yp);if((dx<0&&xp>=cx)||(dx>0&&xp<=cx)||(dy<0&&yp>=cy)||(dy>0&&yp<=cy))continue;p=dx?abs(xp-cx):abs(yp-cy);s=dx?abs(yp-cy):abs(xp-cx);if(p<bp||(p==bp&&s<bs)){bp=p;bs=s;best=f;}}return best;}
int main(int argc,char **argv){int x=3,y=1,focus=-1,key=0,mx=0,my=0,mbcol,idx,dirty=1,erase=1,changed,click,kbd_dest=0,kbd_nav=0;unsigned mb=0;if(acc_help(argc,argv,"!SOL","A compact mouse and keyboard Klondike solitaire game."))return 0;if(!acc_begin(argv[0],"Solitaire",0))return 1;card_font(1);acc_box(x,y,73,22,"Solitaire");game_timer_base_x=x+15;game_timer_y=y+19;acc_set_idle_hook(game_timer_tick);new_game();
 while(key!=27){if(dirty){render(x,y,focus,erase,kbd_nav,kbd_dest);dirty=erase=0;}acc_button(x+3,y+19,"  Refresh  ",focus==13);acc_button(x+63,y+19,"  Exit  ",focus==14);acc_wait(&key,&mx,&my,&mb);click=(mb&1)&&!(mb&ACC_MOUSE_MOVED);if(click){kbd_dest=0;kbd_nav=0;}if(click&&my==y+19){if(mx>=x+3&&mx<x+13){focus=13;new_game();dirty=erase=1;key=0;}else if(mx>=x+63&&mx<x+69){focus=14;key=27;}}else if(click&&my==y+2){changed=0;if(mx>=x+3&&mx<x+8){focus=0;changed=activate(focus);dirty=1;}else if(mx>=x+11&&mx<x+24){focus=1;if(nw){game_timer_start();if(selected==1){selected=0;erase=1;}else{selected=1;selcol=selidx=0;erase=1;}}dirty=1;}else for(idx=0;idx<4;idx++)if(mx>=x+38+idx*8&&mx<x+43+idx*8){focus=2+idx;changed=activate(focus);dirty=1;break;}if(changed)erase=1;key=0;}else if(click&&my>=y+4&&my<=y+17){for(mbcol=0;mbcol<COLS;mbcol++)if(mx>=x+3+mbcol*10&&mx<x+9+mbcol*10){focus=6+mbcol;changed=0;idx=row_index(mbcol,my-y);if(idx==-2){reveal[mbcol]=!reveal[mbcol];if(reveal[mbcol])cursor[mbcol]=first_face(mbcol);else cursor[mbcol]=tn[mbcol]-1;dirty=1;break;}if(idx>=0&&selected==2&&selcol==mbcol&&selidx==idx){selected=0;erase=1;}else if(idx==-3){if(selected&&valid_tableau_dest(mbcol))changed=move_tableau(mbcol);}else if(idx>=0&&selected&&selected==2&&selcol==mbcol){select_tableau(mbcol,idx);erase=1;}else if(idx>=0&&selected){changed=activate(focus);}else if(idx>=0){select_tableau(mbcol,idx);erase=1;}dirty=1;if(changed)erase=1;break;}key=0;}if(won()){acc_notice("Solitaire","Congratulations - you won!");new_game();focus=-1;kbd_nav=0;kbd_dest=0;dirty=erase=1;}if(key==9||key==271){kbd_nav=1;kbd_dest=0;if(selected){selected=0;erase=1;}focus=tab_focus(focus,key==271);if(focus>=6&&focus<=12){mbcol=focus-6;if(tn[mbcol])cursor[mbcol]=tn[mbcol]-1;}dirty=1;key=0;}else if(kbd_dest&&(key==256+75||key==256+77||key==256+72||key==256+80)){int nf=dest_move(focus,key==256+75?-1:key==256+77?1:0,key==256+72?-1:key==256+80?1:0);focus=nf;kbd_nav=1;dirty=1;key=0;}else if(key==256+75){kbd_nav=1;if(focus>=6&&focus<=12){focus=6+((focus-6+6)%7);mbcol=focus-6;if(tn[mbcol])cursor[mbcol]=tn[mbcol]-1;}dirty=1;key=0;}else if(key==256+77){kbd_nav=1;if(focus>=6&&focus<=12){focus=6+((focus-6+1)%7);mbcol=focus-6;if(tn[mbcol])cursor[mbcol]=tn[mbcol]-1;}dirty=1;key=0;}else if((key==256+72||key==256+80)&&focus>=6&&focus<=12&&!kbd_dest){kbd_nav=1;mbcol=focus-6;idx=first_face(mbcol);if(key==256+72){if(tn[mbcol]-idx>11&&!reveal[mbcol]&&cursor[mbcol]<=sol_vstart(mbcol)){reveal[mbcol]=1;cursor[mbcol]=idx;}else if(cursor[mbcol]>idx)cursor[mbcol]--;}else{if(tn[mbcol]-idx>11&&reveal[mbcol]&&cursor[mbcol]>=sol_vend(mbcol)-1){reveal[mbcol]=0;cursor[mbcol]=tn[mbcol]-1;}else if(cursor[mbcol]<tn[mbcol]-1)cursor[mbcol]++;}dirty=1;key=0;}else if(key==' '&&selected&&kbd_dest){selected=0;kbd_dest=0;dirty=1;erase=1;key=0;}else if(key==' '&&!selected){kbd_nav=1;if(focus==1&&nw){game_timer_start();selected=1;selcol=selidx=0;kbd_dest=1;}else if(focus>=2&&focus<=5&&foundation[focus-2]){game_timer_start();selected=3;selcol=focus-2;kbd_dest=1;}else if(focus>=6&&focus<=12){mbcol=focus-6;if(tn[mbcol]){select_tableau(mbcol,cursor[mbcol]);kbd_dest=1;}}dirty=1;erase=1;key=0;}else if(key==13&&selected&&kbd_dest){changed=0;if(focus>=2&&focus<=5)changed=move_foundation(focus-2);else if(focus>=6&&focus<=12)changed=move_tableau(focus-6);if(changed){kbd_dest=0;erase=1;}dirty=1;key=0;}else if(key==13){changed=0;if(focus==13){new_game();erase=1;kbd_dest=0;}else if(focus==14)key=27;else changed=activate(focus);if(changed)erase=1;if(key!=27){dirty=1;key=0;}}else if(key!=27)key=0;}
 acc_set_idle_hook(0);card_font(0);acc_end();return 0;}
