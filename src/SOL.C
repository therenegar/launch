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
static int selected=0,selcol=0,selidx=0,cursor[COLS];static unsigned deal_seed=0;
static int last_click_id=-1;static unsigned long last_click_tick=0;
static const unsigned char suit_glyph[4]={3,4,5,6};
static const char ranks[]=" A23456789?JQK";
static int red(int suit){return suit<2;}
static int first_face(int c){int i;for(i=0;i<tn[c];i++)if(card[tab[c][i]].face)return i;return tn[c];}
static void cursor_reset(int c){cursor[c]=first_face(c);if(cursor[c]>=tn[c]&&tn[c])cursor[c]=tn[c]-1;}
static void new_game(void){int order[CARDS],i,j,c,r,p=0,t;for(i=0;i<CARDS;i++){card[i].rank=(unsigned char)(i%13+1);card[i].suit=(unsigned char)(i/13);card[i].face=0;order[i]=i;}deal_seed+=0x9E37U;srand((unsigned)acc_ticks()^deal_seed);for(i=CARDS-1;i>0;i--){j=rand()%(i+1);t=order[i];order[i]=order[j];order[j]=t;}memset(tn,0,sizeof(tn));memset(foundation,0,sizeof(foundation));nw=ns=0;for(c=0;c<COLS;c++)for(r=0;r<=c;r++){t=order[p++];card[t].face=(unsigned char)(r==c);tab[c][tn[c]++]=t;}while(p<CARDS)stock[ns++]=order[p++];for(c=0;c<COLS;c++)cursor_reset(c);selected=0;last_click_id=-1;last_click_tick=0;}
static void rank_chars(int rank,int suit,char *s){s[0]='|';if(rank==10){s[1]='1';s[2]='0';}else{s[1]=ranks[rank];s[2]=' ';}s[3]=(char)suit_glyph[suit];s[4]='|';s[5]=0;}
static void card_chars(int id,char *s){rank_chars(card[id].rank,card[id].suit,s);}
static void draw_empty(int x,int y,int focus){char s[6]={'|',' ',' ',' ','|',0};if(focus){s[0]='[';s[4]=']';}acc_text(x,y,s,ACC_ATTR(SOL_BOARD_BG,15),5);}
static void draw_back(int x,int y,int focus,int behind){char s[6]={'|',' ',(char)21,' ','|',0};(void)behind;if(focus){s[0]='[';s[4]=']';}acc_text(x,y,s,ACC_ATTR(SOL_BACK_BG,SOL_BACK_FG),5);}
static void card_shadow(int x,int y,int background){int i;for(i=1;i<5;i++)acc_put(x+i,y+1,220,ACC_ATTR(background,0));acc_put(x+5,y,245,ACC_ATTR(background,0));acc_put(x+5,y+1,244,ACC_ATTR(background,0));}
static void card_selected(int x,int y,int attr,int background){acc_put(x,y,169,attr);acc_put(x+4,y,170,attr);card_shadow(x,y,background);}
static void draw_face(int x,int y,int id,int mark,int behind){char s[6];int colour=red(card[id].suit)?12:0;int attr=behind?ACC_ATTR(SOL_CARD_DIM_BG,colour):ACC_ATTR(SOL_CARD_BG,colour);card_chars(id,s);if(mark){s[0]='[';s[4]=']';}acc_text(x,y,s,attr,5);}
static void draw_waste_under(int x,int y,int id){char s[6];int colour=red(card[id].suit)?12:0;card_chars(id,s);s[4]=0;acc_text(x,y,s,ACC_ATTR(SOL_CARD_DIM_BG,colour),4);}
static void draw_rank(int x,int y,int rank,int suit,int mark){char s[6];int colour=red(suit)?12:0;rank_chars(rank,suit,s);if(mark){s[0]='[';s[4]=']';}acc_text(x,y,s,ACC_ATTR(SOL_CARD_BG,colour),5);}
static int source_id(void){if(selected==1)return nw?waste[nw-1]:-1;if(selected==2&&selidx<tn[selcol])return tab[selcol][selidx];if(selected==3&&foundation[selcol])return selcol*13+foundation[selcol]-1;return-1;}
static void remove_source(void){int i;if(selected==1)nw--;else if(selected==2){tn[selcol]=selidx;if(tn[selcol])card[tab[selcol][tn[selcol]-1]].face=1;cursor_reset(selcol);}else if(selected==3)foundation[selcol]--;for(i=0;i<COLS;i++)cursor_reset(i);selected=0;}
static int can_place(int id,int dest){int top;if(!tn[dest])return card[id].rank==13;top=tab[dest][tn[dest]-1];return card[top].face&&card[top].rank==card[id].rank+1&&red(card[top].suit)!=red(card[id].suit);}
static int move_tableau(int dest){int id=source_id(),i,n;if(id<0||!can_place(id,dest)||(selected==2&&selcol==dest))return 0;if(selected==2)n=tn[selcol]-selidx;else n=1;if(tn[dest]+n>CARDS)return 0;if(selected==2)for(i=selidx;i<tn[selcol];i++)tab[dest][tn[dest]++]=tab[selcol][i];else tab[dest][tn[dest]++]=id;remove_source();cursor_reset(dest);return 1;}
static int move_foundation(int suit){int id=source_id();if(id<0||card[id].suit!=suit||card[id].rank!=foundation[suit]+1)return 0;if(selected==2&&selidx!=tn[selcol]-1)return 0;if(selected==3)return 0;foundation[suit]++;remove_source();return 1;}
static int double_click(int id){unsigned long now=acc_ticks();int yes=(id>=0&&id==last_click_id&&(unsigned long)(now-last_click_tick)<=9UL);last_click_id=id;last_click_tick=now;return yes;}
static int auto_foundation(void){int id=source_id();if(id<0)return 0;return move_foundation(card[id].suit);}
static int won(void){return foundation[0]==13&&foundation[1]==13&&foundation[2]==13&&foundation[3]==13;}
static void deal_stock(void){int id,n=0;if(ns){while(ns&&n<3){id=stock[--ns];card[id].face=1;waste[nw++]=id;n++;}}else while(nw){id=waste[--nw];card[id].face=0;stock[ns++]=id;}selected=0;}
static int row_index(int col,int row){int ff=first_face(col),start=4+(ff>0),idx;if(row<start)return-1;idx=ff+row-start;return idx<tn[col]?idx:-1;}
static void render(int x,int y,int focus,int erase){char q[6];int c,i,ff,row,id,mark,top,wshow,sy;if(erase){acc_fill(x+1,y+1,71,19,' ',ACC_BG);acc_fill(x+1,y+4,71,14,178,ACC_ATTR(SOL_BOARD_BG,SOL_BOARD_FG));acc_fill(x+1,y+18,71,1,' ',ACC_BG);}if(ns)draw_back(x+3,y+2,focus==0,0);else draw_empty(x+3,y+2,focus==0);if(nw){wshow=nw<3?nw:3;for(i=wshow-1;i>0;i--)draw_waste_under(x+19-i*4,y+2,waste[nw-1-i]);draw_face(x+19,y+2,waste[nw-1],focus==1&&!selected,0);if(selected==1){id=waste[nw-1];card_selected(x+19,y+2,ACC_ATTR(SOL_CARD_BG,red(card[id].suit)?12:0),acc_appearance.background);}}else draw_empty(x+19,y+2,focus==1);for(i=0;i<4;i++){if(foundation[i]){draw_rank(x+38+i*8,y+2,foundation[i],i,focus==2+i&&!selected);if(selected==3&&selcol==i)card_selected(x+38+i*8,y+2,ACC_ATTR(SOL_CARD_BG,red(i)?12:0),acc_appearance.background);}else{q[0]='|';q[1]=' ';q[2]=(char)suit_glyph[i];q[3]=' ';q[4]='|';q[5]=0;acc_text(x+38+i*8,y+2,q,focus==2+i?ACC_SELECT:ACC_BORDER,5);}}for(c=0;c<COLS;c++){ff=first_face(c);row=4;top=tn[c]-1;if(ff>0){draw_back(x+3+c*10,y+row,focus==6+c&&cursor[c]<ff&&!selected,1);row++;}for(i=ff;i<tn[c]&&row<=17;i++,row++){id=tab[c][i];mark=(focus==6+c&&cursor[c]==i&&!selected);draw_face(x+3+c*10,y+row,id,mark,i<top);}if(selected==2&&selcol==c&&selidx>=ff&&selidx<tn[c]){sy=4+(ff>0)+(selidx-ff);if(sy<=17){id=tab[c][selidx];card_selected(x+3+c*10,y+sy,ACC_ATTR(SOL_CARD_BG,red(card[id].suit)?12:0),SOL_BOARD_BG);}}if(!tn[c])draw_empty(x+3+c*10,y+4,focus==6+c&&!selected);}acc_text(x+18,y+19,"Select a card, then its destination",ACC_LABEL,36);}
static void select_tableau(int col,int idx){if(idx>=first_face(col)&&idx<tn[col]){selected=2;selcol=col;selidx=idx;cursor[col]=idx;}}
static int activate(int focus){int col,moved;if(focus==0){deal_stock();return 1;}if(focus==1){if(nw){if(selected==1)selected=0;else selected=1;}return 0;}if(focus>=2&&focus<=5){col=focus-2;if(selected){moved=move_foundation(col);if(!moved&&selected==3&&selcol==col)selected=0;return moved;}if(foundation[col]){selected=3;selcol=col;}return 0;}if(focus>=6&&focus<=12){col=focus-6;if(selected){moved=move_tableau(col);if(!moved&&selected==2&&selcol==col)selected=0;return moved;}select_tableau(col,cursor[col]);}return 0;}
int main(int argc,char **argv){int x=3,y=1,focus=0,key=0,mx=0,my=0,mbcol,idx,dirty=1,erase=1,changed,click;unsigned mb=0;if(acc_help(argc,argv,"!SOL","A compact mouse and keyboard Klondike solitaire game."))return 0;if(!acc_begin(argv[0],"Solitaire",0))return 1;acc_box(x,y,73,22,"Solitaire");new_game();
 while(key!=27){if(dirty){render(x,y,focus,erase);dirty=erase=0;}acc_button(x+3,y+19,"  Refresh  ",focus==13);acc_button(x+63,y+19,"  Close  ",focus==14);acc_wait(&key,&mx,&my,&mb);click=(mb&1)&&!(mb&ACC_MOUSE_MOVED);if(click&&my==y+19){last_click_id=-1;if(mx>=x+3&&mx<x+9){focus=13;new_game();dirty=erase=1;key=0;}else if(mx>=x+63&&mx<x+69)key=27;}else if(click&&my==y+2){changed=0;if(mx>=x+3&&mx<x+8){last_click_id=-1;focus=0;changed=activate(focus);dirty=1;}else if(mx>=x+11&&mx<x+24){focus=1;if(nw&&double_click(waste[nw-1])){selected=1;changed=auto_foundation();if(!changed)selected=1;}else changed=activate(focus);dirty=1;}else for(idx=0;idx<4;idx++)if(mx>=x+38+idx*8&&mx<x+43+idx*8){last_click_id=-1;focus=2+idx;changed=activate(focus);dirty=1;break;}if(changed)erase=1;key=0;}else if(click&&my>=y+4&&my<=y+17){for(mbcol=0;mbcol<COLS;mbcol++)if(mx>=x+3+mbcol*10&&mx<x+9+mbcol*10){focus=6+mbcol;changed=0;idx=row_index(mbcol,my-y);if(idx>=0&&idx==tn[mbcol]-1&&double_click(tab[mbcol][idx])){selected=2;selcol=mbcol;selidx=idx;cursor[mbcol]=idx;changed=auto_foundation();if(!changed){selected=2;selcol=mbcol;selidx=idx;}}else if(selected)changed=activate(focus);else if(idx>=0)select_tableau(mbcol,idx);dirty=1;if(changed)erase=1;break;}key=0;}if(won()){acc_notice("Solitaire","Congratulations - you won!");new_game();focus=0;dirty=erase=1;}if(key==9){focus=(focus+1)%15;dirty=1;key=0;}else if(key==256+75){focus=(focus+14)%15;dirty=1;key=0;}else if(key==256+77){focus=(focus+1)%15;dirty=1;key=0;}else if((key==256+72||key==256+80)&&focus>=6&&focus<=12){mbcol=focus-6;idx=first_face(mbcol);if(key==256+72&&cursor[mbcol]>idx)cursor[mbcol]--;else if(key==256+80&&cursor[mbcol]<tn[mbcol]-1)cursor[mbcol]++;dirty=1;key=0;}else if(key==13||key==' '){changed=0;if(focus==13){new_game();erase=1;}else if(focus==14)key=27;else changed=activate(focus);if(changed)erase=1;if(key!=27){dirty=1;key=0;}}else if(key!=27)key=0;}
 acc_end();return 0;}
