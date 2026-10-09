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
 * File: MKDOWN.C
 * Role: !MD Markdown editor and shared !MDVIEW renderer
 * Build/ownership: Standalone accessory using ACCLIB and MDFONTS.
 * Maintainer contract: Handles editor, preview, graphical Show, Document Map, import normalization, printing and file lifecycle. Large buffers must avoid overflowing 16-bit DGROUP/stack.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* Launch! Markdown - persistent Markdown editor, live text preview, VGA page
   renderer and formatted Epson-compatible printing.  The same source is built
   with MDVIEW_BUILD for the standalone read-only !MDVIEW renderer.
   Microsoft C/C++ 7.0. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <memory.h>
#include <malloc.h>
#include "ACCLIB.H"
#include "TABCLICK.H"
#include "MDFONTS.H"

#define PAGES 10
#define LINES 450
#define OLD_LINES 60
#define COLS 68
#define DOC_COLS 80
#define FULL_COLS 79
#define VIEW 7
#define FULL_VIEW 15
#define DLG_W 74
#define DLG_H 22
#define BTN_OPEN 1
#define BTN_SAVE 2
#define BTN_PRINT 3
#define BTN_SPLIT 4
#define BTN_FOCUS 5
#define BTN_SHOW 6
#define BTN_CHARS 7
#define BTN_TIDY 8
#define BTN_CLOSE 9
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
#define MD_MEDIA ACC_ATTR(7,4)

typedef char MDLINE[DOC_COLS];
static MDLINE far *page[PAGES];
static char paths[PAGES][ACC_PATH];
static unsigned char external[PAGES];
static unsigned char softwrap[PAGES][LINES];
static unsigned char wrap_width[PAGES];
static unsigned char reflow_soft[LINES];
#define CLIP_MAX 60000U /* comfortably below the 64K far-allocation boundary */
static char far *clip=0;static unsigned clip_len=0;
/* view_mode: 0=editor+preview, 1=editor only, 2=preview only. */
static int current=0,count=1,cx=0,cy=0,top=0,view_mode=0,insert_mode=1;
static long sel_anchor=-1L,sel_caret=-1L;
static unsigned char dirty[PAGES];
/* Import buffers are static to avoid consuming the small DOS runtime stack. */
/* Import scratch is deliberately modest: HTML normalization never expands a
   physical input line beyond this bound.  Keeping these buffers small leaves
   real stack headroom in the 16-bit medium-memory build. */
static char import_line[512],import_clean[768],import_link[256];
static int far *heading_row=0,*heading_level=0,heading_count;

static int edit_width(void){int w=wrap_width[current];return w?w:COLS;}
static void clear_selection(void);
static int headings_popup(void);
static int print_popup(int button_x,int button_y);
static void basic_current(void);
static void ps_current(void);
static void md_scrollbar(int x,int y,int rows,int show_menu);
static int ensure_page(int p){if(!wrap_width[p])wrap_width[p]=COLS;if(page[p])return 1;page[p]=(MDLINE far *)_fmalloc((unsigned)(LINES*(unsigned)DOC_COLS));if(!page[p])return 0;_fmemset(page[p],' ',(unsigned)(LINES*(unsigned)DOC_COLS));return 1;}
static int init_pages(void){memset(page,0,sizeof(page));memset(wrap_width,0,sizeof(wrap_width));return ensure_page(0);}
static void free_pages(void){int i;for(i=0;i<PAGES;i++)if(page[i]){_ffree(page[i]);page[i]=0;}}
static void blank_page(int p){if(ensure_page(p))_fmemset(page[p],' ',(unsigned)(LINES*(unsigned)DOC_COLS));}
static int used(int p,int y){int n=DOC_COLS;while(n&&page[p][y][n-1]==' ')n--;return n;}
static int row_used_width(MDLINE far *pg,int y,int width){int n=width;while(n&&pg[y][n-1]==' ')n--;return n;}
static void persistent_path(char *out){acc_path(out,"DATA","MKDOWN.DAT");}
static void persist_load(void){char p[ACC_PATH],b[COLS],magic[4];FILE*f;int i,y,j,last=0,modern=0,nonblank;blank_page(0);memset(softwrap,0,sizeof(softwrap));persistent_path(p);f=fopen(p,"rb");if(f){if(fread(magic,1,4,f)==4&&!memcmp(magic,"MKD2",4))modern=1;else rewind(f);for(i=0;i<PAGES;i++)for(y=0;y<(modern?LINES:OLD_LINES);y++){if(fread(b,1,COLS,f)!=COLS)goto loaded;nonblank=0;for(j=0;j<COLS;j++)if(b[j]!=' '){nonblank=1;break;}if(nonblank&&ensure_page(i)){_fmemcpy(page[i][y],b,COLS);last=i;}}if(modern)fread(softwrap,1,sizeof(softwrap),f);loaded:fclose(f);}count=last+1;if(count<1)count=1;for(i=0;i<count;i++)wrap_width[i]=COLS;}
static void persist_save(void){char p[ACC_PATH],b[COLS];FILE*f;int i,y;persistent_path(p);f=fopen(p,"wb");if(f){fwrite("MKD2",1,4,f);for(i=0;i<PAGES;i++)for(y=0;y<LINES;y++){if(page[i])_fmemcpy(b,page[i][y],COLS);else memset(b,' ',COLS);fwrite(b,1,COLS,f);}fwrite(softwrap,1,sizeof(softwrap),f);fclose(f);}}
static void force_md(char *p){char *dot,*slash=strrchr(p,'\\'),*other=strrchr(p,'/');if(other&&(!slash||other>slash))slash=other;dot=strrchr(p,'.');if(!dot||(slash&&dot<slash)){if(strlen(p)+3<ACC_PATH)strcat(p,".MD");}else strcpy(dot,".MD");}
static int html_eq(const char *a,const char *b){while(*a&&*b){if(tolower((unsigned char)*a)!=tolower((unsigned char)*b))return 0;a++;b++;}return !*a&&!*b;}
static void html_attr(const char *tag,const char *want,char *out,int max)
{const char *p=tag,*q;char key[16];int k,n;out[0]=0;while(*p&&!isspace((unsigned char)*p))p++;while(*p){while(*p&&isspace((unsigned char)*p))p++;if(!*p||*p=='/')break;k=0;while(*p&&*p!='='&&!isspace((unsigned char)*p)&&*p!='/'&&k<15)key[k++]=*p++;key[k]=0;while(*p&&isspace((unsigned char)*p))p++;if(*p!='='){while(*p&&!isspace((unsigned char)*p))p++;continue;}p++;while(*p&&isspace((unsigned char)*p))p++;q=p;if(*p=='\''||*p=='\"'){int quote=*p++;q=p;while(*p&&*p!=quote)p++;n=(int)(p-q);if(*p)p++;}else{q=p;while(*p&&!isspace((unsigned char)*p)&&*p!='/')p++;n=(int)(p-q);}if(html_eq(key,want)){if(n>=max)n=max-1;memcpy(out,q,n);out[n]=0;return;}}}
static void html_put(char *out,int *op,int max,const char *text)
{while(*text&&*op<max-1)out[(*op)++]=*text++;out[*op]=0;}
static void html_clean_line(const char *in,char *out,int max,char *link)
{int ip=0,op=0,j,k,closing;char tag[256],name[24],src[160],alt[128],tmp[320];out[0]=0;while(in[ip]&&op<max-1){if(in[ip]!='<'){out[op++]=in[ip++];out[op]=0;continue;}j=ip+1;while(in[j]&&in[j]!='>')j++;if(!in[j]){out[op++]=in[ip++];out[op]=0;continue;}k=j-ip-1;if(k>=(int)sizeof(tag))k=sizeof(tag)-1;memcpy(tag,in+ip+1,k);tag[k]=0;ip=j+1;j=0;while(tag[j]&&isspace((unsigned char)tag[j]))j++;closing=0;if(tag[j]=='/'){closing=1;j++;while(tag[j]&&isspace((unsigned char)tag[j]))j++;}k=0;while(tag[j]&&!isspace((unsigned char)tag[j])&&tag[j]!='/'&&k<23)name[k++]=tag[j++];name[k]=0;
 if(html_eq(name,"br")){html_put(out,&op,max,"\n");}
 else if(html_eq(name,"hr")){if(op&&out[op-1]!='\n')html_put(out,&op,max,"\n");html_put(out,&op,max,"---\n");}
 else if(html_eq(name,"u")){html_put(out,&op,max,closing?"}}":"{{");}
 else if(html_eq(name,"b")||html_eq(name,"strong")){html_put(out,&op,max,"**");}
 else if(html_eq(name,"i")||html_eq(name,"em")){html_put(out,&op,max,"*");}
 else if(html_eq(name,"img")&&!closing){html_attr(tag,"src",src,sizeof(src));html_attr(tag,"alt",alt,sizeof(alt));if(src[0]){sprintf(tmp,"![%s](%s)",alt,src);html_put(out,&op,max,tmp);}}
 else if(html_eq(name,"a")){if(!closing){html_attr(tag,"href",link,256);html_put(out,&op,max,"[");}else{html_put(out,&op,max,"](");html_put(out,&op,max,link);html_put(out,&op,max,")");link[0]=0;}}
 /* Every other HTML tag is deliberately discarded. */
 }out[op]=0;}
static int load_emit_span(int pg,const char *line,int n,int *row)
{int pos=0,left,cut,w=COLS;softwrap[pg][*row]=0;if(!n){(*row)++;return 1;}while(pos<n&&*row<LINES){left=n-pos;cut=left<w?left:w;if(left>w){while(cut>0&&line[pos+cut]!=' ')cut--;if(!cut)cut=w;}_fmemcpy(page[pg][*row],line+pos,cut);pos+=cut;while(pos<n&&line[pos]==' ')pos++;(*row)++;if(pos<n&&*row<LINES)softwrap[pg][*row]=1;}return *row<LINES;}
static int load_file(int pg,const char *name)
{FILE*f;char *p,*e;int row=0,n;if(!ensure_page(pg))return 0;wrap_width[pg]=COLS;blank_page(pg);memset(softwrap[pg],0,LINES);import_link[0]=0;f=fopen(name,"r");if(!f)return 0;while(fgets(import_line,sizeof(import_line),f)&&row<LINES){n=(int)strlen(import_line);while(n&&(import_line[n-1]=='\r'||import_line[n-1]=='\n'))import_line[--n]=0;/* 3.74: opening is non-destructive; Tidy performs cleanup explicitly. */
 p=import_line;for(;;){e=strchr(p,'\n');if(e){load_emit_span(pg,p,(int)(e-p),&row);p=e+1;if(row>=LINES)break;}else{load_emit_span(pg,p,(int)strlen(p),&row);break;}}}fclose(f);strncpy(paths[pg],name,ACC_PATH-1);paths[pg][ACC_PATH-1]=0;external[pg]=1;return 1;}
static int write_file(int pg,const char *name){FILE*f=fopen(name,"w");char b[DOC_COLS];int y,n,next;if(!f)return 0;for(y=0;y<LINES;y++){n=used(pg,y);if(n){_fmemcpy(b,page[pg][y],n);fwrite(b,1,n,f);}next=y+1<LINES&&softwrap[pg][y+1];if(next){if(n&&used(pg,y+1))fputc(' ',f);}else fputc('\n',f);}if(fclose(f))return 0;strncpy(paths[pg],name,ACC_PATH-1);paths[pg][ACC_PATH-1]=0;external[pg]=1;dirty[pg]=0;return 1;}

static int reflow_emit(MDLINE far *dst,int *outrow,const char *b,int n,int continuation,int width)
{if(*outrow>=LINES)return 0;if(n>width)n=width;if(n>0)_fmemcpy(dst[*outrow],b,n);reflow_soft[*outrow]=(unsigned char)(continuation?1:0);(*outrow)++;return 1;}
static int reflow_page(int p,int new_width)
{
 MDLINE far *tmp;char pending[DOC_COLS*2+2];int old_width,y,r,e,last,n,nextn,i,k,plen=0,cut,drop,outrow=0,cont,emitted;
 int cursor_line=0,cursor_start=0,target_row=0,hard=0,sep;long cursor_off=0,off;
 if(new_width<COLS)new_width=COLS;if(new_width>DOC_COLS)new_width=DOC_COLS;old_width=wrap_width[p]?wrap_width[p]:COLS;if(old_width==new_width)return 1;
 if(p==current){cursor_start=cy;while(cursor_start>0&&softwrap[p][cursor_start])cursor_start--;for(r=1;r<=cursor_start;r++)if(!softwrap[p][r])cursor_line++;for(r=cursor_start;r<cy;r++){n=used(p,r);cursor_off+=n;if(r+1<LINES&&softwrap[p][r+1]&&n&&used(p,r+1))cursor_off++;}n=used(p,cy);cursor_off+=cx<n?cx:n;}
 tmp=(MDLINE far *)_fmalloc((unsigned)(LINES*(unsigned)DOC_COLS));if(!tmp)return 0;_fmemset(tmp,' ',(unsigned)(LINES*(unsigned)DOC_COLS));memset(reflow_soft,0,sizeof(reflow_soft));
 last=LINES-1;while(last>0&&used(p,last)==0&&!softwrap[p][last])last--;
 y=0;while(y<=last){e=y;while(e+1<=last&&softwrap[p][e+1])e++;plen=0;cont=0;emitted=0;
  for(r=y;r<=e;r++){n=used(p,r);for(i=0;i<n;i++){pending[plen++]=page[p][r][i];if(plen>new_width){cut=0;for(k=new_width;k>0;k--)if(pending[k]==' '){cut=k;break;}drop=cut?1:0;if(!cut)cut=new_width;if(!reflow_emit(tmp,&outrow,pending,cut,cont,new_width)){_ffree(tmp);return 0;}emitted=1;cont=1;memmove(pending,pending+cut+drop,plen-cut-drop);plen-=cut+drop;}}
   if(r<e&&n&&(nextn=used(p,r+1))&&n<old_width){pending[plen++]=' ';if(plen>new_width){cut=0;for(k=new_width;k>0;k--)if(pending[k]==' '){cut=k;break;}drop=cut?1:0;if(!cut)cut=new_width;if(!reflow_emit(tmp,&outrow,pending,cut,cont,new_width)){_ffree(tmp);return 0;}emitted=1;cont=1;memmove(pending,pending+cut+drop,plen-cut-drop);plen-=cut+drop;}}
  }
  if(plen||!emitted){if(!reflow_emit(tmp,&outrow,pending,plen,cont,new_width)){_ffree(tmp);return 0;}}
  y=e+1;
 }
 if(p==current){if(cursor_line==0)target_row=0;else{hard=0;for(r=1;r<outrow;r++)if(!reflow_soft[r]){hard++;if(hard==cursor_line){target_row=r;break;}}}off=cursor_off;r=target_row;for(;;){n=row_used_width(tmp,r,new_width);if(r+1<outrow&&reflow_soft[r+1]){nextn=row_used_width(tmp,r+1,new_width);sep=(n&&nextn)?1:0;if(off<=n)break;off-=n+sep;r++;continue;}break;}cy=r;cx=(int)off;if(cx>n)cx=n;if(cx>=new_width)cx=new_width-1;if(cx<0)cx=0;}
 _ffree(page[p]);page[p]=tmp;memcpy(softwrap[p],reflow_soft,LINES);wrap_width[p]=(unsigned char)new_width;clear_selection();return 1;
}

