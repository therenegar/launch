/* Launch! Card Stack accessory. */
#include <stdio.h>
#include <string.h>
#include "ACCLIB.H"

#define CARDS 20
#define CT 22
#define CW 52
#define VISIBLE_LINES 8
#define TEXT_LINES 40

typedef struct { char used,title[CT+1],text[CW*TEXT_LINES]; } CARD;
typedef struct { char used,title[CT+1],text[CW*VISIBLE_LINES]; } OLD_CARD;

static CARD cards[CARDS];
static int count=1,current=0;
static unsigned char stack_softwrap[CARDS][TEXT_LINES];
#define STACK_CLIP_MAX (CW*TEXT_LINES+TEXT_LINES*2+1)
static int stack_sel_anchor=-1,stack_sel_caret=-1,stack_clip_len=0,stack_sel_repaint=0;
static char far stack_clip[STACK_CLIP_MAX];

static void defaults(void)
{
  memset(cards,0,sizeof(cards));
  memset(stack_softwrap,0,sizeof(stack_softwrap));
  cards[0].used=1;
  strcpy(cards[0].title,"Card 1");
  memset(cards[0].text,' ',sizeof(cards[0].text));
}

static void load_cards(void)
{
  char p[ACC_PATH];
  FILE *f;
  long size;
  int i,n;
  OLD_CARD old_card;

  defaults();
  acc_path(p,"DATA","CARDS4.DAT");
  f=fopen(p,"rb");
  if(f){
    fseek(f,0L,SEEK_END);size=ftell(f);rewind(f);
    if(size==(long)(sizeof(int)+sizeof(OLD_CARD)*CARDS)){
      if(fread(&n,sizeof(n),1,f)==1 && n>=1 && n<=CARDS){
        count=n;
        memset(cards,0,sizeof(cards));
        for(i=0;i<CARDS;i++){
          if(fread(&old_card,sizeof(old_card),1,f)!=1){defaults();break;}
          cards[i].used=old_card.used;
          memcpy(cards[i].title,old_card.title,CT+1);
          memset(cards[i].text,' ',sizeof(cards[i].text));
          memcpy(cards[i].text,old_card.text,sizeof(old_card.text));
        }
      }
    } else {
      if(fread(&n,sizeof(n),1,f)==1 && fread(cards,sizeof(cards),1,f)==1 && n>=1 && n<=CARDS)count=n;
      else defaults();
    }
    fclose(f);
  }
  for(i=0;i<CARDS;i++)cards[i].title[CT]=0;
  current=count-1;
}

static int save_cards(void)
{
  char p[ACC_PATH];FILE *f;
  acc_path(p,"DATA","CARDS4.DAT");
  f=fopen(p,"wb");if(!f)return 0;
  fwrite(&count,sizeof(count),1,f);
  fwrite(cards,sizeof(cards),1,f);
  fclose(f);return 1;
}

static int export_cards(char *p)
{
  char name[20],line[CW];FILE *f;int i,y,n,num,last;
  for(num=1;num<10000;num++){
    sprintf(name,"CARDS%d.TXT",num);acc_path(p,"EXPORT",name);if(!acc_exists(p))break;
  }
  if(num==10000)return 0;
  f=fopen(p,"wb");if(!f)return 0;
  for(i=count-1;i>=0;i--){
    memset(line,' ',CW);n=(int)strlen(cards[i].title);if(n>CW-4)n=CW-4;
    memcpy(line,cards[i].title,n);sprintf(name,"%d",i+1);
    memcpy(line+CW-strlen(name),name,strlen(name));fwrite(line,1,CW,f);fputs("\r\n",f);
    memset(line,205,CW);fwrite(line,1,CW,f);fputs("\r\n",f);
    last=TEXT_LINES-1;
    while(last>=VISIBLE_LINES){
      for(n=0;n<CW && cards[i].text[last*CW+n]==' ';n++);
      if(n<CW)break;
      last--;
    }
    for(y=0;y<=last;y++){fwrite(cards[i].text+y*CW,1,CW,f);fputs("\r\n",f);}
    memset(line,196,CW);fwrite(line,1,CW,f);fputs("\r\n\r\n",f);
  }
  fclose(f);return 1;
}

