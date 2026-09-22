/* Launch! Markdown - persistent Markdown editor, live text preview, VGA page
   renderer and formatted Epson-compatible printing.  Microsoft C/C++ 7.0. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <memory.h>
#include <malloc.h>
#include "ACCLIB.H"

#define PAGES 10
#define LINES 450
#define OLD_LINES 60
#define COLS 68
#define VIEW 7
#define FULL_VIEW 15
#define DLG_W 74
#define DLG_H 22
#define BTN_PRINT 1
#define BTN_SAVE 2
#define BTN_SPLIT 3
#define BTN_FOCUS 4
#define BTN_SHOW 5
#define BTN_CLOSE 6
/* Never set the intensity bit in a text-mode background: on standard VGA it
   enables blink.  !POP's selection is Launch!'s one intentional exception. */
#define MD_TEXT ACC_ATTR(7,0)
#define MD_HEADING1 ACC_ATTR(4,14)
#define MD_HEADING2 ACC_ATTR(0,14)
#define MD_HEADING3 ACC_ATTR(6,14)
#define MD_HEADING4 ACC_ATTR(3,4)
#define MD_HEADING5 ACC_ATTR(6,4)
#define MD_MARK ACC_ATTR(7,8)
#define MD_BOLD ACC_ATTR(7,15)
#define MD_ITALIC ACC_ATTR(7,10)
#define MD_BOLDITALIC ACC_ATTR(3,11)
#define MD_UNDERLINE ACC_ATTR(3,10)
#define MD_STRIKE ACC_ATTR(4,10)
#define MD_SUPER ACC_ATTR(5,7)
#define MD_SUB ACC_ATTR(6,0)
#define MD_QUOTE ACC_ATTR(7,6)
#define MD_LINK ACC_ATTR(7,9)
#define MD_CODE ACC_ATTR(0,15)

typedef char MDLINE[COLS];
static MDLINE far *page[PAGES];
static char paths[PAGES][ACC_PATH];
static unsigned char external[PAGES];
static unsigned char softwrap[PAGES][LINES];
#define CLIP_MAX (LINES*(COLS+2)+1)
static char far clip[CLIP_MAX];static int clip_len=0;
/* view_mode: 0=editor+preview, 1=editor only, 2=preview only. */
static int current=0,count=1,cx=0,cy=0,top=0,view_mode=0,dirty=0,insert_mode=1,sel_anchor=-1,sel_caret=-1;

static int ensure_page(int p){if(page[p])return 1;page[p]=(MDLINE far *)_fmalloc((unsigned)(LINES*COLS));if(!page[p])return 0;_fmemset(page[p],' ',(unsigned)(LINES*COLS));return 1;}
static int init_pages(void){memset(page,0,sizeof(page));return ensure_page(0);}
static void free_pages(void){int i;for(i=0;i<PAGES;i++)if(page[i]){_ffree(page[i]);page[i]=0;}}
static void blank_page(int p){if(ensure_page(p))_fmemset(page[p],' ',(unsigned)LINES*COLS);}
static int used(int p,int y){int n=COLS;while(n&&page[p][y][n-1]==' ')n--;return n;}
static void persistent_path(char *out){acc_path(out,"DATA","MKDOWN.DAT");}
static void persist_load(void){char p[ACC_PATH],b[COLS],magic[4];FILE*f;int i,y,j,last=0,modern=0,nonblank;blank_page(0);memset(softwrap,0,sizeof(softwrap));persistent_path(p);f=fopen(p,"rb");if(f){if(fread(magic,1,4,f)==4&&!memcmp(magic,"MKD2",4))modern=1;else rewind(f);for(i=0;i<PAGES;i++)for(y=0;y<(modern?LINES:OLD_LINES);y++){if(fread(b,1,COLS,f)!=COLS)goto loaded;nonblank=0;for(j=0;j<COLS;j++)if(b[j]!=' '){nonblank=1;break;}if(nonblank&&ensure_page(i)){_fmemcpy(page[i][y],b,COLS);last=i;}}if(modern)fread(softwrap,1,sizeof(softwrap),f);loaded:fclose(f);}count=last+1;if(count<1)count=1;}
static void persist_save(void){char p[ACC_PATH],b[COLS];FILE*f;int i,y;persistent_path(p);f=fopen(p,"wb");if(f){fwrite("MKD2",1,4,f);for(i=0;i<PAGES;i++)for(y=0;y<LINES;y++){if(page[i])_fmemcpy(b,page[i][y],COLS);else memset(b,' ',COLS);fwrite(b,1,COLS,f);}fwrite(softwrap,1,sizeof(softwrap),f);fclose(f);}}
static void force_md(char *p){char *dot,*slash=strrchr(p,'\\'),*other=strrchr(p,'/');if(other&&(!slash||other>slash))slash=other;dot=strrchr(p,'.');if(!dot||(slash&&dot<slash)){if(strlen(p)+3<ACC_PATH)strcat(p,".MD");}else strcpy(dot,".MD");}
static int load_file(int pg,const char *name){FILE*f;char line[512];int row=0,col=0,i,n;if(!ensure_page(pg))return 0;blank_page(pg);f=fopen(name,"r");if(!f)return 0;while(fgets(line,sizeof(line),f)&&row<LINES){n=(int)strlen(line);while(n&&(line[n-1]=='\r'||line[n-1]=='\n'))line[--n]=0;for(i=0;i<n&&row<LINES;i++){if(col==COLS){row++;col=0;}if(row<LINES)page[pg][row][col++]=line[i];}row++;col=0;}fclose(f);strncpy(paths[pg],name,ACC_PATH-1);paths[pg][ACC_PATH-1]=0;force_md(paths[pg]);external[pg]=1;return 1;}
static int write_file(int pg,const char *name){FILE*f=fopen(name,"w");char b[COLS];int y,n;if(!f)return 0;for(y=0;y<LINES;y++){n=used(pg,y);if(n){_fmemcpy(b,page[pg][y],n);fwrite(b,1,n,f);}fputc('\n',f);}if(fclose(f))return 0;strncpy(paths[pg],name,ACC_PATH-1);paths[pg][ACC_PATH-1]=0;external[pg]=1;dirty=0;return 1;}

