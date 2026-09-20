/* Launch! To-Dos accessory. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <stdlib.h>
#include "ACCLIB.H"

#define MAX_ITEMS 64
#define TITLE_LEN 38
#define TAG_LEN 12
#define DATE_LEN 10
#define DATE_INPUT_LEN 31
#define NOTE_LEN 120
#define VIEW_ROWS 15
#define MAX_TAGS 8

#define GLYPH_U_L 212
#define GLYPH_U_R 189
#define GLYPH_C_L 213
#define GLYPH_C_R 190

typedef struct {
  unsigned char used,type,done,priority;
  char title[TITLE_LEN+1];
  char tag[TAG_LEN+1];
  char due[DATE_LEN+1];
  char note[NOTE_LEN+1];
} TODO_ITEM;

static TODO_ITEM items[MAX_ITEMS];
static int item_count=0;
static int row_map[VIEW_ROWS];
static unsigned char row_detail[VIEW_ROWS];
static void install_glyphs(void){}
static void restore_glyphs(void){}

static int ctrl_down(void)
{
  unsigned char far *p=(unsigned char far *)(((unsigned long)0x40<<16)|0x17);
  return ((*p)&4)!=0;
}

static int days_in_month(int y,int m)
{
  static const int d[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  int n=d[m-1];if(m==2&&((y%4==0&&y%100!=0)||y%400==0))n++;return n;
}

static long date_serial(int y,int m,int d)
{
  long a=(14-m)/12,yy=(long)y+4800-a,mm=(long)m+12*a-3;
  return (long)d+(153*mm+2)/5+365*yy+yy/4-yy/100+yy/400-32045;
}

static void serial_date(long z,int *y,int *m,int *d)
{
  long a=z+32044,b=(4*a+3)/146097,c=a-(146097*b)/4,e=(4*c+3)/1461,f=c-(1461*e)/4,g=(5*f+2)/153;
  *d=(int)(f-(153*g+2)/5+1);*m=(int)(g+3-12*(g/10));*y=(int)(100*b+e-4800+g/10);
}

static int valid_date(int y,int m,int d)
{
  return y>=1980&&y<=2099&&m>=1&&m<=12&&d>=1&&d<=days_in_month(y,m);
}

static int month_name(const char *s)
{
  static const char *mon[12]={"jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"};int i;
  for(i=0;i<12;i++)if(!strnicmp(s,mon[i],3))return i+1;return 0;
}

static int weekday_name(const char *s)
{
  static const char *wd[7]={"sunday","monday","tuesday","wednesday","thursday","friday","saturday"};int i;
  for(i=0;i<7;i++)if(!stricmp(s,wd[i])||!strnicmp(s,wd[i],3))return i;return -1;
}

static int country_date_order(char *sep)
{
  /* INT 21h/AH=38h returns the country-information block in DS:DX.
     TODOS.C uses the medium memory model, so ordinary data pointers are
     near pointers in DGROUP and DOS must receive the existing DS plus the
     near offset.  The previous intdosx()/FP_SEG(info) implementation could
     supply the wrong segment and silently fall back to US ordering. */
  unsigned char info[34];
  union REGS r;
  int order=1;

  memset(info,0,sizeof(info));
  memset(&r,0,sizeof(r));
  r.h.ah=0x38;
  r.h.al=0;
  r.x.dx=(unsigned)info;
  intdos(&r,&r);

  if(!r.x.cflag){
    order=(int)(info[0]|((unsigned)info[1]<<8));
    *sep=info[11]?(char)info[11]:'/';
  } else *sep='/';

  if(order<0||order>2)order=1;
  return order;
}

static int parse_date_text(const char *src,char out[DATE_LEN+1])
{
  char s[64],*p,*q;struct dosdate_t now;int y,m,d,wd,target,delta,a,b,c,n,order;long z;
  strncpy(s,src,sizeof(s)-1);s[sizeof(s)-1]=0;
  p=s;while(*p&&isspace((unsigned char)*p))p++;memmove(s,p,strlen(p)+1);for(n=(int)strlen(s);n>0&&isspace((unsigned char)s[n-1]);n--)s[n-1]=0;
  for(p=s;*p;p++)*p=(char)tolower((unsigned char)*p);
  if(!s[0]){out[0]=0;return 1;}
  _dos_getdate(&now);y=now.year;m=now.month;d=now.day;wd=now.dayofweek;
  if(!strcmp(s,"today")){}
  else if(!strcmp(s,"tomorrow")){z=date_serial(y,m,d)+1;serial_date(z,&y,&m,&d);}
  else if(!strcmp(s,"next month")){m++;if(m>12){m=1;y++;}if(d>days_in_month(y,m))d=days_in_month(y,m);}
  else {
    p=s;if(!strncmp(p,"next ",5))p+=5;target=weekday_name(p);
    if(target>=0){delta=(target-wd+7)%7;if(delta==0)delta=7;z=date_serial(y,m,d)+delta;serial_date(z,&y,&m,&d);}
    else {
      /* ISO form first. */
      if(sscanf(s,"%d-%d-%d",&a,&b,&c)==3&&a>1900){y=a;m=b;d=c;}
      else {
        /* 15 Sep, 15 September, optional year. */
        q=s;while(*q&&isdigit((unsigned char)*q))q++;if(q>s){a=atoi(s);while(*q&&(*q==' '||*q=='-'||*q=='/'))q++;b=month_name(q);if(b){while(*q&&isalpha((unsigned char)*q))q++;while(*q&&isspace((unsigned char)*q))q++;c=*q?atoi(q):y;d=a;m=b;y=c;}else b=0;}else b=0;
        if(!b){
          char sep;order=country_date_order(&sep);a=b=c=0;
          if(sscanf(s,"%d/%d/%d",&a,&b,&c)<2&&sscanf(s,"%d-%d-%d",&a,&b,&c)<2&&sscanf(s,"%d.%d.%d",&a,&b,&c)<2)return 0;
          if(c==0)c=y;else if(c<100)c+=2000;
          if(order==0){m=a;d=b;y=c;}else if(order==2){y=(a<100?a+2000:a);m=b;d=c;}else{d=a;m=b;y=c;}
        }
      }
    }
  }
  if(!valid_date(y,m,d))return 0;sprintf(out,"%04d-%02d-%02d",y,m,d);return 1;
}

