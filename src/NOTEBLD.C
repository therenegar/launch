/* Launch! Note - tabbed/page notebook accessory. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ACCLIB.H"
#define NW 60
#define EW 59
#define NL 100
#define NV 13
#define MAXTABS 32
#define MAXPAGES 10
#define TABNAME 12
static char note[NW*NL];
static unsigned char softwrap[NL];
static char tabs[MAXTABS][TABNAME+1];
static unsigned char pages[MAXTABS];
static int ntab=0,ctab=0,cpage=0;

/* Transient external-file pages.  Their working copies live only for this
   invocation and are never written to NOTE.IDX or the normal NOTE page files. */
#define MAX_EXTERNAL 10
#define NOTE_CLIP_MAX (EW*NL+NL*2+1)
static int persistent_ntab=0,external_tab=-1,external_saved_pages=0,external_count=0,external_runtime_tab=0;
static char external_path[MAX_EXTERNAL][ACC_PATH];
static char external_tmp[MAX_EXTERNAL][ACC_PATH];
static unsigned char external_dirty[MAX_EXTERNAL];
static int persistent_dirty=0,note_discard=0;

/* Keyboard selection is linear through the editable 56-column text area.
   Clipboard storage is FAR so it does not consume the small-model DGROUP. */
static int note_sel_anchor=-1,note_sel_caret=-1,note_clip_len=0,note_sel_repaint=0;
static char far note_clip[NOTE_CLIP_MAX];
static void data_name(char *n,int t,int p){sprintf(n,"N%02dP%02d.DAT",t,p);}
static void wrap_name(char *n,int t,int p){sprintf(n,"N%02dP%02d.WRP",t,p);}
static int external_page_index(void)
{
 if(external_tab<0||ctab!=external_tab)return -1;
 return(cpage>=0&&cpage<external_count)?cpage:-1;
}
static int persistent_pages_for(int t)
{
 if(t==external_tab&&!external_runtime_tab)return external_saved_pages;
 return pages[t];
}
static void meta_save(void)
{
 char p[ACC_PATH];FILE*f;int i,save_tabs=persistent_ntab?persistent_ntab:ntab;unsigned char pc;
 acc_path(p,"DATA","NOTE.IDX");f=fopen(p,"wb");if(!f)return;
 fwrite(&save_tabs,1,sizeof(save_tabs),f);
 for(i=0;i<save_tabs;i++){fwrite(tabs[i],1,TABNAME+1,f);pc=(unsigned char)persistent_pages_for(i);fwrite(&pc,1,1,f);}
 fclose(f);
}
static void meta_load(void)
{
 char p[ACC_PATH];FILE*f;int i;acc_path(p,"DATA","NOTE.IDX");f=fopen(p,"rb");
 if(f){fread(&ntab,1,sizeof(ntab),f);if(ntab<0||ntab>MAXTABS)ntab=0;for(i=0;i<ntab;i++){fread(tabs[i],1,TABNAME+1,f);fread(&pages[i],1,1,f);if(pages[i]<1)pages[i]=1;if(pages[i]>MAXPAGES)pages[i]=MAXPAGES;}fclose(f);}
 if(!ntab){ntab=1;strcpy(tabs[0],"Notes");pages[0]=1;persistent_ntab=ntab;meta_save();}
 persistent_ntab=ntab;
}
static void external_temp_name(char *out,int n)
{
 char name[16];sprintf(name,"NEX%02d.TMP",n);acc_path(out,"DATA",name);
}
static void external_import(int n)
{
 FILE*f;int c,row=0,col=0,spaces;
 memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));
 f=fopen(external_path[n],"rb");if(!f)return;
 while((c=fgetc(f))!=EOF&&row<NL){
  if(c=='\r')continue;
  if(c=='\n'){if(++row>=NL)break;col=0;softwrap[row]=0;continue;}
  if(c=='\t'){
   spaces=4-(col&3);
   while(spaces--&&row<NL){if(col>=EW){if(++row>=NL)break;col=0;softwrap[row]=1;}note[row*NW+col++]=' ';}
   continue;
  }
  if(c<32)continue;
  if(col>=EW){if(++row>=NL)break;col=0;softwrap[row]=1;}
  note[row*NW+col++]=(char)c;
 }
 fclose(f);
}
static void external_work_load(int n)
{
 FILE*f;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));
 f=fopen(external_tmp[n],"rb");
 if(f){fread(note,1,sizeof(note),f);fread(softwrap,1,sizeof(softwrap),f);fclose(f);}
 else external_import(n);
}
static void external_work_save(int n)
{
 FILE*f=fopen(external_tmp[n],"wb");if(!f)return;
 fwrite(note,1,sizeof(note),f);fwrite(softwrap,1,sizeof(softwrap),f);fclose(f);
}
static int external_write_original(int n)
{
 FILE*f;int r,q,last=NL-1;
 while(last>=0){q=EW;while(q&&note[last*NW+q-1]==' ')q--;if(q)break;last--;}
 f=fopen(external_path[n],"wb");if(!f)return 0;
 for(r=0;r<=last;r++){
  q=EW;while(q&&note[r*NW+q-1]==' ')q--;
  if(q)fwrite(note+r*NW,1,q,f);
  if(r<last&&!softwrap[r+1])fputs("\r\n",f);
 }
 fclose(f);external_work_save(n);external_dirty[n]=0;return 1;
}
static void external_cleanup(void)
{
 int i;for(i=0;i<external_count;i++)remove(external_tmp[i]);
}
static void external_setup(int argc,char **argv)
{
 int i,found=-1;if(argc<=1)return;
 external_count=argc-1;if(external_count>MAX_EXTERNAL)external_count=MAX_EXTERNAL;
 for(i=0;i<external_count;i++){
  strncpy(external_path[i],argv[i+1],ACC_PATH-1);external_path[i][ACC_PATH-1]=0;
  external_temp_name(external_tmp[i],i);remove(external_tmp[i]);
 }
 if(!external_count)return;
 for(i=0;i<persistent_ntab;i++)if(!stricmp(tabs[i],"External")){found=i;break;}
 if(found>=0){
  external_tab=found;external_saved_pages=pages[found];external_runtime_tab=0;
  pages[found]=(unsigned char)external_count;
 }else if(ntab<MAXTABS){
  external_tab=ntab;external_saved_pages=0;external_runtime_tab=1;
  strcpy(tabs[ntab],"External");pages[ntab]=(unsigned char)external_count;ntab++;
 }else{
  external_count=0;external_tab=-1;return;
 }
 ctab=external_tab;cpage=0;
}
static void page_load(void)
{
 char p[ACC_PATH],n[20];FILE*f;int ep=external_page_index();
 memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));
 if(ep>=0){external_work_load(ep);return;}
 if(!pages[ctab])return;data_name(n,ctab,cpage);acc_path(p,"DATA",n);f=fopen(p,"rb");if(f){fread(note,1,sizeof(note),f);fclose(f);}
 wrap_name(n,ctab,cpage);acc_path(p,"DATA",n);f=fopen(p,"rb");if(f){fread(softwrap,1,sizeof(softwrap),f);fclose(f);}
}
static void page_save(void)
{
 char p[ACC_PATH],n[20];FILE*f;int ep=external_page_index();
 if(ep>=0){external_work_save(ep);return;}
 if(!pages[ctab])return;data_name(n,ctab,cpage);acc_path(p,"DATA",n);f=fopen(p,"wb");if(f){fwrite(note,1,sizeof(note),f);fclose(f);}
 wrap_name(n,ctab,cpage);acc_path(p,"DATA",n);f=fopen(p,"wb");if(f){fwrite(softwrap,1,sizeof(softwrap),f);fclose(f);}
}
static int logical_number(int line){int i,n=0;for(i=0;i<=line;i++)if(i==0||!softwrap[i])n++;return n;}
static int note_sel_low(void){return note_sel_anchor<note_sel_caret?note_sel_anchor:note_sel_caret;}
static int note_sel_high(void){return note_sel_anchor>note_sel_caret?note_sel_anchor:note_sel_caret;}
static int note_has_selection(void){return note_sel_anchor>=0&&note_sel_caret>=0&&note_sel_anchor!=note_sel_caret;}
static int note_selected(int line,int col){int p=line*EW+col;return note_has_selection()&&p>=note_sel_low()&&p<note_sel_high();}
static void note_selection_clear(void){note_sel_anchor=note_sel_caret=-1;}
static int note_view_rows=NV;
static void row_draw(int x,int y,int line,int top){char b[5];int c,a=ACC_ATTR(acc_appearance.controls_bg,(acc_appearance.controls_fg&7)|8);if(line>=top&&line<top+note_view_rows){acc_fill(x,y+line-top,4,1,' ',ACC_CONTROL);if(line==0||!softwrap[line]){sprintf(b,"%3d",logical_number(line));acc_text(x,y+line-top,b,a,3);}acc_text(x+4,y+line-top,note+line*NW,ACC_CONTROL,EW);for(c=0;c<EW;c++)if(note_selected(line,c))acc_put(x+4+c,y+line-top,note[line*NW+c],ACC_SELECT);}}
static int note_editor_focus=0;
static void cursor_draw(int x,int y,int cx,int cy,int top){if(note_editor_focus&&cy>=top&&cy<top+note_view_rows){acc_caret_set(x+4+cx,y+cy-top);}else acc_caret_hide();}
static int last_note_line(void){int l,q;for(l=NL-1;l>0;l--){for(q=0;q<EW;q++)if(note[l*NW+q]!=' ')return l;}return 0;}
static void view_draw(int x,int y,int cx,int cy,int top){int r,last=last_note_line();for(r=0;r<note_view_rows;r++)row_draw(x,y,top+r,top);cursor_draw(x,y,cx,cy,top);acc_fill(x+4+EW,y,1,note_view_rows,' ',ACC_CONTROL);if(top>0)acc_put(x+4+EW,y,30,ACC_CONTROL);if(top+note_view_rows<=last)acc_put(x+4+EW,y+note_view_rows-1,31,ACC_CONTROL);}
static int input_name(char *out,int edit)
{
 int w=38,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=0,focus=-1;unsigned b=0;if(!edit)out[0]=0;else pos=(int)strlen(out);
 for(;;){
  /* Spawned dialogs reuse the established Add/Edit form treatment. */
  acc_subbox(x,y,w,h,edit?"Edit tab":"New tab",1);
  acc_text(x+3,y+2,"Name:",ACC_LABEL,5);
  acc_fill(x+9,y+2,20,1,' ',focus==0?ACC_SELECT:ACC_CONTROL);acc_text(x+9,y+2,out,focus==0?ACC_SELECT:ACC_CONTROL,pos);if(focus==0)acc_caret_set(x+9+pos,y+2);else acc_caret_hide();
  acc_button(x+2,y+6,"  OK  ",focus==1);acc_button(x+10,y+6,"  Cancel  ",focus==2);acc_wait(&k,&mx,&my,&b);
  if((b&1)&&my==y+2&&mx>=x+9&&mx<x+29){focus=0;k=0;continue;}
  if((b&1)&&my==y+6){if(mx>=x+2&&mx<x+9){focus=1;k=13;}else if(mx>=x+10&&mx<x+21){focus=2;k=13;}}
  if(k==9||k==271){if(focus<0)focus=(k==271)?2:0;else focus=(k==271)?(focus+2)%3:(focus+1)%3;k=0;continue;}if(k==27)return 0;
  if(k==13&&focus==1){if(pos){out[pos]=0;return 1;}k=0;continue;}if(k==13&&focus==2)return 0;
  if(focus==0){if(k==256+71)pos=0;else if(k==256+79)pos=(int)strlen(out);else if(k==8&&pos)out[--pos]=0;else if(k>=32&&k<127&&pos<TABNAME){out[pos++]=(char)k;out[pos]=0;}}
  k=0;
 }
}
static int confirm_remove(const char *msg)
{
 int w=36,h=8,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,f=-1;unsigned b=0;
 for(;;){
  acc_subbox(x,y,w,h,"Remove",1);acc_text(x+3,y+2,msg,ACC_LABEL,(int)strlen(msg));
  acc_button(x+3,y+5,"  OK  ",f==0);acc_button(x+11,y+5,"  Cancel  ",f==1);
  acc_wait(&k,&mx,&my,&b);
  if(b&1){
   if(my==y+5&&mx>=x+3&&mx<x+9)return 1;
   if(my==y+5&&mx>=x+11&&mx<x+17)return 0;
   continue;
  }
  if(k==27)return 0;
  if(k==9||k==271){if(f<0)f=(k==271)?1:0;else f=!f;continue;}
  if(k==13&&f>=0)return f==0;
 }
}

