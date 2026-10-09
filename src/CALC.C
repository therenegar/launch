/*
    __                           __    __
   / /   ____ ___  ______  _____/ /_  / /
  / /   / __ `/ / / / __ \/ ___/ __ \/ / 
 / /___/ /_/ / /_/ / / / /__/ / / /_/  
/_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
Launch! for DOS ---------------------
*/
/*
 * MAINTAINER NOTES - Launch! 3.79
 * File: CALC.C
 * Role: !CALC calculator
 * Build/ownership: Accessory calculator with optional printing.
 * Maintainer contract: /PRINT must gracefully fall back to normal operation when no LPT1 hardware is detected.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* Launch! Calculator accessory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dos.h>
#include "ACCLIB.H"

static const char *keys[18]={"7","8","9","%","R","4","5","6","*","/","1","2","3","+","-","0",".","="};
static const char *labels[18]={"7","8","9","%","\373","4","5","6","x","\366","1","2","3","+","-","0",".","="};
static const int ky[18]={0,0,0,0,0,2,2,2,2,2,4,4,4,4,4,6,6,6};
static const int kx[18]={2,6,10,16,20,2,6,10,16,20,2,6,10,16,20,2,10,16};
static const int kw[18]={3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,7,3,7};

#define CALC_SEG_BASE 128
#define CALC_LOWER_HALF 222
#define CALC_KEY_LEFT 193
#define CALC_KEY_RIGHT 139
#define TAPE_MAX 64
#define TAPE_VISIBLE 16

typedef struct {char value[24];char op;} TAPE_REC;
static TAPE_REC tape[TAPE_MAX];
static int tape_count=0;
static int tape_view=0; /* 0 = newest page; positive = rows scrolled toward older entries */
static int print_mode=0;
static FILE *print_file=0;

static unsigned char calc_old_glyphs[11][32];
static unsigned char calc_old_lower_half[32];
static unsigned char calc_old_key[2][32];
static int calc_use_segments=1;
static const unsigned char calc_keycap16[2][16]={
 {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F},
 {0xF0,0xFC,0xFC,0xFE,0xFE,0xFE,0xFE,0xFA,0xFE,0xFA,0xFA,0xFA,0xF2,0xE6,0x1C,0xF0}
};
static const unsigned char calc_keycap14[2][14]={
 {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F},
 {0xF0,0xFC,0xFE,0xFE,0xFE,0xFA,0xFE,0xFA,0xFA,0xFA,0xF2,0xE6,0x1C,0xF0}
};