static int has_selection(void){return sel_anchor>=0&&sel_caret>=0&&sel_anchor!=sel_caret;}
static int sel_low(void){return sel_anchor<sel_caret?sel_anchor:sel_caret;}
static int sel_high(void){return sel_anchor>sel_caret?sel_anchor:sel_caret;}
static void clear_selection(void){sel_anchor=sel_caret=-1;}
static int shift_down(void){return (*(unsigned char far *)(((unsigned long)0x40<<16)|0x17)&3)!=0;}
static void selection_move(int oldpos,int newpos,int shift){if(shift){if(sel_anchor<0)sel_anchor=oldpos;sel_caret=newpos;if(sel_caret==sel_anchor)clear_selection();}else clear_selection();}
static void copy_selection(void){int lo,hi,p,row,lastrow,n=0;if(!has_selection())return;lo=sel_low();hi=sel_high();lastrow=(hi-1)/COLS;for(p=lo;p<hi&&n<CLIP_MAX-3;p++){row=p/COLS;clip[n++]=page[current][row][p%COLS];if(p+1<hi&&(p+1)%COLS==0&&row<lastrow&&!softwrap[current][row+1]){clip[n++]='\r';clip[n++]='\n';}}clip_len=n;}
static void delete_selection(void){int lo,hi,fr,lr,r,start,end,n;if(!has_selection())return;lo=sel_low();hi=sel_high();fr=lo/COLS;lr=(hi-1)/COLS;for(r=fr;r<=lr;r++){start=r==fr?lo%COLS:0;end=r==lr?((hi-1)%COLS)+1:COLS;n=end-start;if(n>0){_fmemmove(page[current][r]+start,page[current][r]+end,COLS-end);_fmemset(page[current][r]+COLS-n,' ',n);}}cy=fr;cx=lo%COLS;clear_selection();dirty=1;}
static void word_wrap(void){int start=COLS-1,len,i,can=1;if(cy>=LINES-1)return;if(page[current][cy][COLS-1]==' '){cy++;cx=0;softwrap[current][cy]=1;return;}while(start>0&&page[current][cy][start-1]!=' ')start--;if(start<=0){cy++;cx=0;softwrap[current][cy]=1;return;}len=COLS-start;for(i=COLS-len;i<COLS;i++)if(page[current][cy+1][i]!=' ')can=0;if(!can){cy++;cx=0;softwrap[current][cy]=1;return;}_fmemmove(page[current][cy+1]+len,page[current][cy+1],COLS-len);_fmemcpy(page[current][cy+1],page[current][cy]+start,len);_fmemset(page[current][cy]+start,' ',COLS-start);cy++;cx=len;softwrap[current][cy]=1;}
static void insert_one(int ch){if(insert_mode&&cx<COLS-1)_fmemmove(page[current][cy]+cx+1,page[current][cy]+cx,COLS-cx-1);page[current][cy][cx]=(char)ch;if(cx<COLS-1)cx++;else word_wrap();dirty=1;}
static void paste_clip(void){int i,c;if(has_selection())delete_selection();for(i=0;i<clip_len&&cy<LINES;i++){c=(unsigned char)clip[i];if(c=='\r')continue;if(c=='\n'){if(cy<LINES-1){cy++;cx=0;softwrap[current][cy]=0;}continue;}insert_one(c);}clear_selection();}
static void remove_row(int row){int r;for(r=row;r<LINES-1;r++){_fmemcpy(page[current][r],page[current][r+1],COLS);softwrap[current][r]=softwrap[current][r+1];}_fmemset(page[current][LINES-1],' ',COLS);softwrap[current][LINES-1]=0;}
static void join_next_line(void){int n2,take,room;if(cy>=LINES-1)return;n2=used(current,cy+1);room=COLS-cx;if(room<0)room=0;take=n2<room?n2:room;if(take)_fmemcpy(page[current][cy]+cx,page[current][cy+1],take);if(take>=n2)remove_row(cy+1);else{_fmemmove(page[current][cy+1],page[current][cy+1]+take,COLS-take);_fmemset(page[current][cy+1]+COLS-take,' ',take);}dirty=1;}
static void join_previous_line(void){int prev,n,take,room;if(cy<=0)return;prev=used(current,cy-1);n=used(current,cy);room=COLS-prev;if(room<=0){cy--;cx=COLS-1;return;}take=n<room?n:room;if(take)_fmemcpy(page[current][cy-1]+prev,page[current][cy],take);if(take>=n)remove_row(cy);else{_fmemmove(page[current][cy],page[current][cy]+take,COLS-take);_fmemset(page[current][cy]+COLS-take,' ',take);}cy--;cx=prev;dirty=1;}
static void split_line(void){int r,n,tail;if(cy>=LINES-1)return;for(r=LINES-1;r>cy+1;r--){_fmemcpy(page[current][r],page[current][r-1],COLS);softwrap[current][r]=softwrap[current][r-1];}_fmemset(page[current][cy+1],' ',COLS);n=used(current,cy);tail=n>cx?n-cx:0;if(tail)_fmemcpy(page[current][cy+1],page[current][cy]+cx,tail);_fmemset(page[current][cy]+cx,' ',COLS-cx);softwrap[current][cy+1]=0;cy++;cx=0;dirty=1;}
static int editor_key(int key,int rows){int oldpos=cy*COLS+cx,shift=shift_down();if(key==3){copy_selection();return 1;}if(key==24){copy_selection();delete_selection();return 1;}if(key==22||key==16){paste_clip();return 1;}if(key==256+75&&cx>0)cx--;else if(key==256+77&&cx<COLS-1)cx++;else if(key==256+72&&cy>0)cy--;else if(key==256+80&&cy<LINES-1)cy++;else if(key==256+71)cx=0;else if(key==256+79){cx=COLS-1;while(cx>0&&page[current][cy][cx]==' ')cx--;if(page[current][cy][cx]!=' '&&cx<COLS-1)cx++;}else if(key==256+73){cy-=rows;if(cy<0)cy=0;}else if(key==256+81){cy+=rows;if(cy>=LINES)cy=LINES-1;}else if(key==256+82){clear_selection();insert_mode=!insert_mode;return 1;}else if(key==256+83){if(has_selection())delete_selection();else if(cx>=used(current,cy))join_next_line();else{_fmemmove(page[current][cy]+cx,page[current][cy]+cx+1,COLS-cx-1);page[current][cy][COLS-1]=' ';dirty=1;}return 1;}else if(key==8){if(has_selection())delete_selection();else if(cx>0){cx--;_fmemmove(page[current][cy]+cx,page[current][cy]+cx+1,COLS-cx-1);page[current][cy][COLS-1]=' ';dirty=1;}else join_previous_line();return 1;}else if(key==13&&cy<LINES-1){if(has_selection())delete_selection();split_line();clear_selection();return 1;}else if((key>=32&&key<=255)||(key>=513&&key<=767)){if(has_selection())delete_selection();insert_one(key>=512?key-512:key);clear_selection();return 1;}else return 0;selection_move(oldpos,cy*COLS+cx,shift);return 1;}

