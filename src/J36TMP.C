/* Launch! Journal - daily journal accessory. */
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include "ACCLIB.H"
#define NW 63
#define NL 100
#define NV 12
static char note[NW*NL];static unsigned char softwrap[NL];static int jy,jm,jd;
#define JW (NW-4)
#define JOURNAL_CLIP_MAX (JW*(NL-2)+NL*2+1)
static int journal_sel_anchor=-1,journal_sel_caret=-1,journal_clip_len=0,journal_sel_repaint=0;
static char far journal_clip[JOURNAL_CLIP_MAX];
static int leap(int y){return y%4==0&&(y%100!=0||y%400==0);}static int mdays(int m,int y){static int d[12]={31,28,31,30,31,30,31,31,30,31,30,31};return m==2&&leap(y)?29:d[m-1];}
static void stepday(int d){jd+=d;if(jd<1){if(--jm<1){jm=12;jy--;}jd=mdays(jm,jy);}else if(jd>mdays(jm,jy)){jd=1;if(++jm>12){jm=1;jy++;}}}
static int jweekday(int y,int m,int d){int t[12]={0,3,2,5,0,3,5,1,4,6,2,4};if(m<3)y--;return(y+y/4-y/100+y/400+t[m-1]+d)%7;}
static void fname(char*n){sprintf(n,"J%04d%02d%02d.DAT",jy,jm,jd);}static void wrapname(char*n){sprintf(n,"J%04d%02d%02d.WRP",jy,jm,jd);}static void marks(void){int r;char b[52];static char*m[]={"","January","February","March","April","May","June","July","August","September","October","November","December"};static char*w[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};for(r=0;r<NL;r++){note[r*NW]=(char)196;note[r*NW+1]=(char)9;note[r*NW+2]=note[r*NW+3]=' ';}memset(note,' ',NW*2);sprintf(b,"%s, %s %d %d",w[jweekday(jy,jm,jd)],m[jm],jd,jy);memcpy(note+4,b,strlen(b));for(r=4;r<NW;r++)note[NW+r]=(char)215;}
static int editable(int l){return l>1;}static void load(void){char p[ACC_PATH],n[20];FILE*f;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));fname(n);acc_path(p,"DATA",n);f=fopen(p,"rb");if(f){fread(note,1,sizeof(note),f);fclose(f);}wrapname(n);acc_path(p,"DATA",n);f=fopen(p,"rb");if(f){fread(softwrap,1,sizeof(softwrap),f);fclose(f);}marks();}
static void save(void){char p[ACC_PATH],n[20];FILE*f;fname(n);acc_path(p,"DATA",n);f=fopen(p,"wb");if(f){fwrite(note,1,sizeof(note),f);fclose(f);}wrapname(n);acc_path(p,"DATA",n);f=fopen(p,"wb");if(f){fwrite(softwrap,1,sizeof(softwrap),f);fclose(f);}acc_path(p,"DATA","JOURNAL.IDX");f=fopen(p,"a");if(f){fprintf(f,"%04d%02d%02d\n",jy,jm,jd);fclose(f);}}
static int last_text_line(void){int l,q;for(l=NL-1;l>=2;l--){for(q=NW-1;q>=4;q--)if(note[l*NW+q]!=' ')return l;}return 2;}
static int journal_sel_low(void){return journal_sel_anchor<journal_sel_caret?journal_sel_anchor:journal_sel_caret;}
static int journal_sel_high(void){return journal_sel_anchor>journal_sel_caret?journal_sel_anchor:journal_sel_caret;}
static int journal_has_selection(void){return journal_sel_anchor>=0&&journal_sel_caret>=0&&journal_sel_anchor!=journal_sel_caret;}
static void journal_selection_clear(void){journal_sel_anchor=journal_sel_caret=-1;}
static int journal_selected(int line,int col){int p=(line-2)*JW+(col-4);return line>=2&&col>=4&&journal_has_selection()&&p>=journal_sel_low()&&p<journal_sel_high();}
static void jrow(int x,int y,int line,int top){int c;if(line>=top&&line<top+NV){acc_text(x,y+2+line-top,note+line*NW,ACC_CONTROL,NW);for(c=4;c<NW;c++)if(journal_selected(line,c))acc_put(x+c,y+2+line-top,note[line*NW+c],ACC_SELECT);}}
static void jscroll(int x,int y,int top){int last=last_text_line();acc_fill(x+NW,y,1,NV+2,' ',ACC_CONTROL);if(top>2)acc_put(x+NW,y+2,30,ACC_CONTROL);if(top+NV<=last)acc_put(x+NW,y+NV+1,31,ACC_CONTROL);}
static int journal_editor_focus=0;
static void view(int x,int y,int cx,int cy,int top){int r;char b[64];static char*m[]={"","January","February","March","April","May","June","July","August","September","October","November","December"};static char*w[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};sprintf(b,"%s, %s %d %d",w[jweekday(jy,jm,jd)],m[jm],jd,jy);
 /* The notebook paper continues behind the fixed date and ruled line. */
 acc_fill(x,y,NW+1,NV+2,' ',ACC_CONTROL);acc_fill(x+NW+1,y,1,NV+2,' ',ACC_BG);
 acc_put(x,y,196,ACC_CONTROL);acc_put(x+1,y,9,ACC_CONTROL);acc_put(x,y+1,196,ACC_CONTROL);acc_put(x+1,y+1,9,ACC_CONTROL);
 acc_text(x+4,y,b,ACC_CONTROL,(int)strlen(b));for(r=4;r<NW;r++)acc_put(x+r,y+1,215,ACC_ATTR(acc_appearance.controls_bg,acc_appearance.main_title));for(r=0;r<NV;r++)jrow(x,y,top+r,top);if(journal_editor_focus&&cy>=top&&cy<top+NV)acc_put(x+cx,y+2+cy-top,note[cy*NW+cx],ACC_SELECT);jscroll(x,y,top);}