static void calc_segments_install(void)
{
 int i,h=acc_font_height();unsigned char g[32];
 calc_use_segments=1;
 for(i=0;i<11;i++){acc_glyph_read(CALC_SEG_BASE+i,calc_old_glyphs[i]);acc_glyph_library(64+i,CALC_SEG_BASE+i);}
 acc_glyph_read(CALC_LOWER_HALF,calc_old_lower_half);memset(g,0,sizeof(g));for(i=h/2;i<h;i++)g[i]=0xFF;acc_glyph_write(CALC_LOWER_HALF,g);
 acc_glyph_read(CALC_KEY_LEFT,calc_old_key[0]);acc_glyph_read(CALC_KEY_RIGHT,calc_old_key[1]);
 memset(g,0,sizeof(g));if(h==14)memcpy(g,calc_keycap14[0],14);else memcpy(g,calc_keycap16[0],16);acc_glyph_write(CALC_KEY_LEFT,g);
 memset(g,0,sizeof(g));if(h==14)memcpy(g,calc_keycap14[1],14);else memcpy(g,calc_keycap16[1],16);acc_glyph_write(CALC_KEY_RIGHT,g);
}
static void calc_segments_restore(void)
{
 int i;if(calc_use_segments)for(i=0;i<11;i++)acc_glyph_write(CALC_SEG_BASE+i,calc_old_glyphs[i]);
 acc_glyph_write(CALC_LOWER_HALF,calc_old_lower_half);acc_glyph_write(CALC_KEY_LEFT,calc_old_key[0]);acc_glyph_write(CALC_KEY_RIGHT,calc_old_key[1]);
}
static unsigned char calc_display_char(unsigned char c){if(!calc_use_segments)return c;if(c>='0'&&c<='9')return(unsigned char)(CALC_SEG_BASE+(c-'0'));if(c=='.')return(unsigned char)(CALC_SEG_BASE+10);return c;}
static int digit_count(const char *s)
{
 int n=0;for(;*s;s++)if(*s>='0'&&*s<='9')n++;return n;
}
static void format_display(char *s,double v)
{
 int prec;
 /* Twelve numeric digits are allowed; decimal point and sign do not count. */
 for(prec=12;prec>=1;prec--){sprintf(s,"%.*g",prec,v);if(digit_count(s)<=12)return;}
 strcpy(s,"OVERFLOW");
}
static void output_box(int x,int y,int w,double v,const char *entry)
{
 char s[40];int i,start;
 if(entry&&*entry){strncpy(s,entry,14);s[14]=0;}else format_display(s,v);
 for(i=0;i<w;i++){acc_put(x+i,y,CALC_LOWER_HALF,ACC_ATTR(acc_appearance.background,0));acc_put(x+i,y+1,' ',0x00);acc_put(x+i,y+2,223,ACC_ATTR(acc_appearance.background,0));}
 start=x+w-1-(int)strlen(s);for(i=0;s[i];i++)acc_put(start+i,y+1,calc_display_char((unsigned char)s[i]),0x0A);
}
static void calc_keycap(int x,int y,int width,const char *label,int selected,int equals)
{
 int a,capfg,ca,n,start,i;
 if(selected){a=ACC_SELECT;capfg=acc_appearance.selected_bg;}
 else if(equals){a=ACC_ATTR(4,15);capfg=4;}
 else {a=ACC_CONTROL;capfg=acc_appearance.controls_bg;}
 ca=ACC_ATTR(acc_appearance.background,capfg);
 acc_put(x,y,CALC_KEY_LEFT,ca);for(i=1;i<width-1;i++)acc_put(x+i,y,' ',a);acc_put(x+width-1,y,CALC_KEY_RIGHT,ca);
 n=(int)strlen(label);if(n>width-2)n=width-2;start=x+(width-n)/2;acc_text(start,y,label,a,n);
}
static void draw_keys(int x,int y,int focus){int i;for(i=0;i<18;i++)calc_keycap(x+kx[i],y+ky[i],kw[i],labels[i],focus==i,i==17);}
static int key_index(int ch){int i;if(ch=='r'||ch=='R')return 4;for(i=0;i<18;i++)if(keys[i][0]==ch&&keys[i][1]==0)return i;return-1;}
static void flash_key(int x,int y,int index,int restore){unsigned long t;if(index<0)return;draw_keys(x,y,index);t=acc_ticks();while(acc_ticks()==t);t=acc_ticks();while(acc_ticks()==t);draw_keys(x,y,restore);}
static int move_key(int focus,int key)
{
 int i,best=focus,bestscore=32767,fx,fy,ix,iy,dx,dy,score;
 if(focus<0||focus>=18)return 0;fx=kx[focus]+kw[focus]/2;fy=ky[focus];
 for(i=0;i<18;i++){if(i==focus)continue;ix=kx[i]+kw[i]/2;iy=ky[i];dx=ix-fx;dy=iy-fy;if((key==256+75&&dx>=0)||(key==256+77&&dx<=0)||(key==256+72&&dy>=0)||(key==256+80&&dy<=0))continue;score=(key==256+75||key==256+77)?(abs(dx)*100+abs(dy)):(abs(dy)*100+abs(dx));if(score<bestscore){bestscore=score;best=i;}}
 return best;
}
static void tape_print(const TAPE_REC *r)
{
 char line[22],left[24],right[24],*dot;int ln,start,i;
 if(!print_file)return;memset(line,' ',21);line[21]=0;
 if((unsigned char)r->op==196){for(i=2;i<19;i++)line[i]='-';}
 else {
  dot=strchr(r->value,'.');
  if(dot){ln=(int)(dot-r->value);if(ln>22)ln=22;memcpy(left,r->value,ln);left[ln]=0;strncpy(right,dot,22);right[22]=0;}
  else{strncpy(left,r->value,22);left[22]=0;right[0]=0;}
  start=15-(int)strlen(left);if(start<0)start=0;for(i=0;left[i]&&start+i<21;i++)line[start+i]=left[i];
  for(i=0;right[i]&&15+i<21;i++)line[15+i]=right[i];
  if(r->op)line[19]=r->op;
 }
 fwrite(line,1,21,print_file);fputs("\r\n",print_file);fflush(print_file);
}

