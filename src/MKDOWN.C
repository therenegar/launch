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
#define BTN_CHARS 6
#define BTN_CLOSE 7
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
static int current=0,count=1,cx=0,cy=0,top=0,view_mode=0,insert_mode=1,sel_anchor=-1,sel_caret=-1;
static unsigned char dirty[PAGES];

static int ensure_page(int p){if(page[p])return 1;page[p]=(MDLINE far *)_fmalloc((unsigned)(LINES*COLS));if(!page[p])return 0;_fmemset(page[p],' ',(unsigned)(LINES*COLS));return 1;}
static int init_pages(void){memset(page,0,sizeof(page));return ensure_page(0);}
static void free_pages(void){int i;for(i=0;i<PAGES;i++)if(page[i]){_ffree(page[i]);page[i]=0;}}
static void blank_page(int p){if(ensure_page(p))_fmemset(page[p],' ',(unsigned)LINES*COLS);}
static int used(int p,int y){int n=COLS;while(n&&page[p][y][n-1]==' ')n--;return n;}
static void persistent_path(char *out){acc_path(out,"DATA","MKDOWN.DAT");}
static void persist_load(void){char p[ACC_PATH],b[COLS],magic[4];FILE*f;int i,y,j,last=0,modern=0,nonblank;blank_page(0);memset(softwrap,0,sizeof(softwrap));persistent_path(p);f=fopen(p,"rb");if(f){if(fread(magic,1,4,f)==4&&!memcmp(magic,"MKD2",4))modern=1;else rewind(f);for(i=0;i<PAGES;i++)for(y=0;y<(modern?LINES:OLD_LINES);y++){if(fread(b,1,COLS,f)!=COLS)goto loaded;nonblank=0;for(j=0;j<COLS;j++)if(b[j]!=' '){nonblank=1;break;}if(nonblank&&ensure_page(i)){_fmemcpy(page[i][y],b,COLS);last=i;}}if(modern)fread(softwrap,1,sizeof(softwrap),f);loaded:fclose(f);}count=last+1;if(count<1)count=1;}
static void persist_save(void){char p[ACC_PATH],b[COLS];FILE*f;int i,y;persistent_path(p);f=fopen(p,"wb");if(f){fwrite("MKD2",1,4,f);for(i=0;i<PAGES;i++)for(y=0;y<LINES;y++){if(page[i])_fmemcpy(b,page[i][y],COLS);else memset(b,' ',COLS);fwrite(b,1,COLS,f);}fwrite(softwrap,1,sizeof(softwrap),f);fclose(f);}}
static void force_md(char *p){char *dot,*slash=strrchr(p,'\\'),*other=strrchr(p,'/');if(other&&(!slash||other>slash))slash=other;dot=strrchr(p,'.');if(!dot||(slash&&dot<slash)){if(strlen(p)+3<ACC_PATH)strcat(p,".MD");}else strcpy(dot,".MD");}
static int load_file(int pg,const char *name){FILE*f;char line[512];int row=0,pos,n,left,cut;if(!ensure_page(pg))return 0;blank_page(pg);memset(softwrap[pg],0,LINES);f=fopen(name,"r");if(!f)return 0;while(fgets(line,sizeof(line),f)&&row<LINES){n=(int)strlen(line);while(n&&(line[n-1]=='\r'||line[n-1]=='\n'))line[--n]=0;pos=0;softwrap[pg][row]=0;if(!n){row++;continue;}while(pos<n&&row<LINES){left=n-pos;cut=left<COLS?left:COLS;if(left>COLS){while(cut>0&&line[pos+cut]!=' ')cut--;if(!cut)cut=COLS;}_fmemcpy(page[pg][row],line+pos,cut);pos+=cut;while(pos<n&&line[pos]==' ')pos++;row++;if(pos<n&&row<LINES)softwrap[pg][row]=1;}}fclose(f);strncpy(paths[pg],name,ACC_PATH-1);paths[pg][ACC_PATH-1]=0;force_md(paths[pg]);external[pg]=1;return 1;}
static int write_file(int pg,const char *name){FILE*f=fopen(name,"w");char b[COLS];int y,n,next;if(!f)return 0;for(y=0;y<LINES;y++){n=used(pg,y);if(n){_fmemcpy(b,page[pg][y],n);fwrite(b,1,n,f);}next=y+1<LINES&&softwrap[pg][y+1];if(next){if(n&&used(pg,y+1))fputc(' ',f);}else fputc('\n',f);}if(fclose(f))return 0;strncpy(paths[pg],name,ACC_PATH-1);paths[pg][ACC_PATH-1]=0;external[pg]=1;dirty[pg]=0;return 1;}

static int has_selection(void){return sel_anchor>=0&&sel_caret>=0&&sel_anchor!=sel_caret;}
static int sel_low(void){return sel_anchor<sel_caret?sel_anchor:sel_caret;}
static int sel_high(void){return sel_anchor>sel_caret?sel_anchor:sel_caret;}
static void clear_selection(void){sel_anchor=sel_caret=-1;}
static int shift_down(void){return (*(unsigned char far *)(((unsigned long)0x40<<16)|0x17)&3)!=0;}
static void selection_move(int oldpos,int newpos,int shift){if(shift){if(sel_anchor<0)sel_anchor=oldpos;sel_caret=newpos;if(sel_caret==sel_anchor)clear_selection();}else clear_selection();}
static void copy_selection(void){int lo,hi,p,row,lastrow,n=0;if(!has_selection())return;lo=sel_low();hi=sel_high();lastrow=(hi-1)/COLS;for(p=lo;p<hi&&n<CLIP_MAX-3;p++){row=p/COLS;clip[n++]=page[current][row][p%COLS];if(p+1<hi&&(p+1)%COLS==0&&row<lastrow&&!softwrap[current][row+1]){clip[n++]='\r';clip[n++]='\n';}}clip_len=n;}
static void delete_selection(void){int lo,hi,fr,lr,r,start,end,n;if(!has_selection())return;lo=sel_low();hi=sel_high();fr=lo/COLS;lr=(hi-1)/COLS;for(r=fr;r<=lr;r++){start=r==fr?lo%COLS:0;end=r==lr?((hi-1)%COLS)+1:COLS;n=end-start;if(n>0){_fmemmove(page[current][r]+start,page[current][r]+end,COLS-end);_fmemset(page[current][r]+COLS-n,' ',n);}}cy=fr;cx=lo%COLS;clear_selection();dirty[current]=1;}
static void word_wrap(void){int start=COLS-1,len,i,can=1;if(cy>=LINES-1)return;if(page[current][cy][COLS-1]==' '){cy++;cx=0;softwrap[current][cy]=1;return;}while(start>0&&page[current][cy][start-1]!=' ')start--;if(start<=0){cy++;cx=0;softwrap[current][cy]=1;return;}len=COLS-start;for(i=COLS-len;i<COLS;i++)if(page[current][cy+1][i]!=' ')can=0;if(!can){cy++;cx=0;softwrap[current][cy]=1;return;}_fmemmove(page[current][cy+1]+len,page[current][cy+1],COLS-len);_fmemcpy(page[current][cy+1],page[current][cy]+start,len);_fmemset(page[current][cy]+start,' ',COLS-start);cy++;cx=len;softwrap[current][cy]=1;}
static void insert_one(int ch){if(insert_mode&&cx<COLS-1)_fmemmove(page[current][cy]+cx+1,page[current][cy]+cx,COLS-cx-1);page[current][cy][cx]=(char)ch;if(cx<COLS-1)cx++;else word_wrap();dirty[current]=1;}
static void paste_clip(void){int i,c;if(has_selection())delete_selection();for(i=0;i<clip_len&&cy<LINES;i++){c=(unsigned char)clip[i];if(c=='\r')continue;if(c=='\n'){if(cy<LINES-1){cy++;cx=0;softwrap[current][cy]=0;}continue;}insert_one(c);}clear_selection();}
static void remove_row(int row){int r;for(r=row;r<LINES-1;r++){_fmemcpy(page[current][r],page[current][r+1],COLS);softwrap[current][r]=softwrap[current][r+1];}_fmemset(page[current][LINES-1],' ',COLS);softwrap[current][LINES-1]=0;}
static void join_next_line(void){int n2,take,room;if(cy>=LINES-1)return;n2=used(current,cy+1);room=COLS-cx;if(room<0)room=0;take=n2<room?n2:room;if(take)_fmemcpy(page[current][cy]+cx,page[current][cy+1],take);if(take>=n2)remove_row(cy+1);else{_fmemmove(page[current][cy+1],page[current][cy+1]+take,COLS-take);_fmemset(page[current][cy+1]+COLS-take,' ',take);}dirty[current]=1;}
static void join_previous_line(void){int prev,n,take,room;if(cy<=0)return;prev=used(current,cy-1);n=used(current,cy);room=COLS-prev;if(room<=0){cy--;cx=COLS-1;return;}take=n<room?n:room;if(take)_fmemcpy(page[current][cy-1]+prev,page[current][cy],take);if(take>=n)remove_row(cy);else{_fmemmove(page[current][cy],page[current][cy]+take,COLS-take);_fmemset(page[current][cy]+COLS-take,' ',take);}cy--;cx=prev;dirty[current]=1;}
static void split_line(void){int r,n,tail,indent=0;if(cy>=LINES-1)return;while(indent<COLS&&page[current][cy][indent]==' ')indent++;if(indent>=COLS)indent=0;for(r=LINES-1;r>cy+1;r--){_fmemcpy(page[current][r],page[current][r-1],COLS);softwrap[current][r]=softwrap[current][r-1];}_fmemset(page[current][cy+1],' ',COLS);n=used(current,cy);tail=n>cx?n-cx:0;if(tail>COLS-indent)tail=COLS-indent;if(tail)_fmemcpy(page[current][cy+1]+indent,page[current][cy]+cx,tail);_fmemset(page[current][cy]+cx,' ',COLS-cx);softwrap[current][cy+1]=0;cy++;cx=indent;dirty[current]=1;}
static int editor_key(int key,int rows){int oldpos=cy*COLS+cx,shift=shift_down();if(key==3){copy_selection();return 1;}if(key==24){copy_selection();delete_selection();return 1;}if(key==22||key==16){paste_clip();return 1;}if(key==256+0x77){cy=0;cx=0;}else if(key==256+0x75){cy=LINES-1;while(cy>0&&!used(current,cy))cy--;cx=used(current,cy);if(cx>=COLS)cx=COLS-1;}else if(key==256+75&&cx>0)cx--;else if(key==256+77&&cx<COLS-1)cx++;else if(key==256+72&&cy>0)cy--;else if(key==256+80&&cy<LINES-1)cy++;else if(key==256+71)cx=0;else if(key==256+79){cx=COLS-1;while(cx>0&&page[current][cy][cx]==' ')cx--;if(page[current][cy][cx]!=' '&&cx<COLS-1)cx++;}else if(key==256+73){cy-=rows;if(cy<0)cy=0;}else if(key==256+81){cy+=rows;if(cy>=LINES)cy=LINES-1;}else if(key==256+82){clear_selection();insert_mode=!insert_mode;return 1;}else if(key==256+83){if(has_selection())delete_selection();else if(cx>=used(current,cy))join_next_line();else{_fmemmove(page[current][cy]+cx,page[current][cy]+cx+1,COLS-cx-1);page[current][cy][COLS-1]=' ';dirty[current]=1;}return 1;}else if(key==8){if(has_selection())delete_selection();else if(cx>0){cx--;_fmemmove(page[current][cy]+cx,page[current][cy]+cx+1,COLS-cx-1);page[current][cy][COLS-1]=' ';dirty[current]=1;}else join_previous_line();return 1;}else if(key==13&&cy<LINES-1){if(has_selection())delete_selection();split_line();clear_selection();return 1;}else if((key>=32&&key<=255)||(key>=513&&key<=767)){if(has_selection())delete_selection();insert_one(key>=512?key-512:key);clear_selection();return 1;}else return 0;selection_move(oldpos,cy*COLS+cx,shift);return 1;}

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
{int r,c,pos,lo=has_selection()?sel_low():-1,hi=has_selection()?sel_high():-1;char b[COLS+1];for(r=0;r<rows;r++){int line=top+r;if(line<LINES){_fmemcpy(b,page[current][line],COLS);b[COLS]=0;}else memset(b,' ',COLS),b[COLS]=0;acc_text(x,y+r,b,ACC_CONTROL,COLS);if(lo>=0)for(c=0;c<COLS;c++){pos=line*COLS+c;if(pos>=lo&&pos<hi)acc_put(x+c,y+r,b[c],ACC_SELECT);}}if(cursor&&cy>=top&&cy<top+rows){acc_put(x+cx,y+cy-top,page[current][cy][cx],ACC_SELECT);acc_caret_set(x+cx,y+cy-top);}else if(!cursor)acc_caret_hide();}