static void note_remove_page_files(int tab,int page,int count)
{
 char a[ACC_PATH],b[ACC_PATH],n1[20],n2[20];int p;
 data_name(n1,tab,page);acc_path(a,"DATA",n1);remove(a);
 wrap_name(n1,tab,page);acc_path(a,"DATA",n1);remove(a);
 for(p=page+1;p<count;p++){
  data_name(n1,tab,p);data_name(n2,tab,p-1);acc_path(a,"DATA",n1);acc_path(b,"DATA",n2);remove(b);rename(a,b);
  wrap_name(n1,tab,p);wrap_name(n2,tab,p-1);acc_path(a,"DATA",n1);acc_path(b,"DATA",n2);remove(b);rename(a,b);
 }
}

static void note_remove_tab_files(int tab)
{
 char a[ACC_PATH],b[ACC_PATH],n1[20],n2[20];int t,p;
 for(p=0;p<(int)pages[tab];p++){
  data_name(n1,tab,p);acc_path(a,"DATA",n1);remove(a);
  wrap_name(n1,tab,p);acc_path(a,"DATA",n1);remove(a);
 }
 for(t=tab+1;t<persistent_ntab;t++)for(p=0;p<persistent_pages_for(t);p++){
  data_name(n1,t,p);data_name(n2,t-1,p);acc_path(a,"DATA",n1);acc_path(b,"DATA",n2);remove(b);rename(a,b);
  wrap_name(n1,t,p);wrap_name(n2,t-1,p);acc_path(a,"DATA",n1);acc_path(b,"DATA",n2);remove(b);rename(a,b);
 }
}