static void display_date(const char *iso,char out[11])
{
  int y,m,d,order;char sep='/';if(sscanf(iso,"%d-%d-%d",&y,&m,&d)!=3){strncpy(out,iso,10);out[10]=0;return;}order=country_date_order(&sep);
  if(order==0)sprintf(out,"%02d%c%02d%c%04d",m,sep,d,sep,y);
  else if(order==2)sprintf(out,"%04d%c%02d%c%02d",y,sep,m,sep,d);
  else sprintf(out,"%02d%c%02d%c%04d",d,sep,m,sep,y);
}

static void defaults(void)
{
  memset(items,0,sizeof(items));item_count=3;
  items[0].used=1;items[0].type=0;strcpy(items[0].title,"Getting started");
  items[1].used=1;items[1].type=1;items[1].priority=1;strcpy(items[1].title,"Add your first task");strcpy(items[1].tag,"Launch");
  items[2].used=1;items[2].type=1;items[2].priority=2;strcpy(items[2].title,"Press E to edit a task");strcpy(items[2].note,"Tasks can carry a due date, tag, priority and a longer description. Space marks a task done.");
}

static void load_items(void)
{
  char p[ACC_PATH];FILE *f;int n;
  defaults();acc_path(p,"DATA","TODOS.DAT");f=fopen(p,"rb");if(!f)return;
  if(fread(&n,sizeof(n),1,f)==1&&n>=0&&n<=MAX_ITEMS&&fread(items,sizeof(items),1,f)==1)item_count=n;
  fclose(f);
}
static void save_items(void)
{
  char p[ACC_PATH];FILE *f;acc_path(p,"DATA","TODOS.DAT");f=fopen(p,"wb");if(!f)return;
  fwrite(&item_count,sizeof(item_count),1,f);fwrite(items,sizeof(items),1,f);fclose(f);
}

static void print_rule(FILE *f){int i;for(i=0;i<72;i++)fputc('-',f);fputs("\r\n",f);}
static void print_wrapped(FILE *f,const char *text,int indent)
{
  char line[73];int pos=0,n,i,width=72-indent;const char *p=text,*start,*end;
  if(width<20)width=20;
  while(*p){while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n')p++;if(!*p)break;start=p;end=p;n=0;
    while(*p&&*p!='\r'&&*p!='\n'&&n<width){if(*p==' ')end=p;p++;n++;}
    if(n>=width&&*p&&end>start)p=end;else end=p;
    while(end>start&&end[-1]==' ')end--;for(i=0;i<indent;i++)line[pos++]=' ';n=(int)(end-start);if(n>72-pos)n=72-pos;memcpy(line+pos,start,n);pos+=n;line[pos]=0;fputs(line,f);fputs("\r\n",f);pos=0;
    if(*p=='\r')p++;if(*p=='\n')p++;
  }
}
static int build_visible(int *map,const char *filter,int hide_done,int sort_mode);
static int print_tasks(const char *filter,int hide_done,int sort_mode)
{
  FILE *f=fopen("LPT1","wb");int map[MAX_ITEMS],n,i,idx,have_group=0;char due[11],meta[96];
  if(!f)return 0;
  n=build_visible(map,filter,hide_done,sort_mode);
  fputs("TO-DO LIST\r\n",f);print_rule(f);
  for(i=0;i<n;i++){
    idx=map[i];if(!items[idx].used)continue;
    if(items[idx].type==0){if(have_group){fputs("\r\n",f);print_rule(f);}fputs(items[idx].title,f);fputs("\r\n",f);print_rule(f);have_group=1;continue;}
    fprintf(f,"[%c] %s\r\n",items[idx].done?'X':' ',items[idx].title);
    meta[0]=0;
    if(items[idx].due[0]){display_date(items[idx].due,due);sprintf(meta,"Due: %s",due);}
    if(items[idx].tag[0]){if(meta[0])strcat(meta,"   ");strcat(meta,"Tag: ");strcat(meta,items[idx].tag);}
    if(items[idx].priority){char pbuf[24];sprintf(pbuf,"Priority: %d",(int)items[idx].priority);if(meta[0])strcat(meta,"   ");strcat(meta,pbuf);}
    if(meta[0]){fputs("    ",f);fputs(meta,f);fputs("\r\n",f);}
    if(items[idx].note[0])print_wrapped(f,items[idx].note,4);
    fputs("\r\n",f);
  }
  fputc('\f',f);fclose(f);return 1;
}

static int field_key(char *buf,int maxlen,int key,int *pos)
{
  int len=(int)strlen(buf),ch;
  if(*pos>len)*pos=len;
  if(key==256+75&&*pos>0){(*pos)--;return 1;}
  if(key==256+77&&*pos<len){(*pos)++;return 1;}
  if(key==256+71){*pos=0;return 1;}
  if(key==256+79){*pos=len;return 1;}
  if(key==256+83&&*pos<len){memmove(buf+*pos,buf+*pos+1,len-*pos);return 1;}
  if(key==8&&*pos>0){memmove(buf+*pos-1,buf+*pos,len-*pos+1);(*pos)--;return 1;}
  if((key>=32&&key<=255)||(key>=513&&key<=767)){
    ch=key>=512?key-512:key;
    if(len<maxlen){memmove(buf+*pos+1,buf+*pos,len-*pos+1);buf[*pos]=(char)ch;(*pos)++;return 1;}
  }
  return 0;
}

static void form_field(int x,int y,int labelw,const char *label,const char *value,int width,int active,int cursor)
{
  int attr=ACC_ATTR(acc_appearance.controls_bg,0),len=(int)strlen(value),left=0,show;
  acc_text(x,y,label,ACC_LABEL,labelw);
  acc_fill(x+labelw,y,width,1,' ',active?ACC_SELECT:attr);
  if(cursor>=width)left=cursor-width+1;
  show=len-left;if(show>width)show=width;
  if(show>0)acc_text(x+labelw,y,value+left,active?ACC_SELECT:attr,show);
  if(active&&cursor-left>=0&&cursor-left<width)acc_put(x+labelw+cursor-left,y,(cursor<len)?value[cursor]:' ',ACC_SELECT);
}