/* Markdown block helpers.  These deliberately operate on the editor's fixed
   68-column rows so Preview and Show make the same structural decisions. */
#define MD_TCOLS 8
#define MD_LOGICAL 512
typedef struct {int cols,pipe;int start[MD_TCOLS],width[MD_TCOLS];int logical,lstart,slot;} MDTABLE;
static void md_line(int p,int row,char *s,int *n)
{if(row<0||row>=LINES){s[0]=0;*n=0;return;}_fmemcpy(s,page[p][row],COLS);s[COLS]=0;*n=COLS;while(*n&&s[*n-1]==' ')(*n)--;s[*n]=0;}
static int md_lead(const char *s,int n){int i=0;while(i<n&&s[i]==' ')i++;return i;}
static int md_log_start(int p,int row){if(row<0)return 0;if(row>=LINES)row=LINES-1;while(row>0&&softwrap[p][row])row--;return row;}
static int md_log_end(int p,int row){row=md_log_start(p,row);while(row+1<LINES&&softwrap[p][row+1])row++;return row;}
static int md_log_next(int p,int row){int e=md_log_end(p,row);return e+1<LINES?e+1:-1;}
static int md_log_prev(int p,int row){int s=md_log_start(p,row);return s>0?md_log_start(p,s-1):-1;}
static void md_log_line(int p,int row,char *s,int *n)
{int r,e,k=0,u,prev=COLS;row=md_log_start(p,row);e=md_log_end(p,row);for(r=row;r<=e&&k<MD_LOGICAL;r++){u=used(p,r);/* load_file only discards a separator space when the previous chunk
   ended before column 68.  A full 68-column chunk is a hard split and must
   be rejoined without inventing a space (important for long table rules). */if(r>row&&k<MD_LOGICAL&&u&&prev<COLS)s[k++]=' ';if(u>MD_LOGICAL-k)u=MD_LOGICAL-k;if(u){_fmemcpy(s+k,page[p][r],u);k+=u;}prev=used(p,r);}s[k]=0;*n=k;}
static int md_blank(int p,int row){return row<0||row>=LINES||used(p,row)==0;}
static int md_list_marker(const char *s,int n,int pos,int *after,int *ordered)
{int k;if(pos+1<n&&(s[pos]=='-'||s[pos]=='*'||s[pos]=='+')&&s[pos+1]==' '){*after=pos+2;*ordered=0;return 1;}if(pos<n&&isdigit((unsigned char)s[pos])){k=pos;while(k<n&&isdigit((unsigned char)s[k]))k++;if(k+1<n&&s[k]=='.'&&s[k+1]==' '){*after=k+2;*ordered=1;return 1;}}return 0;}
static int md_fence_text(const char *s,int n)
{int i=md_lead(s,n),j;char c;if(i>3||i>=n)return 0;c=s[i];if(c!='`'&&c!='~')return 0;j=i;while(j<n&&s[j]==c)j++;return j-i>=3?(int)(unsigned char)c:0;}
static int line_fence_kind(int p,int row){char s[COLS+1];int n;md_line(p,row,s,&n);return md_fence_text(s,n);}
static int line_fence(int p,int row){return line_fence_kind(p,row)!=0;}
static int code_fence_before(int p,int row)
{int i,k=0,f;for(i=0;i<row;i++){f=line_fence_kind(p,i);if(f&&(!k||f==k))k=k?0:f;}return k;}
static int code_before(int p,int row){return code_fence_before(p,row)!=0;}
static int md_enclosing_list_indent(int p,int row,int lead)
{char s[COLS+1];int n,l,after,ordered,r;for(r=row-1;r>=0&&r>=row-32;r--){md_line(p,r,s,&n);if(!n)continue;l=md_lead(s,n);after=ordered=0;if(l<lead&&md_list_marker(s,n,l,&after,&ordered))return l;if(l<lead)return -1;}return -1;}
static int md_indented_code_raw(int p,int row)
{char s[COLS+1];int n,lead,after=0,ordered=0,li;md_line(p,row,s,&n);lead=md_lead(s,n);if(lead<4||lead>=n)return 0;if(md_list_marker(s,n,lead,&after,&ordered))return 0;li=md_enclosing_list_indent(p,row,lead);if(li>=0&&lead<li+8)return 0;return 1;}
static int md_indented_code(int p,int row)
{int r=row;if(row<0||row>=LINES)return 0;while(r>0&&softwrap[p][r])r--;return md_indented_code_raw(p,r);}
static int md_definition_line(int p,int row,int *textpos)
{char s[COLS+1];int n,i;md_line(p,row,s,&n);i=md_lead(s,n);if(i<n&&s[i]==':'&&i+1<n&&(s[i+1]==' '||s[i+1]=='\t')){i+=2;while(i<n&&s[i]==' ')i++;if(textpos)*textpos=i;return 1;}return 0;}
static int md_definition_term(int p,int row)
{int dummy;if(row<0||row+1>=LINES||used(p,row)==0)return 0;return md_definition_line(p,row+1,&dummy);}
static int md_pure_dash_rule(const char *s,int n)
{int i=md_lead(s,n),d=0;for(;i<n;i++){if(s[i]=='-')d++;else if(s[i]!=' ')return 0;}return d>=3;}
static int md_table_header_text(const char *s,int n,MDTABLE *t)
{int i=0,start,gap,c=0;memset(t,0,sizeof(*t));while(i<n&&s[i]==' ')i++;while(i<n&&c<MD_TCOLS){start=i;while(i<n&&s[i]!=' ')i++;if(i<=start)break;t->start[c]=start;c++;gap=0;while(i<n&&s[i]==' '){gap++;i++;}if(i<n&&gap<2)return 0;}if(c<2)return 0;t->cols=c;for(i=0;i<c;i++){if(i+1<c)t->width[i]=t->start[i+1]-t->start[i]-1;else t->width[i]=n-t->start[i];if(t->width[i]<3)t->width[i]=3;}return 1;}
static int md_table_sep_text(const char *s,int n,MDTABLE *t)
{int i=0,c=0,st,dashes,haspipe=0;memset(t,0,sizeof(*t));while(i<n){while(i<n&&(s[i]==' '||s[i]=='|')){if(s[i]=='|')haspipe=1;i++;}if(i>=n)break;st=i;dashes=0;while(i<n&&(s[i]=='-'||s[i]==':')){if(s[i]=='-')dashes++;i++;}if(dashes<3)return 0;if(c>=MD_TCOLS)return 0;t->start[c]=st;t->width[c]=i-st;if(t->width[c]<3)t->width[c]=3;c++;while(i<n&&s[i]==' ')i++;if(i<n&&s[i]!='|'){
   /* A second column may simply be separated by two or more spaces. */
   if(i<n&&s[i]!='-'&&s[i]!=':')return 0;
  }
 }
 if(c<2)return 0;t->cols=c;t->pipe=haspipe;return 1;}
