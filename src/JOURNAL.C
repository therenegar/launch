/* Launch! Journal - daily journal accessory. */
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include "ACCLIB.H"
#define NW 63
#define NL 100
#define NV 12
static char note[NW*NL];static unsigned char softwrap[NL];static int jy,jm,jd;
static int leap(int y){return y%4==0&&(y%100!=0||y%400==0);}static int mdays(int m,int y){static int d[12]={31,28,31,30,31,30,31,31,30,31,30,31};return m==2&&leap(y)?29:d[m-1];}
static void stepday(int d){jd+=d;if(jd<1){if(--jm<1){jm=12;jy--;}jd=mdays(jm,jy);}else if(jd>mdays(jm,jy)){jd=1;if(++jm>12){jm=1;jy++;}}}
static int jweekday(int y,int m,int d){int t[12]={0,3,2,5,0,3,5,1,4,6,2,4};if(m<3)y--;return(y+y/4-y/100+y/400+t[m-1]+d)%7;}
static void fname(char*n){sprintf(n,"J%04d%02d%02d.DAT",jy,jm,jd);}static void wrapname(char*n){sprintf(n,"J%04d%02d%02d.WRP",jy,jm,jd);}static void marks(void){int r;char b[52];static char*m[]={"","January","February","March","April","May","June","July","August","September","October","November","December"};static char*w[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};for(r=0;r<NL;r++){note[r*NW]=(char)196;note[r*NW+1]=(char)9;note[r*NW+2]=note[r*NW+3]=' ';}memset(note,' ',NW*2);sprintf(b,"%s, %s %d %d",w[jweekday(jy,jm,jd)],m[jm],jd,jy);memcpy(note+4,b,strlen(b));for(r=4;r<NW;r++)note[NW+r]=(char)215;}
static int editable(int l){return l>1;}static void load(void){char p[ACC_PATH],n[20];FILE*f;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));fname(n);acc_path(p,"DATA",n);f=fopen(p,"rb");if(f){fread(note,1,sizeof(note),f);fclose(f);}wrapname(n);acc_path(p,"DATA",n);f=fopen(p,"rb");if(f){fread(softwrap,1,sizeof(softwrap),f);fclose(f);}marks();}
static void save(void){char p[ACC_PATH],n[20];FILE*f;fname(n);acc_path(p,"DATA",n);f=fopen(p,"wb");if(f){fwrite(note,1,sizeof(note),f);fclose(f);}wrapname(n);acc_path(p,"DATA",n);f=fopen(p,"wb");if(f){fwrite(softwrap,1,sizeof(softwrap),f);fclose(f);}acc_path(p,"DATA","JOURNAL.IDX");f=fopen(p,"a");if(f){fprintf(f,"%04d%02d%02d\n",jy,jm,jd);fclose(f);}}
static int last_text_line(void){int l,q;for(l=NL-1;l>=2;l--){for(q=NW-1;q>=4;q--)if(note[l*NW+q]!=' ')return l;}return 2;}
static void jrow(int x,int y,int line,int top){if(line>=top&&line<top+NV)acc_text(x,y+2+line-top,note+line*NW,ACC_CONTROL,NW);}
static void jscroll(int x,int y,int top){int last=last_text_line();acc_fill(x+NW,y,1,NV+2,' ',ACC_CONTROL);if(top>2)acc_put(x+NW,y+2,30,ACC_CONTROL);if(top+NV<=last)acc_put(x+NW,y+NV+1,31,ACC_CONTROL);}
static void view(int x,int y,int cx,int cy,int top){int r;char b[64];static char*m[]={"","January","February","March","April","May","June","July","August","September","October","November","December"};static char*w[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};sprintf(b,"%s, %s %d %d",w[jweekday(jy,jm,jd)],m[jm],jd,jy);
 /* The notebook paper continues behind the fixed date and ruled line. */
 acc_fill(x,y,NW+1,NV+2,' ',ACC_CONTROL);acc_fill(x+NW+1,y,1,NV+2,' ',ACC_BG);
 acc_put(x,y,196,ACC_CONTROL);acc_put(x+1,y,9,ACC_CONTROL);acc_put(x,y+1,196,ACC_CONTROL);acc_put(x+1,y+1,9,ACC_CONTROL);
 acc_text(x+4,y,b,ACC_CONTROL,(int)strlen(b));for(r=4;r<NW;r++)acc_put(x+r,y+1,215,ACC_ATTR(acc_appearance.controls_bg,acc_appearance.main_title));for(r=0;r<NV;r++)jrow(x,y,top+r,top);if(cy>=top&&cy<top+NV)acc_put(x+cx,y+2+cy-top,note[cy*NW+cx],ACC_SELECT);jscroll(x,y,top);}