static void note_area(int x,int y,const char *value,int active,int cursor)
{
  int attr=ACC_ATTR(acc_appearance.controls_bg,0),i,row,col,n=(int)strlen(value),p=0;char line[41];
  for(row=0;row<3;row++){
    for(col=0;col<40;col++)line[col]=' ';line[40]=0;
    col=0;while(p<n&&col<40&&value[p]!='\n')line[col++]=value[p++];if(p<n&&value[p]=='\n')p++;
    acc_text(x,y+row,line,active?ACC_SELECT:attr,40);
  }
  if(active){
    row=0;col=0;for(i=0;i<cursor&&i<n;i++){if(value[i]=='\n'||col>=39){row++;col=0;}else col++;if(row>2){row=2;col=39;break;}}
    if(row<3)acc_put(x+col,y+row,(cursor<n&&value[cursor]!='\n')?value[cursor]:' ',ACC_SELECT);
  }
}

static void date_example(char *out)
{
  int order;char sep='/';order=country_date_order(&sep);
  if(order==0)sprintf(out,"e.g. 12%c30%c2026, Tomorrow, Monday, Next Fri",sep,sep);
  else if(order==2)sprintf(out,"e.g. 2026%c12%c30, Tomorrow, Monday, Next Fri",sep,sep);
  else sprintf(out,"e.g. 30%c12%c2026, Tomorrow, Monday, Next Fri",sep,sep);
}

static void draw_priority(int x,int y,int active,unsigned char priority)
{
  char text[8];int attr=active?ACC_SELECT:ACC_ATTR(acc_appearance.controls_bg,0);
  if(priority)sprintf(text,"  %d   %c",(int)priority,(char)175);else sprintf(text,"  -   %c",(char)175);
  acc_fill(x,y,7,1,' ',attr);acc_text(x,y,text,attr,7);
}
static void draw_priority_preview(int x,int y,unsigned char priority)
{
  int fg=acc_appearance.controls_bg;
  if(priority==1)fg=12;else if(priority==2)fg=11;else if(priority==3)fg=1;
  acc_put(x,y,GLYPH_U_L,ACC_ATTR(acc_appearance.background,fg));
  acc_put(x+1,y,GLYPH_U_R,ACC_ATTR(acc_appearance.background,fg));
}

static void redraw_task_field(int x,int y,int field,const TODO_ITEM *tmp,const char *datebuf,int pos)
{
  if(field==0)form_field(x+3,y+4,13,"Title",tmp->title,40,1,pos);
  else if(field==1)form_field(x+3,y+6,13,"Due date",datebuf,24,1,pos);
  else if(field==2)form_field(x+3,y+9,13,"Tag",tmp->tag,18,1,pos);
  else if(field==3){draw_priority(x+16,y+11,1,tmp->priority);draw_priority_preview(x+24,y+11,tmp->priority);}
  else if(field==4)note_area(x+16,y+13,tmp->note,1,pos);
  else if(field==5)acc_button(x+2,y+16,"  OK  ",1);
  else if(field==6)acc_button(x+10,y+16," Cancel ",1);
}

static void redraw_task_field_off(int x,int y,int field,const TODO_ITEM *tmp,const char *datebuf,int pos)
{
  if(field==0)form_field(x+3,y+4,13,"Title",tmp->title,40,0,pos);
  else if(field==1)form_field(x+3,y+6,13,"Due date",datebuf,24,0,pos);
  else if(field==2)form_field(x+3,y+9,13,"Tag",tmp->tag,18,0,pos);
  else if(field==3){draw_priority(x+16,y+11,0,tmp->priority);draw_priority_preview(x+24,y+11,tmp->priority);}
  else if(field==4)note_area(x+16,y+13,tmp->note,0,pos);
  else if(field==5)acc_button(x+2,y+17,"  OK  ",0);
  else if(field==6)acc_button(x+10,y+17," Cancel ",0);
}

static void draw_task_form(int x,int y,int w,int h,const TODO_ITEM *tmp,const char *datebuf,int field,int pos,const char *group)
{
  char hint[64],grp[64];acc_subbox(x,y,w,h,"Task",1);
  sprintf(grp,"in group: %s",(group&&*group)?group:"(none)");acc_text(x+3,y+2,grp,ACC_HEADING,56);
  form_field(x+3,y+4,13,"Title",tmp->title,40,field==0,pos);
  form_field(x+3,y+6,13,"Due date",datebuf,24,field==1,pos);date_example(hint);acc_text(x+16,y+7,hint,ACC_TEXT,43);
  form_field(x+3,y+9,13,"Tag",tmp->tag,18,field==2,pos);
  acc_text(x+3,y+11,"Priority",ACC_LABEL,13);draw_priority(x+16,y+11,field==3,tmp->priority);draw_priority_preview(x+24,y+11,tmp->priority);
  acc_text(x+3,y+13,"Description",ACC_LABEL,13);note_area(x+16,y+13,tmp->note,field==4,pos);
  acc_button(x+2,y+17,"  OK  ",field==5);acc_button(x+10,y+17," Cancel ",field==6);
}