static int has_selection(void){return sel_anchor>=0&&sel_caret>=0&&sel_anchor!=sel_caret;}
static long sel_low(void){return sel_anchor<sel_caret?sel_anchor:sel_caret;}
static long sel_high(void){return sel_anchor>sel_caret?sel_anchor:sel_caret;}
static void clear_selection(void){sel_anchor=sel_caret=-1L;}
static int shift_down(void){return (*(unsigned char far *)(((unsigned long)0x40<<16)|0x17)&3)!=0;}
static void selection_move(long oldpos,long newpos,int shift){if(shift){if(sel_anchor<0)sel_anchor=oldpos;sel_caret=newpos;if(sel_caret==sel_anchor)clear_selection();}else clear_selection();}
static void copy_selection(void){long lo,hi,p;int row,lastrow,w=edit_width();unsigned n=0;if(!has_selection())return;if(!clip){clip=(char far *)_fmalloc(CLIP_MAX);if(!clip){acc_notice("Clipboard","Not enough memory for clipboard.");return;}}lo=sel_low();hi=sel_high();lastrow=(int)((hi-1L)/w);for(p=lo;p<hi&&n<CLIP_MAX-3U;p++){row=(int)(p/w);clip[n++]=page[current][row][(int)(p%w)];if(p+1L<hi&&(p+1L)%w==0&&row<lastrow&&!softwrap[current][row+1]){clip[n++]='\r';clip[n++]='\n';}}clip_len=n;}
static void delete_selection(void){long lo,hi;int fr,lr,r,start,end,n,w=edit_width();if(!has_selection())return;lo=sel_low();hi=sel_high();fr=(int)(lo/w);lr=(int)((hi-1L)/w);for(r=fr;r<=lr;r++){start=r==fr?(int)(lo%w):0;end=r==lr?(int)(((hi-1L)%w)+1L):w;n=end-start;if(n>0){_fmemmove(page[current][r]+start,page[current][r]+end,w-end);_fmemset(page[current][r]+w-n,' ',n);}}cy=fr;cx=(int)(lo%w);clear_selection();dirty[current]=1;}
static void word_wrap(void){int w=edit_width(),start=w-1,len,i,can=1;if(cy>=LINES-1)return;if(page[current][cy][w-1]==' '){cy++;cx=0;softwrap[current][cy]=1;return;}while(start>0&&page[current][cy][start-1]!=' ')start--;if(start<=0){cy++;cx=0;softwrap[current][cy]=1;return;}len=w-start;for(i=w-len;i<w;i++)if(page[current][cy+1][i]!=' ')can=0;if(!can){cy++;cx=0;softwrap[current][cy]=1;return;}_fmemmove(page[current][cy+1]+len,page[current][cy+1],w-len);_fmemcpy(page[current][cy+1],page[current][cy]+start,len);_fmemset(page[current][cy]+start,' ',w-start);cy++;cx=len;softwrap[current][cy]=1;}
static void insert_one(int ch){int w=edit_width();if(insert_mode&&cx<w-1)_fmemmove(page[current][cy]+cx+1,page[current][cy]+cx,w-cx-1);page[current][cy][cx]=(char)ch;if(cx<w-1)cx++;else word_wrap();dirty[current]=1;}
static void paste_clip(void){unsigned i;int c;if(!clip||!clip_len)return;if(has_selection())delete_selection();for(i=0;i<clip_len&&cy<LINES;i++){c=(unsigned char)clip[i];if(c=='\r')continue;if(c=='\n'){if(cy<LINES-1){cy++;cx=0;softwrap[current][cy]=0;}continue;}insert_one(c);}clear_selection();}
static void remove_row(int row){int r;for(r=row;r<LINES-1;r++){_fmemcpy(page[current][r],page[current][r+1],DOC_COLS);softwrap[current][r]=softwrap[current][r+1];}_fmemset(page[current][LINES-1],' ',DOC_COLS);softwrap[current][LINES-1]=0;}
static void join_next_line(void){int w=edit_width(),n2,take,room;if(cy>=LINES-1)return;n2=used(current,cy+1);room=w-cx;if(room<0)room=0;take=n2<room?n2:room;if(take)_fmemcpy(page[current][cy]+cx,page[current][cy+1],take);if(take>=n2)remove_row(cy+1);else{_fmemmove(page[current][cy+1],page[current][cy+1]+take,w-take);_fmemset(page[current][cy+1]+w-take,' ',take);}dirty[current]=1;}
static void join_previous_line(void){int w=edit_width(),prev,n,take,room;if(cy<=0)return;prev=used(current,cy-1);n=used(current,cy);room=w-prev;if(room<=0){cy--;cx=w-1;return;}take=n<room?n:room;if(take)_fmemcpy(page[current][cy-1]+prev,page[current][cy],take);if(take>=n)remove_row(cy);else{_fmemmove(page[current][cy],page[current][cy]+take,w-take);_fmemset(page[current][cy]+w-take,' ',take);}cy--;cx=prev;dirty[current]=1;}
static void split_line(void){int w=edit_width(),r,n,tail,indent=0;if(cy>=LINES-1)return;while(indent<w&&page[current][cy][indent]==' ')indent++;if(indent>=w)indent=0;for(r=LINES-1;r>cy+1;r--){_fmemcpy(page[current][r],page[current][r-1],DOC_COLS);softwrap[current][r]=softwrap[current][r-1];}_fmemset(page[current][cy+1],' ',DOC_COLS);n=used(current,cy);tail=n>cx?n-cx:0;if(tail>w-indent)tail=w-indent;if(tail)_fmemcpy(page[current][cy+1]+indent,page[current][cy]+cx,tail);_fmemset(page[current][cy]+cx,' ',w-cx);softwrap[current][cy+1]=0;cy++;cx=indent;dirty[current]=1;}
static int editor_key(int key,int rows){int w=edit_width(),shift=shift_down();long oldpos=(long)cy*w+cx;if(key==3){copy_selection();return 1;}if(key==24){copy_selection();delete_selection();return 1;}if(key==22){paste_clip();return 1;}if(key==256+0x77){cy=0;cx=0;}else if(key==256+0x75){cy=LINES-1;while(cy>0&&!used(current,cy))cy--;cx=used(current,cy);if(cx>=w)cx=w-1;}else if(key==256+75&&cx>0)cx--;else if(key==256+77&&cx<w-1)cx++;else if(key==256+72&&cy>0)cy--;else if(key==256+80&&cy<LINES-1)cy++;else if(key==256+71)cx=0;else if(key==256+79){cx=w-1;while(cx>0&&page[current][cy][cx]==' ')cx--;if(page[current][cy][cx]!=' '&&cx<w-1)cx++;}else if(key==256+73){cy-=rows;if(cy<0)cy=0;}else if(key==256+81){cy+=rows;if(cy>=LINES)cy=LINES-1;}else if(key==256+82){clear_selection();insert_mode=!insert_mode;return 1;}else if(key==256+83){if(has_selection())delete_selection();else if(cx>=used(current,cy))join_next_line();else{_fmemmove(page[current][cy]+cx,page[current][cy]+cx+1,w-cx-1);page[current][cy][w-1]=' ';dirty[current]=1;}return 1;}else if(key==8){if(has_selection())delete_selection();else if(cx>0){cx--;_fmemmove(page[current][cy]+cx,page[current][cy]+cx+1,w-cx-1);page[current][cy][w-1]=' ';dirty[current]=1;}else join_previous_line();return 1;}else if(key==13&&cy<LINES-1){if(has_selection())delete_selection();split_line();clear_selection();return 1;}else if((key>=32&&key<=255)||(key>=513&&key<=767)){if(has_selection())delete_selection();insert_one(key>=512?key-512:key);clear_selection();return 1;}else return 0;selection_move(oldpos,(long)cy*w+cx,shift);return 1;}

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
static void draw_source(int x,int y,int rows,int cursor,int width)
{int r,c;long pos,lo=has_selection()?sel_low():-1L,hi=has_selection()?sel_high():-1L;char b[DOC_COLS+1];if(width<COLS)width=COLS;if(width>DOC_COLS)width=DOC_COLS;for(r=0;r<rows;r++){int line=top+r;if(line<LINES){_fmemcpy(b,page[current][line],width);b[width]=0;}else memset(b,' ',width),b[width]=0;acc_text(x,y+r,b,ACC_CONTROL,width);if(lo>=0)for(c=0;c<width;c++){pos=(long)line*width+c;if(pos>=lo&&pos<hi)acc_put(x+c,y+r,b[c],ACC_SELECT);}}if(cursor&&cy>=top&&cy<top+rows){acc_caret_set(x+cx,y+cy-top);}else if(!cursor)acc_caret_hide();}

/* Markdown block helpers follow the page's active soft-wrap width.  Normal
   editing uses 68 columns; full-screen uses 79 text columns with the
   scrollbar occupying the screen-edge column. */
#define MD_TCOLS 8
#define MD_LOGICAL 512
typedef struct {int cols,pipe;int start[MD_TCOLS],width[MD_TCOLS];int logical,lstart,slot;} MDTABLE;
static void md_line(int p,int row,char *s,int *n)
{int w=wrap_width[p]?wrap_width[p]:COLS;if(row<0||row>=LINES){s[0]=0;*n=0;return;}_fmemcpy(s,page[p][row],w);s[w]=0;*n=w;while(*n&&s[*n-1]==' ')(*n)--;s[*n]=0;}
static int md_lead(const char *s,int n){int i=0;while(i<n&&s[i]==' ')i++;return i;}
static int md_soft_quote_column(int p,int row)
{int r,n,i;char s[DOC_COLS+1];if(row<=0||!softwrap[p][row])return-1;r=row;while(r>0&&softwrap[p][r])r--;md_line(p,r,s,&n);i=md_lead(s,n);return(i<n&&s[i]=='>')?i:-1;}
static int md_log_start(int p,int row){if(row<0)return 0;if(row>=LINES)row=LINES-1;while(row>0&&softwrap[p][row])row--;return row;}
static int md_log_end(int p,int row){row=md_log_start(p,row);while(row+1<LINES&&softwrap[p][row+1])row++;return row;}
static int md_log_next(int p,int row){int e=md_log_end(p,row);return e+1<LINES?e+1:-1;}
static int md_log_prev(int p,int row){int s=md_log_start(p,row);return s>0?md_log_start(p,s-1):-1;}
static void md_log_line(int p,int row,char *s,int *n)
{int r,e,k=0,u,w=wrap_width[p]?wrap_width[p]:COLS,prev;prev=w;row=md_log_start(p,row);e=md_log_end(p,row);for(r=row;r<=e&&k<MD_LOGICAL;r++){u=used(p,r);/* A soft wrap only represents a discarded separator space when the
   previous chunk ended before the active editor width.  A completely full
   chunk is a hard split and is rejoined without inventing a space. */if(r>row&&k<MD_LOGICAL&&u&&prev<w)s[k++]=' ';if(u>MD_LOGICAL-k)u=MD_LOGICAL-k;if(u){_fmemcpy(s+k,page[p][r],u);k+=u;}prev=used(p,r);}s[k]=0;*n=k;}
static int md_blank(int p,int row){return row<0||row>=LINES||used(p,row)==0;}
static int md_list_marker(const char *s,int n,int pos,int *after,int *ordered)
{int k;if(pos+1<n&&(s[pos]=='-'||s[pos]=='*'||s[pos]=='+')&&s[pos+1]==' '){*after=pos+2;*ordered=0;return 1;}if(pos<n&&isdigit((unsigned char)s[pos])){k=pos;while(k<n&&isdigit((unsigned char)s[k]))k++;if(k+1<n&&s[k]=='.'&&s[k+1]==' '){*after=k+2;*ordered=1;return 1;}}return 0;}
static int md_fence_text(const char *s,int n)
{int i=md_lead(s,n),j;char c;if(i>3||i>=n)return 0;c=s[i];if(c!='`'&&c!='~')return 0;j=i;while(j<n&&s[j]==c)j++;return j-i>=3?(int)(unsigned char)c:0;}
static int line_fence_kind(int p,int row){char s[DOC_COLS+1];int n;md_line(p,row,s,&n);return md_fence_text(s,n);}
static int line_fence(int p,int row){return line_fence_kind(p,row)!=0;}
static int code_fence_before(int p,int row)
{int i,k=0,f;for(i=0;i<row;i++){f=line_fence_kind(p,i);if(f&&(!k||f==k))k=k?0:f;}return k;}
static int code_before(int p,int row){return code_fence_before(p,row)!=0;}
static int md_enclosing_list_indent(int p,int row,int lead)
{char s[DOC_COLS+1];int n,l,after,ordered,r;for(r=row-1;r>=0&&r>=row-32;r--){md_line(p,r,s,&n);if(!n)continue;l=md_lead(s,n);after=ordered=0;if(l<lead&&md_list_marker(s,n,l,&after,&ordered))return l;if(l<lead)return -1;}return -1;}
static int md_indented_code_raw(int p,int row)
{char s[DOC_COLS+1];int n,lead,after=0,ordered=0,li;md_line(p,row,s,&n);lead=md_lead(s,n);if(lead<4||lead>=n)return 0;if(md_list_marker(s,n,lead,&after,&ordered))return 0;li=md_enclosing_list_indent(p,row,lead);if(li>=0&&lead<li+8)return 0;return 1;}
static int md_indented_code(int p,int row)
{int r=row;if(row<0||row>=LINES)return 0;while(r>0&&softwrap[p][r])r--;return md_indented_code_raw(p,r);}
static int md_definition_line(int p,int row,int *textpos)
{char s[DOC_COLS+1];int n,i;md_line(p,row,s,&n);i=md_lead(s,n);if(i<n&&s[i]==':'&&i+1<n&&(s[i+1]==' '||s[i+1]=='\t')){i+=2;while(i<n&&s[i]==' ')i++;if(textpos)*textpos=i;return 1;}return 0;}
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
{char s[DOC_COLS+1],a[DOC_COLS+1];int n,an;md_line(p,row,s,&n);if(md_table_sep_text(s,n,t))return 1;if(!md_pure_dash_rule(s,n))return 0;if(row>0){md_line(p,row-1,a,&an);if(md_table_header_text(a,an,t))return 1;}if(row+1<LINES){md_line(p,row+1,a,&an);if(md_table_header_text(a,an,t))return 1;}return 0;}
static int md_table_same(const MDTABLE *a,const MDTABLE *b)
{int i;if(a->cols!=b->cols)return 0;for(i=0;i<a->cols;i++)if(a->width[i]!=b->width[i]&&a->start[i]!=b->start[i])return 0;return 1;}
static int md_table_rowish(int p,int row,const MDTABLE *t)
{char s[DOC_COLS+1];int n,i,non=0;md_line(p,row,s,&n);if(!n)return 0;i=md_lead(s,n);if(i>=n)return 0;if(!strnicmp(s+i,"Table:",6)||s[i]=='#'||s[i]=='>')return 0;if(t->pipe){for(i=0;i<n;i++)if(s[i]=='|')return 1;return 0;}for(i=0;i<t->cols;i++){int st=t->start[i],en=(i+1<t->cols)?t->start[i+1]:n,j;if(st>=n)continue;if(en>n)en=n;for(j=st;j<en;j++)if(s[j]!=' '){non++;break;}}return non>=1;}
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
/* Unsupported media is represented by a labelled placeholder rather than
   silently disappearing or leaking raw markup into Preview/Show/Print. */