static void plain_line(int p,int row,char *out,int *style)
{
  int n=used(p,row),i=0,j=0,bold=0;char c;*style=0;
  while(i<n&&page[p][row][i]==' ')i++;
  if(i<n&&page[p][row][i]=='#'){while(i<n&&page[p][row][i]=='#'){(*style)++;i++;}while(i<n&&page[p][row][i]==' ')i++;}
  else if(i+1<n&&(page[p][row][i]=='-'||page[p][row][i]=='*'||page[p][row][i]=='+')&&page[p][row][i+1]==' '){out[j++]=(char)7;out[j++]=' ';i+=2;*style=7;}
  else if(i<n&&page[p][row][i]=='>'){out[j++]=(char)179;out[j++]=' ';i++;while(i<n&&page[p][row][i]==' ')i++;*style=7;}
  if(n-i>=3&&((page[p][row][i]=='-'&&page[p][row][i+1]=='-'&&page[p][row][i+2]=='-')||(page[p][row][i]=='*'&&page[p][row][i+1]=='*'&&page[p][row][i+2]=='*'))){while(j<COLS)out[j++]=(char)196;out[COLS]=0;*style=7;return;}
  while(i<n&&j<COLS){c=page[p][row][i++];if(c=='['){int k=i;while(k<n&&page[p][row][k]!=']')k++;if(k<n&&k+1<n&&page[p][row][k+1]=='('){while(i<k&&j<COLS)out[j++]=page[p][row][i++];i=k+2;while(i<n&&page[p][row][i]!=')')i++;if(i<n)i++;*style=9;continue;}}if((c=='{'&&i<n&&page[p][row][i]=='{')||(c=='}'&&i<n&&page[p][row][i]=='}')){i++;continue;}if((c=='*'||c=='_')&&i<n&&page[p][row][i]==c){i++;bold=!bold;continue;}if(c=='*'||c=='_'||c=='`')continue;if(c=='|'&&(j==0||out[j-1]!=' '))c=(char)179;out[j++]=c;}
  while(j<COLS)out[j++]=' ';out[COLS]=0;if(bold&&!*style)*style=8;
}
static int heading_attr(int h){if(h<=1)return MD_HEADING1;if(h==2)return MD_HEADING2;if(h==3)return MD_HEADING3;if(h==4)return MD_HEADING4;return MD_HEADING5;}
static void draw_source(int x,int y,int rows,int cursor)
{int r,c,pos,lo=has_selection()?sel_low():-1,hi=has_selection()?sel_high():-1;char b[COLS+1];for(r=0;r<rows;r++){int line=top+r;if(line<LINES){_fmemcpy(b,page[current][line],COLS);b[COLS]=0;}else memset(b,' ',COLS),b[COLS]=0;acc_text(x,y+r,b,ACC_CONTROL,COLS);if(lo>=0)for(c=0;c<COLS;c++){pos=line*COLS+c;if(pos>=lo&&pos<hi)acc_put(x+c,y+r,b[c],ACC_SELECT);}}if(cursor&&cy>=top&&cy<top+rows)acc_put(x+cx,y+cy-top,page[current][cy][cx],ACC_SELECT);}
static int line_fence(int p,int row){int i=0,n=used(p,row);while(i<n&&page[p][row][i]==' ')i++;return i+2<n&&page[p][row][i]=='`'&&page[p][row][i+1]=='`'&&page[p][row][i+2]=='`';}
static int code_before(int p,int row){int i,on=0;for(i=0;i<row;i++)if(line_fence(p,i))on=!on;return on;}
static int table_rule(const char *s,int n){int i,dash=0,pipe=0;for(i=0;i<n;i++){if(s[i]=='-')dash++;else if(s[i]=='|')pipe++;else if(s[i]!=' '&&s[i]!=':')return 0;}return dash>=3&&pipe>0;}
static int inline_attr(int bold,int italic,int underline,int strike,int code,int super,int sub)
{if(code)return MD_CODE;if(strike)return MD_STRIKE;if(underline)return MD_UNDERLINE;if(super)return MD_SUPER;if(sub)return MD_SUB;if(bold&&italic)return MD_BOLDITALIC;if(italic)return MD_ITALIC;if(bold)return MD_BOLD;return MD_TEXT;}
static void preview_line(int x,int y,int p,int row,int in_code)
{
  char s[COLS+1];int n,i=0,o=0,h=0,bold=0,italic=0,underline=0,strike=0,code=in_code,super=0,sub=0,quote=0,a,k;char c;
  _fmemcpy(s,page[p][row],COLS);s[COLS]=0;n=COLS;while(n&&s[n-1]==' ')n--;
  acc_fill(x,y,COLS,1,' ',in_code?MD_CODE:MD_TEXT);
  if(line_fence(p,row))return;
  while(i<n&&s[i]==' ')i++;
  if(!in_code&&i<n&&s[i]=='#'){while(i<n&&s[i]=='#'&&h<6){h++;i++;}while(i<n&&s[i]==' ')i++;a=heading_attr(h);for(k=0;k<h&&o<COLS;k++)acc_put(x+o++,y,30,a);if(o<COLS)acc_put(x+o++,y,' ',a);while(i<n&&o<COLS)acc_put(x+o++,y,s[i++],a);return;}
  if(!in_code&&table_rule(s,n)){for(i=0;i<n&&o<COLS;i++)acc_put(x+o++,y,s[i]=='|'?197:196,MD_TEXT);return;}
  if(!in_code&&i+2<n&&((s[i]=='-'&&s[i+1]=='-'&&s[i+2]=='-')||(s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*'))){while(o<COLS)acc_put(x+o++,y,196,MD_MARK);return;}
  if(!in_code&&i+1<n&&(s[i]=='-'||s[i]=='*'||s[i]=='+')&&s[i+1]==' '){acc_put(x+o++,y,7,MD_MARK);acc_put(x+o++,y,' ',MD_TEXT);i+=2;}
  else if(!in_code&&i<n&&s[i]=='>'){quote=1;acc_put(x+o++,y,179,MD_QUOTE);acc_put(x+o++,y,' ',MD_TEXT);i++;while(i<n&&s[i]==' ')i++;}
  else if(!in_code&&isdigit((unsigned char)s[i])){k=i;while(k<n&&isdigit((unsigned char)s[k]))k++;if(k+1<n&&s[k]=='.'&&s[k+1]==' '){while(i<=k&&o<COLS)acc_put(x+o++,y,s[i++],MD_MARK);if(o<COLS)acc_put(x+o++,y,' ',MD_TEXT);i++;}}
  else if(!in_code&&i+2<n&&s[i]=='['&&(s[i+1]==' '||s[i+1]=='x'||s[i+1]=='X')&&s[i+2]==']'){acc_put(x+o++,y,(s[i+1]==' ')?250:251,MD_MARK);acc_put(x+o++,y,' ',MD_TEXT);i+=3;while(i<n&&s[i]==' ')i++;}
  while(i<n&&o<COLS){
    if(!in_code&&i+3<n&&!strnicmp(s+i,"<u>",3)){underline=1;i+=3;continue;}if(!in_code&&i+4<n&&!strnicmp(s+i,"</u>",4)){underline=0;i+=4;continue;}
    if(!in_code&&i+5<n&&!strnicmp(s+i,"<del>",5)){strike=1;i+=5;continue;}if(!in_code&&i+6<n&&!strnicmp(s+i,"</del>",6)){strike=0;i+=6;continue;}
    if(!in_code&&i+1<n&&s[i]=='~'&&s[i+1]=='~'){strike=!strike;i+=2;continue;}
    if(!in_code&&i+2<n&&((s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*')||(s[i]=='_'&&s[i+1]=='_'&&s[i+2]=='_'))){bold=!bold;italic=!italic;i+=3;continue;}
    if(!in_code&&i+1<n&&s[i]=='*'&&s[i+1]=='*'){bold=!bold;i+=2;continue;}
    if(!in_code&&i+1<n&&s[i]=='_'&&s[i+1]=='_'){italic=!italic;i+=2;continue;}
    if(!in_code&&(s[i]=='*'||s[i]=='_')){italic=!italic;i++;continue;}
    if(!in_code&&i+1<n&&s[i]=='{'&&s[i+1]=='{'){underline=1;i+=2;continue;}
    if(!in_code&&i+1<n&&s[i]=='}'&&s[i+1]=='}'){underline=0;i+=2;continue;}
    if(!in_code&&i+5<n&&!strnicmp(s+i,"<sup>",5)){super=1;i+=5;continue;}if(!in_code&&i+6<n&&!strnicmp(s+i,"</sup>",6)){super=0;i+=6;continue;}
    if(!in_code&&i+5<n&&!strnicmp(s+i,"<sub>",5)){sub=1;i+=5;continue;}if(!in_code&&i+6<n&&!strnicmp(s+i,"</sub>",6)){sub=0;i+=6;continue;}
    if(!in_code&&s[i]=='`'){code=!code;i++;continue;}
    if(!in_code&&s[i]=='['){k=i+1;while(k<n&&s[k]!=']')k++;if(k<n&&k+1<n&&s[k+1]=='('){i++;while(i<k&&o<COLS)acc_put(x+o++,y,s[i++],MD_LINK);i=k+2;while(i<n&&s[i]!=')')i++;if(i<n)i++;continue;}}
    if(!in_code&&s[i]=='<'&&i+7<n&&!strnicmp(s+i+1,"http",4)){k=i+1;while(k<n&&s[k]!='>')k++;if(k<n){i++;while(i<k&&o<COLS)acc_put(x+o++,y,s[i++],MD_LINK);i++;continue;}}
    c=s[i++];if(!in_code&&c=='|')c=(char)179;a=inline_attr(bold,italic,underline,strike,code,super,sub);if(quote&&a==MD_TEXT)a=MD_QUOTE;acc_put(x+o++,y,c,a);
  }
}
static void draw_preview(int x,int y,int rows)
{int r,line,on=code_before(current,top);for(r=0;r<rows;r++){line=top+r;if(line<LINES){preview_line(x,y+r,current,line,on);if(line_fence(current,line))on=!on;}else acc_fill(x,y+r,COLS,1,' ',MD_TEXT);}}
static int page_token_x(int x,int pg){int i,xx=x;char s[6];for(i=0;i<pg;i++){sprintf(s," %d ",i+1);xx+=(int)strlen(s)+1;}return xx;}
static void page_bar(int x,int y)
{int i,xx=x,n,a,w;char s[6],shown[29],*base;acc_fill(x,y,COLS+1,1,' ',ACC_BG);for(i=0;i<count;i++){sprintf(s," %d ",i+1);w=(int)strlen(s);a=i==current?ACC_CONTROL:ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg);acc_text(xx,y,s,a,w);xx+=w+1;}if(count<PAGES)acc_put(xx,y,'+',ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg));if(external[current]&&paths[current][0]){base=strrchr(paths[current],'\\');if(!base)base=strrchr(paths[current],'/');base=base?base+1:paths[current];n=(int)strlen(base);if(n>28){base+=n-28;n=28;}memcpy(shown,base,n);shown[n]=0;acc_text(x+COLS-n,y,shown,ACC_HEADING,n);}else acc_text(x+COLS-7,y,"Unsaved",ACC_TITLE,7);}
static void toolbar(int x,int y,int focus)
{acc_button(x,y," Export ",focus==BTN_SAVE);acc_button(x+8,y," Print ",focus==BTN_PRINT);acc_put(x+16,y,179,ACC_BORDER);acc_button(x+18,y," Split ",focus==BTN_SPLIT);acc_button(x+27,y," Focus ",focus==BTN_FOCUS);acc_button(x+36,y," Show ",focus==BTN_SHOW);acc_button(x+61,y," Close ",focus==BTN_CLOSE);}
static void draw_ui(int x,int y,int focus)
{int i,rows=view_mode==0?VIEW:FULL_VIEW,line_attr=ACC_ATTR(acc_appearance.background,acc_appearance.border);acc_box(x,y,DLG_W,DLG_H,"Markdown");if(view_mode==0){draw_source(x+2,y+2,VIEW,focus==0);for(i=0;i<DLG_W-2;i++)acc_put(x+1+i,y+9,196,line_attr);draw_preview(x+2,y+10,VIEW);}else if(view_mode==1)draw_source(x+2,y+2,FULL_VIEW,focus==0);else draw_preview(x+2,y+2,FULL_VIEW);acc_scrollbar(x+70,y+2,rows,top,LINES,rows);page_bar(x+3,y+17);for(i=0;i<DLG_W-6;i++)acc_put(x+3+i,y+18,196,ACC_BORDER);toolbar(x+3,y+19,focus);}

static int filename_dialog(char *out)
{int w=50,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=(int)strlen(out),f=0;unsigned mb=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Save Markdown",1);acc_text(x+3,y+2,"Filename:",ACC_LABEL,10);acc_fill(x+13,y+2,32,1,' ',f==0?ACC_SELECT:ACC_CONTROL);acc_text(x+13,y+2,out,f==0?ACC_SELECT:ACC_CONTROL,31);acc_button(x+15,y+6," Save ",f==1);acc_button(x+25,y+6," Cancel ",f==2);acc_wait(&k,&mx,&my,&mb);if(k==27||((mb&1)&&my==y+6&&mx>=x+25)){acc_modal_end();return 0;}if((mb&1)&&my==y+2){f=0;k=0;}else if((mb&1)&&my==y+6&&mx>=x+15&&mx<x+22){f=1;k=13;}if(k==9||k==271){f=k==271?(f+2)%3:(f+1)%3;k=0;continue;}if(k==13&&f==2){acc_modal_end();return 0;}if(k==13&&f==1){if(pos){force_md(out);acc_modal_end();return 1;}k=0;continue;}if(f==0){if(k==8&&pos)out[--pos]=0;else if(k>=32&&k<127&&pos<ACC_PATH-4){out[pos++]=(char)k;out[pos]=0;}}k=0;}}
static void save_current(void)
{char name[ACC_PATH],full[ACC_PATH],msg[ACC_PATH+24];if(external[current])strcpy(name,paths[current]);else strcpy(name,"DOCUMENT.MD");if(!external[current]&&!filename_dialog(name))return;if(!external[current]&&!strchr(name,'\\')&&!strchr(name,'/')&&!strchr(name,':'))acc_path(full,"EXPORT",name);else strcpy(full,name);if(write_file(current,full)){sprintf(msg,"Saved to\n%s",full);acc_notice("Save",msg);}else acc_notice("Save Error","Unable to save Markdown file.");}

static unsigned char far showfont[4096],showfontb[4096],showfonti[4096];static int showfontb_loaded=0,showfonti_loaded=0;
static int graph_load_file(const char *file,unsigned char far *dest){char name[ACC_PATH];FILE*f;unsigned i;int c;acc_path(name,"",file);f=fopen(name,"rb");if(!f)return 0;for(i=0;i<4096&&(c=fgetc(f))!=EOF;i++)dest[i]=(unsigned char)c;fclose(f);return i==4096;}
static int graph_load_font(void){unsigned font_segment=0,font_offset=0,i;unsigned char far*src;if(graph_load_file("PROFONT.FNT",showfont)){showfontb_loaded=graph_load_file("PROFONTB.FNT",showfontb);showfonti_loaded=graph_load_file("PROFONTI.FNT",showfonti);return 1;}
#ifndef __GNUC__
 _asm {
  push bp
  mov ax,1130h
  mov bh,6
  int 10h
  mov ax,es
  mov font_segment,ax
  mov font_offset,bp
  pop bp
 }
#endif
 if(!font_segment)return 0;src=(unsigned char far*)(((unsigned long)font_segment<<16)|font_offset);for(i=0;i<4096;i++)showfont[i]=src[i];showfontb_loaded=showfonti_loaded=0;return 1;}
static void graph_clear(int colour){unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p;outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);_fmemset(v,(colour&(1<<p))?255:0,(unsigned)38400L);}outp(0x3C4,2);outp(0x3C5,15);}
static void graph_band(int y0,int y1,int colour){unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p,y;outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);for(y=y0;y<=y1;y++)_fmemset(v+y*80,(colour&(1<<p))?255:0,80);}outp(0x3C4,2);outp(0x3C5,15);}
static void graph_page(void){graph_clear(1);graph_band(464,479,7);}
/* BIOS pixel plotting is deliberately used here.  The MediaGX VGA-compatible
   hardware does not preserve the planar latches expected by the former direct
   write-mode-2 routine, producing coloured vertical fragments.  INT 10h/0Ch
   is slower but adapter-safe and exact for this print-preview workload. */
static void gpixel(int x,int y,int c){union REGS r;if(x<0||x>=640||y<0||y>=480)return;memset(&r,0,sizeof(r));r.h.ah=0x0C;r.h.al=(unsigned char)c;r.h.bh=0;r.x.cx=(unsigned)x;r.x.dx=(unsigned)y;int86(0x10,&r,&r);}
static int gchar_width(int scale){return 8*scale;}
static void gchar(int x,int y,unsigned char ch,int scale,int colour,int style){int r,b,sx,sy,slant,use_italic=(style&2)&&!(style&1)&&showfonti_loaded,synthetic=(style&1)&&!showfontb_loaded;unsigned char bits;unsigned char far*font=((style&1)&&showfontb_loaded)?showfontb:(use_italic?showfonti:showfont);for(r=0;r<16;r++){bits=font[(unsigned)ch*16+r];slant=((style&2)&&!use_italic)?(15-r)/5:0;for(b=0;b<8;b++)if(bits&(0x80>>b))for(sy=0;sy<scale;sy++)for(sx=0;sx<scale;sx++){gpixel(x+(b+slant)*scale+sx,y+r*scale+sy,colour);if(synthetic)gpixel(x+(b+slant)*scale+sx+1,y+r*scale+sy,colour);}}if(style&4)for(b=0;b<8*scale;b++)gpixel(x+b,y+14*scale,colour);if(style&8)for(b=0;b<8*scale;b++)gpixel(x+b,y+8*scale,colour);}
static void gtext(int x,int y,const char*s,int scale,int colour){int w=gchar_width(scale);while(*s){gchar(x,y,(unsigned char)*s++,scale,colour,0);x+=w;}}
static int video_is_vga(void){union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);return r.h.al==0x1A;}
static void graph_rule(int y,int double_line){int x;for(x=32;x<608;x++)gpixel(x,y,7);if(double_line)for(x=32;x<608;x++)gpixel(x,y+2,7);}
static int graph_line(int p,int row,int ypos,int in_code)
{
 char s[COLS+1];int n=used(p,row),i=0,x=32,h=0,scale=1,height=16;
 int bold=0,italic=0,underline=0,strike=0,quote=0,inline_code,k,style,w;
 _fmemcpy(s,page[p][row],COLS);s[COLS]=0;
 while(i<n&&s[i]==' ')i++;
 if(in_code){
  graph_band(ypos-2,ypos+height+2,0);
  while(i<n&&x+gchar_width(scale)<632){
   gchar(x,ypos,(unsigned char)s[i++],scale,7,0);x+=gchar_width(scale);
  }
  return height+5;
 }
 if(i<n&&s[i]=='#'){
  while(i<n&&s[i]=='#'&&h<6){h++;i++;}
  while(i<n&&s[i]==' ')i++;scale=h==1?2:1;height=16*scale;bold=1;
 }else if(i+1<n&&(s[i]=='-'||s[i]=='*'||s[i]=='+')&&s[i+1]==' '){
  gchar(x,ypos,7,scale,7,0);x+=gchar_width(scale)*2;i+=2;
 }else if(i<n&&s[i]=='>'){
  quote=1;italic=1;for(k=0;k<height;k++)gpixel(x,ypos+k,7);
  x+=12;i++;while(i<n&&s[i]==' ')i++;
 }
 inline_code=strchr(s+i,'`')!=0;
 if(inline_code)graph_band(ypos-1,ypos+height,0);
 w=gchar_width(scale);
 while(i<n&&x+w<632){
  if(i+3<=n&&!strnicmp(s+i,"<u>",3)){underline=1;i+=3;continue;}
  if(i+4<=n&&!strnicmp(s+i,"</u>",4)){underline=0;i+=4;continue;}
  if(i+2<=n&&s[i]=='{'&&s[i+1]=='{'){underline=1;i+=2;continue;}
  if(i+2<=n&&s[i]=='}'&&s[i+1]=='}'){underline=0;i+=2;continue;}
  if(i+2<=n&&s[i]=='~'&&s[i+1]=='~'){strike=!strike;i+=2;continue;}
  if(i+3<=n&&((s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*')||
                (s[i]=='_'&&s[i+1]=='_'&&s[i+2]=='_'))){
   bold=!bold;italic=!italic;i+=3;continue;
  }
  if(i+2<=n&&s[i]=='*'&&s[i+1]=='*'){bold=!bold;i+=2;continue;}
  if(i+2<=n&&s[i]=='_'&&s[i+1]=='_'){italic=!italic;i+=2;continue;}
  if(s[i]=='*'||s[i]=='_'){italic=!italic;i++;continue;}
  if(s[i]=='`'){i++;continue;}
  if(s[i]=='['){
   k=i+1;while(k<n&&s[k]!=']')k++;
   if(k<n&&k+1<n&&s[k+1]=='('){
    i++;
    while(i<k&&x+w<632){
     style=(bold?1:0)|(italic?2:0)|(underline?4:0)|(strike?8:0);
     gchar(x,ypos,(unsigned char)s[i++],scale,11,style);x+=w;
    }
    i=k+2;while(i<n&&s[i]!=')')i++;if(i<n)i++;continue;
   }
  }
  style=(bold?1:0)|(italic?2:0)|(underline?4:0)|(strike?8:0);
  gchar(x,ypos,(unsigned char)s[i++],scale,7,style);x+=w;
 }
 if(h==1){graph_rule(ypos+height+1,1);return height+8;}
 if(h==2){graph_rule(ypos+height+1,0);return height+6;}
 return height+(quote?5:4);
}
static void graph_render(int p,int first)
{int row,ypos=12,last=first+22,on=code_before(p,first);if(last>LINES)last=LINES;graph_page();for(row=first;row<last&&ypos<445;row++){if(line_fence(p,row)){on=!on;ypos+=3;continue;}ypos+=graph_line(p,row,ypos,on);}gtext(8,464,"Esc to close",1,1);gtext(408,464,"Up/Down/PgUp/Dn to scroll",1,1);}
static void graph_show(void)
{union REGS r;unsigned key;int first=0,p=current,scan;if(!video_is_vga()){acc_notice("Show","VGA display adapter required.");return;}if(!graph_load_font()){acc_notice("Show","Unable to read the VGA 8x16 font.");return;}acc_mouse_display(0);memset(&r,0,sizeof(r));r.h.ah=0;r.h.al=0x12;int86(0x10,&r,&r);graph_render(p,first);for(;;){key=_bios_keybrd(_KEYBRD_READ);if((key&255)==27)break;scan=(key>>8)&255;if(scan==0x48||scan==0x49){first-=20;if(first<0)first=0;}else if(scan==0x50||scan==0x51){first+=20;if(first>LINES-1)first=LINES-1;}else if(scan==0x4B&&p>0){p--;first=0;}else if(scan==0x4D&&p+1<count){p++;first=0;}else continue;graph_render(p,first);}outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);acc_restore_screen();acc_mouse_reapply_cursor();acc_mouse_display(1);}