static int md_table_sep_at(int p,int row,MDTABLE *t)
{char s[COLS+1],a[COLS+1];int n,an;md_line(p,row,s,&n);if(md_table_sep_text(s,n,t))return 1;if(!md_pure_dash_rule(s,n))return 0;if(row>0){md_line(p,row-1,a,&an);if(md_table_header_text(a,an,t))return 1;}if(row+1<LINES){md_line(p,row+1,a,&an);if(md_table_header_text(a,an,t))return 1;}return 0;}
static int md_table_same(const MDTABLE *a,const MDTABLE *b)
{int i;if(a->cols!=b->cols)return 0;for(i=0;i<a->cols;i++)if(a->width[i]!=b->width[i]&&a->start[i]!=b->start[i])return 0;return 1;}
static int md_table_rowish(int p,int row,const MDTABLE *t)
{char s[COLS+1];int n,i,non=0;md_line(p,row,s,&n);if(!n)return 0;i=md_lead(s,n);if(i>=n)return 0;if(!strnicmp(s+i,"Table:",6)||s[i]=='#'||s[i]=='>')return 0;if(t->pipe){for(i=0;i<n;i++)if(s[i]=='|')return 1;return 0;}for(i=0;i<t->cols;i++){int st=t->start[i],en=(i+1<t->cols)?t->start[i+1]:n,j;if(st>=n)continue;if(en>n)en=n;for(j=st;j<en;j++)if(s[j]!=' '){non++;break;}}return non>=1;}
static void md_table_fit(MDTABLE *t)
{int i,sum=0,avail=COLS-(t->cols+1),best;if(avail<t->cols*3)avail=t->cols*3;for(i=0;i<t->cols;i++){if(t->width[i]<3)t->width[i]=3;sum+=t->width[i];}while(sum>avail){best=-1;for(i=0;i<t->cols;i++)if(t->width[i]>3&&(best<0||t->width[i]>t->width[best]))best=i;if(best<0)break;t->width[best]--;sum--;}}
static int md_pipe_sep_log_at(int p,int row,MDTABLE *t)
{char s[MD_LOGICAL+1];int n;if(row<0||row>=LINES||row!=md_log_start(p,row))return 0;md_log_line(p,row,s,&n);if(!md_table_sep_text(s,n,t)||!t->pipe)return 0;md_table_fit(t);return 1;}
static int md_pipe_rowish_log(int p,int row,const MDTABLE *t)
{char s[MD_LOGICAL+1];int n,i,pipes=0;if(row<0||row>=LINES)return 0;md_log_line(p,row,s,&n);if(!n)return 0;for(i=0;i<n;i++)if(s[i]=='|')pipes++;return pipes>=t->cols+1;}
/* Pipe tables need logical-line handling because an imported Markdown line may
   be soft-wrapped across several 68-column editor rows.  Each physical row is
   retained as a display slot so wide cells can continue on the following row. */
static int md_pipe_table_context(int p,int row,MDTABLE *t,int *border)
{int ls=md_log_start(p,row),slot=row-ls,pr,nx,nn,sr;MDTABLE z;char q[MD_LOGICAL+1];int qn;
 if(md_pipe_sep_log_at(p,ls,&z)){*t=z;t->logical=1;t->slot=slot;t->lstart=-1;if(slot==0){*border=2;return 2;}*border=0;return 1;}
 md_log_line(p,ls,q,&qn);
 if(!qn){
  nx=md_log_next(p,ls);nn=nx>=0?md_log_next(p,nx):-1;if(nx>=0&&nn>=0&&md_pipe_sep_log_at(p,nn,&z)&&md_pipe_rowish_log(p,nx,&z)){*t=z;t->logical=1;t->slot=0;t->lstart=-1;*border=1;return 2;}
  pr=md_log_prev(p,ls);if(pr>=0){sr=pr;while(sr>=0){if(md_pipe_sep_log_at(p,sr,&z))break;md_log_line(p,sr,q,&qn);if(!qn){sr=-1;break;}sr=md_log_prev(p,sr);}if(sr>=0&&pr!=sr&&md_pipe_rowish_log(p,pr,&z)){*t=z;t->logical=1;t->slot=0;t->lstart=-1;*border=3;return 2;}}
  return 0;
 }
 nx=md_log_next(p,ls);if(nx>=0&&md_pipe_sep_log_at(p,nx,&z)&&md_pipe_rowish_log(p,ls,&z)){*t=z;t->logical=1;t->lstart=ls;t->slot=slot;*border=0;return 1;}
 sr=md_log_prev(p,ls);while(sr>=0){if(md_pipe_sep_log_at(p,sr,&z)){if(md_pipe_rowish_log(p,ls,&z)){*t=z;t->logical=1;t->lstart=ls;t->slot=slot;*border=0;return 1;}return 0;}md_log_line(p,sr,q,&qn);if(!qn)break;sr=md_log_prev(p,sr);}
 return 0;
}
/* Return 1 for a content row and 2 for a border row.  border is 1=top,
   2=middle, 3=bottom. */
