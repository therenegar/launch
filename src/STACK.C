/* Launch! Stack accessory. */
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

static void defaults(void)
{
  memset(cards,0,sizeof(cards));
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

static void card_box(int x,int y,int index,int active,int title_focus,int text_focus,int cx,int cy,int top)
{
  char number[8];int i,r,line,w=56,h=12;
  int nattr=ACC_ATTR(acc_appearance.controls_bg,acc_appearance.titles);
  acc_fill(x,y,w,h,' ',ACC_CONTROL);
  acc_put(x,y,218,ACC_CONTROL);acc_put(x+w-1,y,191,ACC_CONTROL);
  acc_put(x,y+h-1,192,ACC_CONTROL);acc_put(x+w-1,y+h-1,217,ACC_CONTROL);
  for(i=1;i<w-1;i++){acc_put(x+i,y,196,ACC_CONTROL);acc_put(x+i,y+h-1,196,ACC_CONTROL);}
  for(i=1;i<h-1;i++){acc_put(x,y+i,179,ACC_CONTROL);acc_put(x+w-1,y+i,179,ACC_CONTROL);}
  acc_text(x+2,y+1,cards[index].title,active&&title_focus?ACC_SELECT:(active?ACC_HEADING:ACC_CONTROL),CT);
  sprintf(number,"%d",index+1);acc_text(x+w-2-(int)strlen(number),y+1,number,nattr,(int)strlen(number));
  for(i=2;i<w-2;i++)acc_put(x+i,y+2,205,ACC_ATTR(acc_appearance.controls_bg,12));
  if(active){
    for(r=0;r<VISIBLE_LINES;r++){line=top+r;acc_text(x+2,y+3+r,cards[index].text+line*CW,ACC_CONTROL,CW);}
    acc_scrollbar(x+w-2,y+3,VISIBLE_LINES,top,TEXT_LINES,VISIBLE_LINES);
    if(text_focus && cy>=top && cy<top+VISIBLE_LINES)
      acc_put(x+2+cx,y+3+cy-top,cards[index].text[cy*CW+cx],ACC_SELECT);
  }
}

static void select_card(int target){if(target>=0&&target<count)current=target;}
static int lower_card(int offset){int n;if(!count)return 0;n=(current-offset)%count;if(n<0)n+=count;return n;}
static int higher_card(void){return count?(current+1)%count:0;}

static void cards_draw(int x,int y,int focus,int cx,int cy,int top)
{
  acc_fill(x,y-4,60,17,' ',ACC_BG);
  if(count>=3)card_box(x+4,y-4,lower_card(2),0,0,0,0,0,0);
  if(count>=2)card_box(x+2,y-2,lower_card(1),0,0,0,0,0,0);
  card_box(x,y,current,1,focus==0,focus==1,cx,cy,top);
  acc_shadow(x,y,56,12);
}

int main(int argc,char **argv)
{
  char counter[12],exported[ACC_PATH],message[ACC_PATH+32];
  int x,y,px,cx=0,cy=0,top=0,tp=0,key=0,mx=0,my=0,focus=1,i,dirty=1,insert=1,ch;
  int oldcy,oldtop,base;
  unsigned mb=0;
  if(acc_help(argc,argv,"!STACK","A persistent stack of editable titled text cards."))return 0;
  if(!acc_begin(argv[0],"Stack",0))return 1;
  load_cards();x=(acc_cols-68)/2+3;px=x+1;y=(acc_rows-22)/2+6;acc_box(x-3,y-6,68,22,"Stack");

  while(key!=27){
    if(dirty){cards_draw(px,y,focus,cx,cy,top);dirty=0;}
    acc_button(x,y+13,"  <  ",focus==2);
    sprintf(counter,"%d",current+1);i=(int)strlen(counter);
    acc_text(x+7,y+13,"",ACC_LABEL,5);acc_text(x+7,y+13,counter,ACC_HEADING,i);
    sprintf(counter,"/%d",count);acc_text(x+7+i,y+13,counter,ACC_LABEL,(int)strlen(counter));
    acc_button(x+12,y+13,"  >  ",focus==3);acc_button(x+18,y+13,"  Add  ",focus==4);
    acc_button(x+26,y+13,"  Delete  ",focus==5);acc_button(x+37,y+13,"  Export  ",focus==6);
    acc_button(x+52,y+13,"  Close  ",focus==7);
    acc_wait(&key,&mx,&my,&mb);

    if((mb&1)&&count>=2&&my==y-1&&mx>=px+4&&mx<px+26){select_card(lower_card(1));cx=cy=top=0;dirty=1;key=0;continue;}
    if((mb&1)&&count>=3&&my==y-3&&mx>=px+6&&mx<px+28){select_card(lower_card(2));cx=cy=top=0;dirty=1;key=0;continue;}
    if((mb&1)&&my==y+1&&mx>=px+2&&mx<px+24){int len=(int)strlen(cards[current].title);focus=0;tp=mx-(px+2);if(tp>len)tp=len;dirty=1;key=0;continue;}
    if((mb&1)&&mx==px+54&&my>=y+3&&my<y+3+VISIBLE_LINES){
      if(my==y+3&&top>0)top--;
      else if(my==y+3+VISIBLE_LINES-1&&top<TEXT_LINES-VISIBLE_LINES)top++;
      else if(my>y+3&&my<y+3+VISIBLE_LINES-1)top=(my-(y+3)-1)*(TEXT_LINES-VISIBLE_LINES)/(VISIBLE_LINES-2);
      if(cy<top)cy=top;if(cy>=top+VISIBLE_LINES)cy=top+VISIBLE_LINES-1;
      focus=1;dirty=1;key=0;continue;
    }
    if((mb&1)&&my>=y+3&&my<y+3+VISIBLE_LINES&&mx>=px+2&&mx<px+2+CW){focus=1;cx=mx-(px+2);cy=top+my-(y+3);dirty=1;key=0;continue;}
    if((mb&1)&&my==y+13){if(mx<x+6)focus=2;else if(mx>=x+12&&mx<x+18)focus=3;else if(mx>=x+18&&mx<x+26)focus=4;else if(mx>=x+26&&mx<x+37)focus=5;else if(mx>=x+37&&mx<x+48)focus=6;else if(mx>=x+52)focus=7;key=13;}

    if(key==27)break;
    if(key==9){focus=(focus+1)%8;dirty=1;key=0;continue;}
    if(key==13&&focus>=2){
      if(focus==2){select_card(higher_card());cx=cy=top=0;}
      else if(focus==3){select_card(lower_card(1));cx=cy=top=0;}
      else if(focus==4&&count<CARDS){cards[count].used=1;sprintf(cards[count].title,"Card %d",count+1);memset(cards[count].text,' ',sizeof(cards[count].text));count++;select_card(count-1);cx=cy=top=0;}
      else if(focus==5&&count>1){for(i=current;i<count-1;i++)cards[i]=cards[i+1];count--;if(current>=count)current=count-1;cx=cy=top=0;}
      else if(focus==6){if(!export_cards(exported))acc_notice("Export","Unable to export stack.");else{sprintf(message,"Stack exported to\n%s",exported);acc_notice("Export",message);}}
      else if(focus==7)key=27;
      dirty=1;if(key!=27)key=0;continue;
    }

    if(focus==0){
      int len=(int)strlen(cards[current].title);
      if(key==256+75&&tp>0)tp--;else if(key==256+77&&tp<len)tp++;else if(key==256+71)tp=0;else if(key==256+79)tp=len;else if(key==256+82)insert=!insert;
      else if(key==256+83&&tp<len)memmove(cards[current].title+tp,cards[current].title+tp+1,len-tp);
      else if(key==8&&tp>0){memmove(cards[current].title+tp-1,cards[current].title+tp,len-tp+1);tp--;}
      else if((key>=32&&key<=255)||(key>=513&&key<=767)){ch=key>=512?key-512:key;if(insert&&len<CT){memmove(cards[current].title+tp+1,cards[current].title+tp,len-tp+1);cards[current].title[tp++]=(char)ch;}else if(!insert){if(tp<len)cards[current].title[tp++]=(char)ch;else if(len<CT){cards[current].title[tp++]=(char)ch;cards[current].title[tp]=0;}}}
      acc_text(px+2,y+1,cards[current].title,ACC_SELECT,CT);key=0;
    } else if(focus==1){
      oldcy=cy;oldtop=top;base=cy*CW;
      if(key==256+75&&cx>0)cx--;
      else if(key==256+77&&cx<CW-1)cx++;
      else if(key==256+72&&cy>0)cy--;
      else if(key==256+80&&cy<TEXT_LINES-1)cy++;
      else if(key==256+71)cx=0;
      else if(key==256+79){cx=CW-1;while(cx>0&&cards[current].text[cy*CW+cx]==' ')cx--;if(cards[current].text[cy*CW+cx]!=' '&&cx<CW-1)cx++;}
      else if(key==256+82)insert=!insert;
      else if(key==256+83){memmove(cards[current].text+base+cx,cards[current].text+base+cx+1,CW-cx-1);cards[current].text[base+CW-1]=' ';}
      else if(key==8){if(cx>0)cx--;cards[current].text[cy*CW+cx]=' ';}
      else if(key==13){if(cy<TEXT_LINES-1){cy++;cx=0;}}
      else if((key>=32&&key<=255)||(key>=513&&key<=767)){ch=key>=512?key-512:key;if(insert&&cx<CW-1)memmove(cards[current].text+base+cx+1,cards[current].text+base+cx,CW-cx-1);cards[current].text[base+cx]=(char)ch;if(cx<CW-1)cx++;else if(cy<TEXT_LINES-1){cy++;cx=0;}}
      else if(key!=27)key=0;
      if(cy<top)top=cy;if(cy>=top+VISIBLE_LINES)top=cy-VISIBLE_LINES+1;
      if(top<0)top=0;if(top>TEXT_LINES-VISIBLE_LINES)top=TEXT_LINES-VISIBLE_LINES;
      if(top!=oldtop || oldcy<top || oldcy>=top+VISIBLE_LINES){dirty=1;}
      else {
        acc_text(px+2,y+3+oldcy-top,cards[current].text+oldcy*CW,ACC_CONTROL,CW);
        if(cy!=oldcy)acc_text(px+2,y+3+cy-top,cards[current].text+cy*CW,ACC_CONTROL,CW);
        acc_scrollbar(px+54,y+3,VISIBLE_LINES,top,TEXT_LINES,VISIBLE_LINES);
        acc_put(px+2+cx,y+3+cy-top,cards[current].text[cy*CW+cx],ACC_SELECT);
      }
      key=0;
    } else if(key!=27)key=0;
  }
  save_cards();acc_end();return 0;
}