static void tape_add(const char *entry,double value,char op)
{
 int i;char s[24];
 if(entry&&*entry){strncpy(s,entry,23);s[23]=0;}else format_display(s,value);
 if(tape_count<TAPE_MAX)i=tape_count++;
 else{for(i=1;i<TAPE_MAX;i++)tape[i-1]=tape[i];i=TAPE_MAX-1;}
 strncpy(tape[i].value,s,23);tape[i].value[23]=0;tape[i].op=op;
 tape_print(&tape[i]);
 tape_view=0;
}
static void tape_clear(void)
{
 tape_count=0;
 tape_view=0;
}
static void tape_faux_shadow(int x,int y,int w,int h)
{
 int i,j;unsigned short far *v=(unsigned short far *)(((unsigned long)0xB800U<<16)|0UL);unsigned short cell;
 for(j=1;j<=h;j++)for(i=0;i<2;i++)if(x+w+i<acc_cols&&y+j<acc_rows){cell=v[(y+j)*acc_cols+x+w+i];acc_put(x+w+i,y+j,cell&255,ACC_ATTR(0,8));}
 if(y+h<acc_rows)for(i=1;i<=w+1;i++)if(x+i<acc_cols){cell=v[(y+h)*acc_cols+x+i];acc_put(x+i,y+h,cell&255,ACC_ATTR(0,8));}
}
static void tape_value(int x,int y,int w,const TAPE_REC *r)
{
 char left[24],right[24],op[2];const char *dot;int ln,decimal_col,attr,start,i;
 if((unsigned char)r->op==196){for(i=2;i<w-2;i++)acc_put(x+i,y,196,ACC_ATTR(7,1));return;}
 dot=strchr(r->value,'.');
 if(dot){ln=(int)(dot-r->value);if(ln>22)ln=22;memcpy(left,r->value,ln);left[ln]=0;strncpy(right,dot,22);right[22]=0;}
 else{strncpy(left,r->value,22);left[22]=0;right[0]=0;}
 /* Fixed decimal column: values line up whether or not they contain a point. */
 decimal_col=x+w-6;
 attr=(r->value[0]=='-')?ACC_ATTR(7,4):ACC_ATTR(7,1);
 start=decimal_col-(int)strlen(left);acc_text(start,y,left,attr,(int)strlen(left));
 if(right[0])acc_text(decimal_col,y,right,attr,(int)strlen(right));
 if(r->op){op[0]=r->op;op[1]=0;acc_text(x+w-2,y,op,attr,1);}
}
static void draw_tape(int x,int y)
{
 int i,w=21,h=19,first,n,maxview;
 acc_fill(x,y,w,h,' ',ACC_ATTR(7,1));
 maxview=tape_count>TAPE_VISIBLE?tape_count-TAPE_VISIBLE:0;
 if(tape_view>maxview)tape_view=maxview;
 first=tape_count-TAPE_VISIBLE-tape_view;if(first<0)first=0;
 n=tape_count-first;if(n>TAPE_VISIBLE)n=TAPE_VISIBLE;
 /* Paper-feed arrows sit just beyond the right edge, exactly as CALC2.ASC.
    They appear only when more tape exists in that direction. */
 if(first>0)acc_put(x+w-2,y,30,ACC_ATTR(7,1));
 if(tape_view>0)acc_put(x+w-2,y+h-2,31,ACC_ATTR(7,1));
 for(i=0;i<n;i++)tape_value(x,y+1+i,w,&tape[first+i]);
 tape_faux_shadow(x,y,w,h);
}
#define CALC_MODE_BAS 0
#define CALC_MODE_SCI 1
#define CALC_MODE_FIN 2
#define SCI_KEY_COUNT 24
#define FIN_KEY_COUNT 10

static void draw_calc_toolbar(int bx,int by,int w,int h,int focus,int mode)
{
 const char *mode_text=mode==CALC_MODE_BAS?" BAS ":(mode==CALC_MODE_SCI?" SCI ":" FIN ");
 /* CALC.ASC uses deliberately tighter toolbar padding than the suite default:
    C/CE/mode occupy 3/4/5 cells and Exit is a compact four-cell icon button. */
 acc_button(bx+3,by+h-3," C ",focus==18);
 acc_button(bx+8,by+h-3," CE ",focus==19);
 acc_button(bx+14,by+h-3,mode_text,focus==20);
 acc_button(bx+w-8,by+h-3," \326\351 ",focus==21);
}

