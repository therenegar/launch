/* Launch! Note - tabbed/page notebook accessory. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ACCLIB.H"
#define NW 60
#define EW 56
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
static void data_name(char *n,int t,int p){sprintf(n,"N%02dP%02d.DAT",t,p);}
static void wrap_name(char *n,int t,int p){sprintf(n,"N%02dP%02d.WRP",t,p);}
static void meta_save(void){char p[ACC_PATH];FILE*f;int i;acc_path(p,"DATA","NOTE.IDX");f=fopen(p,"wb");if(!f)return;fwrite(&ntab,1,sizeof(ntab),f);for(i=0;i<ntab;i++){fwrite(tabs[i],1,TABNAME+1,f);fwrite(&pages[i],1,1,f);}fclose(f);}
static void meta_load(void){char p[ACC_PATH];FILE*f;int i;acc_path(p,"DATA","NOTE.IDX");f=fopen(p,"rb");if(f){fread(&ntab,1,sizeof(ntab),f);if(ntab<0||ntab>MAXTABS)ntab=0;for(i=0;i<ntab;i++){fread(tabs[i],1,TABNAME+1,f);fread(&pages[i],1,1,f);if(pages[i]<1)pages[i]=1;if(pages[i]>MAXPAGES)pages[i]=MAXPAGES;}fclose(f);}if(!ntab){ntab=1;strcpy(tabs[0],"Notes");pages[0]=1;meta_save();}}
static void page_load(void){char p[ACC_PATH],n[20];FILE*f;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));if(!pages[ctab])return;data_name(n,ctab,cpage);acc_path(p,"DATA",n);f=fopen(p,"rb");if(f){fread(note,1,sizeof(note),f);fclose(f);}wrap_name(n,ctab,cpage);acc_path(p,"DATA",n);f=fopen(p,"rb");if(f){fread(softwrap,1,sizeof(softwrap),f);fclose(f);}}
static void page_save(void){char p[ACC_PATH],n[20];FILE*f;if(!pages[ctab])return;data_name(n,ctab,cpage);acc_path(p,"DATA",n);f=fopen(p,"wb");if(f){fwrite(note,1,sizeof(note),f);fclose(f);}wrap_name(n,ctab,cpage);acc_path(p,"DATA",n);f=fopen(p,"wb");if(f){fwrite(softwrap,1,sizeof(softwrap),f);fclose(f);}}
static int logical_number(int line){int i,n=0;for(i=0;i<=line;i++)if(i==0||!softwrap[i])n++;return n;}
static void row_draw(int x,int y,int line,int top){char b[5];int a=ACC_ATTR(acc_appearance.controls_bg,(acc_appearance.controls_fg&7)|8);if(line>=top&&line<top+NV){acc_fill(x,y+line-top,4,1,' ',ACC_CONTROL);if(line==0||!softwrap[line]){sprintf(b,"%3d",logical_number(line));acc_text(x,y+line-top,b,a,3);}acc_text(x+4,y+line-top,note+line*NW,ACC_CONTROL,EW);}}
static void cursor_draw(int x,int y,int cx,int cy,int top){if(cy>=top&&cy<top+NV)acc_put(x+4+cx,y+cy-top,note[cy*NW+cx],ACC_SELECT);}
static int last_note_line(void){int l,q;for(l=NL-1;l>0;l--){for(q=0;q<EW;q++)if(note[l*NW+q]!=' ')return l;}return 0;}
static void view_draw(int x,int y,int cx,int cy,int top){int r,last=last_note_line();for(r=0;r<NV;r++)row_draw(x,y,top+r,top);cursor_draw(x,y,cx,cy,top);acc_fill(x+4+EW,y,1,NV,' ',ACC_CONTROL);if(top>0)acc_put(x+4+EW,y,30,ACC_CONTROL);if(top+NV<=last)acc_put(x+4+EW,y+NV-1,31,ACC_CONTROL);}
static int input_name(char *out)
{
 int w=38,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=0,focus=0;unsigned b=0;out[0]=0;
 for(;;){
  /* Spawned dialogs reuse the established Add/Edit form treatment. */
  acc_subbox(x,y,w,h,"New tab",1);
  acc_text(x+3,y+2,"Name:",ACC_LABEL,5);
  acc_fill(x+9,y+2,20,1,' ',focus==0?ACC_SELECT:ACC_CONTROL);acc_text(x+9,y+2,out,focus==0?ACC_SELECT:ACC_CONTROL,pos);
  acc_button(x+2,y+6,"  OK  ",focus==1);acc_button(x+10,y+6,"  Cancel  ",focus==2);acc_wait(&k,&mx,&my,&b);
  if((b&1)&&my==y+2&&mx>=x+9&&mx<x+29){focus=0;k=0;continue;}
  if((b&1)&&my==y+6){if(mx>=x+2&&mx<x+9){focus=1;k=13;}else if(mx>=x+10&&mx<x+21){focus=2;k=13;}}
  if(k==9||k==271){focus=(k==271)?(focus+2)%3:(focus+1)%3;k=0;continue;}if(k==27)return 0;
  if((k==13||k==' ')&&focus==1){if(pos){out[pos]=0;return 1;}k=0;continue;}if((k==13||k==' ')&&focus==2)return 0;
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
 for(t=tab+1;t<ntab;t++)for(p=0;p<(int)pages[t];p++){
  data_name(n1,t,p);data_name(n2,t-1,p);acc_path(a,"DATA",n1);acc_path(b,"DATA",n2);remove(b);rename(a,b);
  wrap_name(n1,t,p);wrap_name(n2,t-1,p);acc_path(a,"DATA",n1);acc_path(b,"DATA",n2);remove(b);rename(a,b);
 }
}