static int stack_sel_low(void){return stack_sel_anchor<stack_sel_caret?stack_sel_anchor:stack_sel_caret;}
static int stack_sel_high(void){return stack_sel_anchor>stack_sel_caret?stack_sel_anchor:stack_sel_caret;}
static int stack_has_selection(void){return stack_sel_anchor>=0&&stack_sel_caret>=0&&stack_sel_anchor!=stack_sel_caret;}
static void stack_selection_clear(void){stack_sel_anchor=stack_sel_caret=-1;}
static int stack_selected(int line,int col){int p=line*CW+col;return stack_has_selection()&&p>=stack_sel_low()&&p<stack_sel_high();}

static void card_box(int x,int y,int index,int active,int title_focus,int title_edit,int title_pos,int text_focus,int cx,int cy,int top)
{
  char number[8];int i,r,line,w=56,h=12;
  int dimfg=(acc_appearance.controls_fg&7)|8;
  int borderattr=active?ACC_CONTROL:ACC_ATTR(acc_appearance.controls_bg,dimfg);
  int titleattr,nattr;
  if(!active)titleattr=ACC_ATTR(acc_appearance.controls_bg,dimfg);
  else if(title_edit)titleattr=ACC_SELECT;
  else if(title_focus)titleattr=ACC_ATTR(acc_appearance.background,acc_appearance.titles);
  else titleattr=ACC_ATTR(acc_appearance.controls_bg,acc_appearance.main_title);
  nattr=active?ACC_ATTR(acc_appearance.controls_bg,acc_appearance.titles):ACC_ATTR(acc_appearance.controls_bg,dimfg);
  acc_fill(x,y,w,h,' ',ACC_CONTROL);
  acc_put(x,y,218,borderattr);acc_put(x+w-1,y,191,borderattr);
  acc_put(x,y+h-1,192,borderattr);acc_put(x+w-1,y+h-1,217,borderattr);
  for(i=1;i<w-1;i++){acc_put(x+i,y,196,borderattr);acc_put(x+i,y+h-1,196,borderattr);}
  for(i=1;i<h-1;i++){acc_put(x,y+i,179,borderattr);acc_put(x+w-1,y+i,179,borderattr);}
  acc_text(x+2,y+1,cards[index].title,titleattr,CT);
  if(active&&title_edit){int cp=title_pos;if(cp<0)cp=0;if(cp>=CT)cp=CT-1;acc_caret_set(x+2+cp,y+1);}
  sprintf(number,"%d",index+1);acc_text(x+w-2-(int)strlen(number),y+1,number,nattr,(int)strlen(number));
  for(i=2;i<w-2;i++)acc_put(x+i,y+2,205,ACC_ATTR(acc_appearance.controls_bg,12));
  if(active){
    if(!title_edit&&!text_focus)acc_caret_hide();
    for(r=0;r<VISIBLE_LINES;r++){int c;line=top+r;acc_text(x+2,y+3+r,cards[index].text+line*CW,ACC_CONTROL,CW);if(text_focus)for(c=0;c<CW;c++)if(stack_selected(line,c))acc_put(x+2+c,y+3+r,cards[index].text[line*CW+c],ACC_SELECT);}
    /* Compact scroll treatment: only show navigation arrows when that
       direction can actually scroll.  No persistent track. */
    acc_put(x+w-2,y+3,top>0?30:' ',ACC_CONTROL);
    acc_put(x+w-2,y+3+VISIBLE_LINES-1,top<TEXT_LINES-VISIBLE_LINES?31:' ',ACC_CONTROL);
    if(text_focus && cy>=top && cy<top+VISIBLE_LINES){acc_caret_set(x+2+cx,y+3+cy-top);}else if(active&&!title_edit)acc_caret_hide();
  }
}