static int calpick(void)
{
 static char *mn[]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
 int w=26,h=11,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,m=jm,yr=jy,sel=jd,r,c,first,n,focus=-1;unsigned b=0;
 for(;;){char t[16];
  /* Compact date picker: no Launch! title and no toolbar divider. */
  acc_subbox(x,y,w,h,"Go To",0);sprintf(t,"%s %04d",mn[m],yr);
  acc_put(x+3,y+2,17,focus==0?ACC_SELECT:ACC_BORDER);acc_text(x+(w-(int)strlen(t))/2,y+2,t,ACC_HEADING,(int)strlen(t));acc_put(x+w-4,y+2,16,focus==2?ACC_SELECT:ACC_BORDER);
  acc_text(x+3,y+3,"Su Mo Tu We Th Fr Sa",ACC_LABEL,20);
  {static int tt[]={0,3,2,5,0,3,5,1,4,6,2,4};int yy=yr-(m<3);first=(yy+yy/4-yy/100+yy/400+tt[m-1]+1)%7;}n=mdays(m,yr);if(sel>n)sel=n;
  for(r=0;r<6;r++)for(c=0;c<7;c++){int d=r*7+c-first+1;if(d>=1&&d<=n){char q[3];sprintf(q,"%2d",d);acc_text(x+3+c*3,y+4+r,q,(d==sel&&focus==1)?ACC_SELECT:ACC_BORDER,2);}}
  acc_shadow(x,y,w,h);acc_wait(&k,&mx,&my,&b);if(k==27)return 0;
  if(k==9||k==271){if(focus<0)focus=(k==271)?2:0;else focus=(focus+(k==271?2:1))%3;}
  else if(focus==1&&k==256+75){if(sel>1)sel--;}
  else if(focus==1&&k==256+77){if(sel<n)sel++;}
  else if(focus==1&&k==256+72){if(sel>7)sel-=7;}
  else if(focus==1&&k==256+80){if(sel+7<=n)sel+=7;}
  else if(k==13&&focus==0){if(--m<1){m=12;yr--;}sel=1;}
  else if(k==13&&focus==2){if(++m>12){m=1;yr++;}sel=1;}
  else if(k==13&&focus==1){jy=yr;jm=m;jd=sel;return 1;}
  else if(b&1){if(mx<x||mx>=x+w||my<y||my>=y+h)return 0;else if(my==y+2&&mx==x+3){focus=0;if(--m<1){m=12;yr--;}sel=1;}else if(my==y+2&&mx==x+w-4){focus=2;if(++m>12){m=1;yr++;}sel=1;}else if(my>=y+4&&my<y+10&&mx>=x+3&&mx<x+24){c=(mx-(x+3))/3;r=my-(y+4);if(c>=0&&c<7){int d=r*7+c-first+1;if(d>=1&&d<=n){focus=1;jy=yr;jm=m;jd=d;return 1;}}}}
  k=0;
 }
}
static int export_all(char*out){char p[ACC_PATH],name[20],date[16],last[16]="";FILE*idx,*f,*in;int n,r,q;for(n=1;n<10000;n++){sprintf(name,"JOURNAL%d.TXT",n);acc_path(p,"EXPORT",name);if(!acc_exists(p))break;}strcpy(out,p);f=fopen(p,"w");if(!f)return 0;acc_path(p,"DATA","JOURNAL.IDX");idx=fopen(p,"r");if(idx){while(fgets(date,sizeof(date),idx)){date[strcspn(date,"\r\n")]=0;if(!strcmp(date,last))continue;strcpy(last,date);sprintf(name,"J%s.DAT",date);acc_path(p,"DATA",name);in=fopen(p,"rb");if(!in)continue;fread(note,1,sizeof(note),in);fclose(in);fprintf(f,"%s\n",date);fputs("------------------------------------------------------------\n",f);for(r=2;r<NL;r++){q=NW;while(q&&note[r*NW+q-1]==' ')q--;fwrite(note+r*NW,1,q,f);fputc('\n',f);}fputs("------------------------------------------------------------\n",f);}fclose(idx);}fclose(f);return 1;}
static int palette_codes[256];
static int palette_count=0;
static void palette_build(void)
{
 int c;palette_count=0;
 for(c=0;c<256;c++)if(!acc_glyph_is_custom(c))palette_codes[palette_count++]=c;
}
static int palette_index_of(int code)
{
 int i;for(i=0;i<palette_count;i++)if(palette_codes[i]==code)return i;return 0;
}
static void palette_draw_cell(int x,int y,int index,int selected)
{
 int code=palette_codes[index],col=index&31,row=index>>5,px=x+2+col,py=y+2+row;
 int attr=ACC_ATTR(acc_appearance.background,selected?acc_appearance.folders:acc_appearance.border);
 acc_put(px,py,code,attr);
}
static void palette_status(int x,int y,int code)
{
 char b[48],c;
 c=(code>=32)?(char)code:' ';
 sprintf(b,"Char: %c  Decimal: %3d  Hex: %02X",c,code,code);
 acc_fill(x+2,y+11,32,1,' ',ACC_BG);acc_text(x+2,y+11,b,ACC_LABEL,(int)strlen(b));
}
static int character_palette(void)
{
 int w=36,h=14,x=(acc_cols-w)/2,y=(acc_rows-h)/2,i,index,old=-1,key=0,mx=0,my=0;unsigned mb=0;
 /* Compact DOS Navigator-style palette.  Custom Launch! runtime positions are
    omitted completely; remaining CP437 characters pack together with no gaps. */
 palette_build();index=-1;
 acc_subbox(x,y,w,h,"Characters",0);
 for(i=0;i<palette_count;i++)palette_draw_cell(x,y,i,0);
 palette_status(x,y,palette_codes[palette_index_of(128)]);
 acc_shadow(x,y,w,h);
 for(;;){
  acc_wait(&key,&mx,&my,&mb);
  if(mb&ACC_MOUSE_MOVED){
   if(mx>=x+2&&mx<x+34&&my>=y+2&&my<y+10){int n=(my-(y+2))*32+(mx-(x+2));if(n<palette_count&&n!=index){old=index;index=n;if(old>=0)palette_draw_cell(x,y,old,0);palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);}}
   key=0;continue;
  }
  if(mb&1){if(mx>=x+2&&mx<x+34&&my>=y+2&&my<y+10){int n=(my-(y+2))*32+(mx-(x+2));if(n<palette_count)return palette_codes[n];}else if(mx<x||mx>=x+w||my<y||my>=y+h)return -1;continue;}
  if(key==27)return -1;if((key==9||key==271)&&index<0){index=palette_index_of(128);palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);continue;}if(index<0)continue;old=index;
  if(key==256+75){if(index>0)index--;}
  else if(key==256+77){if(index+1<palette_count)index++;}
  else if(key==256+72){index-=32;if(index<0)index=old;}
  else if(key==256+80){if(index+32<palette_count)index+=32;}
  else if(key==13)return palette_codes[index];
  else continue;
  if(index!=old){palette_draw_cell(x,y,old,0);palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);}
 }
}