static void print_current(void)
{FILE*f=fopen("LPT1","wb");char line[COLS+1];int row,style,n;if(!f){acc_notice("Print","Unable to open LPT1.");return;}fputs("\033@",f);for(row=0;row<LINES;row++){plain_line(current,row,line,&style);n=COLS;while(n&&line[n-1]==' ')n--;line[n]=0;if(style==1)fputs("\033E\033W\001",f);else if(style>1&&style<=6)fputs("\033E\033-\001",f);fputs(line,f);if(style==1)fputs("\033W\000\033F",f);else if(style>1&&style<=6)fputs("\033-\000\033F",f);fputs("\r\n",f);}fputc('\f',f);fclose(f);acc_notice("Print","Formatted Markdown sent to LPT1.");}
static void document_stats(unsigned long *chars,unsigned long *words)
{int y,i,n,inword=0;char c;*chars=*words=0;for(y=0;y<LINES;y++){n=used(current,y);*chars+=(unsigned long)n;if(n&&y+1<LINES)(*chars)++;for(i=0;i<n;i++){c=page[current][y][i];if(isalnum((unsigned char)c)){if(!inword){(*words)++;inword=1;}}else inword=0;}inword=0;}}
static void focus_draw(void)
{char b[COLS+1],s[42];int r,c,line,pos,lo=has_selection()?sel_low():-1,hi=has_selection()?sel_high():-1,panel=ACC_ATTR(acc_appearance.background,acc_appearance.labels),status=ACC_ATTR(acc_appearance.labels,acc_appearance.background);unsigned long chars,words;acc_clear(panel);for(r=0;r<24;r++){line=top+r;if(line<LINES){_fmemcpy(b,page[current][line],COLS);b[COLS]=0;acc_text(0,r,b,panel,COLS);if(lo>=0)for(c=0;c<COLS;c++){pos=line*COLS+c;if(pos>=lo&&pos<hi)acc_put(c,r,b[c],ACC_SELECT);}}}if(cy>=top&&cy<top+24)acc_put(cx,cy-top,page[current][cy][cx],ACC_SELECT);document_stats(&chars,&words);sprintf(s," Chars: %lu   Words: %lu",chars,words);acc_fill(0,24,acc_cols,1,' ',status);acc_text(0,24,s,status,(int)strlen(s));}
static void focus_mode(void)
{int key=0,mx,my;unsigned mb;focus_draw();while(key!=27){acc_wait(&key,&mx,&my,&mb);if(key!=27)editor_key(key,24);if(cy<top)top=cy;if(cy>=top+24)top=cy-23;if(key!=27)focus_draw();}acc_restore_screen();acc_mouse_reapply_cursor();acc_mouse_display(1);key=0;}