static int edit_task(TODO_ITEM *t,const char *group)
{
  TODO_ITEM tmp=*t;int w=62,h=20,x=(acc_cols-w)/2,y=(acc_rows-h)/2,key=0,mx=0,my=0,field=-1,oldfield,pos=0,len;unsigned mb=0;char parsed[DATE_LEN+1],datebuf[DATE_INPUT_LEN+1];
  if(tmp.due[0])display_date(tmp.due,datebuf);else datebuf[0]=0;
  draw_task_form(x,y,w,h,&tmp,datebuf,field,pos,group);
  while(key!=27){
    acc_wait(&key,&mx,&my,&mb);
    if(key==27)return 0;
    oldfield=field;
    if((mb&1)){
      if(my==y+4&&mx>=x+16&&mx<x+56){field=0;pos=(int)strlen(tmp.title);}
      else if(my==y+6&&mx>=x+16&&mx<x+40){field=1;pos=(int)strlen(datebuf);}
      else if(my==y+9&&mx>=x+16&&mx<x+34){field=2;pos=(int)strlen(tmp.tag);}
      else if(my==y+11&&mx>=x+16&&mx<x+23){field=3;if(mx<x+19){if(tmp.priority==0)tmp.priority=3;else tmp.priority--;}else{tmp.priority=(unsigned char)((tmp.priority+1)%4);}redraw_task_field(x,y,field,&tmp,datebuf,pos);key=0;continue;}
      else if(my>=y+13&&my<=y+15&&mx>=x+16&&mx<x+56){field=4;pos=(int)strlen(tmp.note);}
      else if(my==y+h-3&&mx>=x+2&&mx<x+8){field=5;key=13;}
      else if(my==y+h-3&&mx>=x+10&&mx<x+16){field=6;key=13;}
      if(field!=oldfield){redraw_task_field_off(x,y,oldfield,&tmp,datebuf,pos);redraw_task_field(x,y,field,&tmp,datebuf,pos);}
      if(key!=13){key=0;continue;}
    }
    if(key==9||key==271){if(field>=0)redraw_task_field_off(x,y,field,&tmp,datebuf,pos);if(field<0)field=(key==271)?6:0;else field=(key==271)?(field+6)%7:(field+1)%7;pos=0;if(field==0)pos=(int)strlen(tmp.title);else if(field==1)pos=(int)strlen(datebuf);else if(field==2)pos=(int)strlen(tmp.tag);else if(field==4)pos=(int)strlen(tmp.note);redraw_task_field(x,y,field,&tmp,datebuf,pos);key=0;continue;}
    if(field==3&&(key==256+75||key==256+77||key==' '||key==13)){
      if(key==256+75){if(tmp.priority==0)tmp.priority=3;else tmp.priority--;}else tmp.priority=(unsigned char)((tmp.priority+1)%4);redraw_task_field(x,y,field,&tmp,datebuf,pos);key=0;continue;
    }
    if(field==4&&key==13){len=(int)strlen(tmp.note);if(len<NOTE_LEN){memmove(tmp.note+pos+1,tmp.note+pos,len-pos+1);tmp.note[pos++]='\n';redraw_task_field(x,y,field,&tmp,datebuf,pos);}key=0;continue;}
    if(key==13){
      if(field<0){key=0;continue;}
      if(field==5){if(datebuf[0]&&!parse_date_text(datebuf,parsed)){acc_notice("Date","Date not understood. Try tomorrow, next Wednesday, 15 Sep or 15/09.");draw_task_form(x,y,w,h,&tmp,datebuf,field,pos,group);key=0;continue;}if(datebuf[0])strcpy(tmp.due,parsed);else tmp.due[0]=0;*t=tmp;return 1;}
      if(field==6)return 0;redraw_task_field_off(x,y,field,&tmp,datebuf,pos);field++;if(field>6)field=0;pos=0;if(field==0)pos=(int)strlen(tmp.title);else if(field==1)pos=(int)strlen(datebuf);else if(field==2)pos=(int)strlen(tmp.tag);else if(field==4)pos=(int)strlen(tmp.note);redraw_task_field(x,y,field,&tmp,datebuf,pos);key=0;continue;
    }
    if(field==0&&field_key(tmp.title,TITLE_LEN,key,&pos)){redraw_task_field(x,y,field,&tmp,datebuf,pos);key=0;continue;}
    if(field==1&&field_key(datebuf,DATE_INPUT_LEN,key,&pos)){redraw_task_field(x,y,field,&tmp,datebuf,pos);key=0;continue;}
    if(field==2&&field_key(tmp.tag,TAG_LEN,key,&pos)){redraw_task_field(x,y,field,&tmp,datebuf,pos);key=0;continue;}
    if(field==4&&field_key(tmp.note,NOTE_LEN,key,&pos)){redraw_task_field(x,y,field,&tmp,datebuf,pos);key=0;continue;}
    key=0;
  }
  return 0;
}

static int edit_heading(TODO_ITEM *t)
{
  char title[TITLE_LEN+1];int w=58,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,key=0,mx=0,my=0,pos,dirty=1,field=-1;unsigned mb=0;
  strcpy(title,t->title);pos=(int)strlen(title);
  while(key!=27){
    /* One clear row follows the Title/input line before the toolbar divider. */
    if(dirty){acc_subbox(x,y,w,h,"Group",1);form_field(x+3,y+3,10,"Title",title,40,field==0,pos);acc_button(x+3,y+h-3,"  OK  ",field==1);acc_button(x+10,y+h-3," Cancel ",field==2);dirty=0;}
    acc_wait(&key,&mx,&my,&mb);
    if(key==27)return 0;
    if((mb&1)&&my==y+3&&mx>=x+13&&mx<x+53){field=0;pos=(int)strlen(title);dirty=1;key=0;continue;}
    if((mb&1)&&my==y+h-3){if(mx>=x+2&&mx<x+8){field=1;key=13;}else if(mx>=x+10&&mx<x+16){field=2;key=13;}}
    if(key==9||key==271){if(field<0)field=(key==271)?2:0;else field=(key==271)?(field+2)%3:(field+1)%3;dirty=1;key=0;continue;}
    if(key==13){if(field<0){key=0;continue;}if(field==1){strcpy(t->title,title);return 1;}if(field==2)return 0;field=1;dirty=1;key=0;continue;}
    if(field==0&&field_key(title,TITLE_LEN,key,&pos)){dirty=1;key=0;continue;}
    key=0;
  }
  return 0;
}

static int task_under_collapsed_heading(int index)
{
  int i;
  if(index<0||index>=item_count||items[index].type==0)return 0;
  for(i=index-1;i>=0;i--)if(items[i].type==0)return items[i].done!=0;
  return 0;
}