#define MDM_IMAGE 1
#define MDM_VIDEO 2
#define MDM_AUDIO 3
#define MDM_EMBED 4
typedef struct {int kind;char name[48];char alt[48];} MDMEDIA;
static int md_ci_eq(char a,char b){return tolower((unsigned char)a)==tolower((unsigned char)b);}
static int md_ci_at(const char*s,int n,int pos,const char*pat)
{int i,l=(int)strlen(pat);if(pos<0||pos+l>n)return 0;for(i=0;i<l;i++)if(!md_ci_eq(s[pos+i],pat[i]))return 0;return 1;}
static int md_ci_find(const char*s,int n,const char*pat)
{int i,l=(int)strlen(pat);for(i=0;i+l<=n;i++)if(md_ci_at(s,n,i,pat))return i;return -1;}
static void md_trim_copy(char*out,int max,const char*s,int a,int b)
{int n;if(max<1)return;while(a<b&&isspace((unsigned char)s[a]))a++;while(b>a&&isspace((unsigned char)s[b-1]))b--;if(a<b&&s[a]=='<'&&s[b-1]=='>'){a++;b--;}if(a<b&&(s[a]=='\''||s[a]=='\"'))a++;if(b>a&&(s[b-1]=='\''||s[b-1]=='\"'))b--;n=b-a;if(n>=max)n=max-1;if(n>0)memcpy(out,s+a,n);out[n]=0;}
static void md_media_name(char*out,int max,const char*src)
{const char*b=src,*q;int n;if(!src||!*src){strncpy(out,"embedded",max-1);out[max-1]=0;return;}for(q=src;*q;q++)if(*q=='/'||*q=='\\')b=q+1;n=0;while(b[n]&&b[n]!='?'&&b[n]!='#'&&!isspace((unsigned char)b[n])&&n<max-1){out[n]=b[n];n++;}out[n]=0;if(!n){strncpy(out,"embedded",max-1);out[max-1]=0;}}
static int md_html_attr(const char*s,int n,int tagpos,const char*attr,char*out,int max)
{int end=tagpos,i,a,b,l=(int)strlen(attr);while(end<n&&s[end]!='>')end++;for(i=tagpos;i+l+1<end;i++)if(md_ci_at(s,end,i,attr)){int k=i+l;while(k<end&&isspace((unsigned char)s[k]))k++;if(k>=end||s[k]!='=')continue;k++;while(k<end&&isspace((unsigned char)s[k]))k++;if(k>=end)continue;if(s[k]=='\"'||s[k]=='\''){char q=s[k++];a=k;while(k<end&&s[k]!=q)k++;b=k;}else{a=k;while(k<end&&!isspace((unsigned char)s[k])&&s[k]!='>')k++;b=k;}md_trim_copy(out,max,s,a,b);return out[0]!=0;}return 0;}
static int md_media_parse(const char*s,int n,MDMEDIA*m)
{int i,j,k,p=-1,kind=0;char src[96];memset(m,0,sizeof(*m));src[0]=0;
 /* Standard Markdown image: ![alt text](path-or-url "optional title"). */
 for(i=0;i+3<n;i++)if(s[i]=='!'&&s[i+1]=='['){j=i+2;while(j<n&&s[j]!=']')j++;if(j<n&&j+1<n&&s[j+1]=='('){k=j+2;while(k<n&&isspace((unsigned char)s[k]))k++;p=k;while(k<n&&s[k]!=')'&&!isspace((unsigned char)s[k]))k++;md_trim_copy(src,sizeof(src),s,p,k);md_trim_copy(m->alt,sizeof(m->alt),s,i+2,j);m->kind=MDM_IMAGE;md_media_name(m->name,sizeof(m->name),src);return 1;}}
 /* Raw HTML media commonly encountered in Markdown documents. */
 if((p=md_ci_find(s,n,"<img"))>=0)kind=MDM_IMAGE;
 else if((p=md_ci_find(s,n,"<video"))>=0)kind=MDM_VIDEO;
 else if((p=md_ci_find(s,n,"<audio"))>=0)kind=MDM_AUDIO;
 else if((p=md_ci_find(s,n,"<iframe"))>=0||(p=md_ci_find(s,n,"<embed"))>=0||(p=md_ci_find(s,n,"<object"))>=0)kind=MDM_EMBED;
 if(!kind)return 0;m->kind=kind;
 if(kind==MDM_EMBED&&!md_html_attr(s,n,p,"src",src,sizeof(src)))md_html_attr(s,n,p,"data",src,sizeof(src));
 else if(kind!=MDM_EMBED)md_html_attr(s,n,p,"src",src,sizeof(src));
 if(!md_html_attr(s,n,p,"alt",m->alt,sizeof(m->alt)))md_html_attr(s,n,p,"title",m->alt,sizeof(m->alt));
 md_media_name(m->name,sizeof(m->name),src);return 1;
}
static const char*md_media_label(int kind)
{return kind==MDM_IMAGE?"Image":(kind==MDM_VIDEO?"Video":(kind==MDM_AUDIO?"Audio":"Embed"));}
static void md_media_desc(const MDMEDIA*m,char*out,int max)
{int left;const char*l=md_media_label(m->kind);if(max<2)return;sprintf(out,"%s: %s",l,m->name[0]?m->name:"embedded");if(m->alt[0]){left=max-1-(int)strlen(out);if(left>3){strncat(out," | ",left);left=max-1-(int)strlen(out);if(left>0)strncat(out,m->alt,left);}}out[max-1]=0;}
static int inline_attr(int bold,int italic,int underline,int strike,int code,int super,int sub)
{if(code)return MD_CODE;if(strike)return MD_STRIKE;if(underline)return MD_UNDERLINE;if(super)return MD_SUPER;if(sub)return MD_SUB;if(bold&&italic)return MD_BOLDITALIC;if(italic)return MD_ITALIC;if(bold)return MD_BOLD;return MD_TEXT;}
static void preview_line(int x,int y,int p,int row,int in_code)
{
 char s[DOC_COLS+1],tb[DOC_COLS+1],mdesc[DOC_COLS+1];MDTABLE tt;MDMEDIA media;int n,i=0,o=0,h=0,bold=0,italic=0,underline=0,strike=0,code=in_code,super=0,sub=0,quote=0,a,k;
 int tctx,tborder=0,lead,after=0,ordered=0,defpos=0,qcol;char c;
 md_line(p,row,s,&n);acc_fill(x,y,COLS,1,' ',in_code?MD_CODE:MD_TEXT);
 if(line_fence(p,row))return;
 if(in_code){while(i<n&&o<COLS)acc_put(x+o++,y,s[i++],MD_CODE);return;}
 if(md_media_parse(s,n,&media)){int dl;md_media_desc(&media,mdesc,COLS-3);dl=(int)strlen(mdesc);acc_put(x,y,179,MD_MEDIA);acc_put(x+1,y,' ',MD_MEDIA);for(i=0;i<dl&&i<COLS-4;i++)acc_put(x+2+i,y,(unsigned char)mdesc[i],MD_MEDIA);for(;i<COLS-4;i++)acc_put(x+2+i,y,' ',MD_MEDIA);acc_put(x+COLS-2,y,' ',MD_MEDIA);acc_put(x+COLS-1,y,179,MD_MEDIA);return;}
 tctx=md_table_context(p,row,&tt,&tborder);if(tctx){n=md_table_make_row(p,row,&tt,tctx==2?tborder:0,tb);for(i=0;i<n&&i<COLS;i++)acc_put(x+i,y,(unsigned char)tb[i],MD_TEXT);return;}
 if(md_indented_code(p,row)){i=4;while(i<n&&o<COLS)acc_put(x+o++,y,s[i++],MD_CODE);return;}
 lead=md_lead(s,n);qcol=md_soft_quote_column(p,row);
 if(qcol>=0){quote=1;i=lead;o=qcol;if(o<COLS)acc_put(x+o++,y,179,MD_QUOTE);if(o<COLS)acc_put(x+o++,y,' ',MD_TEXT);}
 else if(md_definition_term(p,row)){i=lead;while(i<n&&o<COLS)acc_put(x+o++,y,s[i++],MD_BOLD);return;}
 else if(md_definition_line(p,row,&defpos)){o=4;i=defpos;}
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
{int i,rows=view_mode==0?VIEW:FULL_VIEW,line_attr=ACC_ATTR(acc_appearance.background,acc_appearance.border);if(view_mode==0){draw_source(x+2,y+2,VIEW,focus==0,COLS);for(i=0;i<DLG_W-2;i++)acc_put(x+1+i,y+9,196,line_attr);draw_preview(x+2,y+10,VIEW);}else if(view_mode==1)draw_source(x+2,y+2,FULL_VIEW,focus==0,COLS);else draw_preview(x+2,y+2,FULL_VIEW);md_scrollbar(x+70,y+2,rows,view_mode!=2);}
static void draw_edit_refresh(int x,int y,int rows,int key,int oldtop)
{int on;if(top!=oldtop||key==8||key==13||key==24||key==22||key==256+83){draw_panes(x,y,0);return;}draw_source(x+2,y+2,rows,1,COLS);if(view_mode==0&&((key>=32&&key<=255)||(key>=513&&key<=767))&&cy>=top&&cy<top+VIEW){on=code_before(current,cy);preview_line(x+2,y+10+cy-top,current,cy,on);}md_scrollbar(x+70,y+2,rows,view_mode!=2);}
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

static void tidy_current(void)
{
 /* Build the tidied document in separate far storage.  Never compact the live
    page in place: page[] is a far allocation and Tidy may remove rows. */
 MDLINE far *clean;
 unsigned total=(unsigned)(LINES*(unsigned)DOC_COLS);
 int r,w=wrap_width[current],out=0,blank=0,i,j,in_tag;
 char src[DOC_COLS+1],dst[DOC_COLS+1];
 clean=(MDLINE far *)_fmalloc(total);
 if(!clean){acc_notice("Tidy","Not enough memory to tidy this document.");return;}
 _fmemset(clean,' ',total);
 if(w<1||w>DOC_COLS)w=COLS;
 for(r=0;r<LINES;r++){
  int n=used(current,r);
  if(n>DOC_COLS)n=DOC_COLS;
  if(n)_fmemcpy(src,page[current][r],n);
  src[n]=0;
  while(n>0&&src[n-1]==' ')src[--n]=0;
  j=0;in_tag=0;
  for(i=0;i<n&&j<w;i++){
   unsigned char c=(unsigned char)src[i];
   if(c=='<'){in_tag=1;continue;}
   if(in_tag){if(c=='>')in_tag=0;continue;}
   if(c>=128)continue;
   if(c=='\t')c=' ';
   if(c==' '&&j&&dst[j-1]==' ')continue;
   dst[j++]=(char)c;
  }
  while(j&&dst[j-1]==' ')j--;
  dst[j]=0;
  /* Normalize common heading/list spacing without ever exceeding the row. */
  if(j&&dst[0]=='#'){
   i=0;while(i<j&&i<5&&dst[i]=='#')i++;
   if(i<j&&dst[i]!=' '&&j+1<w){memmove(dst+i+1,dst+i,(size_t)(j-i+1));dst[i]=' ';j++;}
  }
  if(j>1&&(dst[0]=='-'||dst[0]=='*'||dst[0]=='+')&&dst[1]!=' '&&j+1<w){
   memmove(dst+2,dst+1,(size_t)j);dst[1]=' ';j++;
  }
  if(!j){if(blank)continue;blank=1;}else blank=0;
  if(out>=LINES)break;
  if(j>DOC_COLS)j=DOC_COLS;
  if(j)_fmemcpy(clean[out],dst,j);
  out++;
 }
 _fmemcpy(page[current],clean,total);
 _ffree(clean);
 _fmemset(softwrap[current],0,LINES);
 dirty[current]=1;cx=cy=top=0;clear_selection();
}
static void toolbar(int x,int y,int focus)
{acc_button(x,y," Open ",focus==BTN_OPEN);acc_button(x+7,y," Save ",focus==BTN_SAVE);acc_button(x+14,y," Print ",focus==BTN_PRINT);acc_put(x+21,y,179,ACC_BORDER);acc_button(x+23,y," Split ",focus==BTN_SPLIT);acc_button(x+31,y," Focus ",focus==BTN_FOCUS);acc_button(x+39,y," Show ",focus==BTN_SHOW);acc_put(x+46,y,179,ACC_BORDER);acc_button(x+48,y," \001 ",focus==BTN_CHARS);acc_button(x+52,y," Tidy ",focus==BTN_TIDY);acc_button(x+62,y," Exit ",focus==BTN_CLOSE);}

static void draw_ui(int x,int y,int focus)
{int i;acc_box(x,y,DLG_W,DLG_H,"Markdown");draw_panes(x,y,focus);page_bar(x+3,y+17);for(i=0;i<DLG_W-6;i++)acc_put(x+3+i,y+18,196,ACC_BORDER);toolbar(x+3,y+19,focus);}

static int md_open_dialog(char *out){int w=60,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=0,f=0;unsigned mb=0;out[0]=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Open Markdown",1);acc_text(x+3,y+2,"Filename(s):",ACC_LABEL,12);acc_fill(x+15,y+2,40,1,' ',f==0?ACC_SELECT:ACC_CONTROL);acc_text(x+15,y+2,out,f==0?ACC_SELECT:ACC_CONTROL,40);if(f==0)acc_caret_set(x+15+(pos<40?pos:39),y+2);else acc_caret_hide();acc_button(x+3,y+6," Open ",f==1);acc_button(x+11,y+6," Cancel ",f==2);acc_wait(&k,&mx,&my,&mb);if(k==27){acc_modal_end();return 0;}if((mb&1)&&my==y+2){f=0;k=0;}else if((mb&1)&&my==y+6&&mx>=x+3&&mx<x+9){f=1;k=13;}else if((mb&1)&&my==y+6&&mx>=x+11&&mx<x+19){f=2;k=13;}if(k==9||k==271){f=k==271?(f+2)%3:(f+1)%3;k=0;continue;}if(k==13&&f==2){acc_modal_end();return 0;}if(k==13&&(f==0||f==1)){if(pos){acc_modal_end();return 1;}k=0;continue;}if(f==0){int n=(int)strlen(out);if(k==256+71)pos=0;else if(k==256+79)pos=n;else if(k==8&&pos){memmove(out+pos-1,out+pos,n-pos+1);pos--;}else if(k>=32&&k<127&&n<ACC_PATH-1){memmove(out+pos+1,out+pos,n-pos+1);out[pos++]=(char)k;}}k=0;}}
static void md_resolve_path(const char *name,char *full){if(strchr(name,'\\')||strchr(name,'/')||strchr(name,':')){strncpy(full,name,ACC_PATH-1);full[ACC_PATH-1]=0;}else sprintf(full,"%s\\%s",acc_directory,name);}
static int md_text_file(const char *path){FILE *f;int c;f=fopen(path,"rb");if(!f)return 0;while((c=fgetc(f))!=EOF){if(c==0||(c<32&&c!='\r'&&c!='\n'&&c!='\t'&&c!='\f')){fclose(f);return 0;}}fclose(f);return 1;}
static int md_open_files(const char *list){static char work[ACC_PATH],full[ACC_PATH],msg[ACC_PATH+32];char *q,*start;int first=-1;strncpy(work,list,ACC_PATH-1);work[ACC_PATH-1]=0;q=work;while(*q&&count<PAGES){while(*q==' ')q++;if(!*q)break;start=q;while(*q&&*q!=' ')q++;if(*q)*q++=0;md_resolve_path(start,full);if(!acc_exists(full)){sprintf(msg,"File %s doesn't exist!",start);acc_notice("Error",msg);continue;}if(!md_text_file(full)){acc_notice("Error","Unsupported file type! Markdown documents only.");continue;}if(load_file(count,full)){if(first<0)first=count;count++;}else{sprintf(msg,"Unable to open:\n%s",full);acc_notice("Open",msg);}}if(first>=0){current=first;cx=cy=top=0;clear_selection();return 1;}return 0;}
static int filename_dialog(char *out)
{int w=50,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=(int)strlen(out),f=0;unsigned mb=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Save Markdown",1);acc_text(x+3,y+2,"Filename:",ACC_LABEL,10);acc_fill(x+13,y+2,32,1,' ',f==0?ACC_SELECT:ACC_CONTROL);acc_text(x+13,y+2,out,f==0?ACC_SELECT:ACC_CONTROL,31);if(f==0)acc_caret_set(x+13+(pos<31?pos:30),y+2);else acc_caret_hide();acc_button(x+15,y+6," Save ",f==1);acc_button(x+25,y+6," Cancel ",f==2);acc_wait(&k,&mx,&my,&mb);if(k==27||((mb&1)&&my==y+6&&mx>=x+25)){acc_modal_end();return 0;}if((mb&1)&&my==y+2){f=0;k=0;}else if((mb&1)&&my==y+6&&mx>=x+15&&mx<x+22){f=1;k=13;}if(k==9||k==271){f=k==271?(f+2)%3:(f+1)%3;k=0;continue;}if(k==13&&f==2){acc_modal_end();return 0;}if(k==13&&f==1){if(pos){force_md(out);acc_modal_end();return 1;}k=0;continue;}if(f==0){if(k==256+71){pos=0;}else if(k==256+79){pos=(int)strlen(out);}else if(k==8&&pos)out[--pos]=0;else if(k>=32&&k<127&&pos<ACC_PATH-4){out[pos++]=(char)k;out[pos]=0;}}k=0;}}
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
static unsigned char far *graph_fonts[GF_COUNT];static unsigned char graph_font_owned[GF_COUNT];static unsigned char graph_font_height[GF_COUNT]={16,16,16,16,16,24,19,16,16,16,16,16};static int graph_role=GF_BODY;
#define GLINK_MAX 48
typedef struct {int x0,y0,x1,y1;char target[ACC_PATH];} GLINK;
static GLINK far *graph_links=0;static int graph_link_count=0;
/* F2 appearance cycles shared by graphical Show and !MDVIEW.  VGA colour
   numbers are deliberate: only explicitly Bright colours use the intensity
   bit.  Scheme 6 is the private !MDVIEW /HELP appearance and is not part of
   the normal F2 cycle. */
typedef struct {unsigned char fg,bg,link;} MDCOLOR;
static const MDCOLOR md_colours[7]={
 {0,7,1},    /* Black on White; Blue links */
 {10,0,10},  /* Bright Green on Black; Bright Green links */
 {15,0,9},   /* Bright White on Black; Bright Blue links */
 {7,1,11},   /* White on Blue; Bright Cyan links */
 {1,7,9},    /* Blue on White; Bright Blue links */
 {0,15,1},   /* Black on Bright White; Blue links */
 {15,3,1}    /* /HELP: Bright White on Cyan; Blue links */
};
static int graph_colour_scheme=0,graph_colour_locked=0;
static int focus_colour_scheme=3;
static int graph_fg(void){return md_colours[graph_colour_scheme].fg;}
static int graph_bg(void){return md_colours[graph_colour_scheme].bg;}
static int graph_link_colour(void){return md_colours[graph_colour_scheme].link;}
static int focus_attr(void){return ACC_ATTR(md_colours[focus_colour_scheme].bg,md_colours[focus_colour_scheme].fg);}
/* Focus is text mode, so disable VGA blink while it is active.  This makes
   attribute bit 7 the background-intensity bit and allows the requested
   Black on Bright White appearance.  Restore ordinary blink semantics on
   return to the Launch! UI. */
static void focus_bright_backgrounds(int enable){union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1003;r.h.bl=(unsigned char)(enable?0:1);r.h.bh=0;int86(0x10,&r,&r);}
static int md_link_is_web(const char *t)
{if(!t)return 0;return !strnicmp(t,"http:",5)||!strnicmp(t,"https:",6)||!strnicmp(t,"ftp:",4)||!strnicmp(t,"mailto:",7)||strstr(t,"://")!=0;}
static int md_link_is_markdown(const char *t)
{char b[ACC_PATH],*hash,*dot,*slash,*other;int n;if(!t||!*t||md_link_is_web(t))return 0;strncpy(b,t,ACC_PATH-1);b[ACC_PATH-1]=0;hash=strchr(b,'#');if(hash)*hash=0;n=(int)strlen(b);while(n&&isspace((unsigned char)b[n-1]))b[--n]=0;if(!n)return 0;slash=strrchr(b,'\\');other=strrchr(b,'/');if(other&&(!slash||other>slash))slash=other;dot=strrchr(b,'.');return dot&&(!slash||dot>slash)&&!stricmp(dot,".MD");}
static int graph_link_supported(const char *t)
{if(!t||!*t)return 0;if(*t=='#')return 1;
#ifdef MDVIEW_BUILD
 return md_link_is_markdown(t);
#else
 return 0;
#endif
}
static void graph_link_add(int x0,int y0,int x1,int y1,const char*t){int n;if(!graph_links||graph_link_count>=GLINK_MAX||!graph_link_supported(t))return;n=(int)strlen(t);if(n>=ACC_PATH)n=ACC_PATH-1;graph_links[graph_link_count].x0=x0;graph_links[graph_link_count].y0=y0;graph_links[graph_link_count].x1=x1;graph_links[graph_link_count].y1=y1;_fmemcpy(graph_links[graph_link_count].target,t,n);graph_links[graph_link_count].target[n]=0;graph_link_count++;}
static void md_slug(const char*s,char*out,int max){int i=0,o=0,dash=0;while(s[i]&&o<max-1){unsigned char c=(unsigned char)s[i++];if(isalnum(c)){out[o++]=(char)tolower(c);dash=0;}else if((c==' '||c=='-'||c=='_')&&o&&!dash){out[o++]='-';dash=1;}}while(o&&out[o-1]=='-')o--;out[o]=0;}
static int graph_find_anchor(int p,const char*target){int r,n,i,h;char b[DOC_COLS+1],slug[64];const char*t=target&&*target=='#'?target+1:target;if(!t||!*t)return -1;for(r=0;r<LINES;r++){md_line(p,r,b,&n);i=md_lead(b,n);h=0;if(i<n&&b[i]=='#'){while(i<n&&b[i]=='#'&&h<6){h++;i++;}while(i<n&&b[i]==' ')i++;md_slug(b+i,slug,sizeof(slug));if(!stricmp(slug,t))return r;}}return -1;}
static unsigned char far *helvfont[4];static unsigned char helvh[4],helvfirst[4],helvlast[4];static int helv_loaded=0;
static void graph_fonts_free(void){int i;for(i=0;i<GF_COUNT;i++){if(graph_fonts[i]&&graph_font_owned[i])_ffree(graph_fonts[i]);graph_fonts[i]=0;graph_font_owned[i]=0;}}
static int graph_fonts_load(void)
{/* H1/H2 remain private embedded !MD resources.  Other Show roles use
    the Launch! FONT.DAT collection: body Serif, code Clean, H3 Extra,
    H4 News, H5 Max Elite, blockquotes Big. */
 static const signed char slot[GF_COUNT]={32,26,5,23,2,-1,-1,34,7,13,3,31};
 char path[ACC_PATH];FILE*f;int i,c;unsigned j;
 memset(graph_fonts,0,sizeof(graph_fonts));memset(graph_font_owned,0,sizeof(graph_font_owned));
 graph_fonts[GF_H1]=(unsigned char far*)md_h1_font;
 graph_fonts[GF_H2]=(unsigned char far*)md_h2_font;
 acc_path(path,"","FONT.DAT");f=fopen(path,"rb");if(!f)return 0;
 for(i=0;i<GF_COUNT;i++){
  if(slot[i]<0)continue;
  graph_fonts[i]=(unsigned char far*)_fmalloc(4096U);graph_font_owned[i]=1;
  if(!graph_fonts[i]||fseek(f,(long)slot[i]*4096L,SEEK_SET)){fclose(f);graph_fonts_free();return 0;}
  for(j=0;j<4096U;j++){c=fgetc(f);if(c==EOF){fclose(f);graph_fonts_free();return 0;}graph_fonts[i][j]=(unsigned char)c;}
 }
 fclose(f);return 1;
}
static unsigned read16(FILE*f){int a=fgetc(f),b=fgetc(f);return(unsigned)(a|(b<<8));}
static unsigned long read32(FILE*f){unsigned long a=(unsigned)fgetc(f),b=(unsigned)fgetc(f),c=(unsigned)fgetc(f),d=(unsigned)fgetc(f);return a|(b<<8)|(c<<16)|(d<<24);}
static void helv_free(void){int i;for(i=0;i<4;i++)if(helvfont[i]){_ffree(helvfont[i]);helvfont[i]=0;}helv_loaded=0;}
static int helv_load(void)
{char name[ACC_PATH],magic[4];FILE*f;unsigned long off[4];unsigned size[4],i,j;int count,c;memset(helvfont,0,sizeof(helvfont));acc_path(name,"","HELVE.BMF");f=fopen(name,"rb");if(!f)return 0;if(fread(magic,1,4,f)!=4||memcmp(magic,"HBF1",4)){fclose(f);return 0;}count=fgetc(f);if(count!=4){fclose(f);return 0;}for(i=0;i<4;i++){helvh[i]=(unsigned char)fgetc(f);(void)fgetc(f);helvfirst[i]=(unsigned char)fgetc(f);helvlast[i]=(unsigned char)fgetc(f);off[i]=read32(f);size[i]=read16(f);if(size[i]<570){fclose(f);helv_free();return 0;}}for(i=0;i<4;i++){helvfont[i]=(unsigned char far*)_fmalloc(size[i]);if(!helvfont[i]||fseek(f,(long)off[i],SEEK_SET)){fclose(f);helv_free();return 0;}for(j=0;j<size[i];j++){c=fgetc(f);if(c==EOF){fclose(f);helv_free();return 0;}helvfont[i][j]=(unsigned char)c;}}fclose(f);helv_loaded=1;return 1;}
static void graph_box(int x0,int y0,int x1,int y1,int colour);
static void graph_frame(int x0,int y0,int x1,int y1,int colour);
static void graph_direct_state(void){outp(0x3CE,0);outp(0x3CF,0);outp(0x3CE,1);outp(0x3CF,0);outp(0x3CE,3);outp(0x3CF,0);outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);}
static void graph_clear(int colour){unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p;graph_direct_state();for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);_fmemset(v,(colour&(1<<p))?255:0,(unsigned)38400L);}outp(0x3C4,2);outp(0x3C5,15);}
static void graph_band(int y0,int y1,int colour){unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p,y;graph_direct_state();for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);for(y=y0;y<=y1;y++)_fmemset(v+y*80,(colour&(1<<p))?255:0,80);}outp(0x3C4,2);outp(0x3C5,15);}
#define GP_L 16
#define GP_R 623
#define GP_T 20
#define GP_B 478
#define GP_TEXT_L 56
#define GP_TEXT_R 584
static void graph_dither(int x0,int y0,int x1,int y1)
{unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p,y,b0,b1;
 if(x0<0)x0=0;if(x1>639)x1=639;if(y0<0)y0=0;if(y1>479)y1=479;
 if((x0&7)==0&&(x1&7)==7){b0=x0>>3;b1=x1>>3;graph_direct_state();for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);for(y=y0;y<=y1;y++)_fmemset(v+y*80+b0,p==0?(unsigned char)((y&1)?0xAA:0x55):0,(unsigned)(b1-b0+1));}outp(0x3C4,2);outp(0x3C5,15);return;}
 graph_box(x0,y0,x1,y1,1);
}
static void graph_page(void)
{/* Keep the neutral grey surround.  The paper has no outline; its only edge
    treatment is an 8-pixel, byte-aligned solid black shadow on the right.
    The aligned planar fill is intentionally used so the shadow costs only a
    few memory fills rather than thousands of BIOS pixel calls. */
 graph_clear(8);
 graph_box(GP_L,GP_T,GP_R,GP_B,graph_bg());
 graph_box(GP_R+1,GP_T+8,GP_R+8,GP_B,0);
}
/* BIOS pixel plotting is deliberately used here.  The MediaGX VGA-compatible
   hardware does not preserve the planar latches expected by the former direct
   write-mode-2 routine, producing coloured vertical fragments.  INT 10h/0Ch
   is slower but adapter-safe and exact for this print-preview workload. */