static const char *sci_label[SCI_KEY_COUNT]={
 "SIN","COS","TAN","PI","ASIN","ACOS","ATAN","e",
 "LOG","LN","EXP","10^x","x^y","x^2","x^3","SQRT",
 "1/x","n!","+/-","ABS","DEG","MOD","FLOOR","CEIL"
};
static const int sci_x[SCI_KEY_COUNT]={0,9,18,27,0,9,18,27,0,9,18,27,0,9,18,27,0,9,18,27,0,9,18,27};
static const int sci_y[SCI_KEY_COUNT]={0,0,0,0,2,2,2,2,4,4,4,4,6,6,6,6,8,8,8,8,10,10,10,10};
static int sci_degrees=1;

/* Financial mode is a conventional time-value-of-money calculator.  N is
   the number of payment periods, I% is a nominal annual rate, and P/Y is the
   number of payments per year (12 by default).  PV/PMT/FV use the usual cash
   flow sign convention. */
static double fin_n=0.0,fin_i=0.0,fin_pv=0.0,fin_pmt=0.0,fin_fv=0.0,fin_py=12.0;
static int fin_begin=0,fin_compute=0;
static const char *fin_label[FIN_KEY_COUNT]={"N","I%","PV","PMT","FV","P/Y","CPT","+/-","END","CLR TVM"};
static const int fin_x[FIN_KEY_COUNT]={0,9,18,27,0,9,18,27,0,9};
static const int fin_y[FIN_KEY_COUNT]={0,0,0,0,2,2,2,2,4,4};
static const int fin_w[FIN_KEY_COUNT]={8,8,8,8,8,8,8,8,8,17};

static void apply(const char *s,double *v,char *entry,char *op)
{
 double n;if(strlen(s)==1&&((*s>='0'&&*s<='9')||*s=='.')){if(!strcmp(entry,"ERROR")){entry[0]=0;*v=0;*op=0;}if(digit_count(entry)<12&&(*s!='.'||!strchr(entry,'.'))){entry[strlen(entry)+1]=0;entry[strlen(entry)]=*s;}return;}
 n=*entry?atof(entry):*v;
 if(!strcmp(s,"=")&&!*entry){*op=0;return;}
 if(!strcmp(s,"%")){
   if(*op=='*')*v=*v*(n/100.0);
   else if(*op=='/'&&n!=0.0)*v=*v/(n/100.0);
   else if(*op=='+')*v=*v+(*v*n/100.0);
   else if(*op=='-')*v=*v-(*v*n/100.0);
   else *v=n/100.0;
   *op=0;entry[0]=0;return;
 }
 if(!strcmp(s,"R")){if(n>=0.0)*v=sqrt(n);*op=0;entry[0]=0;return;}
 if(*op=='+')*v+=n;else if(*op=='-')*v-=n;else if(*op=='*')*v*=n;else if(*op=='/'&&n!=0)*v/=n;else if(*op=='^')*v=pow(*v,n);else if(*op=='m'&&n!=0)*v=fmod(*v,n);else *v=n;
 entry[0]=0;if(strcmp(s,"="))*op=*s;else *op=0;
}
static void calc_action(const char *s,double *value,char *entry,char *op)
{
 char before[24];
 if(!strcmp(entry,"ERROR") && !(strlen(s)==1&&*s>='0'&&*s<='9')){entry[0]=0;*value=0;*op=0;}
 strncpy(before,entry,23);before[23]=0;
 if(strlen(s)==1&&((*s>='0'&&*s<='9')||*s=='.')){apply(s,value,entry,op);return;}
 if(!strcmp(s,"+")||!strcmp(s,"-")||!strcmp(s,"*")||!strcmp(s,"/")){
   tape_add(before,*value,s[0]);apply(s,value,entry,op);return;
 }
 if(!strcmp(s,"=")){
   tape_add(before,*value,'=');apply(s,value,entry,op);tape_add("",*value,0);tape_add("",0,(char)196);return;
 }
 if(!strcmp(s,"%")||!strcmp(s,"R")){
   tape_add(before,*value,s[0]);apply(s,value,entry,op);tape_add("",*value,0);tape_add("",0,(char)196);
 }
}
static void calc_binary_action(char bop,double *value,char *entry,char *op)
{
 char s[2],before[24];s[0]=bop;s[1]=0;strncpy(before,entry,23);before[23]=0;
 tape_add(before,*value,bop=='m'?'%':bop);apply(s,value,entry,op);
}
static double calc_operand(double value,const char *entry){return (entry&&*entry&&strcmp(entry,"ERROR"))?atof(entry):value;}
static void calc_result(double result,double *value,char *entry,char op)
{
 char b[40];
 if(entry[0]||op){format_display(b,result);strncpy(entry,b,23);entry[23]=0;}
 else {*value=result;entry[0]=0;}
}
static void calc_error(double *value,char *entry,char *op){*value=0.0;strcpy(entry,"ERROR");*op=0;}
static void calc_sign(double *value,char *entry)
{
 if(entry[0]&&strcmp(entry,"ERROR")){if(entry[0]=='-')memmove(entry,entry+1,strlen(entry));else if(strlen(entry)<22){memmove(entry+1,entry,strlen(entry)+1);entry[0]='-';}}
 else *value=-*value;
}