static int visible(int index,const char *filter,int hide_done)
{
  if(index<0||index>=item_count)return 0;
  if(items[index].type==0)return 1;
  if(task_under_collapsed_heading(index))return 0;
  if(hide_done&&items[index].done)return 0;
  if(!filter[0])return 1;
  return !stricmp(items[index].tag,filter);
}

static int due_compare(const char *a,const char *b,int descending)
{
  int ea=!a[0],eb=!b[0],c;
  if(ea!=eb)return ea?1:-1;
  c=stricmp(a,b);return descending?-c:c;
}

static int task_compare(int ia,int ib,int sort_mode)
{
  int c;
  if(sort_mode==1||sort_mode==2){c=due_compare(items[ia].due,items[ib].due,sort_mode==1);if(c)return c;}
  else if(sort_mode==3||sort_mode==4){c=stricmp(items[ia].title,items[ib].title);if(c)return sort_mode==3?-c:c;}
  return ia-ib;
}

static int build_visible(int *map,const char *filter,int hide_done,int sort_mode)
{
  int i,j,k,n=0,gstart,gend,tmp;
  for(i=0;i<item_count;i++)if(visible(i,filter,hide_done))map[n++]=i;
  if(sort_mode==0)return n;
  i=0;
  /* Ungrouped tasks form the first logical group. */
  gstart=0;while(i<n&&items[map[i]].type!=0)i++;gend=i;
  for(j=gstart;j<gend-1;j++)for(k=j+1;k<gend;k++)if(task_compare(map[j],map[k],sort_mode)>0){tmp=map[j];map[j]=map[k];map[k]=tmp;}
  while(i<n){
    i++;gstart=i;while(i<n&&items[map[i]].type!=0)i++;gend=i;
    for(j=gstart;j<gend-1;j++)for(k=j+1;k<gend;k++)if(task_compare(map[j],map[k],sort_mode)>0){tmp=map[j];map[j]=map[k];map[k]=tmp;}
  }
  return n;
}

static int collect_tags(char tags[MAX_TAGS][TAG_LEN+1])
{
  int i,j,n=0;for(i=0;i<item_count;i++)if(items[i].type&&items[i].tag[0]){
    for(j=0;j<n;j++)if(!stricmp(tags[j],items[i].tag))break;
    if(j==n&&n<MAX_TAGS){strncpy(tags[n],items[i].tag,TAG_LEN);tags[n][TAG_LEN]=0;n++;}
  }return n;
}

static int task_text_attr(const TODO_ITEM *t,int selected)
{
  if(selected)return ACC_SELECT;
  if(t->done)return ACC_ATTR(acc_appearance.controls_bg,8);
  return ACC_ATTR(acc_appearance.controls_bg,0);
}

static int checkbox_attr(const TODO_ITEM *t,int selected)
{
  int fg=0;
  (void)selected;
  if(t->done)fg=8;
  else if(t->priority==1)fg=12;
  else if(t->priority==2)fg=11;
  else if(t->priority==3)fg=1;
  return ACC_ATTR(acc_appearance.controls_bg,fg);
}

static void draw_note_lines(int x,int y,const char *note,int width,int selected)
{
  char line[80];int n=(int)strlen(note),p=0,row=0,k,attr=selected?ACC_SELECT:ACC_ATTR(acc_appearance.controls_bg,0);
  while(row<2){
    k=0;while(p<n&&k<width&&note[p]!='\n')line[k++]=note[p++];
    if(p<n&&note[p]=='\n')p++;
    line[k]=0;acc_fill(x,y+row,width,1,' ',attr);if(k)acc_text(x,y+row,line,attr,k);row++;
    if(p>=n&&row>=1)break;
  }
}

static void make_task_title(char *out,const char *title,int maxw)
{
  int n=(int)strlen(title);if(n<=maxw){strcpy(out,title);return;}
  if(maxw<=3){strncpy(out,title,maxw);out[maxw]=0;return;}
  strncpy(out,title,maxw-3);out[maxw-3]=0;strcat(out,"...");
}

static void draw_list(int x,int y,int w,int sel,int top,int expanded,const char *filter,int focus,int hide_done,int sort_mode)
{
  int map[MAX_ITEMS],n=build_visible(map,filter,hide_done,sort_mode),r,idx,yy=0,used=0;char due[16],tags[MAX_TAGS][TAG_LEN+1],bar[70],title[64];int nt,i,pos,selected,base=ACC_ATTR(acc_appearance.controls_bg,0),hattr,tattr,cattr,titlew;
  acc_fill(x,y,w,VIEW_ROWS,' ',base);for(i=0;i<VIEW_ROWS;i++){row_map[i]=-1;row_detail[i]=0;}
  for(r=top;r<n&&used<VIEW_ROWS;r++){
    idx=map[r];yy=y+used;selected=(r==sel&&focus==0);
    if(items[idx].type==0){
      row_map[used]=r;hattr=selected?ACC_SELECT:ACC_ATTR(acc_appearance.controls_bg,acc_appearance.titles);
      acc_fill(x,yy,w,1,' ',hattr);acc_put(x,yy,items[idx].done?16:31,hattr);acc_text(x+2,yy,items[idx].title,hattr,w-2);used++;
    } else {
      row_map[used]=r;tattr=task_text_attr(&items[idx],selected);cattr=checkbox_attr(&items[idx],selected);
      acc_fill(x,yy,w,1,' ',tattr);
      acc_put(x+2,yy,items[idx].done?GLYPH_C_L:GLYPH_U_L,cattr);acc_put(x+3,yy,items[idx].done?GLYPH_C_R:GLYPH_U_R,cattr);
      due[0]=0;if(items[idx].due[0])display_date(items[idx].due,due);
      titlew=items[idx].due[0]?w-18:w-5;if(titlew<4)titlew=4;make_task_title(title,items[idx].title,titlew);
      acc_text(x+5,yy,title,tattr,titlew);
      if(items[idx].due[0])acc_text(x+w-10,yy,due,tattr,10);
      used++;
      if(r==sel&&expanded&&used<VIEW_ROWS){
        if(items[idx].tag[0]){row_map[used]=r;row_detail[used]=1;sprintf(bar,"Tag: %s",items[idx].tag);acc_fill(x,y+used,w,1,' ',base);acc_text(x+4,y+used,bar,base,w-5);used++;}
        if(used<VIEW_ROWS&&items[idx].note[0]){row_map[used]=r;row_detail[used]=1;if(used+1<VIEW_ROWS){row_map[used+1]=r;row_detail[used+1]=1;}draw_note_lines(x+4,y+used,items[idx].note,w-5,0);used+=2;}
      }
    }
  }
  acc_scrollbar(x+w,y,VIEW_ROWS,top,n,VIEW_ROWS);
  nt=collect_tags(tags);pos=0;acc_fill(x,y+VIEW_ROWS,w,1,' ',ACC_BG);acc_text(x,y+VIEW_ROWS,"Tags:",ACC_LABEL,5);pos=6;
  acc_text(x+pos,y+VIEW_ROWS,"All",(!filter[0]&&focus==1)?ACC_SELECT:ACC_TEXT,3);pos+=3;
  for(i=0;i<nt&&pos<w-TAG_LEN-2;i++){acc_text(x+pos,y+VIEW_ROWS,"|",ACC_LABEL,1);pos++;acc_text(x+pos,y+VIEW_ROWS,tags[i],(!stricmp(filter,tags[i])&&focus==1)?ACC_SELECT:ACC_TEXT,(int)strlen(tags[i]));pos+=(int)strlen(tags[i]);}
}