static void gpixel(int x,int y,int c){union REGS r;if(x<0||x>=640||y<0||y>=480)return;memset(&r,0,sizeof(r));r.h.ah=0x0C;r.h.al=(unsigned char)c;r.h.bh=0;r.x.cx=(unsigned)x;r.x.dx=(unsigned)y;int86(0x10,&r,&r);}
static void graph_box(int x0,int y0,int x1,int y1,int colour)
{unsigned char far*v=(unsigned char far *)((unsigned long)0xA000<<16);int p,y,b0,b1;if(x0<0)x0=0;if(x1>639)x1=639;if(y0<0)y0=0;if(y1>479)y1=479;if(x0>x1||y0>y1)return;/* The fenced-code background is byte aligned (24..615).  Fill it directly
   in planar mode instead of issuing an INT 10h pixel call for every pixel.  BIOS pixel plotting leaves VGA Set/Reset state behind, so restore all write-mode registers before every direct fill. */if((x0&7)==0&&(x1&7)==7){b0=x0>>3;b1=x1>>3;graph_direct_state();for(p=0;p<4;p++){outp(0x3C4,2);outp(0x3C5,1<<p);for(y=y0;y<=y1;y++)_fmemset(v+y*80+b0,(colour&(1<<p))?255:0,(unsigned)(b1-b0+1));}outp(0x3C4,2);outp(0x3C5,15);return;}for(y=y0;y<=y1;y++){int x;for(x=x0;x<=x1;x++)gpixel(x,y,colour);}}