static int calpick(void)
{
 static char *mn[]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
 int w=26,h=11,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,m=jm,yr=jy,sel=jd,r,c,first,n,focus=1;unsigned b=0;
 for(;;){char t[16];
  /* Compact date picker: no Launch! title and no toolbar divider. */
  acc_subbox(x,y,w,h,"Go To",0);sprintf(t,"%s %04d",mn[m],yr);
  acc_put(x+3,y+2,17,focus==0?ACC_SELECT:ACC_BORDER);acc_text(x+(w-(int)strlen(t))/2,y+2,t,ACC_HEADING,(int)strlen(t));acc_put(x+w-4,y+2,16,focus==2?ACC_SELECT:ACC_BORDER);
  acc_text(x+3,y+3,"Su Mo Tu We Th Fr Sa",ACC_LABEL,20);
  {static int tt[]={0,3,2,5,0,3,5,1,4,6,2,4};int yy=yr-(m<3);first=(yy+yy/4-yy/100+yy/400+tt[m-1]+1)%7;}n=mdays(m,yr);if(sel>n)sel=n;
  for(r=0;r<6;r++)for(c=0;c<7;c++){int d=r*7+c-first+1;if(d>=1&&d<=n){char q[3];sprintf(q,"%2d",d);acc_text(x+3+c*3,y+4+r,q,(d==sel&&focus==1)?ACC_SELECT:ACC_BORDER,2);}}
  acc_shadow(x,y,w,h);acc_wait(&k,&mx,&my,&b);if(k==27)return 0;
  if(k==9||k==271){focus=(focus+(k==271?2:1))%3;}
  else if(focus==1&&k==256+75){if(sel>1)sel--;}
  else if(focus==1&&k==256+77){if(sel<n)sel++;}
  else if(focus==1&&k==256+72){if(sel>7)sel-=7;}
  else if(focus==1&&k==256+80){if(sel+7<=n)sel+=7;}
  else if((k==13||k==' ')&&focus==0){if(--m<1){m=12;yr--;}sel=1;}
  else if((k==13||k==' ')&&focus==2){if(++m>12){m=1;yr++;}sel=1;}
  else if((k==13||k==' ')&&focus==1){jy=yr;jm=m;jd=sel;return 1;}
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
 palette_build();index=palette_index_of(128);
 acc_subbox(x,y,w,h,"Characters",0);
 for(i=0;i<palette_count;i++)palette_draw_cell(x,y,i,0);
 palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);
 acc_shadow(x,y,w,h);
 for(;;){
  acc_wait(&key,&mx,&my,&mb);
  if(mb&ACC_MOUSE_MOVED){
   if(mx>=x+2&&mx<x+34&&my>=y+2&&my<y+10){int n=(my-(y+2))*32+(mx-(x+2));if(n<palette_count&&n!=index){old=index;index=n;palette_draw_cell(x,y,old,0);palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);}}
   key=0;continue;
  }
  if(mb&1){if(mx>=x+2&&mx<x+34&&my>=y+2&&my<y+10){int n=(my-(y+2))*32+(mx-(x+2));if(n<palette_count)return palette_codes[n];}else if(mx<x||mx>=x+w||my<y||my>=y+h)return -1;continue;}
  if(key==27)return -1;old=index;
  if(key==256+75){if(index>0)index--;}
  else if(key==256+77){if(index+1<palette_count)index++;}
  else if(key==256+72){index-=32;if(index<0)index=old;}
  else if(key==256+80){if(index+32<palette_count)index+=32;}
  else if(key==13||key==' ')return palette_codes[index];
  else continue;
  if(index!=old){palette_draw_cell(x,y,old,0);palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);}
 }
}