static int md_table_context_basic(int p,int row,MDTABLE *t,int *border)
{
 MDTABLE here,up,down;int u=-1,d=-1,i,hasup=0,hasdown=0;
 if(!used(p,row)){
  for(i=row-1;i>=0&&i>=row-12;i--)if(md_table_sep_at(p,i,&up)){u=i;break;}
  for(i=row+1;i<LINES&&i<=row+12;i++)if(md_table_sep_at(p,i,&down)){d=i;break;}
  if(u>=0&&d>=0&&md_table_same(&up,&down)){*t=up;*border=0;return 1;}
  if(row+2<LINES&&md_table_sep_at(p,row+2,&down)&&md_table_rowish(p,row+1,&down)){*t=down;*border=1;return 2;}
  if(u>=0&&row>u+1&&md_table_rowish(p,row-1,&up)){for(i=u+1;i<row;i++)if(!used(p,i))return 0;*t=up;*border=3;return 2;}
  return 0;
 }
 if(md_table_sep_at(p,row,&here)){
  *t=here;
  for(i=row-1;i>=0&&i>=row-12;i--)if(md_table_sep_at(p,i,&up)&&md_table_same(&here,&up)){u=i;hasup=1;break;}
  for(i=row+1;i<LINES&&i<=row+12;i++)if(md_table_sep_at(p,i,&down)&&md_table_same(&here,&down)){d=i;hasdown=1;break;}
  if(hasdown&&!hasup)*border=1;else if(hasup&&!hasdown)*border=3;else *border=2;return 2;
 }
 for(i=row-1;i>=0&&i>=row-12;i--)if(md_table_sep_at(p,i,&up)){u=i;break;}
 for(i=row+1;i<LINES&&i<=row+12;i++)if(md_table_sep_at(p,i,&down)){d=i;break;}
 if(u>=0&&d>=0&&md_table_same(&up,&down)&&row>u&&row<d){*t=up;*border=0;return 1;}
 if(d==row+1&&md_table_rowish(p,row,&down)){*t=down;*border=0;return 1;}
 if(u>=0&&md_table_rowish(p,row,&up)){for(i=u+1;i<row;i++)if(!used(p,i))return 0;*t=up;*border=0;return 1;}
 return 0;
}
static int md_table_context(int p,int row,MDTABLE *t,int *border)
{if(md_pipe_table_context(p,row,t,border))return *border?2:1;memset(t,0,sizeof(*t));return md_table_context_basic(p,row,t,border);}
static void md_table_cells(const char *s,int n,const MDTABLE *t,char cell[MD_TCOLS][COLS+1])
{int c,i,st,en,l;memset(cell,0,sizeof(char)*MD_TCOLS*(COLS+1));if(t->pipe){int pos=0,next;c=0;while(pos<n&&s[pos]==' ')pos++;if(pos<n&&s[pos]=='|')pos++;while(c<t->cols){st=pos;next=pos;while(next<n&&s[next]!='|')next++;en=next;while(st<en&&s[st]==' ')st++;while(en>st&&s[en-1]==' ')en--;l=en-st;if(l>COLS)l=COLS;if(l>0)memcpy(cell[c],s+st,l);cell[c][l]=0;c++;if(next>=n)break;pos=next+1;}return;}for(c=0;c<t->cols;c++){st=t->start[c];en=(c+1<t->cols)?t->start[c+1]:n;if(st>n)st=n;if(en>n)en=n;while(st<en&&s[st]==' ')st++;while(en>st&&s[en-1]==' ')en--;l=en-st;if(l>COLS)l=COLS;memcpy(cell[c],s+st,l);cell[c][l]=0;}for(i=c;i<MD_TCOLS;i++)cell[i][0]=0;}
static int md_table_make_row(int p,int row,const MDTABLE *t,int border,char *out)
{char s[MD_LOGICAL+1],cell[MD_TCOLS][COLS+1];int n,o=0,c,j,w,chL,chJ,chR,off;if(t->logical){if(t->lstart>=0)md_log_line(p,t->lstart,s,&n);else{s[0]=0;n=0;}}else md_line(p,row,s,&n);if(border){chL=border==1?218:(border==3?192:195);chJ=border==1?194:(border==3?193:197);chR=border==1?191:(border==3?217:180);if(o<COLS)out[o++]=(char)chL;for(c=0;c<t->cols&&o<COLS;c++){w=t->width[c];if(w<3)w=3;for(j=0;j<w&&o<COLS;j++)out[o++]=(char)196;if(o<COLS)out[o++]=(char)(c+1==t->cols?chR:chJ);}out[o]=0;return o;}md_table_cells(s,n,t,cell);if(o<COLS)out[o++]=(char)179;for(c=0;c<t->cols&&o<COLS;c++){w=t->width[c];if(w<3)w=3;off=t->logical?t->slot*w:0;for(j=0;j<w&&o<COLS;j++)out[o++]=cell[c][off+j]?cell[c][off+j]:' ';if(o<COLS)out[o++]=(char)179;}out[o]=0;return o;}
static int inline_attr(int bold,int italic,int underline,int strike,int code,int super,int sub)
{if(code)return MD_CODE;if(strike)return MD_STRIKE;if(underline)return MD_UNDERLINE;if(super)return MD_SUPER;if(sub)return MD_SUB;if(bold&&italic)return MD_BOLDITALIC;if(italic)return MD_ITALIC;if(bold)return MD_BOLD;return MD_TEXT;}
static void preview_line(int x,int y,int p,int row,int in_code)
{
 char s[COLS+1],tb[COLS+1];MDTABLE tt;int n,i=0,o=0,h=0,bold=0,italic=0,underline=0,strike=0,code=in_code,super=0,sub=0,quote=0,a,k;
 int tctx,tborder=0,lead,after=0,ordered=0,defpos=0;char c;
 md_line(p,row,s,&n);acc_fill(x,y,COLS,1,' ',in_code?MD_CODE:MD_TEXT);
 if(line_fence(p,row))return;
 if(in_code){while(i<n&&o<COLS)acc_put(x+o++,y,s[i++],MD_CODE);return;}
 tctx=md_table_context(p,row,&tt,&tborder);if(tctx){n=md_table_make_row(p,row,&tt,tctx==2?tborder:0,tb);for(i=0;i<n&&i<COLS;i++)acc_put(x+i,y,(unsigned char)tb[i],MD_TEXT);return;}
 if(md_indented_code(p,row)){i=4;while(i<n&&o<COLS)acc_put(x+o++,y,s[i++],MD_CODE);return;}
 lead=md_lead(s,n);
 if(md_definition_term(p,row)){i=lead;while(i<n&&o<COLS)acc_put(x+o++,y,s[i++],MD_BOLD);return;}
 if(md_definition_line(p,row,&defpos)){o=4;i=defpos;}
 else {
  i=lead;o=lead;if(o>COLS)o=COLS;
  if(lead<=3&&i<n&&s[i]=='#'){while(i<n&&s[i]=='#'&&h<6){h++;i++;}while(i<n&&s[i]==' ')i++;a=heading_attr(h);o=0;while(i<n&&o<COLS)acc_put(x+o++,y,s[i++],a);return;}
  if(md_list_marker(s,n,i,&after,&ordered)){
   if(!ordered){if(o<COLS)acc_put(x+o++,y,7,MD_MARK);if(o<COLS)acc_put(x+o++,y,' ',MD_TEXT);i=after;}
   else {while(i<after-1&&o<COLS)acc_put(x+o++,y,s[i++],MD_MARK);if(o<COLS)acc_put(x+o++,y,' ',MD_TEXT);i=after;}
  }
  else if(i<n&&s[i]=='>'){quote=1;if(o<COLS)acc_put(x+o++,y,179,MD_QUOTE);if(o<COLS)acc_put(x+o++,y,' ',MD_TEXT);i++;while(i<n&&s[i]==' ')i++;}
  else if(i+2<n&&s[i]=='['&&(s[i+1]==' '||s[i+1]=='x'||s[i+1]=='X')&&s[i+2]==']'){if(o<COLS)acc_put(x+o++,y,(s[i+1]==' ')?250:251,MD_MARK);if(o<COLS)acc_put(x+o++,y,' ',MD_TEXT);i+=3;while(i<n&&s[i]==' ')i++;}
 }
 if(!h&&lead<=3&&i+2<n&&((s[i]=='-'&&s[i+1]=='-'&&s[i+2]=='-')||(s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*'))){while(o<COLS)acc_put(x+o++,y,196,MD_MARK);return;}
 while(i<n&&o<COLS){
  if(i+3<=n&&!strnicmp(s+i,"<u>",3)){underline=1;i+=3;continue;}if(i+4<=n&&!strnicmp(s+i,"</u>",4)){underline=0;i+=4;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<del>",5)){strike=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</del>",6)){strike=0;i+=6;continue;}
  if(i+2<=n&&s[i]=='~'&&s[i+1]=='~'){strike=!strike;i+=2;continue;}
  if(i+3<=n&&((s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*')||(s[i]=='_'&&s[i+1]=='_'&&s[i+2]=='_'))){bold=!bold;italic=!italic;i+=3;continue;}
  if(i+2<=n&&s[i]=='*'&&s[i+1]=='*'){bold=!bold;i+=2;continue;}
  if(i+2<=n&&s[i]=='_'&&s[i+1]=='_'){italic=!italic;i+=2;continue;}
  if(s[i]=='*'||s[i]=='_'){italic=!italic;i++;continue;}
  if(i+2<=n&&s[i]=='{'&&s[i+1]=='{'){underline=1;i+=2;continue;}if(i+2<=n&&s[i]=='}'&&s[i+1]=='}'){underline=0;i+=2;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<sup>",5)){super=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</sup>",6)){super=0;i+=6;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<sub>",5)){sub=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</sub>",6)){sub=0;i+=6;continue;}
  if(s[i]=='`'){code=!code;i++;continue;}
  if(s[i]=='['){k=i+1;while(k<n&&s[k]!=']')k++;if(k<n&&k+1<n&&s[k+1]=='('){i++;while(i<k&&o<COLS)acc_put(x+o++,y,s[i++],MD_LINK);i=k+2;while(i<n&&s[i]!=')')i++;if(i<n)i++;continue;}}
  if(s[i]=='<'&&i+7<n&&!strnicmp(s+i+1,"http",4)){k=i+1;while(k<n&&s[k]!='>')k++;if(k<n){i++;while(i<k&&o<COLS)acc_put(x+o++,y,s[i++],MD_LINK);i++;continue;}}
  c=s[i++];a=inline_attr(bold,italic,underline,strike,code,super,sub);if(quote&&a==MD_TEXT)a=MD_QUOTE;acc_put(x+o++,y,c,a);
 }
}
static void draw_preview(int x,int y,int rows)
{int r,line,fence=code_fence_before(current,top),fk;for(r=0;r<rows;r++){line=top+r;if(line<LINES){fk=line_fence_kind(current,line);if(fk){preview_line(x,y+r,current,line,fence!=0);if(!fence)fence=fk;else if(fence==fk)fence=0;}else preview_line(x,y+r,current,line,fence!=0);}else acc_fill(x,y+r,COLS,1,' ',MD_TEXT);acc_put(x+COLS,y+r,' ',MD_TEXT);}}
static void draw_panes(int x,int y,int focus)
{int i,rows=view_mode==0?VIEW:FULL_VIEW,line_attr=ACC_ATTR(acc_appearance.background,acc_appearance.border);if(view_mode==0){draw_source(x+2,y+2,VIEW,focus==0);for(i=0;i<DLG_W-2;i++)acc_put(x+1+i,y+9,196,line_attr);draw_preview(x+2,y+10,VIEW);}else if(view_mode==1)draw_source(x+2,y+2,FULL_VIEW,focus==0);else draw_preview(x+2,y+2,FULL_VIEW);acc_scrollbar(x+70,y+2,rows,top,LINES,rows);}
static void draw_edit_refresh(int x,int y,int rows,int key,int oldtop)
{int on;if(top!=oldtop||key==8||key==13||key==24||key==22||key==16||key==256+83){draw_panes(x,y,0);return;}draw_source(x+2,y+2,rows,1);if(view_mode==0&&((key>=32&&key<=255)||(key>=513&&key<=767))&&cy>=top&&cy<top+VIEW){on=code_before(current,cy);preview_line(x+2,y+10+cy-top,current,cy,on);}acc_scrollbar(x+70,y+2,rows,top,LINES,rows);}
static int page_token_x(int x,int pg){int i,xx=x;char s[6];for(i=0;i<pg;i++){sprintf(s," %d ",i+1);xx+=(int)strlen(s)+1;}return xx;}
static void page_bar(int x,int y)
{int i,xx=x,n,a,w,max;char s[6],shown[30],*base;acc_fill(x,y,COLS+1,1,' ',ACC_BG);for(i=0;i<count;i++){sprintf(s," %d ",i+1);w=(int)strlen(s);a=i==current?ACC_CONTROL:ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg);acc_text(xx,y,s,a,w);xx+=w+1;}if(count<PAGES)acc_put(xx,y,'+',ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg));if(external[current]&&paths[current][0]){base=strrchr(paths[current],'\\');if(!base)base=strrchr(paths[current],'/');base=base?base+1:paths[current];max=dirty[current]?27:28;n=(int)strlen(base);if(n>max){base+=n-max;n=max;}memcpy(shown,base,n);if(dirty[current])shown[n++]='*';shown[n]=0;acc_text(x+COLS-n,y,shown,dirty[current]?ACC_TITLE:ACC_HEADING,n);}else acc_text(x+COLS-7,y,"Unsaved",ACC_TITLE,7);}
static int palette_codes[256],palette_count=0;
static void palette_build(void){int c;palette_count=0;for(c=0;c<256;c++)if(!acc_glyph_is_custom(c))palette_codes[palette_count++]=c;}
static int palette_index_of(int code){int i;for(i=0;i<palette_count;i++)if(palette_codes[i]==code)return i;return 0;}
static void palette_draw_cell(int x,int y,int index,int selected){int code=palette_codes[index],col=index&31,row=index>>5,px=x+2+col,py=y+2+row;int attr=ACC_ATTR(acc_appearance.background,selected?acc_appearance.folders:acc_appearance.border);acc_put(px,py,code,attr);}
static void palette_status(int x,int y,int code){char b[48],c;c=(code>=32)?(char)code:' ';sprintf(b,"Char: %c  Decimal: %3d  Hex: %02X",c,code,code);acc_fill(x+2,y+11,32,1,' ',ACC_BG);acc_text(x+2,y+11,b,ACC_LABEL,(int)strlen(b));}
static int character_palette(void)
{int w=36,h=14,x=(acc_cols-w)/2,y=(acc_rows-h)/2,i,index,old=-1,key=0,mx=0,my=0;unsigned mb=0;palette_build();index=-1;acc_subbox(x,y,w,h,"Characters",0);for(i=0;i<palette_count;i++)palette_draw_cell(x,y,i,0);palette_status(x,y,palette_codes[palette_index_of(128)]);acc_shadow(x,y,w,h);for(;;){acc_wait(&key,&mx,&my,&mb);if(mb&ACC_MOUSE_MOVED){if(mx>=x+2&&mx<x+34&&my>=y+2&&my<y+10){int n=(my-(y+2))*32+(mx-(x+2));if(n<palette_count&&n!=index){old=index;index=n;if(old>=0)palette_draw_cell(x,y,old,0);palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);}}key=0;continue;}if(mb&1){if(mx>=x+2&&mx<x+34&&my>=y+2&&my<y+10){int n=(my-(y+2))*32+(mx-(x+2));if(n<palette_count)return palette_codes[n];}else if(mx<x||mx>=x+w||my<y||my>=y+h)return -1;continue;}if(key==27)return -1;if((key==9||key==271)&&index<0){index=palette_index_of(128);palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);continue;}if(index<0)continue;old=index;if(key==256+75){if(index>0)index--;}else if(key==256+77){if(index+1<palette_count)index++;}else if(key==256+72){index-=32;if(index<0)index=old;}else if(key==256+80){if(index+32<palette_count)index+=32;}else if(key==13)return palette_codes[index];else continue;if(index!=old){palette_draw_cell(x,y,old,0);palette_draw_cell(x,y,index,1);palette_status(x,y,palette_codes[index]);}}}