static int gchar_width(int scale){return 8*scale;}
static void gchar(int x,int y,unsigned char ch,int scale,int colour,int style){int r,b,sx,sy,slant=0,synthetic=0,fh=graph_font_height[graph_role];unsigned char bits;unsigned char far*font=graph_fonts[graph_role]?graph_fonts[graph_role]:graph_fonts[GF_CODE];if(graph_role==GF_BODY){if((style&1)&&(style&2)&&graph_fonts[GF_BOLDITALIC]){font=graph_fonts[GF_BOLDITALIC];fh=graph_font_height[GF_BOLDITALIC];}else if((style&1)&&graph_fonts[GF_BOLD]){font=graph_fonts[GF_BOLD];fh=graph_font_height[GF_BOLD];}else if((style&2)&&graph_fonts[GF_ITALIC]){font=graph_fonts[GF_ITALIC];fh=graph_font_height[GF_ITALIC];}else{synthetic=(style&1)!=0;slant=(style&2)!=0;}}for(r=0;r<fh;r++){bits=font[(unsigned)ch*fh+r];for(b=0;b<8;b++)if(bits&(0x80>>b))for(sy=0;sy<scale;sy++)for(sx=0;sx<scale;sx++){int skew=slant?(fh-1-r)/5:0;gpixel(x+(b+skew)*scale+sx,y+r*scale+sy,colour);if(synthetic)gpixel(x+(b+skew)*scale+sx+1,y+r*scale+sy,colour);}}if(style&4)for(b=0;b<8*scale;b++)gpixel(x+b,y+(fh-2)*scale,colour);if(style&8)for(b=0;b<8*scale;b++)gpixel(x+b,y+(fh/2)*scale,colour);}
static void gtext(int x,int y,const char*s,int scale,int colour){int w=gchar_width(scale);while(*s){gchar(x,y,(unsigned char)*s++,scale,colour,0);x+=w;}}
static int hwidth(int strike,unsigned char ch){unsigned char far*p;if(!helv_loaded||ch<helvfirst[strike]||ch>helvlast[strike])return 8;p=helvfont[strike]+(unsigned)(ch-helvfirst[strike])*6;return p[0];}
static void hchar(int x,int y,unsigned char ch,int strike,int colour,int style)
{unsigned char far *p;unsigned char far *bits;unsigned off;int advance,width,height,rowbytes,r,b,slant=0;if(!helv_loaded||ch<helvfirst[strike]||ch>helvlast[strike]){gchar(x,y,ch,1,colour,style);return;}p=helvfont[strike]+(unsigned)(ch-helvfirst[strike])*6;advance=p[0];width=p[1];height=p[2];off=(unsigned)(p[4]|((unsigned)p[5]<<8));bits=helvfont[strike]+off;rowbytes=(width+7)/8;for(r=0;r<height;r++){slant=(style&2)?(height-1-r)/6:0;for(b=0;b<width;b++)if(bits[r*rowbytes+b/8]&(0x80>>(b&7))){gpixel(x+b+slant,y+r,colour);if(style&1)gpixel(x+b+slant+1,y+r,colour);}}if(style&4)for(b=0;b<advance;b++)gpixel(x+b,y+height-2,colour);if(style&8)for(b=0;b<advance;b++)gpixel(x+b,y+height/2,colour);}
static int doc_width(unsigned char ch,int strike,int scale){(void)ch;(void)strike;return gchar_width(scale);}
static void doc_char(int x,int y,unsigned char ch,int strike,int scale,int colour,int style){(void)strike;gchar(x,y,ch,scale,colour,style);}
static int graph_word_width(const char*s,int i,int n,int strike,int scale){int w=0;while(i<n&&s[i]!=' '){w+=doc_width((unsigned char)s[i],strike,scale);i++;}return w;}
static int video_is_vga(void){union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);return r.h.al==0x1A;}
static void graph_rule(int y,int kind){int x,c=graph_fg();if(kind==3||kind==4){for(x=GP_TEXT_L;x<GP_TEXT_R;x+=(kind==3?12:8)){int e=x+(kind==3?6:1),i;if(e>GP_TEXT_R)e=GP_TEXT_R;for(i=x;i<e;i++)gpixel(i,y,c);}}else{for(x=GP_TEXT_L;x<GP_TEXT_R;x++)gpixel(x,y,c);if(kind==2)for(x=GP_TEXT_L;x<GP_TEXT_R;x++)gpixel(x,y+2,c);}}
static void graph_frame(int x0,int y0,int x1,int y1,int colour)
{int x,y;for(x=x0;x<=x1;x++){gpixel(x,y0,colour);gpixel(x,y1,colour);}for(y=y0;y<=y1;y++){gpixel(x0,y,colour);gpixel(x1,y,colour);}}
static int graph_media_placeholder(const MDMEDIA*m,int ypos)
{char d[64];int i,x=GP_TEXT_L+8;md_media_desc(m,d,sizeof(d));graph_frame(GP_TEXT_L,ypos-3,GP_TEXT_R,ypos+20,4);graph_role=GF_BODY;for(i=0;d[i]&&x+8<GP_TEXT_R-8;i++){gchar(x,ypos,(unsigned char)d[i],1,4,0);x+=8;}return 28;}
static int graph_quote_line(int p,int row,int *skipmark)
{char b[DOC_COLS+1];int n,i,r;if(skipmark)*skipmark=0;md_line(p,row,b,&n);i=md_lead(b,n);if(i<n&&b[i]=='>'){if(skipmark)*skipmark=1;return 1;}if(n<=i||row<=0)return 0;/* Markdown permits lazy continuation lines inside a blockquote paragraph.  Walk back to the nearest blank or explicit quote marker. */for(r=row-1;r>=0;r--){md_line(p,r,b,&n);i=md_lead(b,n);if(n<=i)return 0;if(i<n&&b[i]=='>')return 1;if(i<n&&(b[i]=='#'||(b[i]=='`'&&i+2<n&&b[i+1]=='`'&&b[i+2]=='`')))return 0;}return 0;}
static int graph_rows_consumed=1;
static int graph_plain_candidate(int p,int row)
{
 char b[DOC_COLS+1];int n,i,after=0,ordered=0,dp=0;MDTABLE tt;MDMEDIA mm;
 if(row<0||row>=LINES)return 0;md_line(p,row,b,&n);i=md_lead(b,n);if(i>=n)return 0;
 if(i+2<n&&b[i]=='`'&&b[i+1]=='`'&&b[i+2]=='`')return 0;
 if(b[i]=='#'||b[i]=='>'||md_list_marker(b,n,i,&after,&ordered))return 0;
 if(i+2<n&&((b[i]=='-'&&b[i+1]=='-'&&b[i+2]=='-')||(b[i]=='*'&&b[i+1]=='*'&&b[i+2]=='*')))return 0;
 if(md_table_context(p,row,&tt,&dp)||md_media_parse(b,n,&mm)||md_indented_code(p,row)||md_definition_term(p,row)||md_definition_line(p,row,&dp))return 0;
 return 1;
}
static int graph_line(int p,int row,int ypos,int in_fence)
{
 static char s[1024];char tb[DOC_COLS+1],jb[DOC_COLS+1];MDTABLE tt;MDMEDIA media;int n,i=0,x=GP_TEXT_L,h=0,scale=1,hstrike=1,height=16,ystart=ypos;
 int bold=0,italic=0,underline=0,strike=0,super=0,sub=0,quote=0,inline_code=0,k,style,w,base_x,lines=1,wordw,base_role=GF_BODY;
 int lead,after=0,ordered=0,defpos=0,tctx,tborder=0,indent_code,block_code,qskip=0;
 md_line(p,row,s,&n);graph_rows_consumed=1;
 if(!in_fence&&graph_plain_candidate(p,row)){
  int rr=row+1,jn,ji,sl=n;
  while(rr<LINES&&graph_plain_candidate(p,rr)&&sl<(int)sizeof(s)-2){
   if(sl>=2&&s[sl-1]==' '&&s[sl-2]==' ')break;
   md_line(p,rr,jb,&jn);ji=md_lead(jb,jn);
   if(sl&&s[sl-1]!=' ')s[sl++]=' ';
   while(ji<jn&&sl<(int)sizeof(s)-1)s[sl++]=jb[ji++];
   s[sl]=0;n=sl;graph_rows_consumed++;rr++;
  }
 }
 lead=md_lead(s,n);if(in_fence){i=0;graph_role=GF_CODE;graph_box(GP_TEXT_L-8,ypos-2,GP_TEXT_R+7,ypos+height+2,graph_fg());while(i<n&&x+gchar_width(scale)<GP_TEXT_R){gchar(x,ypos,(unsigned char)s[i++],scale,graph_bg(),0);x+=gchar_width(scale);}return height+5;}if(md_media_parse(s,n,&media))return graph_media_placeholder(&media,ypos);tctx=md_table_context(p,row,&tt,&tborder);if(tctx){n=md_table_make_row(p,row,&tt,tctx==2?tborder:0,tb);graph_role=GF_CODE;x=GP_TEXT_L;for(i=0;i<n&&x+8<GP_TEXT_R;i++){gchar(x,ypos,(unsigned char)tb[i],1,graph_fg(),0);x+=8;}return height;}indent_code=md_indented_code(p,row);block_code=indent_code;
 if(block_code){
  i=4;graph_role=GF_CODE;graph_box(GP_TEXT_L-8,ypos-2,GP_TEXT_R+7,ypos+height+2,graph_fg());
  while(i<n&&x+gchar_width(scale)<GP_TEXT_R){gchar(x,ypos,(unsigned char)s[i++],scale,graph_bg(),0);x+=gchar_width(scale);}return height+5;
 }
 if(i+2<n&&s[i]=='['&&s[i+1]=='^'){base_role=GF_TINY;x=GP_TEXT_L;}
 if(md_definition_term(p,row)){base_role=GF_BOLD;i=lead;x=GP_TEXT_L+lead*8;}
 else if(md_definition_line(p,row,&defpos)){base_role=GF_BODY;i=defpos;x=GP_TEXT_L+32;}
 else {
  i=lead;x=GP_TEXT_L+lead*8;
  if(lead<=3&&i<n&&s[i]=='#'){while(i<n&&s[i]=='#'&&h<6){h++;i++;}while(i<n&&s[i]==' ')i++;base_role=h==1?GF_H1:(h==2?GF_H2:(h==3?GF_H3:(h==4?GF_H4:(h==5?GF_H5:GF_BOLD))));height=graph_font_height[base_role];x=GP_TEXT_L;}
  else if(md_list_marker(s,n,i,&after,&ordered)){
   if(!ordered){graph_role=GF_BODY;gchar(x,ypos,7,1,graph_fg(),0);x+=16;i=after;}
   else {graph_role=GF_BODY;while(i<after-1&&x+8<GP_TEXT_R){gchar(x,ypos,(unsigned char)s[i++],1,graph_fg(),0);x+=8;}x+=8;i=after;}
  }
  else if(graph_quote_line(p,row,&qskip)){quote=1;base_role=GF_QUOTE;for(k=0;k<height;k++)gpixel(GP_TEXT_L,ypos+k,graph_fg());x=GP_TEXT_L+12;if(qskip){i++;while(i<n&&s[i]==' ')i++;}}
 }
 base_x=x;
 if(!h&&lead<=3&&i+3<=n&&((s[i]=='-'&&s[i+1]=='-'&&s[i+2]=='-')||(s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*'))){graph_rule(ypos+height/2,3);return height+4;}
 while(i<n){
  if(inline_code){
   if(s[i]=='`'){inline_code=0;i++;continue;}w=8;if(x+w>=GP_TEXT_R){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(GP_TEXT_L,ypos+k,graph_fg());}graph_box(x,ypos-1,x+7,ypos+height,graph_fg());graph_role=GF_CODE;gchar(x,ypos,(unsigned char)s[i++],1,graph_bg(),0);x+=8;continue;
  }
  if(s[i]=='`'){
   int e=i+1,j,chunk,fit;while(e<n&&s[e]!='`')e++;
   if(e<n){i++;while(i<e){fit=(GP_TEXT_R-x)/8;if(fit<=0){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(GP_TEXT_L,ypos+k,graph_fg());fit=(GP_TEXT_R-x)/8;}chunk=e-i;if(chunk>fit)chunk=fit;if(chunk<=0)break;/* Paint the complete inverse span first, then draw all glyphs.  Per-character background fills could erase neighbouring ProFont pixels on real VGA adapters. */graph_box(x,ypos-1,x+chunk*8-1,ypos+height,graph_fg());graph_role=GF_CODE;for(j=0;j<chunk;j++)gchar(x+j*8,ypos,(unsigned char)s[i+j],1,graph_bg(),0);x+=chunk*8;i+=chunk;if(i<e){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(GP_TEXT_L,ypos+k,graph_fg());}}i=e+1;continue;}inline_code=1;i++;continue;
  }
  if(s[i]=='\\'&&i+1<n){i++;w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=GP_TEXT_R){ypos+=height+4;x=base_x;lines++;}graph_role=base_role;doc_char(x,ypos,(unsigned char)s[i++],hstrike,scale,graph_fg(),0);x+=w;continue;}
  if(s[i]!=' '&&(i==0||s[i-1]==' ')){wordw=graph_word_width(s,i,n,hstrike,scale);if(x>base_x&&x+wordw>=GP_TEXT_R){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(GP_TEXT_L,ypos+k,graph_fg());}}
  w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=GP_TEXT_R){ypos+=height+4;x=base_x;lines++;if(quote)for(k=0;k<height;k++)gpixel(GP_TEXT_L,ypos+k,graph_fg());}
  if(i+3<=n&&!strnicmp(s+i,"<u>",3)){underline=1;i+=3;continue;}if(i+4<=n&&!strnicmp(s+i,"</u>",4)){underline=0;i+=4;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<del>",5)){strike=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</del>",6)){strike=0;i+=6;continue;}
  if(i+2<=n&&s[i]=='~'&&s[i+1]=='~'){strike=!strike;i+=2;continue;}
  if(i+3<=n&&((s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*')||(s[i]=='_'&&s[i+1]=='_'&&s[i+2]=='_'))){bold=!bold;italic=!italic;i+=3;continue;}
  if(i+2<=n&&s[i]=='*'&&s[i+1]=='*'){bold=!bold;i+=2;continue;}
  if(i+2<=n&&s[i]=='_'&&s[i+1]=='_'){italic=!italic;i+=2;continue;}
  if(s[i]=='*'||s[i]=='_'){italic=!italic;i++;continue;}
  if(i+2<=n&&s[i]=='{'&&s[i+1]=='{'){int e=i+2,ub=bold,ui=italic;while(e+1<n&&!(s[e]=='}'&&s[e+1]=='}'))e++;if(e+1<n){i+=2;while(i<e){if(i+3<=e&&((s[i]=='*'&&s[i+1]=='*'&&s[i+2]=='*')||(s[i]=='_'&&s[i+1]=='_'&&s[i+2]=='_'))){ub=!ub;ui=!ui;i+=3;continue;}if(i+2<=e&&s[i]=='*'&&s[i+1]=='*'){ub=!ub;i+=2;continue;}if(i+2<=e&&s[i]=='_'&&s[i+1]=='_'){ub=!ub;i+=2;continue;}if(s[i]=='*'||s[i]=='_'){ui=!ui;i++;continue;}w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=GP_TEXT_R){ypos+=height+4;x=base_x;lines++;}style=(ub?1:0)|(ui?2:0)|4|(strike?8:0);graph_role=base_role;doc_char(x,ypos,(unsigned char)s[i++],hstrike,scale,graph_fg(),style);x+=w;}i=e+2;continue;}underline=1;i+=2;continue;}if(i+2<=n&&s[i]=='}'&&s[i+1]=='}'){underline=0;i+=2;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<sup>",5)){super=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</sup>",6)){super=0;i+=6;continue;}
  if(i+5<=n&&!strnicmp(s+i,"<sub>",5)){sub=1;i+=5;continue;}if(i+6<=n&&!strnicmp(s+i,"</sub>",6)){sub=0;i+=6;continue;}
  if(i+3<n&&s[i]=='['&&s[i+1]=='^'){k=i+2;while(k<n&&s[k]!=']')k++;if(k<n){graph_role=GF_TINY;while(i<=k){gchar(x,ypos-3,(unsigned char)s[i++],1,graph_fg(),0);x+=8;}continue;}}
  if(s[i]=='['){k=i+1;while(k<n&&s[k]!=']')k++;if(k<n&&k+1<n&&s[k+1]=='('){int lx=x,ts=k+2,te=ts;char targ[ACC_PATH];while(te<n&&s[te]!=')'&&!isspace((unsigned char)s[te]))te++;i++;while(i<k){w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=GP_TEXT_R)break;style=(bold?1:0)|(italic?2:0)|4|(strike?8:0);graph_role=base_role;doc_char(x,ypos,(unsigned char)s[i++],hstrike,scale,graph_link_colour(),style);x+=w;}if(te-ts>0){int tl=te-ts;if(tl>=ACC_PATH)tl=ACC_PATH-1;memcpy(targ,s+ts,tl);targ[tl]=0;graph_link_add(lx,ypos,x-1,ypos+height,targ);}i=k+2;while(i<n&&s[i]!=')')i++;if(i<n)i++;continue;}}
  if(s[i]=='<'&&i+7<n&&!strnicmp(s+i+1,"http",4)){k=i+1;while(k<n&&s[k]!='>')k++;if(k<n){i++;while(i<k){w=doc_width((unsigned char)s[i],hstrike,scale);if(x+w>=GP_TEXT_R){ypos+=height+4;x=base_x;lines++;}style=(bold?1:0)|(italic?2:0)|4|(strike?8:0);graph_role=base_role;doc_char(x,ypos,(unsigned char)s[i++],hstrike,scale,graph_link_colour(),style);x+=w;}i++;continue;}}
  style=(bold?1:0)|(italic?2:0)|(underline?4:0)|(strike?8:0);graph_role=(super||sub)?GF_TINY:base_role;doc_char(x,ypos+(super?-4:(sub?4:0)),(unsigned char)s[i++],hstrike,scale,graph_fg(),style);x+=w;
 }
 if(quote)for(k=0;k<lines*height+(lines-1)*4+5;k++)gpixel(GP_TEXT_L,ystart+k,graph_fg());
 if(h==1){graph_rule(ypos+height+1,2);return lines*height+(lines-1)*4+8;}
 if(h==2){graph_rule(ypos+height+1,1);return lines*height+(lines-1)*4+6;}
 if(h==3){graph_rule(ypos+height+1,3);return lines*height+(lines-1)*4+6;}
 if(h==4){graph_rule(ypos+height+1,4);return lines*height+(lines-1)*4+6;}
 return lines*height+(lines-1)*4+(quote?5:4);
}
static int graph_last_row(int p){int r,n;char b[DOC_COLS+1];for(r=LINES-1;r>=0;r--){md_line(p,r,b,&n);if(n>0)return r;}return 0;}
static void graph_page_indicator(int first,int endrow)
{char label[24];int total=(endrow+20)/20,page=first/20+1,n,x,bg,c; if(total<=1)return;if(page>total)page=total;sprintf(label,"%d/%d",page,total);n=(int)strlen(label);x=GP_R-8-n*8+1;if(x<GP_TEXT_L)x=GP_TEXT_L;bg=graph_bg();c=bg==15?7:(bg<8?bg+8:bg);graph_role=GF_TINY;gtext(x,GP_B-16,label,1,c);}
static void graph_render(int p,int first)
{int row,ypos=GP_T+12,last=first+23,fence=code_fence_before(p,first),fk,endrow=graph_last_row(p);if(last>LINES)last=LINES;graph_link_count=0;graph_page();for(row=first;row<last&&ypos<GP_B-14;row++){fk=line_fence_kind(p,row);if(fk){if(!fence)fence=fk;else if(fence==fk)fence=0;ypos+=3;continue;}ypos+=graph_line(p,row,ypos,fence!=0);if(graph_rows_consumed>1)row+=graph_rows_consumed-1;if(row>=endrow){if(ypos<GP_B){graph_box(0,ypos,GP_L-1,479,8);graph_box(GP_L,ypos,GP_R,479,graph_bg());graph_box(GP_R+9,ypos,639,479,8);}break;}}graph_page_indicator(first,endrow);}
static void graph_redraw(int p,int first)
{union REGS r;if(acc_mouse_present){memset(&r,0,sizeof(r));r.x.ax=2;int86(0x33,&r,&r);}graph_render(p,first);if(acc_mouse_present){memset(&r,0,sizeof(r));r.x.ax=1;int86(0x33,&r,&r);}}
#ifdef MDVIEW_BUILD
static void mdview_dirname(const char *path,char *dir)
{const char *a=strrchr(path,'\\'),*b=strrchr(path,'/'),*q=a;if(b&&(!q||b>q))q=b;if(q){int n=(int)(q-path);if(n>=ACC_PATH)n=ACC_PATH-1;memcpy(dir,path,n);dir[n]=0;}else dir[0]=0;}
static void mdview_resolve_link(const char *base,const char *target,char *out,char *anchor)
{char work[ACC_PATH],dir[ACC_PATH],*hash;int i;strncpy(work,target,ACC_PATH-1);work[ACC_PATH-1]=0;hash=strchr(work,'#');anchor[0]=0;if(hash){strncpy(anchor,hash,63);anchor[63]=0;*hash=0;}for(i=0;work[i];i++)if(work[i]=='/')work[i]='\\';if(strchr(work,':')||work[0]=='\\'){strncpy(out,work,ACC_PATH-1);out[ACC_PATH-1]=0;return;}mdview_dirname(base,dir);if(dir[0])sprintf(out,"%s\\%s",dir,work);else{strncpy(out,work,ACC_PATH-1);out[ACC_PATH-1]=0;}}
static int mdview_follow_link(int *p,int *first,const char *target)
{char next[ACC_PATH],anchor[64];int r;if(!target||!*target||md_link_is_web(target))return 0;if(target[0]=='#'){r=graph_find_anchor(*p,target);if(r>=0){*first=r;return 1;}return 0;}if(!md_link_is_markdown(target))return 0;mdview_resolve_link(paths[*p],target,next,anchor);if(!next[0]||!md_text_file(next)||!load_file(0,next))return 0;*p=0;current=0;count=1;*first=0;if(anchor[0]){r=graph_find_anchor(0,anchor);if(r>=0)*first=r;}return 1;}
#endif
static void print_current(void);
static int print_silent=0;
static void graph_show(void)
{
 union REGS r;unsigned key;int first=0,p=current,scan,mx,my,mb,lastmb=0,i,tr,endrow;
 char linktarget[ACC_PATH];
 if(!video_is_vga()){acc_notice("Show","VGA display adapter required.");return;}
 graph_links=(GLINK far *)_fmalloc((unsigned)(GLINK_MAX*sizeof(GLINK)));
 if(!graph_links){acc_notice("Show","Not enough memory for graphical view.");return;}
 if(!graph_fonts_load()){_ffree(graph_links);graph_links=0;acc_notice("Show","Unable to read FONT.DAT.");return;}
 acc_mouse_display(0);memset(&r,0,sizeof(r));r.h.ah=0;r.h.al=0x12;int86(0x10,&r,&r);graph_render(p,first);
 if(acc_mouse_present){memset(&r,0,sizeof(r));r.x.ax=1;int86(0x33,&r,&r);}
 for(;;){
  key=0;if(_bios_keybrd(_KEYBRD_READY))key=_bios_keybrd(_KEYBRD_READ);
  if(key){
   if((key&255)==16){
    /* Show/Viewer has no print UI: Ctrl+P always means formatted ESC/P.
       Return to text mode before the printer path, because the accessory
       dialogs and LPT1 path must never touch the VGA surface. */
    if(acc_mouse_present)acc_mouse_display(0);graph_fonts_free();
    if(graph_links){_ffree(graph_links);graph_links=0;}
    acc_restore_screen();
    print_silent=1;print_current();print_silent=0;
    memset(&r,0,sizeof(r));r.h.ah=0;r.h.al=0x12;int86(0x10,&r,&r);
    graph_links=(GLINK far *)_fmalloc((unsigned)(GLINK_MAX*sizeof(GLINK)));
    if(!graph_links||!graph_fonts_load()){if(graph_links){_ffree(graph_links);graph_links=0;}break;}
    graph_render(p,first);if(acc_mouse_present)acc_mouse_display(1);continue;}
   if((key&255)==27)break;scan=(key>>8)&255;
   if(scan==0x3C&&!graph_colour_locked){graph_colour_scheme=(graph_colour_scheme+1)%6;graph_redraw(p,first);continue;}
   if(scan==0x47){first=0;}
   else if(scan==0x4F){endrow=graph_last_row(p);first=endrow-21;if(first<0)first=0;}
   else if(scan==0x48||scan==0x49){first-=20;if(first<0)first=0;}
   else if(scan==0x50||scan==0x51){first+=20;endrow=graph_last_row(p);if(first>endrow)first=endrow;if(first<0)first=0;}
#ifndef MDVIEW_BUILD
   else if(scan==0x4B&&p>0){p--;first=0;}
   else if(scan==0x4D&&p+1<count){p++;first=0;}
#endif
   else continue;
   graph_redraw(p,first);continue;
  }
  if(acc_mouse_present){
   memset(&r,0,sizeof(r));r.x.ax=3;int86(0x33,&r,&r);mx=r.x.cx;my=r.x.dx;mb=r.x.bx;
   if((mb&1)&&!(lastmb&1)){
    for(i=0;i<graph_link_count;i++)if(mx>=graph_links[i].x0&&mx<=graph_links[i].x1&&my>=graph_links[i].y0&&my<=graph_links[i].y1){
     _fmemcpy(linktarget,graph_links[i].target,ACC_PATH);linktarget[ACC_PATH-1]=0;
#ifdef MDVIEW_BUILD
     if(mdview_follow_link(&p,&first,linktarget))graph_redraw(p,first);
#else
     tr=graph_find_anchor(p,linktarget);if(tr>=0){first=tr;graph_redraw(p,first);}
#endif
     break;
    }
   }
   lastmb=mb;
  }
 }
 if(acc_mouse_present){memset(&r,0,sizeof(r));r.x.ax=2;int86(0x33,&r,&r);}
 graph_fonts_free();_ffree(graph_links);graph_links=0;
 outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);
 acc_restore_screen();acc_mouse_reapply_cursor();acc_mouse_display(1);
}



#pragma code_seg("MDPRINT_TEXT")

/* Epson ESC/P text printing.  Printing deliberately uses printer-resident
   typefaces rather than raster graphics: the Markdown syntax is removed and
   represented by the printer's Roman/Sans/Courier faces and attributes. */
#define PR_ROMAN 0
#define PR_SANS 1
#define PR_COURIER 2
#define PR_PRESTIGE 3
#define PR_SCRIPT 4
#define PR_DRAFT 5
#define PR_DRAFT_COND 6
typedef struct {int font,bold,italic,underline,doublew,script,draft,condensed;} PRSTATE;
static void pr_esc(FILE*f,int cmd){fputc(27,f);fputc(cmd,f);}
static void pr_esc_n(FILE*f,int cmd,int n){fputc(27,f);fputc(cmd,f);fputc(n,f);}
static void pr_init_state(PRSTATE*s){s->font=s->bold=s->italic=s->underline=s->doublew=s->script=s->draft=s->condensed=-1;}
static void pr_emit_style(FILE*f,PRSTATE*s,int font,int bold,int italic,int underline,int doublew,int script)
{
 if(s->script!=script){if(script<0)pr_esc(f,'T');else pr_esc_n(f,'S',script?1:0);s->script=script;}
 if(s->font!=font){int wantdraft=(font==PR_DRAFT||font==PR_DRAFT_COND),wantcond=(font==PR_DRAFT_COND);if(s->draft!=wantdraft){pr_esc_n(f,'x',wantdraft?0:1);s->draft=wantdraft;}if(s->condensed!=wantcond){if(wantcond)pr_esc(f,15);else pr_esc(f,18);s->condensed=wantcond;}if(font<=PR_SCRIPT)pr_esc_n(f,'k',font);s->font=font;}
 if(s->bold!=bold){pr_esc(f,bold?'E':'F');s->bold=bold;}
 if(s->italic!=italic){pr_esc(f,italic?'4':'5');s->italic=italic;}
 if(s->underline!=underline){pr_esc_n(f,'-',underline?1:0);s->underline=underline;}
 if(s->doublew!=doublew){pr_esc_n(f,'W',doublew?1:0);s->doublew=doublew;}
}
/* Buffer styled cells, then wrap at a word boundary within 72 Pica columns.
   Styles travel with each cell, so a wrap cannot apply the next word's style
   to the previous line. Three-column margin and 60 text rows per sheet. */