static void draw_sci_keys(int x,int y,int focus)
{
 int i;const char *lab;for(i=0;i<SCI_KEY_COUNT;i++){lab=(i==20)?(sci_degrees?"DEG":"RAD"):sci_label[i];calc_keycap(x+sci_x[i],y+sci_y[i],8,lab,focus==22+i,0);}
}
static int sci_hit(int x,int y,int mx,int my)
{
 int i;for(i=0;i<SCI_KEY_COUNT;i++)if(my==y+sci_y[i]&&mx>=x+sci_x[i]&&mx<x+sci_x[i]+8)return i;return-1;
}
static void flash_sci(int x,int y,int index,int restore)
{
 unsigned long t;if(index<0)return;draw_sci_keys(x,y,22+index);t=acc_ticks();while(acc_ticks()==t);draw_sci_keys(x,y,restore);
}
static void sci_action(int index,double *value,char *entry,char *op)
{
 double n,r,rad;int i;char b[40];
 if(index==3||index==7){r=(index==3)?3.14159265358979323846:2.71828182845904523536;format_display(b,r);strncpy(entry,b,23);entry[23]=0;return;}
 if(index==12){calc_binary_action('^',value,entry,op);return;}
 if(index==18){calc_sign(value,entry);return;}
 if(index==20){sci_degrees=!sci_degrees;return;}
 if(index==21){calc_binary_action('m',value,entry,op);return;}
 n=calc_operand(*value,entry);r=n;
 switch(index){
  case 0:rad=sci_degrees?n*3.14159265358979323846/180.0:n;r=sin(rad);break;
  case 1:rad=sci_degrees?n*3.14159265358979323846/180.0:n;r=cos(rad);break;
  case 2:rad=sci_degrees?n*3.14159265358979323846/180.0:n;r=tan(rad);break;
  case 4:if(n<-1.0||n>1.0){calc_error(value,entry,op);return;}r=asin(n);if(sci_degrees)r=r*180.0/3.14159265358979323846;break;
  case 5:if(n<-1.0||n>1.0){calc_error(value,entry,op);return;}r=acos(n);if(sci_degrees)r=r*180.0/3.14159265358979323846;break;
  case 6:r=atan(n);if(sci_degrees)r=r*180.0/3.14159265358979323846;break;
  case 8:if(n<=0.0){calc_error(value,entry,op);return;}r=log10(n);break;
  case 9:if(n<=0.0){calc_error(value,entry,op);return;}r=log(n);break;
  case 10:r=exp(n);break;
  case 11:r=pow(10.0,n);break;
  case 13:r=n*n;break;
  case 14:r=n*n*n;break;
  case 15:if(n<0.0){calc_error(value,entry,op);return;}r=sqrt(n);break;
  case 16:if(n==0.0){calc_error(value,entry,op);return;}r=1.0/n;break;
  case 17:if(n<0.0||n>170.0||floor(n)!=n){calc_error(value,entry,op);return;}r=1.0;for(i=2;i<=(int)n;i++)r*=i;break;
  case 19:r=fabs(n);break;
  case 22:r=floor(n);break;
  case 23:r=ceil(n);break;
  default:return;
 }
 calc_result(r,value,entry,*op);
}