static void journal_word_wrap(int *pcx,int *pcy)
{
  int cy=*pcy,start=NW-1,len,width=NW-4,i,can=1;if(cy>=NL-1)return;
  if(note[cy*NW+NW-1]==' '){*pcy=cy+1;*pcx=4;softwrap[cy+1]=1;return;}
  while(start>4&&note[cy*NW+start-1]!=' ')start--;
  if(start<=4){*pcy=cy+1;*pcx=4;softwrap[cy+1]=1;return;}
  len=NW-start;for(i=NW-len;i<NW;i++)if(note[(cy+1)*NW+i]!=' ')can=0;
  if(!can){*pcy=cy+1;*pcx=4;softwrap[cy+1]=1;return;}
  memmove(note+(cy+1)*NW+4+len,note+(cy+1)*NW+4,width-len);
  memcpy(note+(cy+1)*NW+4,note+cy*NW+start,len);memset(note+cy*NW+start,' ',NW-start);
  softwrap[cy+1]=1;*pcy=cy+1;*pcx=4+len;
}
static int journal_shift_down(void){return (*(unsigned char far *)(((unsigned long)0x40<<16)|0x17)&3)!=0;}
static int journal_pos(int cx,int cy){return(cy-2)*JW+(cx-4);}
static void journal_selection_move(int oldpos,int newpos,int shift)
{
 if(shift){journal_sel_repaint=1;if(journal_sel_anchor<0)journal_sel_anchor=oldpos;journal_sel_caret=newpos;if(journal_sel_caret==journal_sel_anchor)journal_selection_clear();}
 else {if(journal_has_selection())journal_sel_repaint=1;journal_selection_clear();}
}
static void journal_copy_selection(void)
{
 int lo,hi,p,row,col,lastrow,n=0;if(!journal_has_selection())return;lo=journal_sel_low();hi=journal_sel_high();lastrow=(hi-1)/JW;
 for(p=lo;p<hi&&n<JOURNAL_CLIP_MAX-3;){
  row=2+p/JW;col=4+p%JW;journal_clip[n++]=note[row*NW+col];p++;
  if(p<hi&&p%JW==0&&p/JW<=lastrow&&!softwrap[row+1]){journal_clip[n++]='\r';journal_clip[n++]='\n';}
 }
 journal_clip_len=n;
}
static void journal_delete_selection(int *pcx,int *pcy)
{
 int lo,hi,fr,lr,r,start,end,count;if(!journal_has_selection())return;
 lo=journal_sel_low();hi=journal_sel_high();fr=2+lo/JW;lr=2+(hi-1)/JW;
 for(r=fr;r<=lr;r++){
  start=(r==fr)?4+lo%JW:4;end=(r==lr)?4+((hi-1)%JW)+1:NW;count=end-start;
  if(count>0){memmove(note+r*NW+start,note+r*NW+end,NW-end);memset(note+r*NW+NW-count,' ',count);}
 }
 *pcy=fr;*pcx=4+lo%JW;journal_selection_clear();
}
static void journal_insert_one(int *pcx,int *pcy,int ch,int insert)
{
 int cx=*pcx,cy=*pcy,base=cy*NW+cx;
 if(insert&&cx<NW-1)memmove(note+base+1,note+base,NW-cx-1);
 note[base]=(char)ch;
 if(cx<NW-1)cx++;else if(cy<NL-1)journal_word_wrap(&cx,&cy);
 *pcx=cx;*pcy=cy;
}
static void journal_paste(int *pcx,int *pcy,int insert)
{
 int i,c;if(journal_has_selection())journal_delete_selection(pcx,pcy);
 for(i=0;i<journal_clip_len&&*pcy<NL;i++){
  c=(unsigned char)journal_clip[i];if(c=='\r')continue;
  if(c=='\n'){if(*pcy<NL-1){(*pcy)++;*pcx=4;softwrap[*pcy]=0;}continue;}
  journal_insert_one(pcx,pcy,c,insert);
 }
}
static int journal_used(int row){int n=NW;while(n>4&&note[row*NW+n-1]==' ')n--;return n-4;}
static void journal_remove_row(int row){int r;for(r=row;r<NL-1;r++){memcpy(note+r*NW+4,note+(r+1)*NW+4,JW);softwrap[r]=softwrap[r+1];}memset(note+(NL-1)*NW+4,' ',JW);softwrap[NL-1]=0;}
static void journal_join_next(int *pcx,int *pcy){int cx=*pcx,cy=*pcy,n,take,room;if(cy>=NL-1)return;n=journal_used(cy+1);room=NW-cx;take=n<room?n:room;if(take)memcpy(note+cy*NW+cx,note+(cy+1)*NW+4,take);if(take>=n)journal_remove_row(cy+1);else{memmove(note+(cy+1)*NW+4,note+(cy+1)*NW+4+take,JW-take);memset(note+(cy+1)*NW+NW-take,' ',take);}}
static void journal_join_previous(int *pcx,int *pcy){int cy=*pcy,prev,n,take,room;if(cy<=2)return;prev=journal_used(cy-1);n=journal_used(cy);room=JW-prev;if(room<=0){*pcy=cy-1;*pcx=NW-1;return;}take=n<room?n:room;if(take)memcpy(note+(cy-1)*NW+4+prev,note+cy*NW+4,take);if(take>=n)journal_remove_row(cy);else{memmove(note+cy*NW+4,note+cy*NW+4+take,JW-take);memset(note+cy*NW+NW-take,' ',take);}*pcy=cy-1;*pcx=4+prev;}
static void journal_split_line(int *pcx,int *pcy){int cx=*pcx,cy=*pcy,r,n,tail;if(cy>=NL-1)return;for(r=NL-1;r>cy+1;r--){memcpy(note+r*NW+4,note+(r-1)*NW+4,JW);softwrap[r]=softwrap[r-1];}memset(note+(cy+1)*NW+4,' ',JW);n=journal_used(cy);tail=n>cx-4?n-(cx-4):0;if(tail)memcpy(note+(cy+1)*NW+4,note+cy*NW+cx,tail);memset(note+cy*NW+cx,' ',NW-cx);softwrap[cy+1]=0;*pcy=cy+1;*pcx=4;}
int main(int argc,char**argv){union REGS q;int x,y,tx,ty,cx=4,cy=2,top=2,key=0,mx=0,my=0,focus=-1,ch,base,insert=1,oldcy,oldtop;unsigned mb=0;char exp[ACC_PATH],msg[ACC_PATH+24];if(acc_help(argc,argv,"!JOURNAL","Daily journal with date navigation."))return 0;if(!acc_begin(argv[0],"Journal",0))return 1;q.h.ah=0x2A;int86(0x21,&q,&q);jy=q.x.cx;jm=q.h.dh;jd=q.h.dl;x=(acc_cols-70)/2;y=(acc_rows-20)/2;tx=x+3;ty=y+2;load();acc_box(x,y,70,20,"Journal");view(tx,ty,cx,cy,top);while(key!=27){
 /* Toolbar: one-cell gap between buttons; separator immediately after Go To. */
 acc_button(x+3,y+17,"  \021  ",focus==1);acc_button(x+10,y+17,"  \020  ",focus==2);acc_button(x+17,y+17,"  Go To  ",focus==3);acc_put(x+28,y+17,179,ACC_BORDER);acc_button(x+30,y+17,"  Chars  ",focus==4);acc_button(x+41,y+17,"  Export  ",focus==5);acc_button(x+49,y+17,"  Print  ",focus==6);acc_button(x+60,y+17,"  Close  ",focus==7);acc_wait(&key,&mx,&my,&mb);
 /* Mouse toolbar activation mirrors keyboard activation. */
 if(mb&1){if(my==y+17){journal_selection_clear();if(mx>=x+2&&mx<x+7){focus=1;key=13;}else if(mx>=x+9&&mx<x+14){focus=2;key=13;}else if(mx>=x+16&&mx<x+25){focus=3;key=13;}else if(mx>=x+29&&mx<x+38){focus=4;key=13;}else if(mx>=x+40&&mx<x+46){focus=5;key=13;}else if(mx>=x+48&&mx<x+54){focus=6;key=13;}else if(mx>=x+60&&mx<x+66){focus=7;key=13;}}}
 if(key==9||key==271){if(focus<0)focus=(key==271)?7:0;else focus=(key==271)?(focus+7)%8:(focus+1)%8;journal_editor_focus=(focus==0);if(focus!=0)journal_selection_clear();view(tx,ty,cx,cy,top);key=0;continue;}
 if(key==13&&focus>0){if(focus==1||focus==2){save();journal_selection_clear();stepday(focus==1?-1:1);load();cx=4;cy=2;top=2;}else if(focus==3){save();journal_selection_clear();if(calpick())load();acc_box(x,y,70,20,"Journal");}else if(focus==4){ch=character_palette();acc_box(x,y,70,20,"Journal");if(ch>=0){if(journal_has_selection())journal_delete_selection(&cx,&cy);note[cy*NW+cx]=(char)ch;if(cx<NW-1)cx++;journal_selection_clear();}}else if(focus==5){save();if(export_all(exp)){sprintf(msg,"Journal exported to\n%s",exp);acc_notice("Export",msg);}}else if(focus==6){FILE*f=fopen("LPT1","wb");int r,q2;if(f){for(r=0;r<NL;r++){q2=NW;while(q2&&note[r*NW+q2-1]==' ')q2--;fwrite(note+r*NW,1,q2,f);fputs("\r\n",f);}fputc('\f',f);fclose(f);}}else key=27;view(tx,ty,cx,cy,top);if(key!=27)key=0;continue;}
 if((mb&1)&&mx>=tx&&mx<=tx+NW&&my>=ty+2&&my<ty+2+NV){focus=0;journal_editor_focus=1;}
 if(focus!=0){if(key!=27)key=0;continue;}
 oldcy=cy;oldtop=top;
 if((mb&1)&&mx==tx+NW&&my>=ty+2&&my<ty+2+NV){int last=last_text_line();if(my==ty+2&&top>2)top--;else if(my==ty+NV+1&&top+NV<=last)top++;cy=top;cx=4;view(tx,ty,cx,cy,top);key=0;continue;}
 if((mb&1)&&mx>=tx&&mx<tx+NW&&my>=ty+2&&my<ty+2+NV){journal_selection_clear();cy=top+(my-(ty+2));if(cy<2)cy=2;cx=mx-tx;if(cx<4)cx=4;key=0;}
 else {int oldpos=journal_pos(cx,cy),shift=journal_shift_down();
  if(key==3){journal_copy_selection();key=0;}
  else if(key==24){journal_copy_selection();journal_delete_selection(&cx,&cy);oldtop=-1;key=0;}
  else if((key==22||key==16)){journal_paste(&cx,&cy,insert);oldtop=-1;key=0;}
  else if(key==256+75&&cx>4){cx--;journal_selection_move(oldpos,journal_pos(cx,cy),shift);}
  else if(key==256+77&&cx<NW-1){cx++;journal_selection_move(oldpos,journal_pos(cx,cy),shift);}
  else if(key==256+72&&cy>2){cy--;journal_selection_move(oldpos,journal_pos(cx,cy),shift);}
  else if(key==256+80&&cy<NL-1){cy++;journal_selection_move(oldpos,journal_pos(cx,cy),shift);}
  else if(key==256+71){cx=4;journal_selection_move(oldpos,journal_pos(cx,cy),shift);}
  else if(key==256+79){cx=NW-1;while(cx>4&&note[cy*NW+cx-1]==' ')cx--;journal_selection_move(oldpos,journal_pos(cx,cy),shift);}
  else if(key==256+73){cy-=NV;if(cy<2)cy=2;journal_selection_move(oldpos,journal_pos(cx,cy),shift);}
  else if(key==256+81){cy+=NV;if(cy>=NL)cy=NL-1;journal_selection_move(oldpos,journal_pos(cx,cy),shift);}
  else if(key==256+82){journal_selection_clear();insert=!insert;}
  else if(key==256+83&&editable(cy)){if(journal_has_selection()){journal_delete_selection(&cx,&cy);oldtop=-1;}else if(cx-4>=journal_used(cy))journal_join_next(&cx,&cy);else{base=cy*NW+cx;memmove(note+base,note+base+1,NW-cx-1);note[cy*NW+NW-1]=' ';}}
  else if(key==8){if(journal_has_selection()){journal_delete_selection(&cx,&cy);oldtop=-1;}else if(cx>4){cx--;base=cy*NW+cx;memmove(note+base,note+base+1,NW-cx-1);note[cy*NW+NW-1]=' ';}else journal_join_previous(&cx,&cy);}
  else if(key==13&&cy<NL-1){if(journal_has_selection()){journal_delete_selection(&cx,&cy);oldtop=-1;}journal_split_line(&cx,&cy);journal_selection_clear();}
  else if(key>=32&&key<=255&&editable(cy)){if(journal_has_selection()){journal_delete_selection(&cx,&cy);oldtop=-1;}journal_insert_one(&cx,&cy,key,insert);journal_selection_clear();}
  else if(key==27)break;else key=0;
 }
 if(journal_sel_repaint){oldtop=-1;journal_sel_repaint=0;}
 if(cy>=top+NV)top=cy-NV+1;if(cy<top)top=cy;if(top<2)top=2;{int last=last_text_line();int max=last-NV+1;if(max<2)max=2;if(top>max)top=max;}
 /* Normal editing is incremental to avoid the visible full-page flash. */
 if(top!=oldtop)view(tx,ty,cx,cy,top);else{jrow(tx,ty,oldcy,top);if(cy!=oldcy)jrow(tx,ty,cy,top);if(cy>=top&&cy<top+NV)acc_put(tx+cx,ty+2+cy-top,note[cy*NW+cx],ACC_SELECT);jscroll(tx,ty,top);}key=0;}save();acc_end();return 0;}