static void toolbar(int x,int y,int focus)
{acc_button(x,y," Export ",focus==BTN_SAVE);acc_button(x+8,y," Print ",focus==BTN_PRINT);acc_put(x+16,y,179,ACC_BORDER);acc_button(x+18,y," Split ",focus==BTN_SPLIT);acc_button(x+27,y," Focus ",focus==BTN_FOCUS);acc_button(x+36,y," Show ",focus==BTN_SHOW);acc_put(x+43,y,179,ACC_BORDER);acc_button(x+45,y," Chars ",focus==BTN_CHARS);acc_button(x+61,y," Close ",focus==BTN_CLOSE);}
static void draw_ui(int x,int y,int focus)
{int i;acc_box(x,y,DLG_W,DLG_H,"Markdown");acc_tooltip_region(x+DLG_W-8,y,2,"Maximize (F11)",1);draw_panes(x,y,focus);page_bar(x+3,y+17);for(i=0;i<DLG_W-6;i++)acc_put(x+3+i,y+18,196,ACC_BORDER);toolbar(x+3,y+19,focus);}

static int filename_dialog(char *out)
{int w=50,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=(int)strlen(out),f=0;unsigned mb=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Save Markdown",1);acc_text(x+3,y+2,"Filename:",ACC_LABEL,10);acc_fill(x+13,y+2,32,1,' ',f==0?ACC_SELECT:ACC_CONTROL);acc_text(x+13,y+2,out,f==0?ACC_SELECT:ACC_CONTROL,31);if(f==0)acc_caret_set(x+13+(pos<31?pos:30),y+2);else acc_caret_hide();acc_button(x+15,y+6," Save ",f==1);acc_button(x+25,y+6," Cancel ",f==2);acc_wait(&k,&mx,&my,&mb);if(k==27||((mb&1)&&my==y+6&&mx>=x+25)){acc_modal_end();return 0;}if((mb&1)&&my==y+2){f=0;k=0;}else if((mb&1)&&my==y+6&&mx>=x+15&&mx<x+22){f=1;k=13;}if(k==9||k==271){f=k==271?(f+2)%3:(f+1)%3;k=0;continue;}if(k==13&&f==2){acc_modal_end();return 0;}if(k==13&&f==1){if(pos){force_md(out);acc_modal_end();return 1;}k=0;continue;}if(f==0){if(k==8&&pos)out[--pos]=0;else if(k>=32&&k<127&&pos<ACC_PATH-4){out[pos++]=(char)k;out[pos]=0;}}k=0;}}
static int save_page(int pg,int notify)
{char name[ACC_PATH],full[ACC_PATH],msg[ACC_PATH+24];if(external[pg])strcpy(name,paths[pg]);else strcpy(name,"DOCUMENT.MD");if(!external[pg]&&!filename_dialog(name))return 0;if(!external[pg]&&!strchr(name,'\\')&&!strchr(name,'/')&&!strchr(name,':'))acc_path(full,"EXPORT",name);else strcpy(full,name);if(write_file(pg,full)){if(notify){sprintf(msg,"Saved to\n%s",full);acc_notice("Save",msg);}return 1;}acc_notice("Save Error","Unable to save Markdown file.");return 0;}
static void save_current(void){(void)save_page(current,1);}
static int any_dirty(void){int i;for(i=0;i<count;i++)if(dirty[i])return 1;return 0;}
static int confirm_close(void)
{int w=48,h=8,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,f=-1;unsigned mb=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Unsaved Changes",1);acc_text(x+3,y+2,"You have unsaved changes!",ACC_LABEL,25);acc_button(x+3,y+5," Save ",f==0);acc_button(x+11,y+5," Discard ",f==1);acc_button(x+38,y+5," Close ",f==2);acc_wait(&k,&mx,&my,&mb);if((mb&1)&&my==y+5){if(mx>=x+3&&mx<x+9){f=0;k=13;}else if(mx>=x+11&&mx<x+20){f=1;k=13;}else if(mx>=x+38&&mx<x+44){f=2;k=13;}}if(k==27){acc_modal_end();return 0;}if(k==9||k==271){if(f<0)f=k==271?2:0;else f=k==271?(f+2)%3:(f+1)%3;k=0;continue;}if(k==13&&f>=0){acc_modal_end();return f==0?2:(f==1?1:0);}}}
static int save_all_dirty(int persistent_mode)
{int i,old=current;if(persistent_mode){persist_save();for(i=0;i<count;i++)if(!external[i])dirty[i]=0;}for(i=0;i<count;i++)if(dirty[i]&&!save_page(i,0)){current=old;return 0;}current=old;return 1;}

#define GF_BODY 0
#define GF_ITALIC 1
#define GF_BOLD 2
#define GF_BOLDITALIC 3
#define GF_CODE 4
#define GF_H1 5
#define GF_H2 6
#define GF_H3 7
#define GF_H4 8
#define GF_H5 9
#define GF_QUOTE 10
#define GF_TINY 11
#define GF_COUNT 12
static unsigned char far *graph_fonts[GF_COUNT];static int graph_role=GF_BODY;
static unsigned char far *helvfont[4];static unsigned char helvh[4],helvfirst[4],helvlast[4];static int helv_loaded=0;
static void graph_fonts_free(void){int i;for(i=0;i<GF_COUNT;i++)if(graph_fonts[i]){_ffree(graph_fonts[i]);graph_fonts[i]=0;}}
static int graph_fonts_load(void)
{/* FONT.DAT omits the BIOS Standard font, so Font Config IDs map to slot ID-1. */
 static const unsigned char slot[GF_COUNT]={2,26,5,23,25,9,9,7,7,8,30,31};char path[ACC_PATH];FILE*f;int i,c;unsigned j;memset(graph_fonts,0,sizeof(graph_fonts));acc_path(path,"","FONT.DAT");f=fopen(path,"rb");if(!f)return 0;for(i=0;i<GF_COUNT;i++){graph_fonts[i]=(unsigned char far*)_fmalloc(4096U);if(!graph_fonts[i]||fseek(f,(long)slot[i]*4096L,SEEK_SET)){fclose(f);graph_fonts_free();return 0;}for(j=0;j<4096U;j++){c=fgetc(f);if(c==EOF){fclose(f);graph_fonts_free();return 0;}graph_fonts[i][j]=(unsigned char)c;}}fclose(f);return 1;}
static unsigned read16(FILE*f){int a=fgetc(f),b=fgetc(f);return(unsigned)(a|(b<<8));}
static unsigned long read32(FILE*f){unsigned long a=(unsigned)fgetc(f),b=(unsigned)fgetc(f),c=(unsigned)fgetc(f),d=(unsigned)fgetc(f);return a|(b<<8)|(c<<16)|(d<<24);}
static void helv_free(void){int i;for(i=0;i<4;i++)if(helvfont[i]){_ffree(helvfont[i]);helvfont[i]=0;}helv_loaded=0;}
static int helv_load(void)
{char name[ACC_PATH],magic[4];FILE*f;unsigned long off[4];unsigned size[4],i,j;int count,c;memset(helvfont,0,sizeof(helvfont));acc_path(name,"","HELVE.BMF");f=fopen(name,"rb");if(!f)return 0;if(fread(magic,1,4,f)!=4||memcmp(magic,"HBF1",4)){fclose(f);return 0;}count=fgetc(f);if(count!=4){fclose(f);return 0;}for(i=0;i<4;i++){helvh[i]=(unsigned char)fgetc(f);(void)fgetc(f);helvfirst[i]=(unsigned char)fgetc(f);helvlast[i]=(unsigned char)fgetc(f);off[i]=read32(f);size[i]=read16(f);if(size[i]<570){fclose(f);helv_free();return 0;}}for(i=0;i<4;i++){helvfont[i]=(unsigned char far*)_fmalloc(size[i]);if(!helvfont[i]||fseek(f,(long)off[i],SEEK_SET)){fclose(f);helv_free();return 0;}for(j=0;j<size[i];j++){c=fgetc(f);if(c==EOF){fclose(f);helv_free();return 0;}helvfont[i][j]=(unsigned char)c;}}fclose(f);helv_loaded=1;return 1;}
static void graph_direct_state(void){outp(0x3CE,0);outp(0x3CF,0);outp(0x3CE,1);outp(0x3CF,0);outp(0x3CE,3);outp(0x3CF,0);outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);}
static void graph_clear(int colour){unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p;graph_direct_state();for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);_fmemset(v,(colour&(1<<p))?255:0,(unsigned)38400L);}outp(0x3C4,2);outp(0x3C5,15);}
static void graph_band(int y0,int y1,int colour){unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p,y;graph_direct_state();for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);for(y=y0;y<=y1;y++)_fmemset(v+y*80,(colour&(1<<p))?255:0,80);}outp(0x3C4,2);outp(0x3C5,15);}
static void graph_page(void){graph_clear(7);graph_band(464,479,0);}
/* BIOS pixel plotting is deliberately used here.  The MediaGX VGA-compatible
   hardware does not preserve the planar latches expected by the former direct
   write-mode-2 routine, producing coloured vertical fragments.  INT 10h/0Ch
   is slower but adapter-safe and exact for this print-preview workload. */