static int note_can_delete(void)
{
 if(ctab==external_tab&&external_count>0)return 0;
 return persistent_pages_for(ctab)>1 || persistent_ntab>1;
}

static int note_next_focus(int focus,int backwards)
{
 static const int order[10]={0,2,1,3,4,5,6,7,8,9};
 int pos=-1,step,i,next;
 if(focus<0)return backwards?9:0;
 for(i=0;i<10;i++)if(order[i]==focus){pos=i;break;}
 if(pos<0)return backwards?9:0;
 step=backwards?-1:1;
 for(i=0;i<10;i++){pos=(pos+step+10)%10;next=order[pos];if(next!=4||note_can_delete())return next;}
 return focus;
}

static int export_current_page(char *out)
{
 char p[ACC_PATH],name[20];FILE *f;int n,r,last,q;
 for(n=1;n<10000;n++){sprintf(name,"NOTE%d.TXT",n);acc_path(p,"EXPORT",name);if(!acc_exists(p))break;}
 if(n==10000)return 0;strcpy(out,p);f=fopen(p,"w");if(!f)return 0;
 last=NL-1;while(last>=0){q=NW;while(q&&note[last*NW+q-1]==' ')q--;if(q)break;last--;}
 for(r=0;r<=last;r++){q=NW;while(q&&note[r*NW+q-1]==' ')q--;fwrite(note+r*NW,1,q,f);fputc('\n',f);}
 fclose(f);return 1;
}
static int export_all(char *out)
{
 char p[ACC_PATH],name[20];FILE*f,*in;int n,t,pg,r,last,pc;
 for(n=1;n<10000;n++){sprintf(name,"NOTE%d.TXT",n);acc_path(p,"EXPORT",name);if(!acc_exists(p))break;}
 if(n==10000)return 0;strcpy(out,p);f=fopen(p,"w");if(!f)return 0;page_save();
 for(t=0;t<persistent_ntab;t++){
  fprintf(f,"=== %s ===\n",tabs[t]);pc=persistent_pages_for(t);
  for(pg=0;pg<pc;pg++){fprintf(f,"--- Page %d ---\n",pg+1);data_name(name,t,pg);acc_path(p,"DATA",name);in=fopen(p,"rb");if(in){fread(note,1,sizeof(note),in);fclose(in);last=NL-1;while(last>=0){r=NW;while(r&&note[last*NW+r-1]==' ')r--;if(r)break;last--;}for(r=0;r<=last;r++){int q=NW;while(q&&note[r*NW+q-1]==' ')q--;fwrite(note+r*NW,1,q,f);fputc('\n',f);}}fputs("------------------------------------------------------------\n",f);}
 }
 fclose(f);page_load();return 1;
}
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


static int note_tab_hover=-1;

static int tab_last_visible(int first)
{
 int i,xx=6;for(i=first;i<ntab;i++){int w=(int)strlen(tabs[i])+4;if(xx+w>64)break;xx+=w+1;}return i-1;
}
static void draw_tabs(int x,int y,int first,int focus)
{
 int i,xx=x+6,last=tab_last_visible(first);acc_fill(x+1,y,65,1,' ',ACC_BG);
 /* Mini navigation buttons only exist when there is something to scroll to. */
 if(first>0)acc_put(x+3,y,17,ACC_BORDER);if(last+1<ntab)acc_put(x+65,y,16,ACC_BORDER);
 for(i=first;i<=last&&i<ntab;i++){int a=(i==ctab)?ACC_CONTROL:ACC_SELECT,w=(int)strlen(tabs[i])+4;acc_fill(xx,y,w,1,' ',a);acc_text(xx+2,y,tabs[i],a,(int)strlen(tabs[i]));if((focus&&i==ctab)||i==note_tab_hover){int fa=ACC_ATTR(acc_appearance.controls_bg,acc_appearance.controls_fg);acc_put(xx,y,169,fa);acc_put(xx+w-1,y,170,fa);}xx+=w+1;}
}
static int page_last_visible(int first){(void)first;return pages[ctab]?pages[ctab]-1:-1;}
/* Release 3.63: page selectors live on a horizontal strip below the editor.
   Each page gets one coloured cell either side of its number.  The current
   page always uses Controls colours; keyboard focus uses Selected Items for
   a different pending page. */