static void select_card(int target){if(target>=0&&target<count)current=target;}
static int lower_card(int offset){int n;if(!count)return 0;n=(current-offset)%count;if(n<0)n+=count;return n;}
static int higher_card(void){return count?(current+1)%count:0;}

static void cards_draw(int x,int y,int focus,int title_edit,int tp,int cx,int cy,int top)
{
  acc_fill(x,y-4,60,17,' ',ACC_BG);
  if(count>=3)card_box(x+4,y-4,lower_card(2),0,0,0,0,0,0,0,0);
  if(count>=2)card_box(x+2,y-2,lower_card(1),0,0,0,0,0,0,0,0);
  card_box(x,y,current,1,focus==0,title_edit,tp,focus==1,cx,cy,top);
}

static void stack_word_wrap(int *pcx,int *pcy)
{
  int cy=*pcy,start=CW-1,len,i,can=1;char *text=cards[current].text;if(cy>=TEXT_LINES-1)return;
  if(text[cy*CW+CW-1]==' '){*pcy=cy+1;*pcx=0;stack_softwrap[current][cy+1]=1;return;}
  while(start>0&&text[cy*CW+start-1]!=' ')start--;
  if(start<=0){*pcy=cy+1;*pcx=0;stack_softwrap[current][cy+1]=1;return;}
  len=CW-start;for(i=CW-len;i<CW;i++)if(text[(cy+1)*CW+i]!=' ')can=0;
  if(!can){*pcy=cy+1;*pcx=0;stack_softwrap[current][cy+1]=1;return;}
  memmove(text+(cy+1)*CW+len,text+(cy+1)*CW,CW-len);
  memcpy(text+(cy+1)*CW,text+cy*CW+start,len);memset(text+cy*CW+start,' ',CW-start);
  stack_softwrap[current][cy+1]=1;*pcy=cy+1;*pcx=len;
}
static int stack_shift_down(void){return (*(unsigned char far *)(((unsigned long)0x40<<16)|0x17)&3)!=0;}
static void stack_selection_move(int oldpos,int newpos,int shift)
{
 if(shift){stack_sel_repaint=1;if(stack_sel_anchor<0)stack_sel_anchor=oldpos;stack_sel_caret=newpos;if(stack_sel_caret==stack_sel_anchor)stack_selection_clear();}
 else {if(stack_has_selection())stack_sel_repaint=1;stack_selection_clear();}
}
static void stack_copy_selection(void)
{
 int lo,hi,p,row,col,n=0;if(!stack_has_selection())return;lo=stack_sel_low();hi=stack_sel_high();
 for(p=lo;p<hi&&n<STACK_CLIP_MAX-3;){row=p/CW;col=p%CW;stack_clip[n++]=cards[current].text[row*CW+col];p++;if(p<hi&&p%CW==0&&!stack_softwrap[current][row+1]){stack_clip[n++]='\r';stack_clip[n++]='\n';}}
 stack_clip_len=n;
}
static void stack_delete_selection(int *pcx,int *pcy)
{
 int lo,hi,fr,lr,r,start,end,count;if(!stack_has_selection())return;lo=stack_sel_low();hi=stack_sel_high();fr=lo/CW;lr=(hi-1)/CW;
 for(r=fr;r<=lr;r++){start=(r==fr)?lo%CW:0;end=(r==lr)?((hi-1)%CW)+1:CW;count=end-start;if(count>0){memmove(cards[current].text+r*CW+start,cards[current].text+r*CW+end,CW-end);memset(cards[current].text+r*CW+CW-count,' ',count);}}
 *pcy=fr;*pcx=lo%CW;stack_selection_clear();
}
static void stack_insert_one(int *pcx,int *pcy,int ch,int insert)
{
 int cx=*pcx,cy=*pcy,base=cy*CW;if(insert&&cx<CW-1)memmove(cards[current].text+base+cx+1,cards[current].text+base+cx,CW-cx-1);cards[current].text[base+cx]=(char)ch;if(cx<CW-1)cx++;else if(cy<TEXT_LINES-1)stack_word_wrap(&cx,&cy);*pcx=cx;*pcy=cy;
}
static void stack_paste(int *pcx,int *pcy,int insert)
{
 int i,c;if(stack_has_selection())stack_delete_selection(pcx,pcy);for(i=0;i<stack_clip_len&&*pcy<TEXT_LINES;i++){c=(unsigned char)stack_clip[i];if(c=='\r')continue;if(c=='\n'){if(*pcy<TEXT_LINES-1){(*pcy)++;*pcx=0;stack_softwrap[current][*pcy]=0;}continue;}stack_insert_one(pcx,pcy,c,insert);}
}
static int stack_used(int row){int n=CW;while(n>0&&cards[current].text[row*CW+n-1]==' ')n--;return n;}
static void stack_remove_row(int row){int r;for(r=row;r<TEXT_LINES-1;r++){memcpy(cards[current].text+r*CW,cards[current].text+(r+1)*CW,CW);stack_softwrap[current][r]=stack_softwrap[current][r+1];}memset(cards[current].text+(TEXT_LINES-1)*CW,' ',CW);stack_softwrap[current][TEXT_LINES-1]=0;}
static void stack_join_next(int *pcx,int *pcy){int cx=*pcx,cy=*pcy,n,take,room;if(cy>=TEXT_LINES-1)return;n=stack_used(cy+1);room=CW-cx;take=n<room?n:room;if(take)memcpy(cards[current].text+cy*CW+cx,cards[current].text+(cy+1)*CW,take);if(take>=n)stack_remove_row(cy+1);else{memmove(cards[current].text+(cy+1)*CW,cards[current].text+(cy+1)*CW+take,CW-take);memset(cards[current].text+(cy+1)*CW+CW-take,' ',take);}}
static void stack_join_previous(int *pcx,int *pcy){int cy=*pcy,prev,n,take,room;if(cy<=0)return;prev=stack_used(cy-1);n=stack_used(cy);room=CW-prev;if(room<=0){*pcy=cy-1;*pcx=CW-1;return;}take=n<room?n:room;if(take)memcpy(cards[current].text+(cy-1)*CW+prev,cards[current].text+cy*CW,take);if(take>=n)stack_remove_row(cy);else{memmove(cards[current].text+cy*CW,cards[current].text+cy*CW+take,CW-take);memset(cards[current].text+cy*CW+CW-take,' ',take);}*pcy=cy-1;*pcx=prev;}
static void stack_split_line(int *pcx,int *pcy){int cx=*pcx,cy=*pcy,r,n,tail,indent=0;if(cy>=TEXT_LINES-1)return;while(indent<CW&&cards[current].text[cy*CW+indent]==' ')indent++;if(indent>=CW)indent=0;for(r=TEXT_LINES-1;r>cy+1;r--){memcpy(cards[current].text+r*CW,cards[current].text+(r-1)*CW,CW);stack_softwrap[current][r]=stack_softwrap[current][r-1];}memset(cards[current].text+(cy+1)*CW,' ',CW);n=stack_used(cy);tail=n>cx?n-cx:0;if(tail>CW-indent)tail=CW-indent;if(tail)memcpy(cards[current].text+(cy+1)*CW+indent,cards[current].text+cy*CW+cx,tail);memset(cards[current].text+cy*CW+cx,' ',CW-cx);stack_softwrap[current][cy+1]=0;*pcy=cy+1;*pcx=indent;}