static void gpixel(int x,int y,int c){union REGS r;if(x<0||x>=640||y<0||y>=480)return;memset(&r,0,sizeof(r));r.h.ah=0x0C;r.h.al=(unsigned char)c;r.h.bh=0;r.x.cx=(unsigned)x;r.x.dx=(unsigned)y;int86(0x10,&r,&r);}
static void graph_box(int x0,int y0,int x1,int y1,int colour)
{unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p,y,b0,b1;if(x0<0)x0=0;if(x1>639)x1=639;if(y0<0)y0=0;if(y1>479)y1=479;if(x0>x1||y0>y1)return;/* The fenced-code background is byte aligned (24..615).  Fill it directly
   in planar mode instead of issuing an INT 10h pixel call for every pixel.  BIOS pixel plotting leaves VGA Set/Reset state behind, so restore all write-mode registers before every direct fill. */if((x0&7)==0&&(x1&7)==7){b0=x0>>3;b1=x1>>3;graph_direct_state();for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);for(y=y0;y<=y1;y++)_fmemset(v+y*80+b0,(colour&(1<<p))?255:0,(unsigned)(b1-b0+1));}outp(0x3C4,2);outp(0x3C5,15);return;}for(y=y0;y<=y1;y++){int x;for(x=x0;x<=x1;x++)gpixel(x,y,colour);}}
static int gchar_width(int scale){return 8*scale;}
static void gchar(int x,int y,unsigned char ch,int scale,int colour,int style){int r,b,sx,sy,slant=0,synthetic=0;unsigned char bits;unsigned char far*font=graph_fonts[graph_role]?graph_fonts[graph_role]:graph_fonts[GF_CODE];if(graph_role==GF_BODY){if((style&1)&&(style&2)&&graph_fonts[GF_BOLDITALIC])font=graph_fonts[GF_BOLDITALIC];else if((style&1)&&graph_fonts[GF_BOLD])font=graph_fonts[GF_BOLD];else if((style&2)&&graph_fonts[GF_ITALIC])font=graph_fonts[GF_ITALIC];else{synthetic=(style&1)!=0;slant=(style&2)!=0;}}for(r=0;r<16;r++){bits=font[(unsigned)ch*16+r];for(b=0;b<8;b++)if(bits&(0x80>>b))for(sy=0;sy<scale;sy++)for(sx=0;sx<scale;sx++){int skew=slant?(15-r)/5:0;gpixel(x+(b+skew)*scale+sx,y+r*scale+sy,colour);if(synthetic)gpixel(x+(b+skew)*scale+sx+1,y+r*scale+sy,colour);}}if(style&4)for(b=0;b<8*scale;b++)gpixel(x+b,y+14*scale,colour);if(style&8)for(b=0;b<8*scale;b++)gpixel(x+b,y+8*scale,colour);}
static void gtext(int x,int y,const char*s,int scale,int colour){int w=gchar_width(scale);while(*s){gchar(x,y,(unsigned char)*s++,scale,colour,0);x+=w;}}
static int hwidth(int strike,unsigned char ch){unsigned char far*p;if(!helv_loaded||ch<helvfirst[strike]||ch>helvlast[strike])return 8;p=helvfont[strike]+(unsigned)(ch-helvfirst[strike])*6;return p[0];}
static void hchar(int x,int y,unsigned char ch,int strike,int colour,int style)
{unsigned char far*p,*bits;unsigned off;int advance,width,height,rowbytes,r,b,slant=0;if(!helv_loaded||ch<helvfirst[strike]||ch>helvlast[strike]){gchar(x,y,ch,1,colour,style);return;}p=helvfont[strike]+(unsigned)(ch-helvfirst[strike])*6;advance=p[0];width=p[1];height=p[2];off=(unsigned)(p[4]|((unsigned)p[5]<<8));bits=helvfont[strike]+off;rowbytes=(width+7)/8;for(r=0;r<height;r++){slant=(style&2)?(height-1-r)/6:0;for(b=0;b<width;b++)if(bits[r*rowbytes+b/8]&(0x80>>(b&7))){gpixel(x+b+slant,y+r,colour);if(style&1)gpixel(x+b+slant+1,y+r,colour);}}if(style&4)for(b=0;b<advance;b++)gpixel(x+b,y+height-2,colour);if(style&8)for(b=0;b<advance;b++)gpixel(x+b,y+height/2,colour);}
static int doc_width(unsigned char ch,int strike,int scale){return helv_loaded?hwidth(strike,ch):gchar_width(scale);}
static void doc_char(int x,int y,unsigned char ch,int strike,int scale,int colour,int style){if(helv_loaded)hchar(x,y,ch,strike,colour,style);else gchar(x,y,ch,scale,colour,style);}
static int graph_word_width(const char*s,int i,int n,int strike,int scale){int w=0;while(i<n&&s[i]!=' '){w+=doc_width((unsigned char)s[i],strike,scale);i++;}return w;}
static int video_is_vga(void){union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);return r.h.al==0x1A;}
static void graph_rule(int y,int kind){int x;if(kind==3){for(x=32;x<608;x+=12){int e=x+6,i;if(e>608)e=608;for(i=x;i<e;i++)gpixel(i,y,0);}}else{for(x=32;x<608;x++)gpixel(x,y,0);if(kind==2)for(x=32;x<608;x++)gpixel(x,y+2,0);}}
static int graph_line(int p,int row,int ypos,int in_fence)
{
 char s[COLS+1],tb[COLS+1];MDTABLE tt;int n,i=0,x=32,h=0,scale=1,hstrike=1,height=16;
 int bold=0,italic=0,underline=0,strike=0,super=0,sub=0,quote=0,inline_code=0,k,style,w,base_x,lines=1,wordw,base_role=GF_BODY;
 int lead,after=0,ordered=0,defpos=0,tctx,tborder=0,indent_code,block_code;
 md_line(p,row,s,&n);lead=md_lead(s,n);if(in_fence){i=0;graph_role=GF_CODE;graph_box(24,ypos-2,615,ypos+height+2,0);while(i<n&&x+gchar_width(scale)<632){gchar(x,ypos,(unsigned char)s[i++],scale,7,0);x+=gchar_width(scale);}return height+5;}tctx=md_table_context(p,row,&tt,&tborder);if(tctx){n=md_table_make_row(p,row,&tt,tctx==2?tborder:0,tb);graph_role=GF_CODE;x=32;for(i=0;i<n&&x+8<632;i++){gchar(x,ypos,(unsigned char)tb[i],1,0,0);x+=8;}return height+5;}indent_code=md_indented_code(p,row);block_code=indent_code;
 if(block_code){
  i=4;graph_role=GF_CODE;graph_box(24,ypos-2,615,ypos+height+2,0);
  while(i<n&&x+gchar_width(scale)<632){gchar(x,ypos,(unsigned char)s[i++],scale,7,0);x+=gchar_width(scale);}return height+5;
 }
 if(i+2<n&&s[i]=='['&&s[i+1]=='^'){base_role=GF_TINY;x=32;}
 if(md_definition_term(p,row)){base_role=GF_BOLD;i=lead;x=32+lead*8;}
 else if(md_definition_line(p,row,&defpos)){base_role=GF_BODY;i=defpos;x=64;}
 else {
  i=lead;x=32+lead*8;
  if(lead<=3&&i<n&&s[i]=='#'){while(i<n&&s[i]=='#'&&h<6){h++;i++;}while(i<n&&s[i]==' ')i++;base_role=h==1?GF_H1:(h==2?GF_H2:(h==3?GF_H3:(h==4?GF_H4:GF_H5)));x=32;}
  else if(md_list_marker(s,n,i,&after,&ordered)){
   if(!ordered){graph_role=GF_BODY;gchar(x,ypos,7,1,0,0);x+=16;i=after;}
   else {graph_role=GF_BODY;while(i<after-1&&x+8<632){gchar(x,ypos,(unsigned char)s[i++],1,0,0);x+=8;}x+=8;i=after;}
  }
  else if(i<n&&s[i]=='>'){quote=1;base_role=GF_QUOTE;for(k=0;k<height;k++)gpixel(x,ypos+k,0);x+=12;i++;while(i<n&&s[i]==' ')i++;}
 }
 base_x=x;
 if(!h&&lead<=3&&i+3<=n&&((s[i]=='-'&&s[i+1]=='-'&&s[i+2]=='-')||(s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*'))){graph_role=GF_BODY;while(x+8<632){gchar(x,ypos,196,1,0,0);x+=8;}return height+4;}
 while(i<n){
  if(inline_code){
   if(s[i]=='`'){inline_code=0;i++;continue;}w=8;if(x+w>=632){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(32,ypos+k,0);}graph_box(x,ypos-1,x+7,ypos+height,0);graph_role=GF_CODE;gchar(x,ypos,(unsigned char)s[i++],1,7,0);x+=8;continue;
  }
  if(s[i]=='`'){
   int e=i+1,j,chunk,fit;while(e<n&&s[e]!='`')e++;
   if(e<n){i++;while(i<e){fit=(632-x)/8;if(fit<=0){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(32,ypos+k,0);fit=(632-x)/8;}chunk=e-i;if(chunk>fit)chunk=fit;if(chunk<=0)break;/* Paint the complete inverse span first, then draw all glyphs.  Per-character background fills could erase neighbouring ProFont pixels on real VGA adapters. */graph_box(x,ypos-1,x+chunk*8-1,ypos+height,0);graph_role=GF_CODE;for(j=0;j<chunk;j++)gchar(x+j*8,ypos,(unsigned char)s[i+j],1,7,0);x+=chunk*8;i+=chunk;if(i<e){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(32,ypos+k,0);}}i=e+1;continue;}inline_code=1;i++;continue;
  }
  if(s[i]=='\\'&&i+1<n){i++;w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=632){ypos+=height+4;x=base_x;lines++;}graph_role=base_role;doc_char(x,ypos,(unsigned char)s[i++],hstrike,scale,0,0);x+=w;continue;}
  if(s[i]!=' '&&(i==0||s[i-1]==' ')){wordw=graph_word_width(s,i,n,hstrike,scale);if(x>base_x&&x+wordw>=632){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(32,ypos+k,0);}}
  w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=632){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(32,ypos+k,0);}
  if(i+3<=n&&!strnicmp(s+i,"<u>",3)){underline=1;i+=3;continue;}if(i+4<=n&&!strnicmp(s+i,"</u>",4)){underline=0;i+=4;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<del>",5)){strike=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</del>",6)){strike=0;i+=6;continue;}
  if(i+2<=n&&s[i]=='~'&&s[i+1]=='~'){strike=!strike;i+=2;continue;}
  if(i+3<=n&&((s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*')||(s[i]=='_'&&s[i+1]=='_'&&s[i+2]=='_'))){bold=!bold;italic=!italic;i+=3;continue;}
  if(i+2<=n&&s[i]=='*'&&s[i+1]=='*'){bold=!bold;i+=2;continue;}
  if(i+2<=n&&s[i]=='_'&&s[i+1]=='_'){italic=!italic;i+=2;continue;}
  if(s[i]=='*'||s[i]=='_'){italic=!italic;i++;continue;}
  if(i+2<=n&&s[i]=='{'&&s[i+1]=='{'){int e=i+2;while(e+1<n&&!(s[e]=='}'&&s[e+1]=='}'))e++;if(e+1<n){i+=2;while(i<e){w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=632){ypos+=height+4;x=base_x;lines++;}style=(bold?1:0)|(italic?2:0)|4|(strike?8:0);graph_role=base_role;doc_char(x,ypos,(unsigned char)s[i++],hstrike,scale,0,style);x+=w;}i=e+2;continue;}underline=1;i+=2;continue;}if(i+2<=n&&s[i]=='}'&&s[i+1]=='}'){underline=0;i+=2;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<sup>",5)){super=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</sup>",6)){super=0;i+=6;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<sub>",5)){sub=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</sub>",6)){sub=0;i+=6;continue;}
  if(i+3<n&&s[i]=='['&&s[i+1]=='^'){k=i+2;while(k<n&&s[k]!=']')k++;if(k<n){graph_role=GF_TINY;while(i<=k){gchar(x,ypos-3,(unsigned char)s[i++],1,0,0);x+=8;}continue;}}
  if(s[i]=='['){k=i+1;while(k<n&&s[k]!=']')k++;if(k<n&&k+1<n&&s[k+1]=='('){i++;while(i<k){w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=632)break;style=(bold?1:0)|(italic?2:0)|4|(strike?8:0);graph_role=base_role;doc_char(x,ypos,(unsigned char)s[i++],hstrike,scale,9,style);x+=w;}i=k+2;while(i<n&&s[i]!=')')i++;if(i<n)i++;continue;}}
  if(s[i]=='<'&&i+7<n&&!strnicmp(s+i+1,"http",4)){k=i+1;while(k<n&&s[k]!='>')k++;if(k<n){i++;while(i<k){w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=632){ypos+=height+4;x=base_x;lines++;}style=(bold?1:0)|(italic?2:0)|4|(strike?8:0);graph_role=base_role;doc_char(x,ypos,(unsigned char)s[i++],hstrike,scale,9,style);x+=w;}i++;continue;}}
  style=(bold?1:0)|(italic?2:0)|(underline?4:0)|(strike?8:0);graph_role=(super||sub)?GF_TINY:base_role;doc_char(x,ypos+(super?-4:(sub?4:0)),(unsigned char)s[i++],hstrike,scale,0,style);x+=w;
 }
 if(h==1){graph_rule(ypos+height+1,2);return lines*height+(lines-1)*4+8;}
 if(h==2){graph_rule(ypos+height+1,1);return lines*height+(lines-1)*4+6;}
 if(h==3){graph_rule(ypos+height+1,3);return lines*height+(lines-1)*4+6;}
 return lines*height+(lines-1)*4+(quote?5:4);
}
static void graph_render(int p,int first)
{int row,ypos=12,last=first+22,fence=code_fence_before(p,first),fk;const char *right="Up/Down/PgUp/Dn=Scroll";if(last>LINES)last=LINES;graph_page();for(row=first;row<last&&ypos<445;row++){fk=line_fence_kind(p,row);if(fk){if(!fence)fence=fk;else if(fence==fk)fence=0;ypos+=3;continue;}ypos+=graph_line(p,row,ypos,fence!=0);}graph_role=GF_CODE;gtext(8,464,"Esc=Close",1,7);gtext(632-(int)strlen(right)*8,464,right,1,7);}
static void graph_show(void)
{union REGS r;unsigned key;int first=0,p=current,scan;if(!video_is_vga()){acc_notice("Show","VGA display adapter required.");return;}/* Show fonts are all sourced from FONT.DAT.  Standalone .FNT files are source/build assets only and are not runtime dependencies. */if(!graph_fonts_load()){acc_notice("Show","Unable to read FONT.DAT.");return;}acc_mouse_display(0);memset(&r,0,sizeof(r));r.h.ah=0;r.h.al=0x12;int86(0x10,&r,&r);graph_render(p,first);for(;;){key=_bios_keybrd(_KEYBRD_READ);if((key&255)==27)break;scan=(key>>8)&255;if(scan==0x48||scan==0x49){first-=20;if(first<0)first=0;}else if(scan==0x50||scan==0x51){first+=20;if(first>LINES-1)first=LINES-1;}else if(scan==0x4B&&p>0){p--;first=0;}else if(scan==0x4D&&p+1<count){p++;first=0;}else continue;graph_render(p,first);}graph_fonts_free();outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);acc_restore_screen();acc_mouse_reapply_cursor();acc_mouse_display(1);}