static void swap_range(int a0,int a1,int b0,int b1)
{
  TODO_ITEM temp[MAX_ITEMS];int n=0,i;
  for(i=a0;i<=a1;i++)temp[n++]=items[i];
  memmove(items+a0,items+b0,(b1-b0+1)*sizeof(TODO_ITEM));
  for(i=0;i<n;i++)items[a0+(b1-b0+1)+i]=temp[i];
}

static int group_end(int start)
{
  int i;if(start<0||start>=item_count)return start;
  for(i=start+1;i<item_count;i++)if(items[i].type==0)break;
  return i-1;
}

static int move_heading_group(int idx,int dir)
{
  int end=group_end(idx),pstart,pend,nstart,nend,size;
  if(dir<0){
    if(idx<=0)return idx;pend=idx-1;pstart=pend;while(pstart>0&&items[pstart].type!=0)pstart--;if(items[pstart].type!=0)return idx;
    swap_range(pstart,pend,idx,end);return pstart;
  }
  nstart=end+1;if(nstart>=item_count||items[nstart].type!=0)return idx;nend=group_end(nstart);size=nend-nstart+1;
  {
    TODO_ITEM temp[MAX_ITEMS];int n=0,i;
    for(i=idx;i<=end;i++)temp[n++]=items[i];
    memmove(items+idx,items+nstart,size*sizeof(TODO_ITEM));
    for(i=0;i<n;i++)items[idx+size+i]=temp[i];
    return idx+size;
  }
}

static int move_task(int idx,int dir)
{
  int b=idx+dir;
  if(b<0||b>=item_count||items[b].type==0)return idx;
  {TODO_ITEM t=items[idx];items[idx]=items[b];items[b]=t;}return b;
}

static int visible_pos_for_index(int idx,const char *filter,int hide_done,int sort_mode)
{
  int map[MAX_ITEMS],n=build_visible(map,filter,hide_done,sort_mode),i;for(i=0;i<n;i++)if(map[i]==idx)return i;return 0;
}

static void delete_item(int idx)
{
  int i;if(idx<0||idx>=item_count)return;for(i=idx;i<item_count-1;i++)items[i]=items[i+1];item_count--;
}

static void delete_selected(int idx)
{
  int end,count;
  if(idx<0||idx>=item_count)return;
  if(items[idx].type!=0){delete_item(idx);return;}
  end=group_end(idx);count=end-idx+1;
  memmove(items+idx,items+end+1,(item_count-end-1)*sizeof(TODO_ITEM));
  item_count-=count;
}

static int task_group_index(int idx)
{int i;if(idx<0||idx>=item_count)return -1;if(items[idx].type==0)return idx;for(i=idx-1;i>=0;i--)if(items[i].type==0)return i;return -1;}
static const char *task_group_name(int idx)
{int g=task_group_index(idx);return g>=0?items[g].title:"(none)";}
static int insert_task_after(int pos,const TODO_ITEM *t)
{int i;if(item_count>=MAX_ITEMS)return -1;if(pos<0)pos=0;if(pos>item_count)pos=item_count;for(i=item_count;i>pos;i--)items[i]=items[i-1];items[pos]=*t;item_count++;return pos;}

static int add_type_popup(int bx,int by)
{
  int w=10,h=4,x=bx,y=by-h,k=0,mx=0,my=0,sel=-1,i,j;unsigned mb=0;
  /* Compact two-choice popup immediately above Add.  The main window is
     redrawn by the caller afterwards, so no separate backing store is needed. */
  acc_put(x,y,218,ACC_BORDER);for(i=1;i<w-1;i++)acc_put(x+i,y,196,ACC_BORDER);acc_put(x+w-1,y,191,ACC_BORDER);
  for(j=1;j<h-1;j++){acc_put(x,y+j,179,ACC_BORDER);acc_put(x+w-1,y+j,179,ACC_BORDER);}
  acc_put(x,y+h-1,192,ACC_BORDER);for(i=1;i<w-1;i++)acc_put(x+i,y+h-1,196,ACC_BORDER);acc_put(x+w-1,y+h-1,217,ACC_BORDER);
  for(;;){
    acc_text(x+1,y+1," Task   ",sel==0?ACC_SELECT:ACC_TEXT,w-2);
    acc_text(x+1,y+2," Group  ",sel==1?ACC_SELECT:ACC_TEXT,w-2);
    acc_wait(&k,&mx,&my,&mb);
    if(mb&ACC_MOUSE_MOVED){if(mx>x&&mx<x+w-1&&my>=y+1&&my<=y+2)sel=my-(y+1);continue;}
    if(mb&1){if(mx>x&&mx<x+w-1&&my>=y+1&&my<=y+2)return my-(y+1)+1;continue;}
    if(k==27)return 0;
    if(k==9||k==271){sel=sel<0?((k==271)?1:0):!sel;continue;}
    if(k==256+72||k==256+80){sel=sel<0?0:!sel;continue;}
    if(k==13&&sel>=0)return sel+1;
  }
}

