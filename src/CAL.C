/* Launch! Calendar accessory - calendar and iCalendar viewer. */
#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ACCLIB.H"
#define MAX_EVENTS 256
#define ICS_LINE 256
typedef struct {int y,m,d,all_day,start_min,end_min,recur,interval;char summary[64],location[64];} EVENT;
static EVENT ev[MAX_EVENTS];static EVENT *day_list[MAX_EVENTS];static int ev_count;
static const char *months[12]={"January","February","March","April","May","June","July","August","September","October","November","December"};
static const char *days[7]={"SUN","MON","TUE","WED","THU","FRI","SAT"};static unsigned char calendar_lines[13][57];
static int leap(int y){return y%4==0&&(y%100!=0||y%400==0);}static int mdays(int m,int y){static int d[12]={31,28,31,30,31,30,31,31,30,31,30,31};return m==1&&leap(y)?29:d[m];}
static int weekday(int y,int m,int d){int t[12]={0,3,2,5,0,3,5,1,4,6,2,4};if(m<2)y--;return(y+y/4-y/100+y/400+t[m]+d)%7;}
static void unescape(char *d,const char*s,int n){int i=0;while(*s&&i<n-1){if(*s=='\\'&&s[1]){s++;if(*s=='n'||*s=='N')d[i++]=' ';else if(*s==','||*s==';'||*s=='\\')d[i++]=*s;else d[i++]=*s;s++;}else d[i++]=*s++;}d[i]=0;}
static int parse_dt(const char*s,int*y,int*m,int*d,int*mins,int*allday){const char*p=strchr(s,':');int hh,mm;if(!p)p=s;else p++;if(strlen(p)<8)return 0;*y=p[0]-'0';*y=*y*10+(p[1]-'0');*y=*y*10+(p[2]-'0');*y=*y*10+(p[3]-'0');*m=p[4]-'0';*m=*m*10+(p[5]-'0');*d=p[6]-'0';*d=*d*10+(p[7]-'0');*allday=(strstr(s,"VALUE=DATE")!=0||strlen(p)==8);*mins=0;if(!*allday&&strlen(p)>=13){hh=p[9]-'0';hh=hh*10+(p[10]-'0');mm=p[11]-'0';mm=mm*10+(p[12]-'0');*mins=hh*60+mm;}return 1;}
static void process_prop(EVENT*e,char*line){char*p=strchr(line,':');int yy,mm,dd,mi,ad;if(!p)return;if(!strncmp(line,"DTSTART",7)&&parse_dt(line,&yy,&mm,&dd,&mi,&ad)){e->y=yy;e->m=mm;e->d=dd;e->start_min=mi;e->all_day=ad;}else if(!strncmp(line,"DTEND",5)&&parse_dt(line,&yy,&mm,&dd,&mi,&ad))e->end_min=mi;else if(!strncmp(line,"SUMMARY",7))unescape(e->summary,p+1,sizeof(e->summary));else if(!strncmp(line,"LOCATION",8))unescape(e->location,p+1,sizeof(e->location));else if(!strncmp(line,"RRULE",5)){char*q=p+1;e->interval=1;if(strstr(q,"FREQ=YEARLY"))e->recur=1;else if(strstr(q,"FREQ=MONTHLY"))e->recur=2;q=strstr(q,"INTERVAL=");if(q){int v=atoi(q+9);if(v>0)e->interval=v;}}}
static void ics_line(EVENT *cur,int *in,char *line)
{
 if(!strcmp(line,"BEGIN:VEVENT")){memset(cur,0,sizeof(*cur));cur->interval=1;*in=1;}
 else if(!strcmp(line,"END:VEVENT")){if(*in&&cur->y&&ev_count<MAX_EVENTS)ev[ev_count++]=*cur;*in=0;}
 else if(*in)process_prop(cur,line);
}
static int load_ics(const char*path)
{
 FILE*f;char raw[ICS_LINE],line[ICS_LINE];EVENT cur;int in=0;
 f=fopen(path,"rt");if(!f)return 0;line[0]=0;memset(&cur,0,sizeof(cur));
 while(fgets(raw,sizeof(raw),f)){
   int n=(int)strlen(raw);while(n&&(raw[n-1]=='\r'||raw[n-1]=='\n'))raw[--n]=0;
   if((raw[0]==' '||raw[0]=='\t')&&line[0]){strncat(line,raw+1,sizeof(line)-strlen(line)-1);continue;}
   if(line[0])ics_line(&cur,&in,line);
   strncpy(line,raw,sizeof(line)-1);line[sizeof(line)-1]=0;
 }
 if(line[0])ics_line(&cur,&in,line);
 fclose(f);
 return 1;
}
static int occurs(const EVENT*e,int y,int m,int d){int months;if(e->y==y&&e->m==m+1&&e->d==d)return 1;if(!e->recur)return 0;if(e->recur==1){if(y<e->y||m+1!=e->m||d!=e->d)return 0;return ((y-e->y)%e->interval)==0;}months=(y-e->y)*12+(m+1-e->m);return months>=0&&d==e->d&&(months%e->interval)==0;}
static int has_event(int y,int m,int d){int i;for(i=0;i<ev_count;i++)if(occurs(&ev[i],y,m,d))return 1;return 0;}
static int line_char(int m){switch(m){case 3:return 179;case 12:return 196;case 10:return 218;case 6:return 191;case 9:return 192;case 5:return 217;case 11:return 195;case 7:return 180;case 14:return 194;case 13:return 193;case 15:return 197;}return ' ';}
static void draw_month(int x,int y,int m,int year,int tm,int ty,int td,int fd,int calfocus){char s[66];int r,c,d,i,xx,yy,start=weekday(year,m,1),n=mdays(m,year),gx=x+2,gy=y+4,sel,today;acc_fill(x+1,y+1,59,17,' ',ACC_BG);sprintf(s,"%s %d",months[m],year);acc_text(x+2+(57-(int)strlen(s))/2,y+2,s,ACC_HEADING,(int)strlen(s));for(c=0;c<7;c++)acc_text(gx+c*8+2,y+3,days[c],ACC_LABEL,3);acc_fill(gx,gy,57,13,' ',ACC_CONTROL);memset(calendar_lines,0,sizeof(calendar_lines));for(r=0;r<6;r++)for(c=0;c<7;c++){d=r*7+c-start+1;if(d<1||d>n)continue;xx=c*8;yy=r*2;for(i=0;i<8;i++){calendar_lines[yy][xx+i]|=8;calendar_lines[yy][xx+i+1]|=4;calendar_lines[yy+2][xx+i]|=8;calendar_lines[yy+2][xx+i+1]|=4;}for(i=0;i<2;i++){calendar_lines[yy+i][xx]|=2;calendar_lines[yy+i+1][xx]|=1;calendar_lines[yy+i][xx+8]|=2;calendar_lines[yy+i+1][xx+8]|=1;}}for(yy=0;yy<13;yy++)for(xx=0;xx<57;xx++)if(calendar_lines[yy][xx])acc_put(gx+xx,gy+yy,line_char(calendar_lines[yy][xx]),ACC_CONTROL);for(r=0;r<6;r++)for(c=0;c<7;c++){d=r*7+c-start+1;if(d<1||d>n)continue;sel=calfocus&&d==fd;today=d==td&&m==tm&&year==ty;sprintf(s,"%-2d",d);acc_text(gx+c*8+2,gy+1+r*2,s,sel?ACC_SELECT:(today?ACC_ATTR(acc_appearance.controls_bg,acc_appearance.titles):ACC_CONTROL),2);if(has_event(year,m,d))acc_put(gx+c*8+6,gy+1+r*2,4,sel?ACC_SELECT:ACC_ATTR(acc_appearance.controls_bg,acc_appearance.main_title));}}
static void fmt_time(char*b,int mins){int h=mins/60,m=mins%60,hh=h%12;if(!hh)hh=12;sprintf(b,"%d:%02d%s",hh,m,h<12?"am":"pm");}
static int cmp_event(const void*a,const void*b){const EVENT*x=*(EVENT**)a,*y=*(EVENT**)b;if(x->all_day!=y->all_day)return y->all_day-x->all_day;return x->start_min-y->start_min;}
static void day_shift(int *year,int *mon,int *day,int delta)
{
  *day+=delta;
  if(*day<1){(*mon)--;if(*mon<0){*mon=11;(*year)--;}*day=mdays(*mon,*year);}
  else if(*day>mdays(*mon,*year)){*day=1;(*mon)++;if(*mon>11){*mon=0;(*year)++;}}
}
static void day_dialog(int year,int mon,int day)
{
  int n,i,top=0,key=0,mx=0,my=0,w=58,h=18,x=(acc_cols-58)/2,y=(acc_rows-18)/2,focus=-1;
  unsigned mb=0;char b[90],t1[16],t2[16];
  static char*wd[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
  for(;;){
    n=0;for(i=0;i<ev_count;i++)if(occurs(&ev[i],year,mon,day))day_list[n++]=&ev[i];
    qsort(day_list,n,sizeof(EVENT*),cmp_event);
    acc_subbox(x,y,w,h,"Day",0);
    sprintf(b,"%s, %d %s %d",wd[weekday(year,mon,day)],day,months[mon],year);
    acc_text(x+3,y+2,b,ACC_HEADING,36);
    acc_button(x+w-17,y+2,"  \021  ",focus==0);
    acc_button(x+w-9,y+2,"  \020  ",focus==1);
    acc_tooltip_clear_regions();
    acc_tooltip_region(x+w-17,y+2,5,"Previous day",1);
    acc_tooltip_region(x+w-9,y+2,5,"Next day",1);
    for(i=0;i<11;i++)acc_fill(x+3,y+4+i,51,1,' ',ACC_BG);
    for(i=top;i<n&&i<top+6;i++){
      int yy=y+4+(i-top)*2;
      if(day_list[i]->all_day){
        sprintf(b,"[ %s ]",day_list[i]->summary[0]?day_list[i]->summary:"All day event");
        acc_text(x+3,yy,b,ACC_CONTROL,51);
      }else{
        fmt_time(t1,day_list[i]->start_min);if(day_list[i]->end_min)fmt_time(t2,day_list[i]->end_min);else t2[0]=0;
        acc_put(x+3,yy,218,ACC_LABEL);acc_text(x+6,yy,t1,ACC_LABEL,8);
        acc_text(x+16,yy,day_list[i]->summary,ACC_TEXT,38);
        acc_put(x+3,yy+1,192,ACC_LABEL);acc_put(x+4,yy+1,16,ACC_LABEL);
        if(t2[0])acc_text(x+6,yy+1,t2,ACC_LABEL,8);
        if(day_list[i]->location[0]){acc_text(x+16,yy+1,"@ ",ACC_LABEL,2);acc_text(x+18,yy+1,day_list[i]->location,ACC_LABEL,35);}
      }
    }
    if(top>0)acc_put(x+w-3,y+4,30,ACC_BORDER);if(top+6<n)acc_put(x+w-3,y+15,31,ACC_BORDER);
    acc_wait(&key,&mx,&my,&mb);
    if(key==27)return;
    if((mb&1)&&my==y+2&&mx>=x+w-17&&mx<x+w-12){focus=0;day_shift(&year,&mon,&day,-1);top=0;key=0;continue;}
    if((mb&1)&&my==y+2&&mx>=x+w-9&&mx<x+w-4){focus=1;day_shift(&year,&mon,&day,1);top=0;key=0;continue;}
    if(key==9||key==271){if(focus<0)focus=(key==271)?1:0;else focus=!focus;key=0;continue;}
    if(key==13&&focus>=0){day_shift(&year,&mon,&day,focus==0?-1:1);top=0;key=0;continue;}
    if(key==256+75){day_shift(&year,&mon,&day,-1);top=0;key=0;continue;}
    if(key==256+77){day_shift(&year,&mon,&day,1);top=0;key=0;continue;}
    if(key==256+72&&top>0)top--;else if(key==256+80&&top+6<n)top++;
    else if(key==256+73){top-=6;if(top<0)top=0;}
    else if(key==256+81){top+=6;if(top+6>n)top=n>6?n-6:0;}
    key=0;
  }
}
static void print_rule(FILE*f,int left,int join,int right){int c,i;fputc(left,f);for(c=0;c<7;c++){for(i=0;i<9;i++)fputc(196,f);fputc(c==6?right:join,f);}fputs("\r\n",f);}
static int print_month(int m,int year){FILE*f;char title[32],b[100],t[16];int r,c,d,i,start=weekday(year,m,1),n=mdays(m,year),pad,j;f=fopen("LPT1","wb");if(!f)return 0;sprintf(title,"%s %d",months[m],year);pad=(71-(int)strlen(title))/2;for(i=0;i<pad;i++)fputc(' ',f);fputs(title,f);fputs("\r\n\r\n",f);fputc(' ',f);for(c=0;c<7;c++)fprintf(f,"%-9s%c",days[c],c==6?'\r':' ');fputs("\n",f);print_rule(f,218,194,191);for(r=0;r<6;r++){for(i=0;i<7;i++){for(c=0;c<7;c++){fputc(179,f);d=r*7+c-start+1;if(i==0&&d>=1&&d<=n){sprintf(b,"%d%s",d,has_event(year,m,d)?" \004":"");fprintf(f,"%-9s",b);}else fputs("         ",f);}fputc(179,f);fputs("\r\n",f);}print_rule(f,r==5?192:195,r==5?193:197,r==5?217:180);}fputs("\r\nEvents\r\n------\r\n",f);for(d=1;d<=n;d++)if(has_event(year,m,d)){fprintf(f,"\r\n%d %s %d\r\n",d,months[m],year);for(j=0;j<ev_count;j++)if(occurs(&ev[j],year,m,d)){if(ev[j].all_day)fprintf(f,"  [ %s ]\r\n",ev[j].summary);else{fmt_time(t,ev[j].start_min);fprintf(f,"  %s - %s\r\n",t,ev[j].summary);if(ev[j].location[0])fprintf(f,"           %s\r\n",ev[j].location);}}}fputc('\f',f);fclose(f);return 1;}
int main(int argc,char**argv){union REGS q;int m,yr,tm,ty,td,calday,key=0,x,y,mx=0,my=0,focus=-1,dirty=1,i,d,st;unsigned b=0;char path[ACC_PATH];if(acc_help(argc,argv,"!CAL","Displays, prints and views iCalendar events."))return 0;if(!acc_begin(argv[0],"Calendar",0))return 1;ev_count=0;if(argc>1){for(i=1;i<argc&&ev_count<MAX_EVENTS;i++){if(strchr(argv[i],'\\')||strchr(argv[i],'/')||strchr(argv[i],':')){strncpy(path,argv[i],ACC_PATH-1);path[ACC_PATH-1]=0;}else acc_path(path,"",argv[i]);load_ics(path);}}else{acc_path(path,"","CAL.ICS");load_ics(path);}q.h.ah=0x2A;int86(0x21,&q,&q);tm=q.h.dh-1;ty=q.x.cx;td=q.h.dl;m=tm;yr=ty;calday=td;x=(acc_cols-61)/2;y=(acc_rows-21)/2;acc_box(x,y,61,21,"Calendar");while(key!=27){if(dirty){draw_month(x,y,m,yr,tm,ty,td,calday,focus==0);dirty=0;}acc_button(x+3,y+18,"  \021  ",focus==1);acc_button(x+10,y+18,"  Today  ",focus==2);acc_button(x+21,y+18,"  \020  ",focus==3);acc_button(x+28,y+18,"  Print  ",focus==4);acc_button(x+51,y+18,"  Close  ",focus==5);acc_wait(&key,&mx,&my,&b);
if((b&1)&&my>=y+5&&my<y+17&&mx>=x+2&&mx<x+59){int cc=(mx-(x+2))/8,rr=(my-(y+4))/2;st=weekday(yr,m,1);d=rr*7+cc-st+1;if(d>=1&&d<=mdays(m,yr)){calday=d;focus=0;dirty=1;if(has_event(yr,m,d)){day_dialog(yr,m,d);acc_box(x,y,61,21,"Calendar");dirty=1;}key=0;continue;}}
if((b&1)&&my==y+18){if(mx>=x+2&&mx<x+8)focus=1;else if(mx>=x+9&&mx<x+19)focus=2;else if(mx>=x+20&&mx<x+26)focus=3;else if(mx>=x+27&&mx<x+37)focus=4;else if(mx>=x+51&&mx<x+59)focus=5;key=13;}
if(key==9||key==271){if(focus<0)focus=(key==271)?5:0;else focus=(key==271)?(focus+5)%6:(focus+1)%6;dirty=1;key=0;continue;}
if(key==256+71){m=tm;yr=ty;calday=td;focus=0;dirty=1;key=0;continue;}
if(focus==0&&(key==256+75||key==256+77||key==256+72||key==256+80)){int delta=(key==256+75)?-1:(key==256+77)?1:(key==256+72)?-7:7;day_shift(&yr,&m,&calday,delta);dirty=1;key=0;continue;}
if(key==13){if(focus==0){day_dialog(yr,m,calday);acc_box(x,y,61,21,"Calendar");dirty=1;}else if(focus==1){if(--m<0){m=11;yr--;}if(calday>mdays(m,yr))calday=mdays(m,yr);dirty=1;}else if(focus==2){m=tm;yr=ty;calday=td;dirty=1;}else if(focus==3){if(++m>11){m=0;yr++;}if(calday>mdays(m,yr))calday=mdays(m,yr);dirty=1;}else if(focus==4){if(!print_month(m,yr))acc_notice("Print","Unable to print calendar to LPT1.");}else key=27;if(key!=27)key=0;}else if(key!=27)key=0;}acc_end();return 0;}