static void print_current(void)
{FILE*f=fopen("LPT1","wb");char line[COLS+1];int row,style,n;if(!f){acc_notice("Print","Unable to open LPT1.");return;}fputs("\033@",f);for(row=0;row<LINES;row++){plain_line(current,row,line,&style);n=COLS;while(n&&line[n-1]==' ')n--;line[n]=0;if(style==1)fputs("\033E\033W\001",f);else if(style>1&&style<=6)fputs("\033E\033-\001",f);fputs(line,f);if(style==1)fputs("\033W\000\033F",f);else if(style>1&&style<=6)fputs("\033-\000\033F",f);fputs("\r\n",f);}fputc('\f',f);fclose(f);acc_notice("Print","Formatted Markdown sent to LPT1.");}
static void document_stats(unsigned long *chars,unsigned long *words)
{int y,i,n,inword=0;char c;*chars=*words=0;for(y=0;y<LINES;y++){n=used(current,y);*chars+=(unsigned long)n;if(n&&y+1<LINES)(*chars)++;for(i=0;i<n;i++){c=page[current][y][i];if(isalnum((unsigned char)c)){if(!inword){(*words)++;inword=1;}}else inword=0;}inword=0;}}
static void focus_draw(void)
{char b[COLS+1],s[42];int r,c,line,pos,left=(acc_cols-COLS)/2,lo=has_selection()?sel_low():-1,hi=has_selection()?sel_high():-1,panel=ACC_ATTR(acc_appearance.background,acc_appearance.labels),status=ACC_ATTR(acc_appearance.labels,acc_appearance.background);unsigned long chars,words;for(r=0;r<24;r++){line=top+r;if(line<LINES){_fmemcpy(b,page[current][line],COLS);b[COLS]=0;acc_text(left,r,b,panel,COLS);if(lo>=0)for(c=0;c<COLS;c++){pos=line*COLS+c;if(pos>=lo&&pos<hi)acc_put(left+c,r,b[c],ACC_SELECT);}}}if(cy>=top&&cy<top+24){acc_put(left+cx,cy-top,page[current][cy][cx],ACC_SELECT);acc_caret_set(left+cx,cy-top);}else acc_caret_hide();document_stats(&chars,&words);sprintf(s," Chars: %lu   Words: %lu",chars,words);acc_fill(0,24,acc_cols,1,' ',status);acc_text(0,24,s,status,(int)strlen(s));}
static void focus_mode(void)
{int key=0,mx,my;unsigned mb;acc_clear(ACC_ATTR(acc_appearance.background,acc_appearance.labels));focus_draw();while(key!=27){acc_wait(&key,&mx,&my,&mb);if(key!=27)editor_key(key,24);if(cy<top)top=cy;if(cy>=top+24)top=cy-23;if(key!=27)focus_draw();}acc_caret_hide();acc_restore_text_screen();acc_mouse_display(1);key=0;}
static void maximize_page_bar(void)
{int i,xx=0,n,a,w,max;char s[6],shown[30],*base;acc_fill(0,24,80,1,' ',ACC_BG);for(i=0;i<count;i++){sprintf(s," %d ",i+1);w=(int)strlen(s);a=i==current?ACC_CONTROL:ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg);acc_text(xx,24,s,a,w);xx+=w+1;}if(count<PAGES)acc_put(xx,24,'+',ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg));if(external[current]&&paths[current][0]){base=strrchr(paths[current],'\\');if(!base)base=strrchr(paths[current],'/');base=base?base+1:paths[current];max=dirty[current]?79:80;n=(int)strlen(base);if(n>max){base+=n-max;n=max;}memcpy(shown,base,n);if(dirty[current])shown[n++]='*';shown[n]=0;acc_text(80-n,24,shown,dirty[current]?ACC_TITLE:ACC_HEADING,n);}else acc_text(73,24,"Unsaved",ACC_TITLE,7);}
static void maximize_draw(void)
{int i,r,rows=view_mode==0?12:24,line_attr=ACC_ATTR(7,acc_appearance.border);acc_clear(ACC_BG);if(view_mode==0){draw_source(0,0,12,1);for(r=0;r<12;r++)acc_fill(COLS,r,11,1,' ',ACC_CONTROL);for(i=0;i<80;i++)acc_put(i,12,196,line_attr);draw_preview(0,13,11);for(r=13;r<24;r++)acc_fill(COLS+1,r,11,1,' ',MD_TEXT);}else if(view_mode==1){draw_source(0,0,24,1);for(r=0;r<24;r++)acc_fill(COLS,r,11,1,' ',ACC_CONTROL);}else{draw_preview(0,0,24);for(r=0;r<24;r++)acc_fill(COLS+1,r,10,1,' ',MD_TEXT);}acc_scrollbar(79,0,rows,top,LINES,rows);maximize_page_bar();}
static void maximize_refresh(int key,int oldtop)
{int on,rows=view_mode==0?12:24;if(top!=oldtop||key==8||key==13||key==24||key==22||key==16||key==256+83){if(view_mode==0){draw_source(0,0,12,1);draw_preview(0,13,11);}else if(view_mode==1)draw_source(0,0,24,1);else draw_preview(0,0,24);acc_scrollbar(79,0,rows,top,LINES,rows);maximize_page_bar();return;}if(view_mode!=2)draw_source(0,0,rows,1);if(view_mode==0&&((key>=32&&key<=255)||(key>=513&&key<=767))&&cy>=top&&cy<top+12){on=code_before(current,cy);preview_line(0,13+cy-top,current,cy,on);}acc_scrollbar(79,0,rows,top,LINES,rows);maximize_page_bar();}
static void maximize_mode(void)
{int key=0,mx=0,my=0,rows,oldtop,editkey;unsigned mb=0;clear_selection();maximize_draw();while(key!=27&&key!=256+0x85&&key!=256+0x57){acc_wait(&key,&mx,&my,&mb);rows=view_mode==0?12:24;if((mb&1)&&view_mode!=2&&mx>=0&&mx<COLS&&((view_mode==0&&my>=0&&my<12)||(view_mode==1&&my>=0&&my<24))){cx=mx;cy=top+my;clear_selection();if(view_mode==0)draw_source(0,0,12,1);else draw_source(0,0,24,1);key=0;continue;}if((mb&1)&&my==24){int pg,xx=0;for(pg=0;pg<count;pg++){char ps[6];int pw;sprintf(ps," %d ",pg+1);pw=(int)strlen(ps);if(mx>=xx&&mx<xx+pw)break;xx+=pw+1;}if(pg<count){current=pg;cx=cy=top=0;clear_selection();maximize_draw();}else if(count<PAGES&&mx==xx){if(!ensure_page(count)){acc_notice("Markdown","Not enough memory for another page.");maximize_draw();key=0;continue;}current=count++;blank_page(current);memset(softwrap[current],0,LINES);paths[current][0]=0;external[current]=0;cx=cy=top=0;clear_selection();maximize_draw();}key=0;continue;}if((mb&1)&&mx==79&&my>=0&&my<rows){if(my==0&&top>0)top--;else if(my==rows-1&&top<LINES-rows)top++;else if(my>0&&my<rows-1)top=(my-1)*(LINES-rows)/(rows-2);if(cy<top)cy=top;if(cy>=top+rows)cy=top+rows-1;maximize_draw();key=0;continue;}editkey=key;oldtop=top;if(view_mode==2){if(key!=27&&key!=256+0x85&&key!=256+0x57)key=0;}else if(key!=27&&key!=256+0x85&&key!=256+0x57&&!editor_key(key,rows))key=0;if(cy<top)top=cy;if(cy>=top+rows)top=cy-rows+1;if(key!=27&&key!=256+0x85&&key!=256+0x57)maximize_refresh(editkey,oldtop);}acc_caret_hide();acc_restore_text_screen();acc_mouse_display(1);}