#define PR_WIDTH 72
static unsigned char pr_cells[PR_WIDTH+1];
static PRSTATE pr_attrs[PR_WIDTH+1],pr_actual;
static int pr_count=0,pr_rows=0,pr_quote_mode=0,pr_media_mode=0;
static void pr_style(FILE*f,PRSTATE*s,int font,int bold,int italic,int underline,int doublew,int script)
{(void)f;(void)doublew;s->font=font;s->bold=bold;s->italic=italic;s->underline=underline;s->doublew=0;s->script=script;}
static void pr_flush(FILE*f,int count,int drop)
{
 int i;PRSTATE *a;
 if(pr_rows>=60){fputc('\f',f);pr_rows=0;}
 pr_emit_style(f,&pr_actual,pr_media_mode?PR_DRAFT_COND:PR_PRESTIGE,0,0,0,0,-1);fputs("   ",f);
 if(pr_quote_mode)pr_emit_style(f,&pr_actual,PR_SCRIPT,0,1,0,0,-1);
 for(i=0;i<count;i++){a=&pr_attrs[i];pr_emit_style(f,&pr_actual,a->font,a->bold,a->italic,a->underline,0,a->script);fputc(pr_cells[i],f);}
 pr_emit_style(f,&pr_actual,PR_PRESTIGE,0,0,0,0,-1);fputs("\r\n",f);pr_rows++;
 pr_count-=count+drop;if(pr_count>0){memmove(pr_cells,pr_cells+count+drop,pr_count);memmove(pr_attrs,pr_attrs+count+drop,pr_count*sizeof(PRSTATE));}
}
static void pr_char(FILE*f,PRSTATE*s,int c)
{
 int cut;
 /* Keep CP437 box-drawing bytes intact for table and rule output. */
 if(c<32)c=' ';
 if(pr_count==PR_WIDTH){cut=PR_WIDTH;while(cut>0&&pr_cells[cut-1]!=' ')cut--;if(cut>0)pr_flush(f,cut-1,1);else pr_flush(f,PR_WIDTH,0);}
 pr_cells[pr_count]=(unsigned char)c;pr_attrs[pr_count++]=*s;
}
static void pr_normal(FILE*f,PRSTATE*s){pr_style(f,s,PR_PRESTIGE,0,0,0,0,-1);}
static void pr_crlf(FILE*f){pr_flush(f,pr_count,0);}
static void pr_rule(FILE*f,PRSTATE*s,int ch,int count)
{int i;pr_style(f,s,PR_SANS,0,0,0,0,-1);for(i=0;i<count;i++)pr_char(f,s,ch);pr_crlf(f);}
static void pr_inline(FILE*f,PRSTATE*s,const char*text,int n,int basefont,int basebold,int baseitalic,int baseunderline,int basedouble)
{
 int i=0,k,bold=basebold,italic=baseitalic,underline=baseunderline,code=0,super=0,sub=0,strike=0;char c;
 while(i<n){
  if(code){if(text[i]=='`'){code=0;i++;continue;}pr_style(f,s,PR_DRAFT,0,0,0,0,-1);pr_char(f,s,(unsigned char)text[i++]);continue;}
  if(text[i]=='\\'&&i+1<n){i++;pr_style(f,s,basefont,bold,italic,underline,0,-1);pr_char(f,s,(unsigned char)text[i++]);continue;}

  if(i+3<=n&&!strnicmp(text+i,"<u>",3)){underline=1;i+=3;continue;}
  if(i+4<=n&&!strnicmp(text+i,"</u>",4)){underline=baseunderline;i+=4;continue;}
  if(i+5<=n&&!strnicmp(text+i,"<del>",5)){strike=1;i+=5;continue;}
  if(i+6<=n&&!strnicmp(text+i,"</del>",6)){strike=0;i+=6;continue;}
  if(i+2<=n&&text[i]=='~'&&text[i+1]=='~'){strike=!strike;i+=2;continue;}
  if(i+3<=n&&((text[i]=='*'&&text[i+1]=='*'&&text[i+2]=='*')||(text[i]=='_'&&text[i+1]=='_'&&text[i+2]=='_'))){bold=!bold;italic=!italic;i+=3;continue;}
  if(i+2<=n&&text[i]=='*'&&text[i+1]=='*'){bold=!bold;i+=2;continue;}
  if(i+2<=n&&text[i]=='_'&&text[i+1]=='_'){bold=!bold;i+=2;continue;}
  if(text[i]=='*'||text[i]=='_'){italic=!italic;i++;continue;}
  if(i+2<=n&&text[i]=='{'&&text[i+1]=='{'){underline=1;i+=2;continue;}
  if(i+2<=n&&text[i]=='}'&&text[i+1]=='}'){underline=baseunderline;i+=2;continue;}
  if(i+5<=n&&!strnicmp(text+i,"<sup>",5)){super=1;i+=5;continue;}
  if(i+6<=n&&!strnicmp(text+i,"</sup>",6)){super=0;i+=6;continue;}
  if(i+5<=n&&!strnicmp(text+i,"<sub>",5)){sub=1;i+=5;continue;}
  if(i+6<=n&&!strnicmp(text+i,"</sub>",6)){sub=0;i+=6;continue;}
  if(text[i]=='`'){code=!code;i++;continue;}
  if(text[i]=='['){
   if(i+2<n&&text[i+1]=='^'){
    k=i+2;while(k<n&&text[k]!=']')k++;if(k<n){pr_style(f,s,PR_PRESTIGE,0,0,0,0,0);while(i<=k)pr_char(f,s,(unsigned char)text[i++]);continue;}
   }
   k=i+1;while(k<n&&text[k]!=']')k++;if(k<n&&k+1<n&&text[k+1]=='('){
    i++;while(i<k){pr_style(f,s,basefont,bold,italic,1,basedouble,-1);pr_char(f,s,(unsigned char)text[i++]);}i=k+2;while(i<n&&text[i]!=')')i++;if(i<n)i++;continue;
   }
  }
  if(text[i]=='<'&&i+7<n&&!strnicmp(text+i+1,"http",4)){
   k=i+1;while(k<n&&text[k]!='>')k++;if(k<n){i++;while(i<k){pr_style(f,s,basefont,bold,italic,1,basedouble,-1);pr_char(f,s,(unsigned char)text[i++]);}i++;continue;}
  }
  c=text[i++];
  pr_style(f,s,code?PR_DRAFT:basefont,bold,italic,underline,basedouble,super?0:(sub?1:-1));
  pr_char(f,s,(unsigned char)c);
  (void)strike; /* ESC/P LQ-100 has no universally safe strike-through toggle. */
 }
 pr_style(f,s,basefont,basebold,baseitalic,baseunderline,basedouble,-1);
}
static void pr_code_line(FILE*f,PRSTATE*s,const char*text,int n,int strip)
{int i=strip;if(i>n)i=n;pr_style(f,s,PR_DRAFT,0,0,0,0,-1);for(;i<n;i++)pr_char(f,s,(unsigned char)text[i]);pr_crlf(f);}
static void pr_media_box(FILE*f,PRSTATE*s,const MDMEDIA*m)
{char d[COLS-3];int i,n,w=PR_WIDTH-2;md_media_desc(m,d,sizeof(d));n=(int)strlen(d);if(n>w-2)n=w-2;
 pr_media_mode=1;pr_style(f,s,PR_DRAFT_COND,0,0,0,0,-1);pr_char(f,s,218);for(i=0;i<w;i++)pr_char(f,s,196);pr_char(f,s,191);pr_crlf(f);
 pr_style(f,s,PR_DRAFT_COND,0,0,0,0,-1);pr_char(f,s,179);pr_char(f,s,' ');for(i=0;i<n;i++)pr_char(f,s,(unsigned char)d[i]);for(;i<w-2;i++)pr_char(f,s,' ');pr_char(f,s,' ');pr_char(f,s,179);pr_crlf(f);
 pr_style(f,s,PR_DRAFT_COND,0,0,0,0,-1);pr_char(f,s,192);for(i=0;i<w;i++)pr_char(f,s,196);pr_char(f,s,217);pr_crlf(f);
 /* Do not leave the physical printer in condensed draft after a media
    block.  Send the reset commands unconditionally: relying only on the
    cached state is unsafe because the printer may have changed state while
    the box rows were flushed. */
 pr_media_mode=0;pr_esc_n(f,'x',1);pr_esc(f,18);pr_esc_n(f,'k',PR_PRESTIGE);
 pr_actual.draft=0;pr_actual.condensed=0;pr_actual.font=PR_PRESTIGE;
 pr_style(f,s,PR_PRESTIGE,0,0,0,0,-1);}
static int pr_quote_row(int p,int row)
{char line[DOC_COLS+1];int n,i;md_line(p,row,line,&n);i=md_lead(line,n);return i<n&&line[i]=='>';}
static int pr_quote_paragraph(FILE*f,PRSTATE*s,int p,int row,int last)
{char text[MD_LOGICAL+1],line[DOC_COLS+1];int ln,j,i,usedn=0,first=1,pos,end,cut,started=0,after,ordered; text[0]=0;
 pr_quote_mode=1;
 /* Markdown permits lazy continuation lines inside a blockquote.  Consume
    those lines as part of the quote instead of sending them through the
    ordinary Prestige paragraph path.  Stop at a blank line or a new block
    construct. */
 while(row<=last){
  md_line(p,row,line,&ln);i=md_lead(line,ln);
  if(i<ln&&line[i]=='>'){i++;while(i<ln&&line[i]==' ')i++;started=1;}
  else {
   if(!started||ln==0)break;
   if(i<ln&&(line[i]=='#'||line[i]=='`'||md_list_marker(line,ln,i,&after,&ordered)))break;
  }
  if(ln-i>0){if(!first&&usedn<MD_LOGICAL-1)text[usedn++]=' ';for(j=i;j<ln&&usedn<MD_LOGICAL-1;j++)text[usedn++]=line[j];text[usedn]=0;first=0;}
  row++;
 }
 pos=0;while(pos<usedn){while(pos<usedn&&text[pos]==' ')pos++;if(pos>=usedn)break;end=pos;cut=0;while(end<usedn&&end-pos<PR_WIDTH-2){if(text[end]==' ')cut=end;end++;}if(end<usedn&&cut>pos)end=cut;pr_style(f,s,PR_SCRIPT,0,1,0,0,-1);pr_char(f,s,179);pr_char(f,s,' ');pr_inline(f,s,text+pos,end-pos,PR_SCRIPT,0,1,0,0);pr_crlf(f);pos=end;while(pos<usedn&&text[pos]==' ')pos++;}
  pr_quote_mode=0;pr_style(f,s,PR_PRESTIGE,0,0,0,0,-1);return row;}
static int pr_paragraph_row(int p,int row)
{char line[DOC_COLS+1];MDTABLE tt;MDMEDIA media;int n,i,after=0,ordered=0,dummy;
 md_line(p,row,line,&n);i=md_lead(line,n);if(i>=n)return 0;
 if(md_fence_text(line,n)||md_media_parse(line,n,&media)||md_table_context(p,row,&tt,&dummy)||md_indented_code(p,row)||md_definition_term(p,row)||md_definition_line(p,row,&dummy))return 0;
 if(line[i]=='#'||line[i]=='>'||md_list_marker(line,n,i,&after,&ordered))return 0;
 if(i+2<n&&((line[i]=='-'&&line[i+1]=='-'&&line[i+2]=='-')||(line[i]=='*'&&line[i+1]=='*'&&line[i+2]=='*')))return 0;
 return 1;
}
static int pr_paragraph(FILE*f,PRSTATE*s,int p,int row,int last)
{char text[MD_LOGICAL+1],line[DOC_COLS+1];int ln,j,usedn=0,first=1;
 text[0]=0;while(row<=last&&pr_paragraph_row(p,row)){md_line(p,row,line,&ln);if(ln){if(!first&&usedn<MD_LOGICAL-1)text[usedn++]=' ';for(j=0;j<ln&&usedn<MD_LOGICAL-1;j++)text[usedn++]=line[j];text[usedn]=0;first=0;}row++;}
 if(usedn){pr_inline(f,s,text,usedn,PR_PRESTIGE,0,0,0,0);pr_crlf(f);}return row;
}
static void pr_markdown_line(FILE*f,PRSTATE*s,int p,int row,int in_fence)
{
 char line[DOC_COLS+1],tb[DOC_COLS+1];MDTABLE tt;MDMEDIA media;int n,i,h=0,lead,after=0,ordered=0,defpos=0,tctx,tborder=0,k;
 md_line(p,row,line,&n);
 if(in_fence){pr_code_line(f,s,line,n,0);return;}
 if(md_media_parse(line,n,&media)){pr_media_box(f,s,&media);return;}
 tctx=md_table_context(p,row,&tt,&tborder);
 if(tctx){n=md_table_make_row(p,row,&tt,tctx==2?tborder:0,tb);pr_style(f,s,PR_SANS,0,0,0,0,-1);for(i=0;i<n;i++)pr_char(f,s,(unsigned char)tb[i]);pr_crlf(f);return;}
 if(md_indented_code(p,row)){pr_code_line(f,s,line,n,4);return;}
 if(!n){pr_normal(f,s);pr_crlf(f);return;}
 lead=md_lead(line,n);
 if(md_definition_term(p,row)){for(i=0;i<lead;i++)pr_char(f,s,' ');pr_inline(f,s,line+lead,n-lead,PR_PRESTIGE,1,0,0,0);pr_crlf(f);return;}
 if(md_definition_line(p,row,&defpos)){for(i=0;i<4;i++)pr_char(f,s,' ');pr_inline(f,s,line+defpos,n-defpos,PR_PRESTIGE,0,0,0,0);pr_crlf(f);return;}
 i=lead;
 if(lead<=3&&i<n&&line[i]=='#'){
  while(i<n&&line[i]=='#'&&h<6){h++;i++;}while(i<n&&line[i]==' ')i++;
  if(h<=2)pr_style(f,s,PR_SANS,1,0,0,h==1, -1);else pr_style(f,s,PR_SANS,1,0,0,0,-1);
  if(h==1){pr_rule(f,s,196,PR_WIDTH);pr_crlf(f);}
  pr_inline(f,s,line+i,n-i,PR_SANS,1,0,0,h==1);pr_crlf(f);
  if(h==1)pr_rule(f,s,205,PR_WIDTH);else if(h==2)pr_rule(f,s,196,PR_WIDTH);else if(h==3)pr_rule(f,s,45,PR_WIDTH);else if(h==4)pr_rule(f,s,46,PR_WIDTH);
  return;
 }
 if(lead<=3&&i+2<n&&((line[i]=='-'&&line[i+1]=='-'&&line[i+2]=='-')||(line[i]=='*'&&line[i+1]=='*'&&line[i+2]=='*'))){pr_rule(f,s,196,PR_WIDTH);return;}
 for(k=0;k<lead;k++)pr_char(f,s,' ');
 if(md_list_marker(line,n,i,&after,&ordered)){
  if(ordered){while(i<after-1)pr_char(f,s,(unsigned char)line[i++]);pr_char(f,s,' ');}else{pr_char(f,s,'*');pr_char(f,s,' ');i=after;}
 }
 else if(i<n&&line[i]=='>'){
  pr_quote_mode=1;pr_style(f,s,PR_SCRIPT,0,1,0,0,-1);pr_char(f,s,179);pr_char(f,s,' ');i++;while(i<n&&line[i]==' ')i++;pr_inline(f,s,line+i,n-i,PR_SCRIPT,0,1,0,0);pr_crlf(f);pr_quote_mode=0;return;
 }
 else if(i+2<n&&line[i]=='['&&(line[i+1]==' '||line[i+1]=='x'||line[i+1]=='X')&&line[i+2]==']'){
  pr_char(f,s,'[');pr_char(f,s,line[i+1]);pr_char(f,s,']');pr_char(f,s,' ');i+=3;while(i<n&&line[i]==' ')i++;
 }
 if(i+4<n&&line[i]=='['&&line[i+1]=='^'){
  k=i+2;while(k<n&&line[k]!=']')k++;if(k<n&&k+1<n&&line[k+1]==':'){pr_style(f,s,PR_PRESTIGE,0,0,0,0,0);for(;i<n;i++)pr_char(f,s,(unsigned char)line[i]);pr_crlf(f);return;}
 }
 pr_inline(f,s,line+i,n-i,PR_PRESTIGE,0,0,0,0);pr_crlf(f);
}
static void print_current(void)
{
 FILE*f;PRSTATE st;int row,last=LINES-1,fence=0,fk;
 while(last>=0&&!used(current,last))last--;
 f=fopen("LPT1","wb");if(!f){acc_notice("Print","Unable to open LPT1.");return;}
 pr_esc(f,'@');             /* Reset printer. */
 pr_esc(f,'P');             /* 10-cpi Pica. */
 pr_esc(f,'2');             /* Normal 1/6-inch line spacing. */
 /* ASCII rules avoid reliance on a printer-specific graphics character table. */
 pr_esc_n(f,'x',1);         /* Letter-quality mode on LQ printers. */
 pr_count=pr_rows=0;pr_init_state(&pr_actual);pr_init_state(&st);pr_normal(f,&st);
 for(row=0;row<=last;row++){
  fk=line_fence_kind(current,row);
  if(fk){if(!fence)fence=fk;else if(fence==fk)fence=0;continue;}
  if(!fence&&pr_quote_row(current,row)){row=pr_quote_paragraph(f,&st,current,row,last)-1;continue;}
  if(!fence&&pr_paragraph_row(current,row)){row=pr_paragraph(f,&st,current,row,last)-1;continue;}
  pr_markdown_line(f,&st,current,row,fence!=0);
 }
 pr_normal(f,&st);pr_emit_style(f,&pr_actual,PR_PRESTIGE,0,0,0,0,-1);fputc('\f',f);row=ferror(f);if(fclose(f)!=0)row=1;if(row){acc_notice("Print","Printer write failed. Check LPT1.");return;}
 if(!print_silent)acc_notice("Print","Formatted document sent to printer on LPT1.");
}

/* Basic printing deliberately sends the document source unchanged.  It does
   not emit ESC/P commands or interpret Markdown, so it remains usable with
   printers and print servers that only accept ordinary text on LPT1. */
static void basic_current(void)
{
 FILE*f;char line[DOC_COLS+1];int row,last=LINES-1,n;
 while(last>=0&&!used(current,last))last--;
 f=fopen("LPT1","wb");if(!f){acc_notice("Print","Unable to open LPT1.");return;}
 for(row=0;row<=last;row++){md_line(current,row,line,&n);if(n)fwrite(line,1,n,f);fputs("\r\n",f);}
 fputc('\f',f);n=ferror(f);if(fclose(f)!=0)n=1;
 if(n){acc_notice("Print","Printer write failed. Check LPT1.");return;}
 if(!print_silent)acc_notice("Print","Plain text sent to printer on LPT1.");
}

/* PostScript export uses the same structural Markdown treatment as the
   ESC/P path, with standard PostScript fonts standing in for the printer's
   resident faces.  It is deliberately a plain Level 2 file so it can be
   converted by Ghostscript or printed by a PostScript-capable host. */
static void ps_escape(FILE*f,const char*s,int n)
{int i,c;for(i=0;i<n;i++){c=(unsigned char)s[i];if(c==179)c='|';else if(c==196||c==205)c='-';else if(c==218||c==191||c==192||c==217||c==195||c==180||c==194||c==193||c==197)c='+';if(c=='('||c==')'||c=='\\')fputc('\\',f);if(c<32||c>126)c='?';fputc(c,f);}}
static void ps_font_size(FILE*f,int font,int bold,int italic,int size)
{const char*n="Times-Roman";if(font==PR_SANS)n=bold?"Helvetica-Bold":"Helvetica";else if(font==PR_COURIER||font==PR_DRAFT||font==PR_DRAFT_COND)n=bold?"Courier-Bold":"Courier";else if(font==PR_SCRIPT)n="ZapfChancery-MediumItalic";else if(font==PR_PRESTIGE)n=bold?"Times-Bold":"Times-Roman";fprintf(f,"/%s findfont %d scalefont setfont\n",n,size);}
static void ps_font(FILE*f,int font,int bold,int italic){ps_font_size(f,font,bold,italic,10);}
static int ps_page_no=1;
/* Keep PostScript scratch storage out of the small DOS stack.  !MD is built
   with a deliberately modest stack, and the previous nested paragraph path
   could corrupt the process while exporting a longer document. */