int main(int argc,char **argv)
{
  int x,y,key=0,mx=0,my=0,focus=0,i,argn=0,rows;unsigned mb=0;
  if(acc_help(argc,argv,"!MKDOWN","Markdown editor with live preview, graphical pages and formatted printing."))return 0;
  if(!acc_begin(argv[0],"Markdown",0))return 1;if(!init_pages()){acc_notice("Markdown","Not enough memory for document pages.");acc_end();return 1;}memset(external,0,sizeof(external));memset(paths,0,sizeof(paths));
  for(i=1;i<argc&&argn<PAGES;i++)if(argv[i][0]!='/'&&argv[i][0]!='-'){if(load_file(argn,argv[i]))argn++;}
  if(argn){count=argn;current=0;}else persist_load();
  x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;draw_ui(x,y,focus);
  while(key!=27){acc_wait(&key,&mx,&my,&mb);rows=view_mode==0?VIEW:FULL_VIEW;
    if(view_mode!=2&&(mb&1)&&my>=y+2&&my<y+2+rows&&mx>=x+2&&mx<x+2+COLS){focus=0;clear_selection();cx=mx-(x+2);cy=top+my-(y+2);draw_ui(x,y,focus);key=0;continue;}
    if((mb&1)&&my==y+17){int pg,xx=x+3;for(pg=0;pg<count;pg++){char ps[6];int pw;sprintf(ps," %d ",pg+1);pw=(int)strlen(ps);if(mx>=xx&&mx<xx+pw)break;xx+=pw+1;}if(pg<count){current=pg;cx=cy=top=0;clear_selection();focus=0;draw_ui(x,y,focus);key=0;continue;}if(count<PAGES&&mx==xx){if(!ensure_page(count)){acc_notice("Markdown","Not enough memory for another page.");draw_ui(x,y,focus);key=0;continue;}current=count++;blank_page(current);memset(softwrap[current],0,LINES);paths[current][0]=0;external[current]=0;cx=cy=top=0;clear_selection();focus=0;draw_ui(x,y,focus);key=0;continue;}}
    if((mb&1)&&mx==x+70&&my>=y+2&&my<y+2+rows){if(my==y+2&&top>0)top--;else if(my==y+1+rows&&top<LINES-rows)top++;else if(my>y+2&&my<y+1+rows)top=(my-y-3)*(LINES-rows)/(rows-2);if(top<0)top=0;if(top>LINES-rows)top=LINES-rows;if(cy<top)cy=top;if(cy>=top+rows)cy=top+rows-1;draw_ui(x,y,focus);key=0;continue;}
    if((mb&1)&&my==y+19){int hit=mx-(x+3);if(hit>=0&&hit<6)focus=BTN_SAVE;else if(hit>=8&&hit<14)focus=BTN_PRINT;else if(hit>=18&&hit<25)focus=BTN_SPLIT;else if(hit>=27&&hit<34)focus=BTN_FOCUS;else if(hit>=36&&hit<42)focus=BTN_SHOW;else if(hit>=61&&hit<67)focus=BTN_CLOSE;else{key=0;continue;}key=13;}
    if(key==9||key==271){focus=key==271?(focus?focus-1:BTN_CLOSE):(focus==BTN_CLOSE?0:focus+1);draw_ui(x,y,focus);key=0;continue;}
    if(focus&&key==13){if(focus==BTN_SHOW){graph_show();draw_ui(x,y,focus);}else if(focus==BTN_PRINT)print_current();else if(focus==BTN_SAVE){save_current();draw_ui(x,y,focus);}else if(focus==BTN_SPLIT){view_mode=(view_mode+1)%3;top=0;if(view_mode==2)clear_selection();draw_ui(x,y,focus);}else if(focus==BTN_FOCUS){focus_mode();draw_ui(x,y,focus);}else key=27;if(key!=27)key=0;continue;}
    if(focus){if(key!=27)key=0;continue;}
    if(view_mode==2){if(key!=27)key=0;}else if(key!=27&&!editor_key(key,rows))key=0;
    if(cy<top)top=cy;if(cy>=top+rows)top=cy-rows+1;draw_ui(x,y,focus);
  }
  if(!argn)persist_save();free_pages();acc_end();return 0;
}