static void fin_clear(void){fin_n=fin_i=fin_pv=fin_pmt=fin_fv=0.0;fin_compute=0;}
static double fin_rate(void){if(fin_py<=0.0)return 0.0;return fin_i/(100.0*fin_py);}
static double fin_equation_rate(double r)
{
 double a,ann,b;if(r<=-0.999999)return 1.0e100;a=pow(1.0+r,fin_n);if(fabs(r)<1.0e-10)ann=fin_n;else ann=(a-1.0)/r;b=fin_begin?(1.0+r):1.0;return fin_pv*a+fin_pmt*ann*b+fin_fv;
}
static int fin_rate_newton(double seed,double *answer)
{
 int i;double r=seed,f,fp,fm,d,h,nr,scale;
 for(i=0;i<80;i++){
   if(r<=-0.9999)r=-0.9999;if(r>100.0)r=100.0;
   f=fin_equation_rate(r);scale=1.0+fabs(fin_pv)+fabs(fin_pmt*fin_n)+fabs(fin_fv);if(fabs(f)<1.0e-9*scale){*answer=r;return 1;}
   h=1.0e-6*(1.0+fabs(r));if(r-h<=-0.999999)h=(r+0.999999)*0.4;if(h<=1.0e-10)return 0;
   fp=fin_equation_rate(r+h);fm=fin_equation_rate(r-h);d=(fp-fm)/(2.0*h);if(fabs(d)<1.0e-14)return 0;
   nr=r-f/d;if(nr<=-0.999999)nr=(r-0.999999)/2.0;if(nr>100.0)nr=(r+100.0)/2.0;
   if(fabs(nr-r)<1.0e-11){r=nr;*answer=r;return fabs(fin_equation_rate(r))<1.0e-7*scale;}r=nr;
 }
 return 0;
}
static int fin_solve(int target,double *answer)
{
 double r=fin_rate(),a,ann,b,c,ratio,seeds[8];int i;
 if(fin_py<=0.0)return 0;
 if(target==1){
   if(fin_n<=0.0)return 0;seeds[0]=r;seeds[1]=0.01;seeds[2]=0.05;seeds[3]=0.10;seeds[4]=0.20;seeds[5]=-0.05;seeds[6]=0.50;seeds[7]=1.0;
   for(i=0;i<8;i++)if(fin_rate_newton(seeds[i],answer)){*answer=*answer*100.0*fin_py;return 1;}return 0;
 }
 if(r<=-1.0)return 0;a=pow(1.0+r,fin_n);ann=fabs(r)<1.0e-10?fin_n:(a-1.0)/r;b=fin_begin?(1.0+r):1.0;
 if(target==0){
   if(fabs(r)<1.0e-10){if(fin_pmt==0.0)return 0;*answer=-(fin_pv+fin_fv)/fin_pmt;return 1;}
   c=fin_pmt*b/r;if(fin_pv+c==0.0)return 0;ratio=(c-fin_fv)/(fin_pv+c);if(ratio<=0.0)return 0;*answer=log(ratio)/log(1.0+r);return 1;
 }
 if(target==2){if(a==0.0)return 0;*answer=-(fin_pmt*ann*b+fin_fv)/a;return 1;}
 if(target==3){if(ann*b==0.0)return 0;*answer=-(fin_pv*a+fin_fv)/(ann*b);return 1;}
 if(target==4){*answer=-(fin_pv*a+fin_pmt*ann*b);return 1;}
 return 0;
}
static double *fin_register(int index)
{
 if(index==0)return &fin_n;if(index==1)return &fin_i;if(index==2)return &fin_pv;if(index==3)return &fin_pmt;if(index==4)return &fin_fv;if(index==5)return &fin_py;return 0;
}
static void fin_action(int index,double *value,char *entry,char *op)
{
 double n,out,*reg;
 if(index>=0&&index<=5){
   reg=fin_register(index);if(fin_compute&&index<=4){if(fin_solve(index,&out)){*reg=out;*value=out;entry[0]=0;}else calc_error(value,entry,op);fin_compute=0;return;}
   n=calc_operand(*value,entry);if(index==5&&n<=0.0){calc_error(value,entry,op);return;}*reg=n;*value=n;entry[0]=0;return;
 }
 if(index==6){fin_compute=!fin_compute;return;}
 if(index==7){calc_sign(value,entry);return;}
 if(index==8){fin_begin=!fin_begin;return;}
 if(index==9){fin_clear();return;}
}
static void fin_value_line(int x,int y,const char *name,double v)
{
 char b[40];int n;format_display(b,v);acc_text(x,y,name,ACC_LABEL,(int)strlen(name));n=(int)strlen(b);if(n>12)n=12;acc_text(x+17-n,y,b,ACC_HEADING,n);
}
static void draw_fin_panel(int x,int y,int focus)
{
 int i;const char *lab;
 acc_text(x,y,"Time Value of Money",ACC_HEADING,19);
 fin_value_line(x,y+1,"N",fin_n);fin_value_line(x+18,y+1,"I%",fin_i);
 fin_value_line(x,y+2,"PV",fin_pv);fin_value_line(x+18,y+2,"PMT",fin_pmt);
 fin_value_line(x,y+3,"FV",fin_fv);fin_value_line(x+18,y+3,"P/Y",fin_py);
 acc_text(x,y+4,fin_begin?"Payments: BEGIN":"Payments: END  ",ACC_LABEL,15);
 for(i=0;i<FIN_KEY_COUNT;i++){lab=fin_label[i];if(i==8)lab=fin_begin?"BEG":"END";calc_keycap(x+fin_x[i],y+6+fin_y[i],fin_w[i],lab,focus==22+i||(i==6&&fin_compute),0);}
}
static int fin_hit(int x,int y,int mx,int my)
{
 int i;for(i=0;i<FIN_KEY_COUNT;i++)if(my==y+6+fin_y[i]&&mx>=x+fin_x[i]&&mx<x+fin_x[i]+fin_w[i])return i;return-1;
}
static void flash_fin(int x,int y,int index,int restore)
{
 unsigned long t;if(index<0)return;draw_fin_panel(x,y,22+index);t=acc_ticks();while(acc_ticks()==t);draw_fin_panel(x,y,restore);
}