static char ps_name_work[ACC_PATH],ps_message_work[ACC_PATH+32];
static char ps_line_work[DOC_COLS+1],ps_clean_work[DOC_COLS+1],ps_table_work[DOC_COLS+1];
static char ps_title_work[DOC_COLS+1],ps_desc_work[DOC_COLS+1];
static char ps_para_work[MD_LOGICAL+1],ps_para_clean_work[MD_LOGICAL+1],ps_src_work[DOC_COLS+1];
static void ps_text_line(FILE*f,int *line,const char*s,int n,int font,int bold,int italic)
{if(*line>=60){fprintf(f,"showpage\n%%%%Page: %d %d\n",++ps_page_no,ps_page_no);*line=0;}ps_font(f,font,bold,italic);fprintf(f,"54 %d moveto (",756-(*line)*12);ps_escape(f,s,n);fprintf(f,") show\n");(*line)++;}
static void ps_heading_line(FILE*f,int *line,const char*s,int n,int size,int bold,int italic)
{if(*line>=60){fprintf(f,"showpage\n%%%%Page: %d %d\n",++ps_page_no,ps_page_no);*line=0;}ps_font_size(f,PR_SANS,bold,italic,size);fprintf(f,"54 %d moveto (",756-(*line)*12);ps_escape(f,s,n);fprintf(f,") show\n");(*line)++;}
static void ps_rule_line(FILE*f,int *line,int kind)
{int y;if(*line>=60){fprintf(f,"showpage\n%%%%Page: %d %d\n",++ps_page_no,ps_page_no);*line=0;}y=756-(*line)*12;fprintf(f,"0 setgray %s setlinewidth ",kind==2?"2":"1");if(kind==3)fprintf(f,"[3 3] 0 setdash ");else if(kind==4)fprintf(f,"[1 3] 0 setdash ");else fprintf(f,"[] 0 setdash ");fprintf(f,"newpath 54 %d moveto 540 %d lineto stroke [] 0 setdash\n",y-3,y-3);(*line)++;}
static void ps_output_name(char*out)
{char *n=ps_name_work,*b,*d;strncpy(n,external[current]?paths[current]:"DOCUMENT.MD",ACC_PATH-5);n[ACC_PATH-5]=0;b=strrchr(n,'\\');d=strrchr(n,'/');if(d&&(!b||d>b))b=d;if(b)b++;else b=n;d=strrchr(b,'.');if(d)*d=0;strcat(b,".PS");acc_path(out,"EXPORT",b);}
static void ps_trim_markdown(char*out,int max,const char*in,int n)
{int i=0,o=0,code=0;while(i<n&&o<max-1){if(in[i]=='`'){code=!code;i++;continue;}if(!code&&((in[i]=='*'&&i+1<n)||(in[i]=='_'&&i+1<n))){i++;continue;}if(!code&&in[i]=='['){int k=i+1;while(k<n&&in[k]!=']')k++;if(k<n&&k+1<n&&in[k+1]=='('){while(i<k&&o<max-1)out[o++]=in[i++];i=k+2;while(i<n&&in[i]!=')')i++;if(i<n)i++;continue;}}out[o++]=in[i++];}out[o]=0;}
static void ps_wrap_text(FILE*f,int *line,const char*s,int n,int font,int bold,int italic)
{char b[PR_WIDTH+1];int i=0,j,k,last;while(i<n){while(i<n&&s[i]==' ')i++;if(i>=n)break;j=i;last=0;while(j<n&&j-i<PR_WIDTH){if(s[j]==' ')last=j;j++;}if(j<n&&last>i)k=last;else k=j;ps_text_line(f,line,s+i,k-i,font,bold,italic);i=k;while(i<n&&s[i]==' ')i++;}}
static void ps_quote_wrap(FILE*f,int *line,const char*s,int n)
{char out[PR_WIDTH+3];int i=0,j,k,last,m;while(i<n){while(i<n&&s[i]==' ')i++;if(i>=n)break;j=i;last=0;while(j<n&&j-i<PR_WIDTH-2){if(s[j]==' ')last=j;j++;}if(j<n&&last>i)k=last;else k=j;m=0;out[m++]='|';out[m++]=' ';while(i<k&&m<PR_WIDTH+2)out[m++]=s[i++];ps_text_line(f,line,out,m,PR_SCRIPT,0,1);while(i<n&&s[i]==' ')i++;}}
static int ps_quote_paragraph(FILE*f,int *line,int p,int row,int last)
{char *text=ps_para_work,*src=ps_src_work;int ln,i,j,usedn=0,first=1,started=0,after,ordered;text[0]=0;
 while(row<=last){
  md_line(p,row,src,&ln);i=md_lead(src,ln);
  if(i<ln&&src[i]=='>'){i++;while(i<ln&&src[i]==' ')i++;started=1;}
  else {
   if(!started||ln==0)break;
   if(i<ln&&(src[i]=='#'||src[i]=='`'||md_list_marker(src,ln,i,&after,&ordered)))break;
  }
  if(ln-i){if(!first&&usedn<MD_LOGICAL-1)text[usedn++]=' ';for(j=i;j<ln&&usedn<MD_LOGICAL-1;j++)text[usedn++]=src[j];text[usedn]=0;first=0;}
  row++;
 }
 if(usedn)ps_quote_wrap(f,line,text,usedn);return row;}
static int ps_paragraph(FILE*f,int *line,int p,int row,int last)
{char *text=ps_para_work,*clean=ps_para_clean_work,*src=ps_src_work;int ln,j,usedn=0,first=1;text[0]=0;while(row<=last&&pr_paragraph_row(p,row)){md_line(p,row,src,&ln);if(ln){if(!first&&usedn<MD_LOGICAL-1)text[usedn++]=' ';for(j=0;j<ln&&usedn<MD_LOGICAL-1;j++)text[usedn++]=src[j];text[usedn]=0;first=0;}row++;}if(usedn){ps_trim_markdown(clean,MD_LOGICAL+1,text,usedn);ps_wrap_text(f,line,clean,(int)strlen(clean),PR_PRESTIGE,0,0);}return row;}
static void ps_current(void)
{FILE*f;char *out=ps_name_work,*message=ps_message_work,*line=ps_line_work,*clean=ps_clean_work,*tb=ps_table_work,*title=ps_title_work,*desc=ps_desc_work;MDTABLE tt;MDMEDIA media;int row,last,n,i,lead,h,tctx,border,code=0,fk,ln=0;
 ps_output_name(out);f=fopen(out,"wb");if(!f){acc_notice("Postscript PS","Unable to create the PS file in EXPORT.");return;}ps_page_no=1;
 fprintf(f,"%%!PS-Adobe-3.0\n%%%%Creator: Launch! !MD\n%%%%Title: (Markdown export)\n%%%%Pages: (atend)\n%%%%EndComments\n%%%%BeginProlog\n%%%%EndProlog\n%%%%Page: 1 1\n");
 last=LINES-1;while(last>=0&&!used(current,last))last--;for(row=0;row<=last;row++){fk=line_fence_kind(current,row);if(fk){if(!code)code=fk;else if(code==fk)code=0;continue;}md_line(current,row,line,&n);if(code){ps_text_line(f,&ln,line,n,PR_DRAFT,0,0);continue;}if(!n){ps_text_line(f,&ln,"",0,PR_PRESTIGE,0,0);continue;}lead=md_lead(line,n);i=lead;
  if(md_media_parse(line,n,&media)){md_media_desc(&media,desc,sizeof(desc));ps_text_line(f,&ln,desc,(int)strlen(desc),PR_DRAFT_COND,0,0);continue;}
  tctx=md_table_context(current,row,&tt,&border);if(tctx){n=md_table_make_row(current,row,&tt,tctx==2?border:0,tb);ps_text_line(f,&ln,tb,n,PR_SANS,0,0);continue;}
  if(md_indented_code(current,row)){ps_text_line(f,&ln,line+4,n>4?n-4:0,PR_DRAFT,0,0);continue;}
  if(i<n&&line[i]=='#'){h=0;while(i<n&&line[i]=='#'&&h<6){h++;i++;}while(i<n&&line[i]==' ')i++;ps_trim_markdown(title,DOC_COLS+1,line+i,n-i);if(h==1){ps_rule_line(f,&ln,1);ps_text_line(f,&ln,"",0,PR_PRESTIGE,0,0);ps_heading_line(f,&ln,title,(int)strlen(title),16,1,0);ps_rule_line(f,&ln,2);}else if(h==2){ps_heading_line(f,&ln,title,(int)strlen(title),14,1,0);ps_rule_line(f,&ln,1);}else if(h==3){ps_heading_line(f,&ln,title,(int)strlen(title),12,1,0);ps_rule_line(f,&ln,3);}else if(h==4){ps_heading_line(f,&ln,title,(int)strlen(title),11,1,1);ps_rule_line(f,&ln,4);}else if(h==5)ps_heading_line(f,&ln,title,(int)strlen(title),10,1,0);else ps_heading_line(f,&ln,title,(int)strlen(title),10,1,1);continue;}
  if(!code&&pr_quote_row(current,row)){row=ps_quote_paragraph(f,&ln,current,row,last)-1;continue;}
  if(!code&&pr_paragraph_row(current,row)){row=ps_paragraph(f,&ln,current,row,last)-1;continue;}
  if(i<n&&line[i]=='>'){while(i<n&&line[i]=='>')i++;while(i<n&&line[i]==' ')i++;ps_trim_markdown(clean,DOC_COLS+1,line+i,n-i);ps_quote_wrap(f,&ln,clean,(int)strlen(clean));continue;}
  ps_trim_markdown(clean,DOC_COLS+1,line+i,n-i);ps_wrap_text(f,&ln,clean,(int)strlen(clean),PR_PRESTIGE,0,0);
 }
 fprintf(f,"showpage\n%%%%Trailer\n%%%%Pages: %d\n%%%%EOF\n",ps_page_no);fclose(f);sprintf(message,"PostScript document saved to\n%s",out);acc_notice("Postscript PS",message);
}

/* Same compact popup geometry and keyboard/mouse behavior as !TODOS Add. */
static int print_popup(int button_x,int button_y)
{return acc_print_popup(button_x,button_y);}

#pragma code_seg()
static void document_stats(unsigned long *chars,unsigned long *words)
{int y,i,n,inword=0;char c;*chars=*words=0;for(y=0;y<LINES;y++){n=used(current,y);*chars+=(unsigned long)n;if(n&&y+1<LINES)(*chars)++;for(i=0;i<n;i++){c=page[current][y][i];if(isalnum((unsigned char)c)){if(!inword){(*words)++;inword=1;}}else inword=0;}inword=0;}}
static int heading_label(int row,char *out,int max)
{int n,i,h,o=0;char b[DOC_COLS+1];md_line(current,row,b,&n);i=md_lead(b,n);h=0;while(i<n&&b[i]=='#'&&h<6){h++;i++;}while(i<n&&b[i]==' ')i++;while(i<n&&o<max-1)out[o++]=b[i++];while(o&&out[o-1]==' ')o--;out[o]=0;return o;}
static int collect_headings(void)
{int r,n,i,h;char b[DOC_COLS+1];heading_count=0;for(r=0;r<LINES&&heading_count<LINES;r++){md_line(current,r,b,&n);i=md_lead(b,n);h=0;if(i<=3&&i<n&&b[i]=='#'){while(i<n&&b[i]=='#'&&h<6){h++;i++;}if(h&&i<n&&b[i]==' '){while(i<n&&b[i]==' ')i++;if(i<n){heading_row[heading_count]=r;heading_level[heading_count]=h;heading_count++;}}}}return heading_count;}
static int headings_popup(void)
{
 int w=54,h=16,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,sel=0,first=0,r,i,ind,maxtext,a,n,result=0;unsigned mb=0;char shown[DOC_COLS+1];
 heading_row=(int far *)_fmalloc((unsigned)(LINES*sizeof(int)));heading_level=(int far *)_fmalloc((unsigned)(LINES*sizeof(int)));
 if(!heading_row||!heading_level){if(heading_row)_ffree(heading_row);if(heading_level)_ffree(heading_level);heading_row=heading_level=0;acc_notice("Document Map","Not enough memory for heading list.");return 0;}
 if(!collect_headings()){_ffree(heading_row);_ffree(heading_level);heading_row=heading_level=0;acc_notice("Document Map","No headings in this document.");return 0;}
 acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Document Map",0);acc_text(x+2,y+2,"Select a heading to navigate to",ACC_LABEL,31);for(r=0;r<10;r++){i=first+r;acc_fill(x+2,y+4+r,w-4,1,' ',i==sel?ACC_SELECT:ACC_CONTROL);if(i<heading_count){ind=heading_level[i]-1;if(ind<0)ind=0;if(ind>5)ind=5;maxtext=w-5-ind;a=i==sel?ACC_SELECT:ACC_CONTROL;n=heading_label(heading_row[i],shown,sizeof(shown));acc_text(x+2+ind,y+4+r,shown,a,n<maxtext?n:maxtext);}}acc_wait(&k,&mx,&my,&mb);if(k==27)break;if((mb&1)&&mx>=x+2&&mx<x+w-2&&my>=y+4&&my<y+14){i=first+my-(y+4);if(i<heading_count){sel=i;k=13;}}if(k==256+72&&sel>0)sel--;else if(k==256+80&&sel+1<heading_count)sel++;else if(k==256+73){sel-=10;if(sel<0)sel=0;}else if(k==256+81){sel+=10;if(sel>=heading_count)sel=heading_count-1;}else if(k==256+71)sel=0;else if(k==256+79)sel=heading_count-1;else if(k==13){cy=heading_row[sel];cx=0;top=cy;clear_selection();result=1;break;}if(sel<first)first=sel;if(sel>=first+10)first=sel-9;k=0;}
 acc_modal_end();_ffree(heading_row);_ffree(heading_level);heading_row=heading_level=0;return result;
}
static void md_scrollbar(int x,int y,int rows,int show_menu)
{if(show_menu){acc_scrollbar(x,y+1,rows-1,top,LINES,rows);acc_put(x,y,240,ACC_CONTROL);if(rows>2)acc_put(x,y+1,30,ACC_CONTROL);}else acc_scrollbar(x,y,rows,top,LINES,rows);}
static int focus_progress(void)
{int y,last=-1,n;unsigned long before=0,total=0,pos;for(y=LINES-1;y>=0;y--)if(used(current,y)){last=y;break;}if(last<0)return 0;for(y=0;y<=last;y++){n=used(current,y);if(y<cy)before+=(unsigned long)n+1UL;total+=(unsigned long)n+(y<last?1UL:0UL);}if(cy>last)return 1000;pos=before+(unsigned long)(cx<used(current,cy)?cx:used(current,cy));if(!total)return 0;if(pos>=total)return 1000;return(int)((pos*1000UL)/total);}
static void focus_draw(void)
{char b[COLS+1],s[42];int r,c,line,left=(acc_cols-COLS)/2,panel=focus_attr(),status=ACC_CONTROL,fill,prog,filled=ACC_ATTR(acc_appearance.controls_bg,acc_appearance.controls_bg);long pos,lo=has_selection()?sel_low():-1L,hi=has_selection()?sel_high():-1L;unsigned long chars,words;for(r=0;r<24;r++){line=top+r;if(line<LINES){_fmemcpy(b,page[current][line],COLS);b[COLS]=0;acc_text(left,r,b,panel,COLS);if(lo>=0)for(c=0;c<COLS;c++){pos=(long)line*COLS+c;if(pos>=lo&&pos<hi)acc_put(left+c,r,b[c],ACC_SELECT);}}}if(cy>=top&&cy<top+24){acc_caret_set(left+cx,cy-top);}else acc_caret_hide();document_stats(&chars,&words);sprintf(s,"Chars: %lu   Words: %lu",chars,words);prog=focus_progress();fill=(prog*12+999)/1000;if(prog==0)fill=0;acc_fill(0,24,acc_cols,1,' ',status);for(c=0;c<12;c++)acc_put(c,24,c<fill?219:176,c<fill?filled:status);acc_text(acc_cols-(int)strlen(s),24,s,status,(int)strlen(s));}
static void focus_mode(void)
{
 int key=0,mx=0,my=0,ch,oldx,oldy;unsigned mb=0;
 acc_close_target_suspend(&oldx,&oldy);acc_modal_begin();acc_input_bounds(0,0,acc_cols,acc_rows);
 focus_bright_backgrounds(1);acc_clear(focus_attr());focus_draw();
 while(key!=27){
  acc_wait(&key,&mx,&my,&mb);
  if(key==256+0x32||key==256+0x92||key==256+0x3F){
   acc_caret_hide();focus_bright_backgrounds(0);
   if(key==256+0x32)headings_popup();
   else if(key==256+0x92){acc_modal_begin();ch=character_palette();acc_modal_end();if(ch>=0){if(has_selection())delete_selection();insert_one(ch);clear_selection();}}
   else graph_show();
   acc_close_target_suspend(0,0);acc_input_bounds(0,0,acc_cols,acc_rows);focus_bright_backgrounds(1);acc_clear(focus_attr());key=0;
  }else if(key==256+0x3C){focus_colour_scheme=(focus_colour_scheme+1)%6;acc_clear(focus_attr());key=0;}
  else if(key==31){view_mode=(view_mode+1)%3;break;}
  else if(key!=27)editor_key(key,24);
  if(cy<top)top=cy;if(cy>=top+24)top=cy-23;if(key!=27)focus_draw();
 }
 acc_caret_hide();focus_bright_backgrounds(0);acc_modal_end();acc_close_target_restore(oldx,oldy);acc_restore_text_screen();acc_mouse_display(1);
}
static void maximize_page_bar(void)
{int i,xx=0,n,a,w,max;char s[6],shown[30],*base;acc_fill(0,24,80,1,' ',ACC_BG);for(i=0;i<count;i++){sprintf(s," %d ",i+1);w=(int)strlen(s);a=i==current?ACC_CONTROL:ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg);acc_text(xx,24,s,a,w);xx+=w+1;}if(count<PAGES)acc_put(xx,24,'+',ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg));if(external[current]&&paths[current][0]){base=strrchr(paths[current],'\\');if(!base)base=strrchr(paths[current],'/');base=base?base+1:paths[current];max=dirty[current]?79:80;n=(int)strlen(base);if(n>max){base+=n-max;n=max;}memcpy(shown,base,n);if(dirty[current])shown[n++]='*';shown[n]=0;acc_text(80-n,24,shown,dirty[current]?ACC_TITLE:ACC_HEADING,n);}else acc_text(73,24,"Unsaved",ACC_TITLE,7);}
static void maximize_draw(void)
{
 int i,r,rows=view_mode==0?12:24,line_attr=ACC_ATTR(7,acc_appearance.border);acc_clear(ACC_BG);
 if(view_mode==0){draw_source(0,0,12,1,FULL_COLS);for(i=0;i<80;i++)acc_put(i,12,196,line_attr);draw_preview(0,13,11);for(r=13;r<24;r++)acc_fill(COLS+1,r,79-COLS,1,' ',MD_TEXT);}
 else if(view_mode==1)draw_source(0,0,24,1,FULL_COLS);
 else{draw_preview(0,0,24);for(r=0;r<24;r++)acc_fill(COLS+1,r,79-COLS,1,' ',MD_TEXT);}
 md_scrollbar(79,0,rows,1);maximize_page_bar();
}
static void maximize_refresh(int key,int oldtop)
{
 int on,rows=view_mode==0?12:24;
 if(top!=oldtop||key==8||key==13||key==24||key==22||key==256+83){if(view_mode==0){draw_source(0,0,12,1,FULL_COLS);draw_preview(0,13,11);}else if(view_mode==1)draw_source(0,0,24,1,FULL_COLS);else draw_preview(0,0,24);md_scrollbar(79,0,rows,1);maximize_page_bar();return;}
 if(view_mode!=2)draw_source(0,0,rows,1,FULL_COLS);if(view_mode==0&&((key>=32&&key<=255)||(key>=513&&key<=767))&&cy>=top&&cy<top+12){on=code_before(current,cy);preview_line(0,13+cy-top,current,cy,on);}md_scrollbar(79,0,rows,1);maximize_page_bar();
}
static int md_prompt_current(void);
static void maximize_mode(void)
{
 int key=0,mx=0,my=0,rows,oldtop,editkey,oldpage,old_close_x,old_close_y;unsigned mb=0;clear_selection();
 acc_close_target_suspend(&old_close_x,&old_close_y);acc_modal_begin();acc_input_bounds(0,0,acc_cols,acc_rows);
 if(view_mode!=2&&!reflow_page(current,FULL_COLS)){acc_modal_end();acc_close_target_restore(old_close_x,old_close_y);acc_notice("Markdown","Not enough memory to expand the full-screen editor.");return;}
 if(cy<top)top=cy;rows=view_mode==0?12:24;if(cy>=top+rows)top=cy-rows+1;maximize_draw();
 while(key!=27&&key!=256+0x85&&key!=256+0x57){
  acc_wait(&key,&mx,&my,&mb);rows=view_mode==0?12:24;
  /* Keep the normal editor shortcuts active in full-screen mode. */
  if(key==19){save_current();maximize_draw();key=0;continue;}
  if(key==15){static char openlist[ACC_PATH];if(view_mode!=2)reflow_page(current,COLS);if(md_open_dialog(openlist))md_open_files(openlist);if(view_mode!=2)reflow_page(current,FULL_COLS);cx=cy=top=0;maximize_draw();key=0;continue;}
  if(key==14){if(md_prompt_current()){blank_page(current);memset(softwrap[current],0,LINES);paths[current][0]=0;external[current]=0;dirty[current]=0;cx=cy=top=0;clear_selection();}maximize_draw();key=0;continue;}
  if(key==256+0x92){int ch;acc_modal_begin();ch=character_palette();acc_modal_end();if(ch>=0){if(has_selection())delete_selection();insert_one(ch);clear_selection();}acc_close_target_suspend(0,0);acc_input_bounds(0,0,acc_cols,acc_rows);maximize_draw();key=0;continue;}
  if(key==31){if(view_mode!=2)reflow_page(current,COLS);view_mode=(view_mode+1)%3;top=0;if(view_mode!=2)reflow_page(current,FULL_COLS);clear_selection();maximize_draw();key=0;continue;}
  if(key==6){if(view_mode!=2)reflow_page(current,COLS);focus_mode();if(view_mode!=2)reflow_page(current,FULL_COLS);maximize_draw();key=0;continue;}
  if(key==256+0x3F){if(view_mode!=2)reflow_page(current,COLS);graph_show();if(view_mode!=2)reflow_page(current,FULL_COLS);maximize_draw();key=0;continue;}if(key==256+0x32){headings_popup();acc_close_target_suspend(0,0);acc_input_bounds(0,0,acc_cols,acc_rows);maximize_draw();key=0;continue;}
  if((mb&1)&&view_mode!=2&&mx>=0&&mx<FULL_COLS&&((view_mode==0&&my>=0&&my<12)||(view_mode==1&&my>=0&&my<24))){cx=mx;cy=top+my;clear_selection();if(view_mode==0)draw_source(0,0,12,1,FULL_COLS);else draw_source(0,0,24,1,FULL_COLS);key=0;continue;}
  if((mb&1)&&my==24){int pg,xx=0;for(pg=0;pg<count;pg++){char ps[6];int pw;sprintf(ps," %d ",pg+1);pw=(int)strlen(ps);if(mx>=xx&&mx<xx+pw)break;xx+=pw+1;}if(pg<count){if(tab_double_click(100+pg))break;oldpage=current;if(view_mode!=2)reflow_page(oldpage,COLS);current=pg;cx=cy=top=0;clear_selection();if(view_mode!=2&&!reflow_page(current,FULL_COLS)){current=oldpage;reflow_page(current,FULL_COLS);acc_notice("Markdown","Not enough memory to expand that page.");}maximize_draw();}else if(count<PAGES&&mx==xx){if(!ensure_page(count)){acc_notice("Markdown","Not enough memory for another page.");maximize_draw();key=0;continue;}oldpage=current;if(view_mode!=2)reflow_page(oldpage,COLS);current=count++;blank_page(current);memset(softwrap[current],0,LINES);wrap_width[current]=COLS;paths[current][0]=0;external[current]=0;cx=cy=top=0;clear_selection();if(view_mode!=2&&!reflow_page(current,FULL_COLS)){current=oldpage;count--;reflow_page(current,FULL_COLS);acc_notice("Markdown","Not enough memory to expand the new page.");}maximize_draw();}key=0;continue;}
  if((mb&1)&&mx==79&&my>=0&&my<rows){if(my==0){headings_popup();acc_close_target_suspend(0,0);acc_input_bounds(0,0,acc_cols,acc_rows);maximize_draw();key=0;continue;}else if(my==1&&top>0)top--;else if(my==rows-1&&top<LINES-rows)top++;else if(my>1&&my<rows-1)top=(my-2)*(LINES-rows)/(rows-3);if(top<0)top=0;if(top>LINES-rows)top=LINES-rows;if(cy<top)cy=top;if(cy>=top+rows)cy=top+rows-1;maximize_draw();key=0;continue;}
  editkey=key;oldtop=top;if(view_mode==2){
   if(key==256+72&&top>0)top--;else if(key==256+80&&top<LINES-rows)top++;
   else if(key==256+73){top-=rows;if(top<0)top=0;}
   else if(key==256+81){top+=rows;if(top>LINES-rows)top=LINES-rows;}
   else if(key!=27&&key!=256+0x85&&key!=256+0x57)key=0;
   if(top!=oldtop){maximize_draw();key=0;continue;}
  }else if(key!=27&&key!=256+0x85&&key!=256+0x57&&!editor_key(key,rows))key=0;
  if(cy<top)top=cy;if(cy>=top+rows)top=cy-rows+1;if(key!=27&&key!=256+0x85&&key!=256+0x57)maximize_refresh(editkey,oldtop);
 }
 if(view_mode!=2)reflow_page(current,COLS);if(cy<top)top=cy;rows=view_mode==0?VIEW:FULL_VIEW;if(cy>=top+rows)top=cy-rows+1;
 acc_caret_hide();acc_modal_end();acc_close_target_restore(old_close_x,old_close_y);acc_restore_text_screen();acc_mouse_display(1);
}