int main(int argc,char **argv)
{
  int w=74,h=22,x,y,lx,ly,key=0,mx=0,my=0,focus=-1,sel=0,top=0,expanded=0,map[MAX_ITEMS],n,idx,i,dirty=1,nt,tag_sel=0,newidx,sort_mode=0,hide_done=0;unsigned mb=0;
  char filter[TAG_LEN+1]="",tags[MAX_TAGS][TAG_LEN+1],hide_label[18];TODO_ITEM t;
  if(acc_help(argc,argv,"!TODOS","A persistent group-and-task to-do list with tags, dates and priorities."))return 0;
  if(!acc_begin(argv[0],"To-Dos",0))return 1;install_glyphs();load_items();x=(acc_cols-w)/2;y=(acc_rows-h)/2;lx=x+3;ly=y+2;acc_box(x,y,w,h,"To-Dos");
  n=build_visible(map,filter,hide_done,sort_mode);
  for(i=0;i<n;i++)if(items[map[i]].type){sel=i;break;}
  while(key!=27){
    n=build_visible(map,filter,hide_done,sort_mode);if(n==0)sel=0;else if(sel>=n)sel=n-1;if(sel<top)top=sel;if(sel>=top+VIEW_ROWS)top=sel-VIEW_ROWS+1;if(top<0)top=0;
    if(dirty){draw_list(lx,ly,w-7,sel,top,expanded,filter,focus,hide_done,sort_mode);dirty=0;}
    hide_label[0]=' ';hide_label[1]=' ';hide_label[2]=(char)GLYPH_C_L;hide_label[3]=(char)GLYPH_C_R;hide_label[4]=' ';hide_label[5]=' ';hide_label[6]=0;
    acc_button(x+3,y+h-3," Add ",focus==2);if(n){acc_button(x+10,y+h-3," Edit ",focus==3);acc_button(x+17,y+h-3," Delete ",focus==4);}else{acc_button_disabled(x+10,y+h-3," Edit ");acc_button_disabled(x+17,y+h-3," Delete ");}acc_button(x+25,y+h-3," Print ",focus==5);acc_put(x+33,y+h-3,179,ACC_BORDER);acc_button(x+35,y+h-3,hide_label,focus==6);acc_button(x+43,y+h-3,"  Sort  ",focus==7);acc_text(x+53,y+h-3,(sort_mode==1)?"Date":(sort_mode==2)?"Date":(sort_mode==3)?"Alph":(sort_mode==4)?"Alph":"None",ACC_LABEL,6);acc_button(x+w-10,y+h-3," Close ",focus==8);
    acc_wait(&key,&mx,&my,&mb);
    if((mb&1)&&my>=ly&&my<ly+VIEW_ROWS&&mx>=lx&&mx<lx+w-7){
      static int last_click_idx=-1;static unsigned long last_click_tick=0;unsigned long now;int row=my-ly;
      if(row_map[row]>=0&&row_map[row]<n){
        sel=row_map[row];idx=map[sel];focus=0;now=acc_ticks();
        if(row_detail[row]){dirty=1;key=0;continue;}
        if(items[idx].type){
          if(mx>=lx+2&&mx<=lx+3){items[idx].done=!items[idx].done;expanded=0;last_click_idx=-1;}
          else if(mx>=lx+5){if(last_click_idx==idx&&(unsigned long)(now-last_click_tick)<=9UL){edit_task(&items[idx],task_group_name(idx));acc_box(x,y,w,h,"To-Dos");last_click_idx=-1;}else{expanded=!expanded;last_click_idx=idx;last_click_tick=now;}}
        } else {items[idx].done=!items[idx].done;expanded=0;last_click_idx=-1;}
        dirty=1;
      }key=0;continue;
    }
    if((mb&1)&&my==ly+VIEW_ROWS){nt=collect_tags(tags);if(mx>=lx+6&&mx<lx+9){filter[0]=0;tag_sel=0;}else{int p=9;for(i=0;i<nt;i++){p++;if(mx>=lx+p&&mx<lx+p+(int)strlen(tags[i])){strcpy(filter,tags[i]);tag_sel=i+1;break;}p+=(int)strlen(tags[i]);}}sel=top=0;focus=1;expanded=0;dirty=1;key=0;continue;}
    if((mb&1)&&my==y+h-3){if(mx>=x+2&&mx<x+8)focus=2;else if(mx>=x+9&&mx<x+15)focus=3;else if(mx>=x+16&&mx<x+22)focus=4;else if(mx>=x+24&&mx<x+30)focus=5;else if(mx>=x+34&&mx<x+40)focus=6;else if(mx>=x+42&&mx<x+51)focus=7;else if(mx>=x+w-10&&mx<x+w-4)focus=8;key=13;}
    if(key==9||key==271){
      static int order[9]={1,0,2,3,4,5,6,7,8};int p,dir=(key==271)?-1:1;
      if(focus<0)focus=(key==271)?8:1;
      else {for(p=0;p<9&&order[p]!=focus;p++);if(p>=9)p=0;do {p=(p+dir+9)%9;focus=order[p];} while(!n&&(focus==3||focus==4));}
      dirty=1;key=0;continue;
    }

    /* Global keyboard shortcuts. */
    if(key=='a'||key=='A'){
      int kind=add_type_popup(x+3,y+h-3);
      acc_box(x,y,w,h,"To-Dos");
      /* The popup is painted over the task viewport.  Repaint the viewport
         even when Escape dismisses it without choosing an item. */
      dirty=1;
      if(kind==1&&item_count<MAX_ITEMS){int g=(n>0)?task_group_index(map[sel]):-1,at=(g>=0)?group_end(g)+1:0;memset(&t,0,sizeof(t));t.used=1;t.type=1;t.priority=0;if(edit_task(&t,g>=0?items[g].title:"(none)")){newidx=insert_task_after(at,&t);filter[0]=0;sort_mode=0;sel=visible_pos_for_index(newidx,filter,hide_done,sort_mode);top=sel>VIEW_ROWS-1?sel-VIEW_ROWS+1:0;}acc_box(x,y,w,h,"To-Dos");dirty=1;}
      else if(kind==2&&item_count<MAX_ITEMS){memset(&t,0,sizeof(t));t.used=1;t.type=0;strcpy(t.title,"New group");if(edit_heading(&t)){items[item_count++]=t;filter[0]=0;sort_mode=0;sel=item_count-1;}acc_box(x,y,w,h,"To-Dos");dirty=1;}
      key=0;continue;
    }
    if(key=='g'||key=='G'){
      if(item_count<MAX_ITEMS){memset(&t,0,sizeof(t));t.used=1;t.type=0;strcpy(t.title,"New group");if(edit_heading(&t)){items[item_count++]=t;filter[0]=0;sort_mode=0;sel=item_count-1;}acc_box(x,y,w,h,"To-Dos");dirty=1;}
      key=0;continue;
    }
    if((key=='e'||key=='E')&&n>0){
      idx=map[sel];if(items[idx].type)edit_task(&items[idx],task_group_name(idx));else edit_heading(&items[idx]);acc_box(x,y,w,h,"To-Dos");dirty=1;key=0;continue;
    }
    if((key=='d'||key=='D')&&n>0){
      idx=map[sel];delete_selected(idx);if(sel>0)sel--;expanded=0;dirty=1;key=0;continue;
    }
    if(key=='s'||key=='S'){sort_mode=(sort_mode+1)%5;sel=top=0;expanded=0;dirty=1;acc_text(x+53,y+h-3,(sort_mode==1)?"Date\031":(sort_mode==2)?"Date\030":(sort_mode==3)?"Alph\031":(sort_mode==4)?"Alph\030":"None",ACC_LABEL,6);key=0;continue;}
    if(key=='h'||key=='H'){hide_done=!hide_done;sel=top=0;expanded=0;dirty=1;key=0;continue;}

    if(focus==0&&n>0){
      idx=map[sel];
      if(key==256+141||key==256+145||(ctrl_down()&&(key==256+72||key==256+80))){
        int dir=(key==256+141||key==256+72)?-1:1;
        if(items[idx].type==0)newidx=move_heading_group(idx,dir);else newidx=move_task(idx,dir);
        sort_mode=0;sel=visible_pos_for_index(newidx,filter,hide_done,sort_mode);if(sel<top)top=sel;if(sel>=top+VIEW_ROWS)top=sel-VIEW_ROWS+1;dirty=1;key=0;continue;
      }
      if(key==256+72&&sel>0){sel--;expanded=0;dirty=1;key=0;continue;}
      if(key==256+80&&sel<n-1){sel++;expanded=0;dirty=1;key=0;continue;}
      if(key==' '&&items[idx].type){items[idx].done=!items[idx].done;if(hide_done&&items[idx].done&&sel>0)sel--;expanded=0;dirty=1;key=0;continue;}
      if(key==13){if(items[idx].type)expanded=!expanded;else {items[idx].done=!items[idx].done;expanded=0;}dirty=1;key=0;continue;}
    }
    if(focus==1){nt=collect_tags(tags);if(key==256+75&&tag_sel>0)tag_sel--;else if(key==256+77&&tag_sel<nt)tag_sel++;else if(key==13){if(tag_sel==0)filter[0]=0;else strcpy(filter,tags[tag_sel-1]);sel=top=0;expanded=0;}dirty=1;key=0;continue;}
    if(key==13&&focus>=2){
      if(focus==2){int kind=add_type_popup(x+3,y+h-3);acc_box(x,y,w,h,"To-Dos");if(kind==1&&item_count<MAX_ITEMS){int g=(n>0)?task_group_index(map[sel]):-1,at=(g>=0)?group_end(g)+1:0;memset(&t,0,sizeof(t));t.used=1;t.type=1;t.priority=0;if(edit_task(&t,g>=0?items[g].title:"(none)")){newidx=insert_task_after(at,&t);filter[0]=0;sort_mode=0;sel=visible_pos_for_index(newidx,filter,hide_done,sort_mode);top=sel>VIEW_ROWS-1?sel-VIEW_ROWS+1:0;}acc_box(x,y,w,h,"To-Dos");}else if(kind==2&&item_count<MAX_ITEMS){memset(&t,0,sizeof(t));t.used=1;t.type=0;strcpy(t.title,"New group");if(edit_heading(&t)){items[item_count++]=t;filter[0]=0;sort_mode=0;sel=item_count-1;}acc_box(x,y,w,h,"To-Dos");}}
      else if(focus==3&&n>0){idx=map[sel];if(items[idx].type)edit_task(&items[idx],task_group_name(idx));else edit_heading(&items[idx]);acc_box(x,y,w,h,"To-Dos");}
      else if(focus==4&&n>0){idx=map[sel];delete_selected(idx);if(sel>0)sel--;expanded=0;}
      else if(focus==5){if(!print_tasks(filter,hide_done,sort_mode))acc_notice("Print","Unable to print to LPT1");acc_box(x,y,w,h,"To-Dos");}
      else if(focus==6){hide_done=!hide_done;sel=top=0;expanded=0;}
      else if(focus==7){sort_mode=(sort_mode+1)%5;sel=top=0;expanded=0;acc_text(x+53,y+h-3,(sort_mode==1)?"Date\031":(sort_mode==2)?"Date\030":(sort_mode==3)?"Alph\031":(sort_mode==4)?"Alph\030":"None",ACC_LABEL,6);}
      else if(focus==8)key=27;
      dirty=1;if(key!=27)key=0;continue;
    }
    if(key!=27)key=0;
  }
  save_items();restore_glyphs();acc_end();return 0;
}