void acc_tooltip_region(int x,int y,int w,const char *text,int active);
void acc_tooltip_clear_regions(void);
int main(int argc,char **argv)
{
  char counter[12],exported[ACC_PATH],message[ACC_PATH+32];
  int x,y,px,cx=0,cy=0,top=0,tp=0,key=0,mx=0,my=0,focus=-1,title_edit=0,i,dirty=1,insert=1,ch;
  int oldcy,oldtop,base;
  unsigned mb=0;
  if(acc_help(argc,argv,"!STACK","A persistent card stack of editable titled text cards."))return 0;
  if(!acc_begin(argv[0],"Card Stack",0))return 1;
  load_cards();x=(acc_cols-68)/2+3;px=x+1;y=(acc_rows-22)/2+6;acc_box(x-3,y-6,68,22,"Card Stack");

  while(key!=27){
    if(dirty){cards_draw(px,y,focus,title_edit,tp,cx,cy,top);dirty=0;}
    if(count>1)acc_button(x,y+13," Prev ",focus==2);else acc_button_disabled(x,y+13," Prev ");
    sprintf(counter,"%d",current+1);i=(int)strlen(counter);
    acc_text(x+7,y+13,"",ACC_LABEL,5);acc_text(x+7,y+13,counter,ACC_HEADING,i);
    sprintf(counter,"/%d",count);acc_text(x+7+i,y+13,counter,ACC_LABEL,(int)strlen(counter));
    if(count>1)acc_button(x+14,y+13," Next ",focus==3);else acc_button_disabled(x+14,y+13," Next ");acc_button(x+21,y+13,"  Add  ",focus==4);
    if(count>1)acc_button(x+29,y+13,"  Delete  ",focus==5);else acc_button_disabled(x+29,y+13,"  Delete  ");acc_button(x+37,y+13,"  Export  ",focus==6);
    acc_button(x+55,y+13,"  Exit  ",focus==7);
    acc_tooltip_clear_regions();acc_tooltip_region(px+2,y+1,CT,"Select to edit title",1);if(count>=2)acc_tooltip_region(px+4,y-1,CT,"Bring card to front",1);if(count>=3)acc_tooltip_region(px+6,y-3,CT,"Bring card to front",1);acc_wait(&key,&mx,&my,&mb);

    if((mb&1)&&count>=2&&my==y-1&&mx>=px+4&&mx<px+26){stack_selection_clear();select_card(lower_card(1));cx=cy=top=0;focus=0;title_edit=0;dirty=1;key=0;continue;}
    if((mb&1)&&count>=3&&my==y-3&&mx>=px+6&&mx<px+28){stack_selection_clear();select_card(lower_card(2));cx=cy=top=0;focus=0;title_edit=0;dirty=1;key=0;continue;}
    if((mb&1)&&my==y+1&&mx>=px+2&&mx<px+24){
      int len;stack_selection_clear();len=(int)strlen(cards[current].title);
      /* Clicking the title always enters editing at the clicked position. */
      focus=0;title_edit=1;tp=mx-(px+2);if(tp<0)tp=0;if(tp>len)tp=len;
      dirty=1;key=0;continue;
    }
    if((mb&1)&&focus==0){title_edit=0;focus=1;dirty=1;}
    if((mb&1)&&mx==px+54&&my>=y+3&&my<y+3+VISIBLE_LINES){
      if(my==y+3&&top>0)top--;
      else if(my==y+3+VISIBLE_LINES-1&&top<TEXT_LINES-VISIBLE_LINES)top++;
      else { key=0; continue; }
      if(cy<top)cy=top;if(cy>=top+VISIBLE_LINES)cy=top+VISIBLE_LINES-1;
      focus=1;title_edit=0;dirty=1;key=0;continue;
    }
    if((mb&1)&&my>=y+3&&my<y+3+VISIBLE_LINES&&mx>=px+2&&mx<px+2+CW){focus=1;title_edit=0;stack_selection_clear();cx=mx-(px+2);cy=top+my-(y+3);dirty=1;key=0;continue;}
    if((mb&1)&&my==y+13){stack_selection_clear();title_edit=0;if(mx>=x&&mx<x+5)focus=2;else if(mx>=x+14&&mx<x+19)focus=3;else if(mx>=x+21&&mx<x+27)focus=4;else if(count>1&&mx>=x+29&&mx<x+35)focus=5;else if(mx>=x+37&&mx<x+43)focus=6;else if(mx>=x+55&&mx<x+61)focus=7;key=13;}

    /* Esc first leaves title editing; a second Esc closes the program. */
    if(key==27&&title_edit){title_edit=0;dirty=1;key=0;continue;}
    if(key==27)break;
    if(key==9||key==271){
      int dir=(key==271)?-1:1;
      title_edit=0;
      if(focus<0)focus=(dir<0)?7:0;
      else do { focus=(focus+dir+8)%8; } while(count<=1&&(focus==2||focus==3||focus==5));
      if(focus!=1)stack_selection_clear();dirty=1;key=0;continue;
    }
    if(key==13&&focus==0){
      stack_selection_clear();
      if(title_edit)title_edit=0;else {title_edit=1;tp=(int)strlen(cards[current].title);}
      dirty=1;key=0;continue;
    }
    if(key==13&&focus>=2){
      if(focus==2){stack_selection_clear();select_card(higher_card());cx=cy=top=0;}
      else if(focus==3){stack_selection_clear();select_card(lower_card(1));cx=cy=top=0;}
      else if(focus==4&&count<CARDS){cards[count].used=1;sprintf(cards[count].title,"Card %d",count+1);memset(cards[count].text,' ',sizeof(cards[count].text));memset(stack_softwrap[count],0,TEXT_LINES);count++;stack_selection_clear();select_card(count-1);cx=cy=top=0;}
      else if(focus==5&&count>1){for(i=current;i<count-1;i++){cards[i]=cards[i+1];memcpy(stack_softwrap[i],stack_softwrap[i+1],TEXT_LINES);}memset(stack_softwrap[count-1],0,TEXT_LINES);count--;if(current>=count)current=count-1;stack_selection_clear();cx=cy=top=0;}
      else if(focus==6){if(!export_cards(exported))acc_notice("Export","Unable to export stack.");else{sprintf(message,"Card Stack exported to\n%s",exported);acc_notice("Export",message);}}
      else if(focus==7)key=27;
      dirty=1;if(key!=27)key=0;continue;
    }

    if(focus==0&&title_edit){
      int len=(int)strlen(cards[current].title);
      if(key==256+75&&tp>0)tp--;else if(key==256+77&&tp<len)tp++;else if(key==256+71)tp=0;else if(key==256+79)tp=len;else if(key==256+82)insert=!insert;
      else if(key==256+83&&tp<len)memmove(cards[current].title+tp,cards[current].title+tp+1,len-tp);
      else if(key==8&&tp>0){memmove(cards[current].title+tp-1,cards[current].title+tp,len-tp+1);tp--;}
      else if((key>=32&&key<=255)||(key>=513&&key<=767)){ch=key>=512?key-512:key;if(insert&&len<CT){memmove(cards[current].title+tp+1,cards[current].title+tp,len-tp+1);cards[current].title[tp++]=(char)ch;}else if(!insert){if(tp<len)cards[current].title[tp++]=(char)ch;else if(len<CT){cards[current].title[tp++]=(char)ch;cards[current].title[tp]=0;}}}
      /* Redraw through card_box on every edit so erased characters, the
         insertion point and the BIOS underscore caret stay synchronized. */
      dirty=1;key=0;
    } else if(focus==0){
      if(key!=27)key=0;
    } else if(focus==1){
      oldcy=cy;oldtop=top;base=cy*CW;
      {int oldpos=cy*CW+cx,shift=stack_shift_down();
      if(key==3){stack_copy_selection();key=0;}
      else if(key==24){stack_copy_selection();stack_delete_selection(&cx,&cy);dirty=1;key=0;}
      else if((key==22||key==16)){stack_paste(&cx,&cy,insert);dirty=1;key=0;}
      else if(key==256+0x77){cy=0;cx=0;stack_selection_clear();}
      else if(key==256+0x75){cy=TEXT_LINES-1;while(cy>0&&stack_used(cy)==0)cy--;cx=stack_used(cy);if(cx>=CW)cx=CW-1;stack_selection_clear();}
      else if(key==256+75&&cx>0){cx--;stack_selection_move(oldpos,cy*CW+cx,shift);}
      else if(key==256+77&&cx<CW-1){cx++;stack_selection_move(oldpos,cy*CW+cx,shift);}
      else if(key==256+72&&cy>0){cy--;stack_selection_move(oldpos,cy*CW+cx,shift);}
      else if(key==256+80&&cy<TEXT_LINES-1){cy++;stack_selection_move(oldpos,cy*CW+cx,shift);}
      else if(key==256+71){cx=0;stack_selection_move(oldpos,cy*CW+cx,shift);}
      else if(key==256+79){cx=stack_used(cy);if(cx>=CW)cx=CW-1;stack_selection_move(oldpos,cy*CW+cx,shift);}
      else if(key==256+73){cy-=VISIBLE_LINES;if(cy<0)cy=0;stack_selection_move(oldpos,cy*CW+cx,shift);}
      else if(key==256+81){cy+=VISIBLE_LINES;if(cy>=TEXT_LINES)cy=TEXT_LINES-1;stack_selection_move(oldpos,cy*CW+cx,shift);}
      else if(key==256+82){stack_selection_clear();insert=!insert;}
      else if(key==256+83){if(stack_has_selection())stack_delete_selection(&cx,&cy);else if(cx>=stack_used(cy))stack_join_next(&cx,&cy);else{base=cy*CW;memmove(cards[current].text+base+cx,cards[current].text+base+cx+1,CW-cx-1);cards[current].text[base+CW-1]=' ';}dirty=1;}
      else if(key==8){if(stack_has_selection())stack_delete_selection(&cx,&cy);else if(cx>0){cx--;base=cy*CW;memmove(cards[current].text+base+cx,cards[current].text+base+cx+1,CW-cx-1);cards[current].text[base+CW-1]=' ';}else stack_join_previous(&cx,&cy);dirty=1;}
      else if(key==13){if(stack_has_selection())stack_delete_selection(&cx,&cy);if(cy<TEXT_LINES-1)stack_split_line(&cx,&cy);stack_selection_clear();dirty=1;}
      else if((key>=32&&key<=255)||(key>=513&&key<=767)){if(stack_has_selection())stack_delete_selection(&cx,&cy);ch=key>=512?key-512:key;stack_insert_one(&cx,&cy,ch,insert);stack_selection_clear();dirty=1;}
      else if(key!=27)key=0;
      }
      if(stack_sel_repaint){dirty=1;stack_sel_repaint=0;}
      if(cy<top)top=cy;if(cy>=top+VISIBLE_LINES)top=cy-VISIBLE_LINES+1;
      if(top<0)top=0;if(top>TEXT_LINES-VISIBLE_LINES)top=TEXT_LINES-VISIBLE_LINES;
      if(top!=oldtop || oldcy<top || oldcy>=top+VISIBLE_LINES || stack_has_selection()){dirty=1;}
      else {
        acc_text(px+2,y+3+oldcy-top,cards[current].text+oldcy*CW,ACC_CONTROL,CW);
        if(cy!=oldcy)acc_text(px+2,y+3+cy-top,cards[current].text+cy*CW,ACC_CONTROL,CW);
        acc_put(px+54,y+3,top>0?30:' ',ACC_CONTROL);
        acc_put(px+54,y+3+VISIBLE_LINES-1,top<TEXT_LINES-VISIBLE_LINES?31:' ',ACC_CONTROL);
        acc_caret_set(px+2+cx,y+3+cy-top);
      }
      key=0;
    } else if(key!=27)key=0;
  }
  save_cards();acc_end();return 0;
}