int main(int argc,char **argv)
{
  int x,y,key=0,mx=0,my=0,focus=0,i,argn=0,rows,editkey,oldtop,done=0,discarded=0,choice;unsigned mb=0;
  if(acc_help(argc,argv,"!MKDOWN","Markdown editor with live preview, graphical pages and formatted printing."))return 0;
  if(!acc_begin(argv[0],"Markdown",0))return 1;if(!init_pages()){acc_notice("Markdown","Not enough memory for document pages.");acc_end();return 1;}memset(external,0,sizeof(external));memset(paths,0,sizeof(paths));memset(dirty,0,sizeof(dirty));
  for(i=1;i<argc&&argn<PAGES;i++)if(argv[i][0]!='/'&&argv[i][0]!='-'){if(load_file(argn,argv[i]))argn++;}
  if(argn){count=argn;current=0;}else persist_load();
  x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;draw_ui(x,y,focus);
  while(!done){acc_wait(&key,&mx,&my,&mb);rows=view_mode==0?VIEW:FULL_VIEW;
    if(key==27){if(!any_dirty()){done=1;break;}choice=confirm_close();if(choice==1){discarded=1;done=1;break;}if(choice==2&&save_all_dirty(!argn)){done=1;break;}draw_ui(x,y,focus);key=0;continue;}
    if((key==256+0x85||key==256+0x57)||((mb&1)&&my==y&&mx>=x+DLG_W-8&&mx<x+DLG_W-6)){maximize_mode();draw_ui(x,y,focus);key=0;continue;}
    if(view_mode!=2&&(mb&1)&&my>=y+2&&my<y+2+rows&&mx>=x+2&&mx<x+2+COLS){focus=0;clear_selection();cx=mx-(x+2);cy=top+my-(y+2);draw_source(x+2,y+2,rows,1);key=0;continue;}
    if((mb&1)&&my==y+17){int pg,xx=x+3;for(pg=0;pg<count;pg++){char ps[6];int pw;sprintf(ps," %d ",pg+1);pw=(int)strlen(ps);if(mx>=xx&&mx<xx+pw)break;xx+=pw+1;}if(pg<count){current=pg;cx=cy=top=0;clear_selection();focus=0;draw_ui(x,y,focus);key=0;continue;}if(count<PAGES&&mx==xx){if(!ensure_page(count)){acc_notice("Markdown","Not enough memory for another page.");draw_ui(x,y,focus);key=0;continue;}current=count++;blank_page(current);memset(softwrap[current],0,LINES);paths[current][0]=0;external[current]=0;cx=cy=top=0;clear_selection();focus=0;draw_ui(x,y,focus);key=0;continue;}}
    if((mb&1)&&mx==x+70&&my>=y+2&&my<y+2+rows){if(my==y+2&&top>0)top--;else if(my==y+1+rows&&top<LINES-rows)top++;else if(my>y+2&&my<y+1+rows)top=(my-y-3)*(LINES-rows)/(rows-2);if(top<0)top=0;if(top>LINES-rows)top=LINES-rows;if(cy<top)cy=top;if(cy>=top+rows)cy=top+rows-1;draw_ui(x,y,focus);key=0;continue;}
    if((mb&1)&&my==y+19){int hit=mx-(x+3);if(hit>=0&&hit<6)focus=BTN_SAVE;else if(hit>=8&&hit<14)focus=BTN_PRINT;else if(hit>=18&&hit<25)focus=BTN_SPLIT;else if(hit>=27&&hit<34)focus=BTN_FOCUS;else if(hit>=36&&hit<42)focus=BTN_SHOW;else if(hit>=45&&hit<52)focus=BTN_CHARS;else if(hit>=61&&hit<67)focus=BTN_CLOSE;else{key=0;continue;}key=13;}
    if(key==9||key==271){focus=key==271?(focus?focus-1:BTN_CLOSE):(focus==BTN_CLOSE?0:focus+1);draw_panes(x,y,focus);toolbar(x+3,y+19,focus);key=0;continue;}
    if(focus&&key==13){if(focus==BTN_SHOW){graph_show();draw_ui(x,y,focus);}else if(focus==BTN_CHARS){int ch=character_palette();draw_ui(x,y,focus);if(ch>=0){if(has_selection())delete_selection();insert_one(ch);clear_selection();draw_ui(x,y,focus);}}else if(focus==BTN_PRINT)print_current();else if(focus==BTN_SAVE){save_current();draw_ui(x,y,focus);}else if(focus==BTN_SPLIT){view_mode=(view_mode+1)%3;top=0;if(view_mode==2)clear_selection();draw_ui(x,y,focus);}else if(focus==BTN_FOCUS){focus_mode();draw_ui(x,y,focus);}else{if(!any_dirty())done=1;else{choice=confirm_close();if(choice==1){discarded=1;done=1;}else if(choice==2&&save_all_dirty(!argn))done=1;else draw_ui(x,y,focus);}}key=0;continue;}
    if(focus){if(key!=27)key=0;continue;}
    editkey=key;oldtop=top;if(view_mode==2){if(key!=27)key=0;}else if(key!=27&&!editor_key(key,rows))key=0;
    if(cy<top)top=cy;if(cy>=top+rows)top=cy-rows+1;if(view_mode!=2){draw_edit_refresh(x,y,rows,editkey,oldtop);page_bar(x+3,y+17);}
  }
  if(!argn&&!discarded)persist_save();free_pages();acc_end();return 0;
}