static int note_can_delete(void)
{
 return pages[ctab]>1 || ntab>1;
}

static int note_next_focus(int focus,int backwards)
{
 int next=focus,tries;
 for(tries=0;tries<10;tries++){
  next=backwards?(next+9)%10:(next+1)%10;
  if(next!=4||note_can_delete())return next;
 }
 return focus;
}

static int export_all(char *out){char p[ACC_PATH],name[20];FILE*f,*in;int n,t,pg,r,last;for(n=1;n<10000;n++){sprintf(name,"NOTE%d.TXT",n);acc_path(p,"EXPORT",name);if(!acc_exists(p))break;}if(n==10000)return 0;strcpy(out,p);f=fopen(p,"w");if(!f)return 0;page_save();for(t=0;t<ntab;t++){fprintf(f,"=== %s ===\n",tabs[t]);for(pg=0;pg<(int)pages[t];pg++){fprintf(f,"--- Page %d ---\n",pg+1);data_name(name,t,pg);acc_path(p,"DATA",name);in=fopen(p,"rb");if(in){fread(note,1,sizeof(note),in);fclose(in);last=NL-1;while(last>=0){r=NW;while(r&&note[last*NW+r-1]==' ')r--;if(r)break;last--;}for(r=0;r<=last;r++){int q=NW;while(q&&note[r*NW+q-1]==' ')q--;fwrite(note+r*NW,1,q,f);fputc('\n',f);}}fputs("------------------------------------------------------------\n",f);}}fclose(f);page_load();return 1;}
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