int main(int argc,char**argv){union REGS q;int x,y,tx,ty,cx=4,cy=2,top=2,key=0,mx=0,my=0,focus=0,ch,base,insert=1,oldcy,oldtop;unsigned mb=0;char exp[ACC_PATH],msg[ACC_PATH+24];if(acc_help(argc,argv,"!JOURNAL","Daily journal with date navigation."))return 0;if(!acc_begin(argv[0],"Journal",0))return 1;q.h.ah=0x2A;int86(0x21,&q,&q);jy=q.x.cx;jm=q.h.dh;jd=q.h.dl;x=(acc_cols-70)/2;y=(acc_rows-20)/2;tx=x+3;ty=y+2;load();acc_box(x,y,70,20,"Journal");view(tx,ty,cx,cy,top);while(key!=27){
 /* Toolbar: one-cell gap between buttons; separator immediately after Go To. */
 acc_button(x+3,y+17,"  \021  ",focus==1);acc_button(x+10,y+17,"  \020  ",focus==2);acc_button(x+17,y+17,"  Go To  ",focus==3);acc_put(x+28,y+17,179,ACC_BORDER);acc_button(x+30,y+17,"  Chars  ",focus==4);acc_button(x+41,y+17,"  Export  ",focus==5);acc_button(x+49,y+17,"  Print  ",focus==6);acc_button(x+60,y+17,"  Close  ",focus==7);acc_wait(&key,&mx,&my,&mb);
 /* Mouse toolbar activation mirrors keyboard activation. */
 if(mb&1){if(my==y+17){if(mx>=x+2&&mx<x+7){focus=1;key=13;}else if(mx>=x+9&&mx<x+14){focus=2;key=13;}else if(mx>=x+16&&mx<x+25){focus=3;key=13;}else if(mx>=x+29&&mx<x+38){focus=4;key=13;}else if(mx>=x+40&&mx<x+46){focus=5;key=13;}else if(mx>=x+48&&mx<x+54){focus=6;key=13;}else if(mx>=x+60&&mx<x+66){focus=7;key=13;}}}
 if(key==9||key==271){focus=(key==271)?((focus+6)%7)+1:(focus%7)+1;key=0;continue;}
 if((key==13||key==' ')&&focus){if(focus==1||focus==2){save();stepday(focus==1?-1:1);load();cx=4;cy=2;top=2;}else if(focus==3){save();if(calpick())load();acc_box(x,y,70,20,"Journal");}else if(focus==4){ch=character_palette();acc_box(x,y,70,20,"Journal");if(ch>=0){note[cy*NW+cx]=(char)ch;if(cx<NW-1)cx++;}}else if(focus==5){save();if(export_all(exp)){sprintf(msg,"Journal exported to\n%s",exp);acc_notice("Export",msg);}}else if(focus==6){FILE*f=fopen("LPT1","wb");int r,q2;if(f){for(r=0;r<NL;r++){q2=NW;while(q2&&note[r*NW+q2-1]==' ')q2--;fwrite(note+r*NW,1,q2,f);fputs("\r\n",f);}fputc('\f',f);fclose(f);}}else key=27;view(tx,ty,cx,cy,top);if(key!=27)key=0;continue;}
 if(focus)focus=0;
 oldcy=cy;oldtop=top;
 if((mb&1)&&mx==tx+NW&&my>=ty+2&&my<ty+2+NV){int last=last_text_line();if(my==ty+2&&top>2)top--;else if(my==ty+NV+1&&top+NV<=last)top++;cy=top;cx=4;view(tx,ty,cx,cy,top);key=0;continue;}
 if((mb&1)&&mx>=tx&&mx<tx+NW&&my>=ty+2&&my<ty+2+NV){cy=top+(my-(ty+2));if(cy<2)cy=2;cx=mx-tx;if(cx<4)cx=4;key=0;}
 else if(key==256+75&&cx>4)cx--;else if(key==256+77&&cx<NW-1)cx++;else if(key==256+72&&cy>2)cy--;else if(key==256+80&&cy<NL-1)cy++;
 else if(key==256+71)cx=4;else if(key==256+79){cx=NW-1;while(cx>4&&note[cy*NW+cx-1]==' ')cx--;}
 else if(key==256+73){cy-=NV;if(cy<2)cy=2;}else if(key==256+81){cy+=NV;if(cy>=NL)cy=NL-1;}else if(key==256+82)insert=!insert;
 else if(key==256+83&&editable(cy)){base=cy*NW+cx;memmove(note+base,note+base+1,NW-cx-1);note[cy*NW+NW-1]=' ';}
 else if(key==8){if(cx>4){cx--;base=cy*NW+cx;memmove(note+base,note+base+1,NW-cx-1);note[cy*NW+NW-1]=' ';}else if(cy>2&&softwrap[cy]){softwrap[cy]=0;cy--;cx=NW;while(cx>4&&note[cy*NW+cx-1]==' ')cx--;if(cx>4){cx--;base=cy*NW+cx;memmove(note+base,note+base+1,NW-cx-1);note[cy*NW+NW-1]=' ';}}}
 else if(key==13&&cy<NL-1){cy++;cx=4;softwrap[cy]=0;}else if(key>=32&&key<=255&&editable(cy)){base=cy*NW+cx;if(insert&&cx<NW-1)memmove(note+base+1,note+base,NW-cx-1);note[base]=(char)key;if(cx<NW-1)cx++;else if(cy<NL-1){cy++;cx=4;softwrap[cy]=1;}}else if(key==27)break;else key=0;
 if(cy>=top+NV)top=cy-NV+1;if(cy<top)top=cy;if(top<2)top=2;{int last=last_text_line();int max=last-NV+1;if(max<2)max=2;if(top>max)top=max;}
 /* Normal editing is incremental to avoid the visible full-page flash. */
 if(top!=oldtop)view(tx,ty,cx,cy,top);else{jrow(tx,ty,oldcy,top);if(cy!=oldcy)jrow(tx,ty,cy,top);if(cy>=top&&cy<top+NV)acc_put(tx+cx,ty+2+cy-top,note[cy*NW+cx],ACC_SELECT);jscroll(tx,ty,top);}key=0;}save();acc_end();return 0;}