static int page_token_width(int page){return page>=9?4:3;}
static int page_token_x(int x,int page)
{
 int i,xx=x;for(i=0;i<page;i++)xx+=page_token_width(i)+1;return xx;
}
static void draw_pages(int x,int y,int first,int focus,int sel)
{
 int i,xx=x,last=page_last_visible(first),a,w,n,max,ep;char b[6],shown[30],*base;(void)first;
 acc_fill(x,y,EW+1,1,' ',ACC_BG);
 for(i=0;i<=last;i++){
  sprintf(b," %d ",i+1);w=(int)strlen(b);
  if(focus&&i==sel&&i!=cpage)a=ACC_SELECT;
  else if(i==cpage)a=ACC_CONTROL;
  else a=ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg);
  acc_text(xx,y,b,a,w);xx+=w+1;
 }
 if(ctab!=external_tab&&(int)pages[ctab]<MAXPAGES){
  a=(focus&&sel==(int)pages[ctab])?ACC_SELECT:ACC_ATTR(acc_appearance.background,acc_appearance.titlebar_fg);
  acc_put(xx,y,'+',a);
 }
 ep=external_page_index();
 if(ep>=0){base=strrchr(external_path[ep],'\\');if(!base)base=strrchr(external_path[ep],'/');base=base?base+1:external_path[ep];max=external_dirty[ep]?27:28;n=(int)strlen(base);if(n>max){base+=n-max;n=max;}memcpy(shown,base,n);if(external_dirty[ep])shown[n++]='*';shown[n]=0;acc_text(x+EW-n,y,shown,external_dirty[ep]?ACC_TITLE:ACC_HEADING,n);}
 else acc_text(x+EW-7,y,"Unsaved",ACC_TITLE,7);
}
static void note_word_wrap(int *pcx,int *pcy)
{
  int cy=*pcy,start=EW-1,len,i,can=1;if(cy>=NL-1)return;
  if(note[cy*NW+EW-1]==' '){*pcy=cy+1;*pcx=0;softwrap[cy+1]=1;return;}
  while(start>0&&note[cy*NW+start-1]!=' ')start--;
  if(start<=0){*pcy=cy+1;*pcx=0;softwrap[cy+1]=1;return;}
  len=EW-start;for(i=EW-len;i<EW;i++)if(note[(cy+1)*NW+i]!=' ')can=0;
  if(!can){*pcy=cy+1;*pcx=0;softwrap[cy+1]=1;return;}
  memmove(note+(cy+1)*NW+len,note+(cy+1)*NW,EW-len);
  memcpy(note+(cy+1)*NW,note+cy*NW+start,len);memset(note+cy*NW+start,' ',EW-start);
  softwrap[cy+1]=1;*pcy=cy+1;*pcx=len;
}
static int note_shift_down(void){return (*(unsigned char far *)(((unsigned long)0x40<<16)|0x17)&3)!=0;}
static void note_selection_move(int oldpos,int newpos,int shift)
{
 if(shift){note_sel_repaint=1;if(note_sel_anchor<0)note_sel_anchor=oldpos;note_sel_caret=newpos;if(note_sel_caret==note_sel_anchor)note_selection_clear();}
 else {if(note_has_selection())note_sel_repaint=1;note_selection_clear();}
}
static void note_copy_selection(void)
{
 int lo,hi,p,row,col,lastrow,n=0;if(!note_has_selection())return;lo=note_sel_low();hi=note_sel_high();lastrow=(hi-1)/EW;
 for(p=lo;p<hi&&n<NOTE_CLIP_MAX-3;){
  row=p/EW;col=p%EW;note_clip[n++]=note[row*NW+col];p++;
  if(p<hi&&p%EW==0&&row<lastrow&&!softwrap[row+1]){note_clip[n++]='\r';note_clip[n++]='\n';}
 }
 note_clip_len=n;
}
static void note_delete_selection(int *pcx,int *pcy)
{
 int lo,hi,fr,lr,r,start,end,count;if(!note_has_selection())return;
 lo=note_sel_low();hi=note_sel_high();fr=lo/EW;lr=(hi-1)/EW;
 for(r=fr;r<=lr;r++){
  start=(r==fr)?lo%EW:0;end=(r==lr)?((hi-1)%EW)+1:EW;count=end-start;
  if(count>0){memmove(note+r*NW+start,note+r*NW+end,EW-end);memset(note+r*NW+EW-count,' ',count);}
 }
 *pcy=fr;*pcx=lo%EW;note_selection_clear();
}
static void note_insert_one(int *pcx,int *pcy,int ch,int insert)
{
 int cx=*pcx,cy=*pcy,base=cy*NW;
 if(insert&&cx<EW-1)memmove(note+base+cx+1,note+base+cx,EW-cx-1);
 note[base+cx]=(char)ch;
 if(cx<EW-1)cx++;else if(cy<NL-1)note_word_wrap(&cx,&cy);
 *pcx=cx;*pcy=cy;
}
static void note_paste(int *pcx,int *pcy,int insert)
{
 int i,c;if(note_has_selection())note_delete_selection(pcx,pcy);
 for(i=0;i<note_clip_len&&*pcy<NL;i++){
  c=(unsigned char)note_clip[i];if(c=='\r')continue;
  if(c=='\n'){if(*pcy<NL-1){(*pcy)++;*pcx=0;softwrap[*pcy]=0;}continue;}
  note_insert_one(pcx,pcy,c,insert);
 }
}
static int note_used(int row){int n=EW;while(n>0&&note[row*NW+n-1]==' ')n--;return n;}
static void note_remove_row(int row){int r;for(r=row;r<NL-1;r++){memcpy(note+r*NW,note+(r+1)*NW,EW);softwrap[r]=softwrap[r+1];}memset(note+(NL-1)*NW,' ',EW);softwrap[NL-1]=0;}
static void note_join_next(int *pcx,int *pcy){int cx=*pcx,cy=*pcy,n,take,room;if(cy>=NL-1)return;n=note_used(cy+1);room=EW-cx;take=n<room?n:room;if(take)memcpy(note+cy*NW+cx,note+(cy+1)*NW,take);if(take>=n)note_remove_row(cy+1);else{memmove(note+(cy+1)*NW,note+(cy+1)*NW+take,EW-take);memset(note+(cy+1)*NW+EW-take,' ',take);}}
static void note_join_previous(int *pcx,int *pcy){int cx,cy=*pcy,prev,n,take,room;if(cy<=0)return;prev=note_used(cy-1);n=note_used(cy);room=EW-prev;if(room<=0){*pcy=cy-1;*pcx=EW-1;return;}take=n<room?n:room;if(take)memcpy(note+(cy-1)*NW+prev,note+cy*NW,take);if(take>=n)note_remove_row(cy);else{memmove(note+cy*NW,note+cy*NW+take,EW-take);memset(note+cy*NW+EW-take,' ',take);}cx=prev;*pcy=cy-1;*pcx=cx;}
static void note_split_line(int *pcx,int *pcy){int cx=*pcx,cy=*pcy,r,n,tail,indent=0;if(cy>=NL-1)return;while(indent<EW&&note[cy*NW+indent]==' ')indent++;if(indent>=EW)indent=0;for(r=NL-1;r>cy+1;r--){memcpy(note+r*NW,note+(r-1)*NW,EW);softwrap[r]=softwrap[r-1];}memset(note+(cy+1)*NW,' ',EW);n=note_used(cy);tail=n>cx?n-cx:0;if(tail>EW-indent)tail=EW-indent;if(tail)memcpy(note+(cy+1)*NW+indent,note+cy*NW+cx,tail);memset(note+cy*NW+cx,' ',EW-cx);softwrap[cy+1]=0;*pcy=cy+1;*pcx=indent;}
static int confirm_external_save(const char *path)
{
 int w=58,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,f=-1;unsigned b=0;char shown[45];
 strncpy(shown,path,44);shown[44]=0;
 for(;;){acc_subbox(x,y,w,h,"Save",1);acc_text(x+3,y+2,"Overwrite the opened file?",ACC_LABEL,27);acc_text(x+3,y+3,shown,ACC_TEXT,(int)strlen(shown));acc_button(x+3,y+6,"  Save  ",f==0);acc_button(x+13,y+6,"  Cancel  ",f==1);acc_wait(&k,&mx,&my,&b);
  if((b&1)&&my==y+6){if(mx>=x+3&&mx<x+11)return 1;if(mx>=x+13&&mx<x+23)return 0;}
  if(k==27)return 0;if(k==9||k==271){if(f<0)f=(k==271)?1:0;else f=!f;continue;}if(k==13&&f>=0)return f==0;
 }
}
static int note_any_dirty(void){int i;if(persistent_dirty)return 1;for(i=0;i<external_count;i++)if(external_dirty[i])return 1;return 0;}
static int note_confirm_close(void)
{int w=48,h=8,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,f=-1;unsigned b=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Unsaved Changes",1);acc_text(x+3,y+2,"You have unsaved changes!",ACC_LABEL,25);acc_button(x+3,y+5," Save ",f==0);acc_button(x+11,y+5," Discard ",f==1);acc_button(x+38,y+5," Close ",f==2);acc_wait(&k,&mx,&my,&b);if((b&1)&&my==y+5){if(mx>=x+3&&mx<x+9){f=0;k=13;}else if(mx>=x+11&&mx<x+20){f=1;k=13;}else if(mx>=x+38&&mx<x+44){f=2;k=13;}}if(k==27){acc_modal_end();return 0;}if(k==9||k==271){if(f<0)f=k==271?2:0;else f=k==271?(f+2)%3:(f+1)%3;k=0;continue;}if(k==13&&f>=0){acc_modal_end();return f==0?2:(f==1?1:0);}}}
static int note_save_all_dirty(void)
{int i;page_save();persistent_dirty=0;for(i=0;i<external_count;i++)if(external_dirty[i]){external_work_load(i);if(!external_write_original(i)){page_load();return 0;}}page_load();return 1;}

static void note_status_right(int right,int y)
{int n,max,ep;char shown[30],*base;ep=external_page_index();if(ep>=0){base=strrchr(external_path[ep],'\\');if(!base)base=strrchr(external_path[ep],'/');base=base?base+1:external_path[ep];max=external_dirty[ep]?27:28;n=(int)strlen(base);if(n>max){base+=n-max;n=max;}memcpy(shown,base,n);if(external_dirty[ep])shown[n++]='*';shown[n]=0;acc_text(right-n,y,shown,external_dirty[ep]?ACC_TITLE:ACC_HEADING,n);}else acc_text(right-7,y,"Unsaved",ACC_TITLE,7);}
static void note_max_draw(int tabfirst,int pagefirst,int psel,int cx,int cy,int top)
{acc_clear(ACC_BG);draw_tabs(0,0,tabfirst,0);note_view_rows=23;note_editor_focus=1;acc_fill(0,1,80,23,' ',ACC_CONTROL);view_draw(0,1,cx,cy,top);draw_pages(4,24,pagefirst,0,psel);acc_fill(60,24,20,1,' ',ACC_BG);}
static void note_maximize(int *pcx,int *pcy,int *ptop,int *ptabfirst,int *ppagefirst,int *ppsel,int *pinsert)
{
 int key=0,mx=0,my=0,cx=*pcx,cy=*pcy,top=*ptop,base,ch,ep,oldpos,shift,insert=*pinsert;unsigned mb=0;
 note_selection_clear();note_max_draw(*ptabfirst,*ppagefirst,*ppsel,cx,cy,top);
 while(key!=27&&key!=256+0x85&&key!=256+0x57){
  acc_wait(&key,&mx,&my,&mb);
  if((mb&1)&&my==0){int i,xx=6,last=tab_last_visible(*ptabfirst);for(i=*ptabfirst;i<=last&&i<ntab;i++){int w=(int)strlen(tabs[i])+4;if(mx>=xx&&mx<xx+w)break;xx+=w+1;}if(i<=last&&i<ntab&&i!=ctab){page_save();ctab=i;cpage=*ppsel=0;*ppagefirst=0;page_load();cx=cy=top=0;}note_max_draw(*ptabfirst,*ppagefirst,*ppsel,cx,cy,top);key=0;continue;}
  if((mb&1)&&my>=1&&my<24&&mx>=4&&mx<4+EW){cx=mx-4;cy=top+my-1;note_selection_clear();note_max_draw(*ptabfirst,*ppagefirst,*ppsel,cx,cy,top);key=0;continue;}
  if((mb&1)&&my==24){int pg=-1,i,xx=4;for(i=0;i<(int)pages[ctab];i++){int pw=page_token_width(i);if(mx>=xx&&mx<xx+pw){pg=i;break;}xx+=pw+1;}if(pg>=0&&pg!=cpage){page_save();cpage=*ppsel=pg;page_load();cx=cy=top=0;note_selection_clear();}note_max_draw(*ptabfirst,*ppagefirst,*ppsel,cx,cy,top);key=0;continue;}
  ep=external_page_index();oldpos=cy*EW+cx;shift=note_shift_down();
  if(key==3){note_copy_selection();key=0;}
  else if(key==24){note_copy_selection();note_delete_selection(&cx,&cy);if(ep>=0)external_dirty[ep]=1;else persistent_dirty=1;}
  else if(key==22||key==16){note_paste(&cx,&cy,insert);if(ep>=0)external_dirty[ep]=1;else persistent_dirty=1;}
  else if(key==256+0x77){cy=0;cx=0;note_selection_clear();}
  else if(key==256+0x75){cy=NL-1;while(cy>0&&note_used(cy)==0)cy--;cx=note_used(cy);if(cx>=EW)cx=EW-1;note_selection_clear();}
  else if(key==256+75&&cx>0){cx--;note_selection_move(oldpos,cy*EW+cx,shift);}
  else if(key==256+77&&cx<EW-1){cx++;note_selection_move(oldpos,cy*EW+cx,shift);}
  else if(key==256+72&&cy>0){cy--;note_selection_move(oldpos,cy*EW+cx,shift);}
  else if(key==256+80&&cy<NL-1){cy++;note_selection_move(oldpos,cy*EW+cx,shift);}
  else if(key==256+71){cx=0;note_selection_move(oldpos,cy*EW+cx,shift);}
  else if(key==256+79){cx=note_used(cy);if(cx>=EW)cx=EW-1;note_selection_move(oldpos,cy*EW+cx,shift);}
  else if(key==256+73){cy-=23;if(cy<0)cy=0;note_selection_move(oldpos,cy*EW+cx,shift);}
  else if(key==256+81){cy+=23;if(cy>=NL)cy=NL-1;note_selection_move(oldpos,cy*EW+cx,shift);}
  else if(key==256+82){note_selection_clear();insert=!insert;}
  else if(key==256+83){if(note_has_selection())note_delete_selection(&cx,&cy);else if(cx>=note_used(cy))note_join_next(&cx,&cy);else{base=cy*NW;memmove(note+base+cx,note+base+cx+1,EW-cx-1);note[base+EW-1]=' ';}if(ep>=0)external_dirty[ep]=1;else persistent_dirty=1;}
  else if(key==8){if(note_has_selection())note_delete_selection(&cx,&cy);else if(cx>0){cx--;base=cy*NW;memmove(note+base+cx,note+base+cx+1,EW-cx-1);note[base+EW-1]=' ';}else note_join_previous(&cx,&cy);if(ep>=0)external_dirty[ep]=1;else persistent_dirty=1;}
  else if(key==13&&cy<NL-1){if(note_has_selection())note_delete_selection(&cx,&cy);note_split_line(&cx,&cy);note_selection_clear();if(ep>=0)external_dirty[ep]=1;else persistent_dirty=1;}
  else if((key>=32&&key<=255)||(key>=513&&key<=767)){if(note_has_selection())note_delete_selection(&cx,&cy);ch=key>=512?key-512:key;note_insert_one(&cx,&cy,ch,insert);note_selection_clear();if(ep>=0)external_dirty[ep]=1;else persistent_dirty=1;}
  else if(key!=27&&key!=256+0x85&&key!=256+0x57)key=0;
  if(cy<top)top=cy;if(cy>=top+23)top=cy-22;if(key!=27&&key!=256+0x85&&key!=256+0x57)note_max_draw(*ptabfirst,*ppagefirst,*ppsel,cx,cy,top);
 }
 *pcx=cx;*pcy=cy;*ptop=top;*pinsert=insert;note_view_rows=NV;note_editor_focus=0;acc_caret_hide();acc_restore_text_screen();acc_mouse_display(1);
}

void acc_tooltip_region(int x,int y,int w,const char *text,int active);
void acc_tooltip_clear_regions(void);
int main(int argc,char**argv)
{
 int x,y,tx,ty,cx=0,cy=0,top=0,key=0,mx=0,my=0,focus=-1,tabfirst=0,pagefirst=0,psel=0,ch,base,oldcy,oldtop,insert=1,done=0,choice;unsigned mb=0;
 char nm[TABNAME+1],exp[ACC_PATH],msg[ACC_PATH+24];
 if(acc_help(argc,argv,"!NOTE","Tabbed, paged persistent note editor."))return 0;if(!acc_begin(argv[0],"Note",0))return 1;
 x=(acc_cols-70)/2;y=(acc_rows-21)/2;tx=x+3;ty=y+3;meta_load();external_setup(argc,argv);psel=cpage;pagefirst=(psel>=10)?psel-9:0;page_load();acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);
 while(!done){int last;
  /* Tight toolbar. Separators consume no surrounding spacer cells. */
  acc_tooltip_clear_regions();acc_button(x+3,y+18,"  Add  ",focus==3);if(note_can_delete())acc_button(x+11,y+18,"  Delete  ",focus==4);else acc_button_disabled(x+11,y+18,"  Delete  ");
  acc_tooltip_region(x+62,y,2,"Maximize (F11)",1);
  /* Canonical toolbar geometry: shadow, one clear cell, separator, one clear cell. */
  acc_put(x+19,y+18,179,ACC_BORDER);acc_button(x+21,y+18,"  Clr  ",focus==5);acc_button(x+30,y+18,"  Chars  ",focus==6);
  acc_put(x+40,y+18,179,ACC_BORDER);if(external_page_index()>=0)acc_button(x+42,y+18,"   Save   ",focus==7);else acc_button(x+42,y+18,"   Save   ",focus==7);acc_button(x+50,y+18,"  Print  ",focus==8);acc_button(x+60,y+18,"  Exit  ",focus==9);
  acc_tooltip_clear_regions();acc_tooltip_region(x+11,y+18,6,pages[ctab]>1?"Delete current page":"Delete current tab",note_can_delete());
  {int ti,ttx=x+6,lastp=tab_last_visible(tabfirst),px=tx+4;for(ti=tabfirst;ti<=lastp&&ti<ntab;ti++){int tw=(int)strlen(tabs[ti])+4;acc_tooltip_region(ttx,y+2,tw,(ti==external_tab&&external_count>0)?"Temporary external files":"Right click to rename",1);ttx+=tw+1;}lastp=page_last_visible(pagefirst);if(ctab==external_tab){for(ti=0;ti<=lastp&&ti<external_count;ti++){int pw=page_token_width(ti);acc_tooltip_region(page_token_x(px,ti),ty+NV,pw,external_path[ti],1);}}else if((int)pages[ctab]<MAXPAGES){int ax=page_token_x(px,pages[ctab]);acc_tooltip_region(ax,ty+NV,1,"Add page",1);}}
  acc_wait(&key,&mx,&my,&mb);
  if((key==256+0x85||key==256+0x57)||((mb&1)&&my==y&&mx>=x+62&&mx<x+64)){note_maximize(&cx,&cy,&top,&tabfirst,&pagefirst,&psel,&insert);acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);key=0;continue;}
  if(mb&ACC_MOUSE_MOVED){note_tab_hover=-1;if(my==y+2){int hi,hx=x+6,hl=tab_last_visible(tabfirst);for(hi=tabfirst;hi<=hl&&hi<ntab;hi++){int hw=(int)strlen(tabs[hi])+4;if(mx>=hx&&mx<hx+hw){note_tab_hover=hi;break;}hx+=hw+1;}}draw_tabs(x,y+2,tabfirst,focus==0);key=0;continue;}
  if((mb&2)&&my==y+2){int ri,rx=x+6,rl=tab_last_visible(tabfirst);for(ri=tabfirst;ri<=rl&&ri<ntab;ri++){int rw=(int)strlen(tabs[ri])+4;if(mx>=rx&&mx<rx+rw){if(ri==external_tab&&external_count>0){key=0;break;}strcpy(nm,tabs[ri]);if(input_name(nm,1)){strcpy(tabs[ri],nm);meta_save();}acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);key=0;break;}rx+=rw+1;}if(ri<=rl)continue;}
  if(mb&1){
   if(my==y+2){last=tab_last_visible(tabfirst);if(mx==x+3&&tabfirst>0){tabfirst--;draw_tabs(x,y+2,tabfirst,focus==0);key=0;continue;}if(mx==x+65&&last+1<ntab){tabfirst++;draw_tabs(x,y+2,tabfirst,focus==0);key=0;continue;}{int i,xx=x+6;for(i=tabfirst;i<=last&&i<ntab;i++){int w=(int)strlen(tabs[i])+4;if(mx>=xx&&mx<xx+w){page_save();note_selection_clear();ctab=i;cpage=psel=0;pagefirst=0;page_load();cx=cy=top=0;focus=0;note_editor_focus=0;row_draw(tx,ty,cy,top);draw_tabs(x,y+2,tabfirst,1);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);key=0;break;}xx+=w+1;}if(i<=last)continue;}}
   if(mx==tx+4+EW&&my>=ty&&my<ty+NV){if(my==ty&&top>0)top--;else if(my==ty+NV-1&&top+NV<=last_note_line())top++;cy=top;cx=0;focus=1;note_editor_focus=1;view_draw(tx,ty,cx,cy,top);key=0;continue;}
   if(my>=ty&&my<ty+NV&&mx>=tx+4&&mx<tx+4+EW){int ocx=cx,ocy=cy;focus=1;note_editor_focus=1;note_selection_clear();cx=mx-(tx+4);cy=top+my-ty;row_draw(tx,ty,ocy,top);if(cy!=ocy)row_draw(tx,ty,cy,top);cursor_draw(tx,ty,cx,cy,top);key=0;continue;}
   if(my==ty+NV&&mx>=tx+4&&mx<tx+4+EW+1){int pg=-1,i,px=tx+4,xx=px;focus=2;note_editor_focus=0;row_draw(tx,ty,cy,top);for(i=0;i<(int)pages[ctab];i++){int pw=page_token_width(i);if(mx>=xx&&mx<xx+pw){pg=i;break;}xx+=pw+1;}if(pg>=0){page_save();note_selection_clear();cpage=psel=pg;page_load();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,1,psel);key=0;continue;}if(ctab!=external_tab&&(int)pages[ctab]<MAXPAGES&&mx==xx){page_save();pages[ctab]++;cpage=psel=pages[ctab]-1;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));meta_save();page_save();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,1,psel);key=0;continue;}}
   if(my==y+18){note_editor_focus=0;row_draw(tx,ty,cy,top);if(mx>=x+2&&mx<x+8){focus=3;key=13;}else if(note_can_delete()&&mx>=x+11&&mx<x+17){focus=4;key=13;}else if(mx>=x+20&&mx<x+27){focus=5;key=13;}else if(mx>=x+29&&mx<x+38){focus=6;key=13;}else if(mx>=x+42&&mx<x+48){focus=7;key=13;}else if(mx>=x+50&&mx<x+56){focus=8;key=13;}else if(mx>=x+60&&mx<x+66){focus=9;key=13;}}
  }
  if(key==9||key==271){focus=note_next_focus(focus,key==271);note_editor_focus=(focus==1);if(focus!=1)note_selection_clear();row_draw(tx,ty,cy,top);cursor_draw(tx,ty,cx,cy,top);key=0;draw_tabs(x,y+2,tabfirst,focus==0);draw_pages(tx+4,ty+NV,pagefirst,focus==2,psel);continue;}
  if(focus==0&&key==13){if(ctab==external_tab&&external_count>0){key=0;continue;}strcpy(nm,tabs[ctab]);if(input_name(nm,1)){strcpy(tabs[ctab],nm);meta_save();}acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,1);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);key=0;continue;}
  if(focus==0&&(key==256+75||key==256+77||key==' ')){page_save();note_selection_clear();if(key==256+75&&ctab>0)ctab--;else if(key==256+77&&ctab+1<ntab)ctab++;if(ctab<tabfirst)tabfirst=ctab;last=tab_last_visible(tabfirst);while(ctab>last){tabfirst++;last=tab_last_visible(tabfirst);}cpage=psel=pagefirst=0;page_load();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);draw_tabs(x,y+2,tabfirst,1);draw_pages(tx+4,ty+NV,pagefirst,0,psel);key=0;continue;}
  if(focus==2&&(key==256+75||key==256+77||key==13||key==' ')){if(key==256+75&&psel>0)psel--;else if(key==256+77&&psel<(int)pages[ctab]-(ctab==external_tab?1:0))psel++;else if(ctab!=external_tab&&(key==13||key==' ')&&psel==(int)pages[ctab]&&(int)pages[ctab]<MAXPAGES){page_save();pages[ctab]++;cpage=psel=pages[ctab]-1;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));meta_save();page_save();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);}else if((key==13||key==' ')&&psel<(int)pages[ctab]&&psel!=cpage){page_save();note_selection_clear();cpage=psel;page_load();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);}draw_pages(tx+4,ty+NV,pagefirst,1,psel);key=0;continue;}
  if(key==13&&focus>=3){
   if(focus==3&&ntab<MAXTABS){
     int made=input_name(nm,0);
     /* New Tab is modal but does not maintain a backing-store.  Always
        reconstruct the parent after OK, Cancel, titlebar Close or Escape. */
     if(made){page_save();note_selection_clear();if(external_runtime_tab){strcpy(tabs[external_tab+1],tabs[external_tab]);pages[external_tab+1]=pages[external_tab];strcpy(tabs[external_tab],nm);pages[external_tab]=1;ctab=external_tab;external_tab++;ntab++;persistent_ntab++;}else{strcpy(tabs[ntab],nm);pages[ntab]=1;ctab=ntab++;persistent_ntab++;}cpage=psel=pagefirst=0;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));meta_save();page_save();while(ctab>tab_last_visible(tabfirst))tabfirst++;cx=cy=top=0;}
     acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);
   }
   else if(focus==4){
     if(persistent_pages_for(ctab)>1){
       if(confirm_remove("Remove the current page?")){
         int oldpages=persistent_pages_for(ctab);page_save();note_remove_page_files(ctab,cpage,oldpages);pages[ctab]--;
         if(cpage>=(int)pages[ctab])cpage=pages[ctab]-1;psel=cpage;if(pagefirst>cpage)pagefirst=cpage;
         meta_save();page_load();cx=cy=top=0;acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);
       } else {acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);}
     } else if(persistent_ntab>1){
       if(confirm_remove("Remove the current tab?")){
         int i;page_save();note_remove_tab_files(ctab);for(i=ctab;i<ntab-1;i++){strcpy(tabs[i],tabs[i+1]);pages[i]=pages[i+1];}
         ntab--;persistent_ntab--;if(external_tab>ctab)external_tab--;if(ctab>=ntab)ctab=ntab-1;cpage=psel=pagefirst=0;meta_save();page_load();cx=cy=top=0;
         if(tabfirst>=ntab)tabfirst=ntab-1;if(tabfirst<0)tabfirst=0;
         acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);
       } else {acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);}
     }
   }
   else if(focus==5){memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));if(external_page_index()>=0)external_dirty[external_page_index()]=1;else persistent_dirty=1;view_draw(tx,ty,0,0,0);draw_pages(tx+4,ty+NV,pagefirst,0,psel);}
   else if(focus==6){ch=character_palette();acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);if(ch>=0){if(note_has_selection())note_delete_selection(&cx,&cy);note[cy*NW+cx]=(char)ch;if(cx<EW-1)cx++;note_selection_clear();if(external_page_index()>=0)external_dirty[external_page_index()]=1;else persistent_dirty=1;}view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);}
   else if(focus==7){int ep=external_page_index();if(ep>=0){if(confirm_external_save(external_path[ep])){if(external_write_original(ep)){sprintf(msg,"Saved to\n%s",external_path[ep]);acc_notice("Save",msg);}else acc_notice("Save Error","Unable to save opened file.");}acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);}else if(export_current_page(exp)){sprintf(msg,"Page saved to\n%s",exp);acc_notice("Save",msg);}}
   else if(focus==8){FILE*f=fopen("LPT1","wb");int r,q;if(f){for(r=0;r<NL;r++){q=NW;while(q&&note[r*NW+q-1]==' ')q--;fwrite(note+r*NW,1,q,f);fputs("\r\n",f);}fputc('\f',f);fclose(f);}}
   else if(focus==9){if(!note_any_dirty())done=1;else{choice=note_confirm_close();if(choice==1){note_discard=1;done=1;}else if(choice==2&&note_save_all_dirty())done=1;else{acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);}}}key=0;continue;
  }
  if(focus!=1){if(key==27){if(!note_any_dirty())done=1;else{choice=note_confirm_close();if(choice==1){note_discard=1;done=1;}else if(choice==2&&note_save_all_dirty())done=1;else{acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);}}}key=0;continue;}
  oldcy=cy;oldtop=top;{int oldpos=cy*EW+cx,shift=note_shift_down(),ep=external_page_index();if(key==24||key==22||key==16||key==256+83||key==8||key==13||(key>=32&&key<=255)||(key>=513&&key<=767)){if(ep>=0)external_dirty[ep]=1;else persistent_dirty=1;}
   if(key==3){note_copy_selection();key=0;}
   else if(key==256+0x77){cy=0;cx=0;note_selection_clear();}
   else if(key==256+0x75){cy=NL-1;while(cy>0&&note_used(cy)==0)cy--;cx=note_used(cy);if(cx>=EW)cx=EW-1;note_selection_clear();}
   else if(key==24){note_copy_selection();note_delete_selection(&cx,&cy);oldtop=-1;key=0;}
   else if((key==22||key==16)){note_paste(&cx,&cy,insert);oldtop=-1;key=0;}
   else if(key==256+75&&cx>0){cx--;note_selection_move(oldpos,cy*EW+cx,shift);}
   else if(key==256+77&&cx<EW-1){cx++;note_selection_move(oldpos,cy*EW+cx,shift);}
   else if(key==256+72&&cy>0){cy--;note_selection_move(oldpos,cy*EW+cx,shift);}
   else if(key==256+80&&cy<NL-1){cy++;note_selection_move(oldpos,cy*EW+cx,shift);}
   else if(key==256+71){cx=0;note_selection_move(oldpos,cy*EW+cx,shift);}
   else if(key==256+79){cx=EW-1;while(cx>0&&note[cy*NW+cx]==' ')cx--;if(note[cy*NW+cx]!=' '&&cx<EW-1)cx++;note_selection_move(oldpos,cy*EW+cx,shift);}
   else if(key==256+73){cy-=NV;if(cy<0)cy=0;note_selection_move(oldpos,cy*EW+cx,shift);}
   else if(key==256+81){cy+=NV;if(cy>=NL)cy=NL-1;note_selection_move(oldpos,cy*EW+cx,shift);}
   else if(key==256+82){note_selection_clear();insert=!insert;}
   else if(key==256+83){if(note_has_selection()){note_delete_selection(&cx,&cy);oldtop=-1;}else if(cx>=note_used(cy))note_join_next(&cx,&cy);else{base=cy*NW;memmove(note+base+cx,note+base+cx+1,EW-cx-1);note[base+EW-1]=' ';}}
   else if(key==8){if(note_has_selection()){note_delete_selection(&cx,&cy);oldtop=-1;}else if(cx>0){cx--;base=cy*NW;memmove(note+base+cx,note+base+cx+1,EW-cx-1);note[base+EW-1]=' ';}else note_join_previous(&cx,&cy);}
   else if(key==13&&cy<NL-1){if(note_has_selection()){note_delete_selection(&cx,&cy);oldtop=-1;}note_split_line(&cx,&cy);note_selection_clear();}
   else if((key>=32&&key<=255)||(key>=513&&key<=767)){if(note_has_selection()){note_delete_selection(&cx,&cy);oldtop=-1;}ch=key>=512?key-512:key;note_insert_one(&cx,&cy,ch,insert);note_selection_clear();}
   else if(key==27){if(!note_any_dirty())done=1;else{choice=note_confirm_close();if(choice==1){note_discard=1;done=1;}else if(choice==2&&note_save_all_dirty())done=1;else{acc_box(x,y,70,21,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(tx+4,ty+NV,pagefirst,0,psel);}}key=0;}else key=0;
  }
  if(note_sel_repaint){oldtop=-1;note_sel_repaint=0;}
  if(cy<top)top=cy;if(cy>=top+NV)top=cy-NV+1;if(top<0)top=0;if(top>NL-NV)top=NL-NV;
  /* A changed viewport must repaint every row.  Previously old line numbers and
     text survived a scroll because only the cursor rows were refreshed. */
  if(top!=oldtop)view_draw(tx,ty,cx,cy,top);else{
   row_draw(tx,ty,oldcy,top);if(cy!=oldcy)row_draw(tx,ty,cy,top);
   /* If wrapping state changed, the following physical row's logical number
      changes immediately too. */
   if(cy+1<NL)row_draw(tx,ty,cy+1,top);
   cursor_draw(tx,ty,cx,cy,top);acc_fill(tx+4+EW,ty,1,NV,' ',ACC_CONTROL);if(top>0)acc_put(tx+4+EW,ty,30,ACC_CONTROL);if(top+NV<=last_note_line())acc_put(tx+4+EW,ty+NV-1,31,ACC_CONTROL);
  }draw_pages(tx+4,ty+NV,pagefirst,focus==2,psel);key=0;
 }
 if(!note_discard){page_save();meta_save();}external_cleanup();acc_end();return 0;
}