static int tab_last_visible(int first)
{
 int i,xx=6;for(i=first;i<ntab;i++){int w=(int)strlen(tabs[i])+4;if(xx+w>64)break;xx+=w+1;}return i-1;
}
static void draw_tabs(int x,int y,int first,int focus)
{
 int i,xx=x+6,last=tab_last_visible(first);acc_fill(x+1,y,65,1,' ',ACC_BG);
 /* Mini navigation buttons only exist when there is something to scroll to. */
 if(first>0)acc_put(x+3,y,17,ACC_BORDER);if(last+1<ntab)acc_put(x+65,y,16,ACC_BORDER);
 for(i=first;i<=last&&i<ntab;i++){int a=ACC_ATTR(acc_appearance.controls_bg,(acc_appearance.controls_fg&7)|8),w=(int)strlen(tabs[i])+4;if(i==ctab)a=ACC_ATTR(acc_appearance.controls_bg,acc_appearance.titles);if(focus&&i==ctab)a=ACC_SELECT;acc_fill(xx,y,w,1,' ',a);acc_text(xx+2,y,tabs[i],a,(int)strlen(tabs[i]));xx+=w+1;}
}
static int page_last_visible(int first){int n=pages[ctab]-first;if(n>10)n=10;return first+n-1;}
static void draw_pages(int x,int y,int first,int focus,int sel)
{
 int i,last=page_last_visible(first);char b[4];acc_fill(x,y,3,NV,' ',ACC_BG);
 if(first>0)acc_put(x,y,30,ACC_BORDER); /* filled up triangle */
 for(i=first;i<=last;i++){sprintf(b,"%d",i+1);acc_text(x,y+1+i-first,b,(focus&&i==sel)?ACC_SELECT:ACC_BORDER,(int)strlen(b));}
 /* '+' follows the last visible page when there is room. */
 if((int)pages[ctab]<MAXPAGES&&last-first+1<10)acc_put(x,y+1+(last>=first?last-first+1:0),'+',(focus&&sel==(int)pages[ctab])?ACC_SELECT:ACC_BORDER);
 if(last+1<(int)pages[ctab])acc_put(x,y+11,31,ACC_BORDER); /* filled down triangle */
}
int main(int argc,char**argv)
{
 int x,y,tx,ty,cx=0,cy=0,top=0,key=0,mx=0,my=0,focus=1,tabfirst=0,pagefirst=0,psel=0,ch,base,oldcy,oldtop,insert=1;unsigned mb=0;
 char nm[TABNAME+1],exp[ACC_PATH],msg[ACC_PATH+24];
 if(acc_help(argc,argv,"!NOTE","Tabbed, paged persistent note editor."))return 0;if(!acc_begin(argv[0],"Note",0))return 1;
 x=(acc_cols-70)/2;y=(acc_rows-20)/2;tx=x+3;ty=y+3;meta_load();page_load();acc_box(x,y,70,20,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,0,psel);
 while(key!=27){int last;
  /* Tight toolbar. Separators consume no surrounding spacer cells. */
  acc_button(x+3,y+17,"  Add  ",focus==3);if(note_can_delete())acc_button(x+11,y+17,"  Delete  ",focus==4);else acc_button_disabled(x+11,y+17,"  Delete  ");
  /* Canonical toolbar geometry: shadow, one clear cell, separator, one clear cell. */
  acc_put(x+19,y+17,179,ACC_BORDER);acc_button(x+21,y+17,"  Clr  ",focus==5);acc_button(x+30,y+17,"  Chars  ",focus==6);
  acc_put(x+40,y+17,179,ACC_BORDER);acc_button(x+42,y+17,"  Export  ",focus==7);acc_button(x+50,y+17,"  Print  ",focus==8);acc_button(x+60,y+17,"  Close  ",focus==9);
  acc_wait(&key,&mx,&my,&mb);
  if(mb&ACC_MOUSE_MOVED){key=0;continue;}
  if(mb&1){
   if(my==y+2){last=tab_last_visible(tabfirst);if(mx==x+3&&tabfirst>0){tabfirst--;draw_tabs(x,y+2,tabfirst,focus==0);key=0;continue;}if(mx==x+65&&last+1<ntab){tabfirst++;draw_tabs(x,y+2,tabfirst,focus==0);key=0;continue;}{int i,xx=x+6;for(i=tabfirst;i<=last&&i<ntab;i++){int w=(int)strlen(tabs[i])+4;if(mx>=xx&&mx<xx+w){page_save();ctab=i;cpage=psel=0;pagefirst=0;page_load();cx=cy=top=0;focus=0;draw_tabs(x,y+2,tabfirst,1);view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,0,psel);key=0;break;}xx+=w+1;}if(i<=last)continue;}}
   if(mx==tx+4+EW&&my>=ty&&my<ty+NV){if(my==ty&&top>0)top--;else if(my==ty+NV-1&&top+NV<=last_note_line())top++;cy=top;cx=0;focus=1;view_draw(tx,ty,cx,cy,top);key=0;continue;}
   if(my>=ty&&my<ty+NV&&mx>=tx+4&&mx<tx+4+EW){int ocx=cx,ocy=cy;focus=1;cx=mx-(tx+4);cy=top+my-ty;row_draw(tx,ty,ocy,top);if(cy!=ocy)row_draw(tx,ty,cy,top);cursor_draw(tx,ty,cx,cy,top);key=0;continue;}
   if(mx>=x+65&&mx<x+68&&my>=ty&&my<ty+NV){int rel=my-ty,lastp=page_last_visible(pagefirst);focus=2;if(rel==0&&pagefirst>0){pagefirst--;draw_pages(x+65,ty,pagefirst,1,psel);key=0;continue;}if(rel==11&&lastp+1<(int)pages[ctab]){pagefirst++;draw_pages(x+65,ty,pagefirst,1,psel);key=0;continue;}if(rel>=1&&rel<=10){int pg=pagefirst+rel-1;if(pg<(int)pages[ctab]){page_save();cpage=psel=pg;page_load();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,1,psel);key=0;continue;}if(pg==(int)pages[ctab]&&(int)pages[ctab]<MAXPAGES){page_save();pages[ctab]++;cpage=psel=pages[ctab]-1;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));meta_save();page_save();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,1,psel);key=0;continue;}}}
   if(my==y+17){if(mx>=x+2&&mx<x+8){focus=3;key=13;}else if(note_can_delete()&&mx>=x+11&&mx<x+17){focus=4;key=13;}else if(mx>=x+20&&mx<x+27){focus=5;key=13;}else if(mx>=x+29&&mx<x+38){focus=6;key=13;}else if(mx>=x+42&&mx<x+48){focus=7;key=13;}else if(mx>=x+50&&mx<x+56){focus=8;key=13;}else if(mx>=x+60&&mx<x+66){focus=9;key=13;}}
  }
  if(key==9||key==271){focus=note_next_focus(focus,key==271);key=0;draw_tabs(x,y+2,tabfirst,focus==0);draw_pages(x+65,ty,pagefirst,focus==2,psel);continue;}
  if(focus==0&&(key==256+75||key==256+77||key==13||key==' ')){page_save();if(key==256+75&&ctab>0)ctab--;else if(key==256+77&&ctab+1<ntab)ctab++;if(ctab<tabfirst)tabfirst=ctab;last=tab_last_visible(tabfirst);while(ctab>last){tabfirst++;last=tab_last_visible(tabfirst);}cpage=psel=pagefirst=0;page_load();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);draw_tabs(x,y+2,tabfirst,1);draw_pages(x+65,ty,pagefirst,0,psel);key=0;continue;}
  if(focus==2&&(key==256+72||key==256+80||key==13||key==' ')){if(key==256+72&&psel>0)psel--;else if(key==256+80&&psel<(int)pages[ctab])psel++;else if((key==13||key==' ')&&psel==(int)pages[ctab]&&(int)pages[ctab]<MAXPAGES){page_save();pages[ctab]++;cpage=psel=pages[ctab]-1;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));meta_save();page_save();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);}else if((key==13||key==' ')&&psel<(int)pages[ctab]&&psel!=cpage){page_save();cpage=psel;page_load();cx=cy=top=0;view_draw(tx,ty,cx,cy,top);}if(psel<pagefirst)pagefirst=psel;if(psel>=pagefirst+10)pagefirst=psel-9;draw_pages(x+65,ty,pagefirst,1,psel);key=0;continue;}
  if((key==13||key==' ')&&focus>=3){
   if(focus==3&&ntab<MAXTABS){
     int made=input_name(nm);
     /* New Tab is modal but does not maintain a backing-store.  Always
        reconstruct the parent after OK, Cancel, titlebar Close or Escape. */
     if(made){page_save();strcpy(tabs[ntab],nm);pages[ntab]=1;ctab=ntab++;cpage=psel=pagefirst=0;memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));meta_save();page_save();while(ctab>tab_last_visible(tabfirst))tabfirst++;cx=cy=top=0;}
     acc_box(x,y,70,20,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,0,psel);
   }
   else if(focus==4){
     if(pages[ctab]>1){
       if(confirm_remove("Remove the current page?")){
         int oldpages=pages[ctab];page_save();note_remove_page_files(ctab,cpage,oldpages);pages[ctab]--;
         if(cpage>=(int)pages[ctab])cpage=pages[ctab]-1;psel=cpage;if(pagefirst>cpage)pagefirst=cpage;
         meta_save();page_load();cx=cy=top=0;acc_box(x,y,70,20,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,0,psel);
       } else {acc_box(x,y,70,20,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,0,psel);}
     } else if(ntab>1){
       if(confirm_remove("Remove the current tab?")){
         int i;page_save();note_remove_tab_files(ctab);for(i=ctab;i<ntab-1;i++){strcpy(tabs[i],tabs[i+1]);pages[i]=pages[i+1];}
         ntab--;if(ctab>=ntab)ctab=ntab-1;cpage=psel=pagefirst=0;meta_save();page_load();cx=cy=top=0;
         if(tabfirst>=ntab)tabfirst=ntab-1;if(tabfirst<0)tabfirst=0;
         acc_box(x,y,70,20,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,0,psel);
       } else {acc_box(x,y,70,20,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,0,psel);}
     }
   }
   else if(focus==5){memset(note,' ',sizeof(note));memset(softwrap,0,sizeof(softwrap));view_draw(tx,ty,0,0,0);}
   else if(focus==6){ch=character_palette();acc_box(x,y,70,20,"Note");draw_tabs(x,y+2,tabfirst,0);view_draw(tx,ty,cx,cy,top);draw_pages(x+65,ty,pagefirst,0,psel);if(ch>=0){note[cy*NW+cx]=(char)ch;if(cx<EW-1)cx++;}}
   else if(focus==7){if(export_all(exp)){sprintf(msg,"Notebook exported to\n%s",exp);acc_notice("Export",msg);}}
   else if(focus==8){FILE*f=fopen("LPT1","wb");int r,q;if(f){for(r=0;r<NL;r++){q=NW;while(q&&note[r*NW+q-1]==' ')q--;fwrite(note+r*NW,1,q,f);fputs("\r\n",f);}fputc('\f',f);fclose(f);}}
   else if(focus==9)key=27;if(key!=27)key=0;continue;
  }
  if(focus!=1){if(key==27)break;key=0;continue;}
  oldcy=cy;oldtop=top;if(key==256+75&&cx>0)cx--;else if(key==256+77&&cx<EW-1)cx++;else if(key==256+72&&cy>0)cy--;else if(key==256+80&&cy<NL-1)cy++;else if(key==256+71)cx=0;else if(key==256+79){cx=EW-1;while(cx>0&&note[cy*NW+cx]==' ')cx--;if(note[cy*NW+cx]!=' '&&cx<EW-1)cx++;}else if(key==256+73){cy-=NV;if(cy<0)cy=0;}else if(key==256+81){cy+=NV;if(cy>=NL)cy=NL-1;}else if(key==256+82)insert=!insert;else if(key==256+83){base=cy*NW;memmove(note+base+cx,note+base+cx+1,EW-cx-1);note[base+EW-1]=' ';}else if(key==8){if(cx>0)note[cy*NW+--cx]=' ';else if(cy>0&&softwrap[cy]){softwrap[cy]=0;cy--;cx=EW;while(cx>0&&note[cy*NW+cx-1]==' ')cx--;if(cx>0)note[cy*NW+--cx]=' ';}}else if(key==13&&cy<NL-1){cy++;cx=0;softwrap[cy]=0;}else if((key>=32&&key<=255)||(key>=513&&key<=767)){ch=key>=512?key-512:key;base=cy*NW;if(insert&&cx<EW-1)memmove(note+base+cx+1,note+base+cx,EW-cx-1);note[base+cx]=(char)ch;if(cx<EW-1)cx++;else if(cy<NL-1){cx=0;cy++;softwrap[cy]=1;}}else if(key==27)break;else key=0;
  if(cy<top)top=cy;if(cy>=top+NV)top=cy-NV+1;if(top<0)top=0;if(top>NL-NV)top=NL-NV;
  /* A changed viewport must repaint every row.  Previously old line numbers and
     text survived a scroll because only the cursor rows were refreshed. */
  if(top!=oldtop)view_draw(tx,ty,cx,cy,top);else{
   row_draw(tx,ty,oldcy,top);if(cy!=oldcy)row_draw(tx,ty,cy,top);
   /* If wrapping state changed, the following physical row's logical number
      changes immediately too. */
   if(cy+1<NL)row_draw(tx,ty,cy+1,top);
   cursor_draw(tx,ty,cx,cy,top);acc_fill(tx+4+EW,ty,1,NV,' ',ACC_CONTROL);if(top>0)acc_put(tx+4+EW,ty,30,ACC_CONTROL);if(top+NV<=last_note_line())acc_put(tx+4+EW,ty+NV-1,31,ACC_CONTROL);
  }key=0;
 }
 page_save();meta_save();acc_end();return 0;
}
