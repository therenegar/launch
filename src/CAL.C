/* Launch! Calendar accessory. */
#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "ACCLIB.H"
static const char *months[12]={"January","February","March","April","May","June","July","August","September","October","November","December"};
static const char *days[7]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
static unsigned char calendar_lines[13][57];
static int leap(int y){return y%4==0&&(y%100!=0||y%400==0);}
static int mdays(int m,int y){static int d[12]={31,28,31,30,31,30,31,31,30,31,30,31};return m==1&&leap(y)?29:d[m];}
static int weekday(int y,int m,int d){int t[12]={0,3,2,5,0,3,5,1,4,6,2,4};if(m<2)y--;return(y+y/4-y/100+y/400+t[m]+d)%7;}
static void print_rule(FILE *f,int left,int join,int right){int c,i;fputc(left,f);for(c=0;c<7;c++){for(i=0;i<9;i++)fputc(196,f);fputc(c==6?right:join,f);}fputs("\r\n",f);}
static int print_month(int m,int year){FILE *f;char title[32];int r,c,d,i,start=weekday(year,m,1),n=mdays(m,year),pad;f=fopen("LPT1","wb");if(!f)return 0;sprintf(title,"%s %d",months[m],year);pad=(71-(int)strlen(title))/2;for(i=0;i<pad;i++)fputc(' ',f);fputs(title,f);fputs("\r\n\r\n",f);fputc(' ',f);for(c=0;c<7;c++)fprintf(f,"%-9s%c",days[c],c==6?'\r':' ');fputs("\n",f);print_rule(f,218,194,191);for(r=0;r<6;r++){for(i=0;i<7;i++){for(c=0;c<7;c++){fputc(179,f);d=r*7+c-start+1;if(i==0&&d>=1&&d<=n)fprintf(f,"%-9d",d);else fputs("         ",f);}fputc(179,f);fputs("\r\n",f);}print_rule(f,r==5?192:195,r==5?193:197,r==5?217:180);}fputc('\f',f);fclose(f);return 1;}
static int line_char(int m){switch(m){case 3:return 179;case 12:return 196;case 10:return 218;case 6:return 191;case 9:return 192;case 5:return 217;case 11:return 195;case 7:return 180;case 14:return 194;case 13:return 193;case 15:return 197;}return ' ';}
static void draw_month(int x,int y,int m,int year,int tm,int ty,int td)
{char s[66];int r,c,d,i,xx,yy,start=weekday(year,m,1),n=mdays(m,year),gx=x+2,gy=y+4,sel;
 acc_fill(x+1,y+1,59,17,' ',ACC_BG);sprintf(s,"%s %d",months[m],year);acc_text(x+2+(57-(int)strlen(s))/2,y+2,s,ACC_HEADING,(int)strlen(s));
 for(c=0;c<7;c++)acc_text(gx+c*8+2,y+3,days[c],ACC_LABEL,3);
 acc_fill(gx,gy,57,13,' ',ACC_CONTROL);
 memset(calendar_lines,0,sizeof(calendar_lines));
 for(r=0;r<6;r++)for(c=0;c<7;c++){d=r*7+c-start+1;if(d<1||d>n)continue;xx=c*8;yy=r*2;for(i=0;i<8;i++){calendar_lines[yy][xx+i]|=8;calendar_lines[yy][xx+i+1]|=4;calendar_lines[yy+2][xx+i]|=8;calendar_lines[yy+2][xx+i+1]|=4;}for(i=0;i<2;i++){calendar_lines[yy+i][xx]|=2;calendar_lines[yy+i+1][xx]|=1;calendar_lines[yy+i][xx+8]|=2;calendar_lines[yy+i+1][xx+8]|=1;}}
 for(yy=0;yy<13;yy++)for(xx=0;xx<57;xx++)if(calendar_lines[yy][xx])acc_put(gx+xx,gy+yy,line_char(calendar_lines[yy][xx]),ACC_CONTROL);
 for(r=0;r<6;r++)for(c=0;c<7;c++){d=r*7+c-start+1;if(d<1||d>n)continue;sel=d==td&&m==tm&&year==ty;sprintf(s,"%-2d",d);acc_text(gx+c*8+2,gy+1+r*2,s,sel?ACC_ATTR(acc_appearance.controls_bg,acc_appearance.titles):ACC_CONTROL,2);}
}
int main(int argc,char **argv)
{union REGS q;int m,yr,tm,ty,td,key=0,x,y,mx=0,my=0,focus=4,dirty=1;unsigned b=0;if(acc_help(argc,argv,"!CAL","Displays and prints a browsable monthly calendar."))return 0;if(!acc_begin(argv[0],"Calendar",0))return 1;q.h.ah=0x2A;int86(0x21,&q,&q);tm=q.h.dh-1;ty=q.x.cx;td=q.h.dl;m=tm;yr=ty;x=(acc_cols-61)/2;y=(acc_rows-21)/2;acc_box(x,y,61,21,"Calendar");
 while(key!=27){if(dirty){draw_month(x,y,m,yr,tm,ty,td);dirty=0;}acc_button(x+3,y+18,"  \021  ",focus==0);acc_button(x+10,y+18,"  Today  ",focus==1);acc_button(x+21,y+18,"  \020  ",focus==2);acc_button(x+28,y+18,"  Print  ",focus==3);acc_button(x+51,y+18,"  Close  ",focus==4);acc_wait(&key,&mx,&my,&b);
  if((b&1)&&my==y+18){if(mx>=x+3&&mx<x+8)focus=0;else if(mx>=x+10&&mx<x+19)focus=1;else if(mx>=x+21&&mx<x+26)focus=2;else if(mx>=x+28&&mx<x+37)focus=3;else if(mx>=x+51&&mx<x+57)focus=4;key=13;}if(key==9||key==271){focus=(key==271)?(focus+4)%5:(focus+1)%5;key=0;}if(key==256+71){m=tm;yr=ty;dirty=1;key=0;}else if(key==256+75){if(--m<0){m=11;yr--;}dirty=1;key=0;}else if(key==256+77){if(++m>11){m=0;yr++;}dirty=1;key=0;}else if(key==13){if(focus==0){if(--m<0){m=11;yr--;}dirty=1;}else if(focus==1){m=tm;yr=ty;dirty=1;}else if(focus==2){if(++m>11){m=0;yr++;}dirty=1;}else if(focus==3){if(!print_month(m,yr))acc_notice("Print","Unable to print calendar to LPT1.");}else key=27;if(key!=27)key=0;}else if(key!=27)key=0;
 }acc_end();return 0;}
