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
static void apply(const char *s,double *v,char *entry,char *op)
{
 double n;if(strlen(s)==1&&((*s>='0'&&*s<='9')||*s=='.')){if(digit_count(entry)<12&&(*s!='.'||!strchr(entry,'.'))){entry[strlen(entry)+1]=0;entry[strlen(entry)]=*s;}return;}
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
 if(*op=='+')*v+=n;else if(*op=='-')*v-=n;else if(*op=='*')*v*=n;else if(*op=='/'&&n!=0)*v/=n;else *v=n;
 entry[0]=0;if(strcmp(s,"="))*op=*s;else *op=0;
}
static void calc_action(const char *s,double *value,char *entry,char *op)
{
 char before[24];
 strncpy(before,entry,23);before[23]=0;
 if(strlen(s)==1&&((*s>='0'&&*s<='9')||*s=='.')){apply(s,value,entry,op);return;}
 if(!strcmp(s,"+")||!strcmp(s,"-")||!strcmp(s,"*")||!strcmp(s,"/")){
   tape_add(before,*value,s[0]);
   apply(s,value,entry,op);
   return;
 }
 if(!strcmp(s,"=")){
   tape_add(before,*value,'=');
   apply(s,value,entry,op);
   tape_add("",*value,0);
   tape_add("",0,(char)196);
   return;
 }
 if(!strcmp(s,"%")||!strcmp(s,"R")){
   tape_add(before,*value,s[0]);
   apply(s,value,entry,op);
   tape_add("",*value,0);
   tape_add("",0,(char)196);
 }
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
 double value=0;char entry[24]="",op=0,t[2];int bx,by,x,y,tx,ty,i,focus=-1,key=0,mx=0,my=0,ki;unsigned mb=0;
 if(acc_help(argc,argv,"!CALC","A simple four-function desktop calculator.  /PRINT sends each tape line to LPT1."))return 0;
 for(i=1;i<argc;i++)if(!stricmp(argv[i],"/PRINT")||!stricmp(argv[i],"-PRINT"))print_mode=1;
 if(!acc_begin(argv[0],"Calculator",0))return 1;
 if(print_mode){if(!calc_lpt1_detected()){acc_notice("Print Error","No printing hardware detected.");print_mode=0;}else{print_file=fopen("LPT1","wb");if(!print_file){acc_notice("Print Error","No printing hardware detected.");print_mode=0;}}}
 calc_segments_install();
 /* CALC2.ASC is the authoritative 80x25 layout.  Keep these coordinates
    relative to an 80x25 canvas so the overlap is exact. */
 bx=(acc_cols-80)/2+17;by=(acc_rows-25)/2+2;x=bx+3;y=by+6;
 tx=(acc_cols-80)/2+44;ty=(acc_rows-25)/2+6;
 while(key!=27){
  /* Tape is deliberately painted first.  The calculator is then painted
     over its left four columns, making the tape sit visually behind it. */
  draw_tape(tx,ty);
  acc_box(bx,by,31,18,"Calculator");
  output_box(x,by+2,25,value,entry);draw_keys(x,y,focus);
  acc_button(bx+3,by+15,"  C  ",focus==18);acc_button(bx+10,by+15,"  CE  ",focus==19);acc_button(bx+21,by+15,"  Exit  ",focus==20);
  acc_wait(&key,&mx,&my,&mb);
  /* Up/Down scroll the calculator paper.  Clicking its overflow arrows does the same. */
  if(key==256+72){if(tape_view<tape_count-TAPE_VISIBLE)tape_view++;key=0;continue;}
  if(key==256+80){if(tape_view>0)tape_view--;key=0;continue;}
  if((mb&1)&&mx==tx+19){
    if(my==ty&&tape_count>TAPE_VISIBLE+tape_view){tape_view++;key=0;continue;}
    if(my==ty+17&&tape_view>0){tape_view--;key=0;continue;}
  }
  if((mb&1)&&my>=y&&my<=y+6&&mx>=x&&mx<x+23){
    for(i=0;i<18;i++){int xx=x+kx[i],yy=y+ky[i];if(mx>=xx&&mx<xx+kw[i]&&my==yy){focus=i;flash_key(x,y,i,focus);calc_action(keys[i],&value,entry,&op);break;}}
    key=0;continue;
  }
  if((mb&1)&&my==by+15){if(mx>=bx+3&&mx<bx+8){focus=18;value=0;entry[0]=op=0;tape_clear();key=0;continue;}else if(mx>=bx+10&&mx<bx+16){focus=19;entry[0]=0;key=0;continue;}else if(mx>=bx+21&&mx<bx+27){focus=20;key=27;continue;}}
  if(key==9||key==271){focus=focus<0?((key==271)?20:0):((key==271)?(focus+20)%21:(focus+1)%21);key=0;}
  else if(focus<18&&(key==256+75||key==256+77||key==256+72||key==256+80)){focus=move_key(focus,key);key=0;}
  else if(key==13){
    ki=key_index('=');flash_key(x,y,ki,focus);calc_action("=",&value,entry,&op);key=0;
  }
  else if(key=='c'||key=='C'){value=0;entry[0]=op=0;tape_clear();key=0;}
  else if(key=='e'||key=='E'||key==256+83){entry[0]=0;key=0;}
  else if(key==' '&&focus>=0){key=0;}
  else if(key==8&&*entry){entry[strlen(entry)-1]=0;key=0;}
  else if((key>='0'&&key<='9')||key=='.'||key=='+'||key=='-'||key=='*'||key=='/'||key=='%'||key=='='||key=='r'||key=='R'){
    ki=key_index(key);t[0]=(char)((key=='r'||key=='R')?'R':key);t[1]=0;flash_key(x,y,ki,focus);calc_action(t,&value,entry,&op);key=0;
  }else if(key!=27)key=0;
 }
 if(print_file){fflush(print_file);fclose(print_file);print_file=0;}
 acc_end_screen();calc_segments_restore();acc_end();return 0;
}