static int calc_lpt1_detected(void)
{
 union REGS r;
 /* INT 11h reports the number of installed parallel adapters in bits 14-15.
    Avoid MK_FP/BDA access: MSC 6 may emit it as an unresolved external. */
 memset(&r,0,sizeof(r));int86(0x11,&r,&r);
 if(((r.x.ax>>14)&3)==0)return 0;
 memset(&r,0,sizeof(r));r.h.ah=2;r.x.dx=0;int86(0x17,&r,&r);
 return (r.h.ah&0x10)!=0;
}

int main(int argc,char **argv)
{
 double value=0;char entry[24]="",op=0,t[2];int bx,by,w,h,x,y,tx,ty,i,focus=-1,key=0,mx=0,my=0,ki,mode=CALC_MODE_BAS,extra,hit;unsigned mb=0;
 if(acc_help(argc,argv,"!CALC","Basic, scientific and financial calculator.  /PRINT sends calculator tape lines to LPT1."))return 0;
 for(i=1;i<argc;i++)if(!stricmp(argv[i],"/PRINT")||!stricmp(argv[i],"-PRINT"))print_mode=1;
 if(!acc_begin(argv[0],"Calculator",0))return 1;
 if(print_mode){if(!calc_lpt1_detected()){acc_notice("Print Error","No printing hardware detected.");print_mode=0;}else{print_file=fopen("LPT1","wb");if(!print_file){acc_notice("Print Error","No printing hardware detected.");print_mode=0;}}}
 calc_segments_install();
 while(key!=27){
  if(mode==CALC_MODE_BAS){w=31;h=18;bx=(acc_cols-80)/2+17;by=(acc_rows-25)/2+2;x=bx+3;y=by+6;tx=(acc_cols-80)/2+44;ty=(acc_rows-25)/2+6;draw_tape(tx,ty);}
  else {w=70;h=22;bx=(acc_cols-w)/2;by=(acc_rows-h)/2;x=bx+42;y=by+6;tx=ty=0;}
  acc_box(bx,by,w,h,"Calculator");
  output_box(bx+3,by+2,w-6,value,entry);
  if(mode==CALC_MODE_SCI){acc_text(bx+3,by+5,sci_degrees?"Scientific - Degrees":"Scientific - Radians",ACC_HEADING,21);draw_sci_keys(bx+3,by+6,focus);acc_put(bx+40,by+5,179,ACC_BORDER);for(i=6;i<18;i++)acc_put(bx+40,by+i,179,ACC_BORDER);}
  else if(mode==CALC_MODE_FIN){draw_fin_panel(bx+3,by+5,focus);acc_put(bx+40,by+5,179,ACC_BORDER);for(i=6;i<18;i++)acc_put(bx+40,by+i,179,ACC_BORDER);}
  draw_keys(x,y,focus);
  draw_calc_toolbar(bx,by,w,h,focus,mode);
  acc_wait(&key,&mx,&my,&mb);
  if(mode==CALC_MODE_BAS){
    if(key==256+72&&focus<0){if(tape_view<tape_count-TAPE_VISIBLE)tape_view++;key=0;continue;}
    if(key==256+80&&focus<0){if(tape_view>0)tape_view--;key=0;continue;}
    if((mb&1)&&mx==tx+19){if(my==ty&&tape_count>TAPE_VISIBLE+tape_view){tape_view++;key=0;continue;}if(my==ty+17&&tape_view>0){tape_view--;key=0;continue;}}
  }
  if((mb&1)&&my>=y&&my<=y+6&&mx>=x&&mx<x+23){
    for(i=0;i<18;i++){int xx=x+kx[i],yy=y+ky[i];if(mx>=xx&&mx<xx+kw[i]&&my==yy){focus=i;flash_key(x,y,i,focus);calc_action(keys[i],&value,entry,&op);break;}}
    key=0;continue;
  }
  if(mode==CALC_MODE_SCI&&(mb&1)){hit=sci_hit(bx+3,by+6,mx,my);if(hit>=0){focus=22+hit;flash_sci(bx+3,by+6,hit,focus);sci_action(hit,&value,entry,&op);key=0;continue;}}
  if(mode==CALC_MODE_FIN&&(mb&1)){hit=fin_hit(bx+3,by+5,mx,my);if(hit>=0){focus=22+hit;flash_fin(bx+3,by+5,hit,focus);fin_action(hit,&value,entry,&op);key=0;continue;}}
  if((mb&1)&&my==by+h-3){
    if(mx>=bx+3&&mx<bx+6){focus=18;value=0;entry[0]=op=0;tape_clear();key=0;continue;}
    else if(mx>=bx+8&&mx<bx+12){focus=19;entry[0]=0;key=0;continue;}
    else if(mx>=bx+14&&mx<bx+19){focus=20;acc_restore_text_screen();mode=(mode+1)%3;focus=-1;key=0;continue;}
    else if(mx>=bx+w-8&&mx<bx+w-4){focus=21;key=27;continue;}
  }
  extra=mode==CALC_MODE_SCI?SCI_KEY_COUNT:(mode==CALC_MODE_FIN?FIN_KEY_COUNT:0);
  if(key==9||key==271){int total=22+extra;focus=focus<0?((key==271)?total-1:0):((key==271)?(focus+total-1)%total:(focus+1)%total);key=0;continue;}
  if(focus<18&&(key==256+75||key==256+77||key==256+72||key==256+80)){focus=move_key(focus,key);key=0;continue;}
  if((key==13||key==' ')&&focus>=18){
    if(focus==18){value=0;entry[0]=op=0;tape_clear();}
    else if(focus==19)entry[0]=0;
    else if(focus==20){acc_restore_text_screen();mode=(mode+1)%3;focus=-1;}
    else if(focus==21){key=27;continue;}
    else if(mode==CALC_MODE_SCI&&focus<22+SCI_KEY_COUNT)sci_action(focus-22,&value,entry,&op);
    else if(mode==CALC_MODE_FIN&&focus<22+FIN_KEY_COUNT)fin_action(focus-22,&value,entry,&op);
    key=0;continue;
  }
  if(key==' '&&focus>=0&&focus<18){calc_action(keys[focus],&value,entry,&op);key=0;continue;}
  if(key==13){ki=key_index('=');flash_key(x,y,ki,focus);calc_action("=",&value,entry,&op);key=0;continue;}
  if(key=='c'||key=='C'){value=0;entry[0]=op=0;tape_clear();key=0;continue;}
  if(key=='e'||key=='E'||key==256+83){entry[0]=0;key=0;continue;}
  if(key==8&&*entry){entry[strlen(entry)-1]=0;key=0;continue;}
  if((key>='0'&&key<='9')||key=='.'||key=='+'||key=='-'||key=='*'||key=='/'||key=='%'||key=='='||key=='r'||key=='R'){
    ki=key_index(key);t[0]=(char)((key=='r'||key=='R')?'R':key);t[1]=0;flash_key(x,y,ki,focus);calc_action(t,&value,entry,&op);key=0;continue;
  }
  if(key!=27)key=0;
 }
 if(print_file){fflush(print_file);fclose(print_file);print_file=0;}
 acc_end_screen();calc_segments_restore();acc_end();return 0;
}