static int md_prompt_current(void)
{
  int choice;if(!dirty[current])return 1;choice=confirm_close();if(choice==0)return 0;if(choice==1)return 1;save_current();return !dirty[current];
}
static void md_remove_current(void)
{
  int i;if(count<=1){blank_page(0);memset(softwrap[0],0,LINES);paths[0][0]=0;external[0]=0;dirty[0]=0;current=0;cx=cy=top=0;clear_selection();return;}
  if(page[current]){_ffree(page[current]);page[current]=0;}for(i=current;i<count-1;i++){page[i]=page[i+1];memcpy(softwrap[i],softwrap[i+1],LINES);wrap_width[i]=wrap_width[i+1];external[i]=external[i+1];dirty[i]=dirty[i+1];strcpy(paths[i],paths[i+1]);}page[count-1]=0;memset(softwrap[count-1],0,LINES);wrap_width[count-1]=COLS;count--;if(current>=count)current=count-1;cx=cy=top=0;clear_selection();
}
#ifdef MDVIEW_BUILD
int main(int argc,char **argv)
{
 int rc=0,i,filearg=0,help_mode=0;
 for(i=1;i<argc;i++){
  if(!stricmp(argv[i],"/HELP")){help_mode=1;continue;}
  if(!strcmp(argv[i],"/?")||!strcmp(argv[i],"-?")){
   puts("!MDVIEW - Launch! Markdown viewer");
   puts("Usage: !MDVIEW file.md");
   return 0;
  }
  if(!filearg)filearg=i;
 }
 if(!filearg){puts("!MDVIEW - Launch! Markdown viewer");puts("Usage: !MDVIEW file.md");return 1;}
 if(!md_link_is_markdown(argv[filearg])){puts("!MDVIEW: Markdown .MD file required.");return 1;}
 if(help_mode){graph_colour_scheme=6;graph_colour_locked=1;}
 if(!acc_begin(argv[0],"Markdown Viewer",0))return 1;
 if(!init_pages()){acc_end();puts("!MDVIEW: not enough memory.");return 1;}
 memset(external,0,sizeof(external));memset(paths,0,sizeof(paths));memset(dirty,0,sizeof(dirty));
 if(!md_text_file(argv[filearg])||!load_file(0,argv[filearg])){free_pages();acc_end();puts("!MDVIEW: unable to open Markdown file.");return 1;}
 count=1;current=0;graph_show();free_pages();acc_end();return rc;
}
#else
int main(int argc,char **argv)
{
  int x,y,key=0,mx=0,my=0,focus=0,i,argn=0,rows,editkey,oldtop,done=0,discarded=0,choice;unsigned mb=0;
  if(acc_help(argc,argv,"!MD","Markdown editor with live preview, graphical pages and formatted printing."))return 0;
  if(!acc_begin(argv[0],"Markdown",0))return 1;if(!init_pages()){acc_notice("Markdown","Not enough memory for document pages.");acc_end();return 1;}memset(external,0,sizeof(external));memset(paths,0,sizeof(paths));memset(dirty,0,sizeof(dirty));
  for(i=1;i<argc&&argn<PAGES;i++)if(argv[i][0]!='/'&&argv[i][0]!='-'){if(load_file(argn,argv[i]))argn++;}
  if(argn){count=argn;current=0;}else{count=1;current=0;blank_page(0);memset(softwrap[0],0,LINES);paths[0][0]=0;external[0]=0;dirty[0]=0;}
  x=(acc_cols-DLG_W)/2;y=(acc_rows-DLG_H)/2;draw_ui(x,y,focus);
  while(!done){acc_wait(&key,&mx,&my,&mb);rows=view_mode==0?VIEW:FULL_VIEW;
    if(key==19){save_current();draw_ui(x,y,focus);key=0;continue;} /* Ctrl+S */
    if(key==31){view_mode=(view_mode+1)%3;top=0;if(view_mode==2)clear_selection();draw_ui(x,y,focus);key=0;continue;} /* Ctrl+/ */
    if(key==6){focus_mode();draw_ui(x,y,focus);key=0;continue;} /* Ctrl+F */
    if(key==256+0x3F){graph_show();draw_ui(x,y,focus);key=0;continue;} /* F5 */
    if(key==14){if(md_prompt_current()){blank_page(current);memset(softwrap[current],0,LINES);paths[current][0]=0;external[current]=0;dirty[current]=0;cx=cy=top=0;clear_selection();draw_ui(x,y,focus);}key=0;continue;}
    if(key==23){if(md_prompt_current()){md_remove_current();draw_ui(x,y,focus);}key=0;continue;}
    if(key==256+0x32){headings_popup();draw_ui(x,y,focus);key=0;continue;}
    if(key==27){if(!any_dirty()){done=1;break;}choice=confirm_close();if(choice==1){discarded=1;done=1;break;}if(choice==2&&save_all_dirty(!argn)){done=1;break;}draw_ui(x,y,focus);key=0;continue;}
    if((key==256+0x85||key==256+0x57)||((mb&1)&&my==y&&mx>=x+DLG_W-8&&mx<x+DLG_W-6)){maximize_mode();draw_ui(x,y,focus);key=0;continue;}
    if(view_mode!=2&&(mb&1)&&my>=y+2&&my<y+2+rows&&mx>=x+2&&mx<x+2+COLS){focus=0;clear_selection();cx=mx-(x+2);cy=top+my-(y+2);draw_source(x+2,y+2,rows,1,COLS);key=0;continue;}
    if((mb&1)&&my==y+17){int pg,xx=x+3;for(pg=0;pg<count;pg++){char ps[6];int pw;sprintf(ps," %d ",pg+1);pw=(int)strlen(ps);if(mx>=xx&&mx<xx+pw)break;xx+=pw+1;}if(pg<count){if(tab_double_click(pg)){maximize_mode();draw_ui(x,y,focus);key=0;continue;}current=pg;cx=cy=top=0;clear_selection();focus=0;draw_ui(x,y,focus);key=0;continue;}if(count<PAGES&&mx==xx){if(!ensure_page(count)){acc_notice("Markdown","Not enough memory for another page.");draw_ui(x,y,focus);key=0;continue;}current=count++;blank_page(current);memset(softwrap[current],0,LINES);paths[current][0]=0;external[current]=0;cx=cy=top=0;clear_selection();focus=0;draw_ui(x,y,focus);key=0;continue;}}
    if((mb&1)&&mx==x+70&&my>=y+2&&my<y+2+rows){if(view_mode!=2&&my==y+2){headings_popup();draw_ui(x,y,focus);key=0;continue;}else if(view_mode!=2&&my==y+3&&top>0)top--;else if(view_mode==2&&my==y+2&&top>0)top--;else if(my==y+1+rows&&top<LINES-rows)top++;else if(my>y+2+(view_mode!=2)&&my<y+1+rows){int off=view_mode!=2?4:3,den=rows-(view_mode!=2?3:2);top=(my-y-off)*(LINES-rows)/den;}if(top<0)top=0;if(top>LINES-rows)top=LINES-rows;if(cy<top)cy=top;if(cy>=top+rows)cy=top+rows-1;draw_ui(x,y,focus);key=0;continue;}
    if((mb&1)&&my==y+19){int hit=mx-(x+3);if(hit>=0&&hit<6)focus=BTN_OPEN;else if(hit>=7&&hit<13)focus=BTN_SAVE;else if(hit>=14&&hit<20)focus=BTN_PRINT;else if(hit>=23&&hit<30)focus=BTN_SPLIT;else if(hit>=31&&hit<37)focus=BTN_FOCUS;else if(hit>=39&&hit<45)focus=BTN_SHOW;else if(hit>=48&&hit<53)focus=BTN_CHARS;else if(hit>=52&&hit<58)focus=BTN_TIDY;else if(hit>=62&&hit<68)focus=BTN_CLOSE;else{key=0;continue;}key=13;}
    if(key==9||key==271){focus=key==271?(focus?focus-1:BTN_CLOSE):(focus==BTN_CLOSE?0:focus+1);draw_panes(x,y,focus);toolbar(x+3,y+19,focus);key=0;continue;}
    if(focus&&key==13){if(focus==BTN_OPEN){static char openlist[ACC_PATH];if(md_open_dialog(openlist))md_open_files(openlist);draw_ui(x,y,focus);}else if(focus==BTN_SHOW){graph_show();draw_ui(x,y,focus);}else if(focus==BTN_CHARS){int ch=character_palette();draw_ui(x,y,focus);if(ch>=0){if(has_selection())delete_selection();insert_one(ch);clear_selection();draw_ui(x,y,focus);}}else if(focus==BTN_TIDY){tidy_current();draw_ui(x,y,focus);}else if(focus==BTN_PRINT){int pm=print_popup(x+14,y+19);if(pm==1)basic_current();else if(pm==2)print_current();else if(pm==3)ps_current();draw_ui(x,y,focus);}else if(focus==BTN_SAVE){save_current();draw_ui(x,y,focus);}else if(focus==BTN_SPLIT){view_mode=(view_mode+1)%3;top=0;if(view_mode==2)clear_selection();draw_ui(x,y,focus);}else if(focus==BTN_FOCUS){focus_mode();draw_ui(x,y,focus);}else{if(!any_dirty())done=1;else{choice=confirm_close();if(choice==1){discarded=1;done=1;}else if(choice==2&&save_all_dirty(!argn))done=1;else draw_ui(x,y,focus);}}key=0;continue;}
    if(focus){if(key!=27)key=0;continue;}
    editkey=key;oldtop=top;if(view_mode==2){
      if(key==256+72&&top>0)top--;else if(key==256+80&&top<LINES-rows)top++;
      else if(key==256+73){top-=rows;if(top<0)top=0;}
      else if(key==256+81){top+=rows;if(top>LINES-rows)top=LINES-rows;}
      else if(key!=27)key=0;
      if(top!=oldtop){draw_panes(x,y,focus);page_bar(x+3,y+17);key=0;continue;}
    }else if(key!=27&&!editor_key(key,rows))key=0;
    if(cy<top)top=cy;if(cy>=top+rows)top=cy-rows+1;if(view_mode!=2){draw_edit_refresh(x,y,rows,editkey,oldtop);page_bar(x+3,y+17);}
  }
  if(clip){_ffree(clip);clip=0;}free_pages();acc_end();return 0;
}

#endif
