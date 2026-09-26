/*
    __                           __    __
   / /   ____ ___  ______  _____/ /_  / /
  / /   / __ `/ / / / __ \/ ___/ __ \/ / 
 / /___/ /_/ / /_/ / / / /__/ / / /_/  
/_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
Launch! for DOS ---------------------
*/
/*
 * MAINTAINER NOTES - Launch! 3.73
 * File: DRAW.C
 * Role: !DRAW 16-color bitmap/pixel editor
 * Build/ownership: Canonical Draw source; build copy is DRAWBLD.C.
 * Maintainer contract: Owns 96x96 canvas, BMP import/export, palette, grid/show modes and file lifecycle. Large image buffers should not live on the 16-bit stack.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* Launch! Pixel Draw: scrollable persistent 96 x 96 colour canvas. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include "ACCLIB.H"
extern void acc_input_bounds(int x,int y,int w,int h);
void acc_mouse_display(int show);
void acc_mouse_reapply_cursor(void);
#define DW 30
#define DH 12
#define CW 96
#define CH 96
static unsigned char pixels[CW*CH],oldglyph[2][32];
static char draw_path[ACC_PATH];
/* Large scratch buffers are static: MSC 6 small-model DOS programs have a
   deliberately small runtime stack, and allocating these in main caused R6000
   before the editor could draw its first frame. */
static char draw_small[ACC_PATH],draw_big[ACC_PATH],draw_message[ACC_PATH*2+48];
static char draw_openname[ACC_PATH],draw_openfull[ACC_PATH];
static unsigned char draw_old_pixels[DW*DH];
static unsigned char draw_bmp_rowbuf[512];
static int draw_external=0,draw_dirty=0;
static const unsigned char gridglyph[2][32]={
 {0x92,0,0,0x80,0,0,0x80,0,0,0x80,0,0,0x80,0,0,0x80},
 {0x92,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
static int grid_on=1;
static void grid_font(int install){int i;for(i=0;i<2;i++){if(install)acc_glyph_read(201+i,oldglyph[i]);acc_glyph_write(201+i,install?gridglyph[i]:oldglyph[i]);}}
static void save_pixels(void){char p[ACC_PATH];FILE *f;acc_path(p,"DATA","PIXELS.DAT");f=fopen(p,"wb");if(f){fwrite(pixels,1,sizeof(pixels),f);fclose(f);}}
static void load_pixels(void){char p[ACC_PATH];FILE *f;long size;int x,y;memset(pixels,0,sizeof(pixels));acc_path(p,"DATA","PIXELS.DAT");f=fopen(p,"rb");if(!f)return;fseek(f,0L,SEEK_END);size=ftell(f);rewind(f);if(size==(long)sizeof(draw_old_pixels)&&fread(draw_old_pixels,1,sizeof(draw_old_pixels),f)==sizeof(draw_old_pixels)){for(y=0;y<DH;y++)for(x=0;x<DW;x++)pixels[y*CW+x]=draw_old_pixels[y*DW+x];}else fread(pixels,1,sizeof(pixels),f);fclose(f);}
static unsigned short get16(const unsigned char *p){return(unsigned short)(p[0]|((unsigned short)p[1]<<8));}
static unsigned long get32(const unsigned char *p){return(unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);}
static void put16(unsigned char *p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);}
static void put32(unsigned char *p,unsigned long v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
static void ega_rgb(int c,unsigned char *r,unsigned char *g,unsigned char *b){if(c==6){*r=170;*g=85;*b=0;}else{*r=(unsigned char)((c&4?170:0)+(c&8?85:0));*g=(unsigned char)((c&2?170:0)+(c&8?85:0));*b=(unsigned char)((c&1?170:0)+(c&8?85:0));}}
static int nearest_ega(unsigned char r,unsigned char g,unsigned char b){long best=0x7FFFFFFFL,d;int i,bi=0,dr,dg,db;unsigned char pr,pg,pb;for(i=0;i<16;i++){ega_rgb(i,&pr,&pg,&pb);dr=(int)r-pr;dg=(int)g-pg;db=(int)b-pb;d=(long)dr*dr+(long)dg*dg+(long)db*db;if(d<best){best=d;bi=i;}}return bi;}
static int draw_bmp_error=0;
static unsigned char bmp_palr[256],bmp_palg[256],bmp_palb[256];
/* Import uncompressed Windows BMP at 1/4/8/16/24/32 bpp.  Oversize images are
   fitted into the 96x96 canvas with nearest-neighbour sampling and all source
   colours are quantized to the Launch!/EGA 16-colour palette. */
static int load_bmp(const char *path){FILE*f;unsigned char hd[54],pe[4],*row=0;unsigned long off,comp,rs,info,colors;long ww,hh;int sw,sh,dw,dh,bottom,x,y,bpp,i;draw_bmp_error=0;f=fopen(path,"rb");if(!f){draw_bmp_error=1;return 0;}if(fread(hd,1,54,f)!=54||hd[0]!='B'||hd[1]!='M'){fclose(f);draw_bmp_error=2;return 0;}off=get32(hd+10);info=get32(hd+14);ww=(long)get32(hd+18);hh=(long)get32(hd+22);bpp=get16(hd+28);comp=get32(hd+30);if(info<40||get16(hd+26)!=1||comp!=0||ww<1||hh==0||(bpp!=1&&bpp!=4&&bpp!=8&&bpp!=16&&bpp!=24&&bpp!=32)){fclose(f);draw_bmp_error=2;return 0;}sw=(int)ww;bottom=hh>0;sh=(int)(bottom?hh:-hh);if(sw<1||sh<1){fclose(f);draw_bmp_error=2;return 0;}rs=(unsigned long)(((unsigned long)sw*bpp+31UL)/32UL*4UL);if(rs>65000UL){fclose(f);draw_bmp_error=3;return 0;}row=(unsigned char*)malloc((unsigned)rs);if(!row){fclose(f);draw_bmp_error=3;return 0;}memset(bmp_palr,0,sizeof(bmp_palr));memset(bmp_palg,0,sizeof(bmp_palg));memset(bmp_palb,0,sizeof(bmp_palb));if(bpp<=8){colors=get32(hd+46);if(!colors)colors=1UL<<bpp;if(colors>256)colors=256;if(fseek(f,14L+(long)info,SEEK_SET)!=0){free(row);fclose(f);draw_bmp_error=2;return 0;}for(i=0;i<(int)colors;i++){if(fread(pe,1,4,f)!=4){free(row);fclose(f);draw_bmp_error=2;return 0;}bmp_palb[i]=pe[0];bmp_palg[i]=pe[1];bmp_palr[i]=pe[2];}}
 dw=sw;dh=sh;if(dw>CW||dh>CH){long sx=(long)CW*10000L/sw,sy=(long)CH*10000L/sh,sc=sx<sy?sx:sy;if(sc<1)sc=1;dw=(int)((long)sw*sc/10000L);dh=(int)((long)sh*sc/10000L);if(dw<1)dw=1;if(dh<1)dh=1;if(dw>CW)dw=CW;if(dh>CH)dh=CH;}memset(pixels,0,sizeof(pixels));for(y=0;y<dh;y++){int srcy=(int)((long)y*sh/dh),filey=bottom?sh-1-srcy:srcy;if(fseek(f,(long)(off+(unsigned long)filey*rs),SEEK_SET)!=0||fread(row,1,(size_t)rs,f)!=(size_t)rs){free(row);fclose(f);draw_bmp_error=2;return 0;}for(x=0;x<dw;x++){int srcx=(int)((long)x*sw/dw),idx=0;unsigned char r=0,g=0,b=0;if(bpp==1){idx=(row[srcx>>3]>>(7-(srcx&7)))&1;r=bmp_palr[idx];g=bmp_palg[idx];b=bmp_palb[idx];}else if(bpp==4){idx=(srcx&1)?(row[srcx>>1]&15):(row[srcx>>1]>>4);r=bmp_palr[idx];g=bmp_palg[idx];b=bmp_palb[idx];}else if(bpp==8){idx=row[srcx];r=bmp_palr[idx];g=bmp_palg[idx];b=bmp_palb[idx];}else if(bpp==16){unsigned v=(unsigned)row[srcx*2]|((unsigned)row[srcx*2+1]<<8);r=(unsigned char)(((v>>10)&31)*255/31);g=(unsigned char)(((v>>5)&31)*255/31);b=(unsigned char)((v&31)*255/31);}else if(bpp==24){b=row[srcx*3];g=row[srcx*3+1];r=row[srcx*3+2];}else{b=row[srcx*4];g=row[srcx*4+1];r=row[srcx*4+2];}pixels[y*CW+x]=(unsigned char)nearest_ega(r,g,b);}}free(row);fclose(f);return 1;}
static int draw_open_dialog(char *out){int w=58,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=0,focus=0;unsigned mb=0;out[0]=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Open Bitmap",1);acc_text(x+3,y+2,"Filename:",ACC_LABEL,9);acc_fill(x+13,y+2,40,1,' ',focus==0?ACC_SELECT:ACC_CONTROL);acc_text(x+13,y+2,out,focus==0?ACC_SELECT:ACC_CONTROL,40);if(focus==0)acc_caret_set(x+13+(pos<40?pos:39),y+2);else acc_caret_hide();acc_button(x+3,y+6," Open ",focus==1);acc_button(x+11,y+6," Cancel ",focus==2);acc_wait(&k,&mx,&my,&mb);if(k==27){acc_modal_end();return 0;}if((mb&1)&&my==y+2){focus=0;k=0;}else if((mb&1)&&my==y+6&&mx>=x+3&&mx<x+9){focus=1;k=13;}else if((mb&1)&&my==y+6&&mx>=x+11&&mx<x+19){focus=2;k=13;}if(k==9||k==271){focus=k==271?(focus+2)%3:(focus+1)%3;k=0;continue;}if(k==13&&focus==2){acc_modal_end();return 0;}if(k==13&&focus==1){if(pos){acc_modal_end();return 1;}k=0;continue;}if(focus==0){if(k==256+71)pos=0;else if(k==256+79)pos=(int)strlen(out);else if(k==8&&pos){memmove(out+pos-1,out+pos,strlen(out)-pos+1);pos--;}else if(k>=32&&k<127&&strlen(out)<ACC_PATH-1){int n=(int)strlen(out);memmove(out+pos+1,out+pos,n-pos+1);out[pos++]=(char)k;}}k=0;}}
static void draw_resolve_path(const char *name,char *full){if(strchr(name,'\\')||strchr(name,'/')||strchr(name,':')){strncpy(full,name,ACC_PATH-1);full[ACC_PATH-1]=0;}else sprintf(full,"%s\\%s",acc_directory,name);}
static int draw_is_bmp(const char *name){const char *e=strrchr(name,'.');return e&&!stricmp(e,".BMP");}
static int write_bmp(const char *path,int x0,int y0,int w,int h,int scale){FILE*f;unsigned char hd[118],r,g,b;unsigned long row=(unsigned long)(((w*scale+1)/2+3)&~3),image=row*(unsigned long)(h*scale),size=118UL+image;int x,y,i,pixw=w*scale;memset(hd,0,sizeof(hd));hd[0]='B';hd[1]='M';put32(hd+2,size);put32(hd+10,118);put32(hd+14,40);put32(hd+18,(unsigned long)pixw);put32(hd+22,(unsigned long)(h*scale));put16(hd+26,1);put16(hd+28,4);put32(hd+34,image);put32(hd+46,16);for(i=0;i<16;i++){ega_rgb(i,&r,&g,&b);hd[54+i*4]=b;hd[55+i*4]=g;hd[56+i*4]=r;}f=fopen(path,"wb");if(!f)return 0;if(fwrite(hd,1,sizeof(hd),f)!=sizeof(hd)){fclose(f);return 0;}if(row>sizeof(draw_bmp_rowbuf)){fclose(f);return 0;}for(y=h*scale-1;y>=0;y--){memset(draw_bmp_rowbuf,0,(size_t)row);for(x=0;x<pixw;x++){unsigned char c=pixels[(y0+y/scale)*CW+x0+x/scale]&15;if(x&1)draw_bmp_rowbuf[x/2]|=c;else draw_bmp_rowbuf[x/2]=(unsigned char)(c<<4);}if(fwrite(draw_bmp_rowbuf,1,(size_t)row,f)!=(size_t)row){fclose(f);return 0;}}return fclose(f)==0;}
static int export_bmps(char *small,char *big){char n1[20],n2[20];int x,y,x0=CW,y0=CH,x1=-1,y1=-1,num;for(y=0;y<CH;y++)for(x=0;x<CW;x++)if(pixels[y*CW+x]){if(x<x0)x0=x;if(x>x1)x1=x;if(y<y0)y0=y;if(y>y1)y1=y;}if(x1<0){x0=y0=x1=y1=0;}for(num=1;num<10000;num++){sprintf(n1,"DRAW%d.BMP",num);sprintf(n2,"DRAW%dB.BMP",num);acc_path(small,"EXPORT",n1);acc_path(big,"EXPORT",n2);if(!acc_exists(small)&&!acc_exists(big))break;}if(num==10000)return 0;if(!write_bmp(small,x0,y0,x1-x0+1,y1-y0+1,1))return 0;if(!write_bmp(big,x0,y0,x1-x0+1,y1-y0+1,10)){remove(small);return 0;}return 1;}
static void crop(int *x0,int *y0,int *x1,int *y1){int x,y;*x0=CW;*y0=CH;*x1=*y1=-1;for(y=0;y<CH;y++)for(x=0;x<CW;x++)if(pixels[y*CW+x]){if(x<*x0)*x0=x;if(x>*x1)*x1=x;if(y<*y0)*y0=y;if(y>*y1)*y1=y;}if(*x1<0)*x0=*y0=*x1=*y1=0;}
static void graph_picture(int x0,int y0,int w,int h,int scale,int sx,int sy){unsigned char far *v=(unsigned char far *)((unsigned long)0xA000<<16);int plane,y,b,bit,px,py;unsigned char value,colour;outp(0x3CE,1);outp(0x3CF,0);outp(0x3CE,3);outp(0x3CF,0);outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);for(plane=0;plane<4;plane++){outp(0x3C4,2);outp(0x3C5,1<<plane);for(y=sy;y<sy+h*scale;y++){py=y0+(y-sy)/scale;for(b=sx/8;b<=(sx+w*scale-1)/8;b++){value=0;for(bit=0;bit<8;bit++){px=b*8+bit;if(px>=sx&&px<sx+w*scale){colour=pixels[py*CW+x0+(px-sx)/scale];if(colour&(1<<plane))value|=(unsigned char)(0x80>>bit);}}v[y*80+b]=value;}}}outp(0x3C4,2);outp(0x3C5,15);}
static void show_picture(void){union REGS r;int x0,y0,x1,y1,w,h,scale,sx,sy;unsigned word;crop(&x0,&y0,&x1,&y1);w=x1-x0+1;h=y1-y0+1;scale=640/w;if(350/h<scale)scale=350/h;if(scale<1)scale=1;sx=(640-w*scale)/2;sy=(350-h*scale)/2;memset(&r,0,sizeof(r));r.h.ah=0;r.h.al=0x10;int86(0x10,&r,&r);graph_picture(x0,y0,w,h,scale,sx,sy);do{word=_bios_keybrd(_KEYBRD_READ);}while((word&255)!=27);acc_restore_screen();}
static void cell_draw(int x,int y,int vx,int vy,int active,int ox,int oy,int selected){int c=pixels[(oy+vy)*CW+ox+vx],attr=ACC_ATTR(c,7),l=grid_on?185:' ',r=grid_on?186:' ';if(active){int pa=ACC_ATTR(acc_appearance.background,selected);acc_put(x+vx*2,y+vy,219,pa);acc_put(x+vx*2+1,y+vy,219,pa);}else{acc_put(x+vx*2,y+vy,l,attr);acc_put(x+vx*2+1,y+vy,r,attr);}}
static void palette_draw(int x,int y,int selected){int i,j,attr,px=x;for(i=0;i<16;i++){int xx=px+(i%8)*5,yy=y+i/8;attr=ACC_ATTR(i,i==0?15:0);for(j=0;j<5;j++)acc_put(xx+j,yy,' ',attr);if(i==selected)acc_put(xx+2,yy,4,ACC_ATTR(i,i<8?15:0));}}
static void draw_filename(int x,int y,int width){char shown[30],*base;int n,max;if(draw_external&&draw_path[0]){base=strrchr(draw_path,'\\');if(!base)base=strrchr(draw_path,'/');base=base?base+1:draw_path;max=draw_dirty?27:28;n=(int)strlen(base);if(n>max){base+=n-max;n=max;}memcpy(shown,base,n);if(draw_dirty)shown[n++]='*';shown[n]=0;acc_text(x+width-n,y,shown,draw_dirty?ACC_TITLE:ACC_HEADING,n);}else acc_text(x+width-7,y,"Unsaved",ACC_TITLE,7);}
static void hscroll(int x,int y,int pos){int i,track=DW*2-2,thumb=x+1+pos*(track-1)/(CW-DW);acc_put(x,y,17,ACC_CONTROL);for(i=1;i<DW*2-1;i++)acc_put(x+i,y,x+i==thumb?219:176,ACC_CONTROL);acc_put(x+DW*2-1,y,16,ACC_CONTROL);}
static void draw_all(int x,int y,int cx,int cy,int ox,int oy,int selected,int focus){int vx,vy;for(vy=0;vy<DH;vy++)for(vx=0;vx<DW;vx++)cell_draw(x,y,vx,vy,focus==0&&ox+vx==cx&&oy+vy==cy,ox,oy,selected);hscroll(x,y+DH,ox);acc_scrollbar(x+DW*2,y,DH,oy,CH,DH);palette_draw(x,y+DH+1,selected);draw_filename(x,y+DH+1,DW*2);}
static void draw_full(int cx,int cy,int ox,int oy,int selected){int vx,vy;acc_clear(ACC_BG);acc_fill(79,0,1,25,' ',ACC_ATTR(0,7));for(vy=0;vy<22;vy++)for(vx=0;vx<39;vx++)cell_draw(0,0,vx,vy,ox+vx==cx&&oy+vy==cy,ox,oy,selected);{int i,track=76,thumb=1+ox*(track-1)/(CW-39);acc_put(0,22,17,ACC_CONTROL);for(i=1;i<77;i++)acc_put(i,22,i==thumb?219:176,ACC_CONTROL);acc_put(77,22,16,ACC_CONTROL);}acc_scrollbar(78,0,22,oy,CH,22);palette_draw(0,23,selected);draw_filename(0,23,78);}
static void draw_maximize(int *pcx,int *pcy,int *pox,int *poy,int *pcolour)
{int key=0,mx=0,my=0,vx,vy,cx=*pcx,cy=*pcy,ox=*pox,oy=*poy,colour=*pcolour,old_close_x,old_close_y;unsigned mb=0;acc_close_target_suspend(&old_close_x,&old_close_y);acc_modal_begin();acc_input_bounds(0,0,acc_cols,acc_rows);draw_full(cx,cy,ox,oy,colour);while(key!=27&&key!=256+0x85&&key!=256+0x57){acc_wait(&key,&mx,&my,&mb);if((mb&3)&&my>=0&&my<22&&mx>=0&&mx<78){int held=(mb&2)?2:1,buttons=held;do{if(my>=0&&my<22&&mx>=0&&mx<78){vx=mx/2;vy=my;cx=ox+vx;cy=oy+vy;pixels[cy*CW+cx]=(held==2)?0:(unsigned char)colour;draw_dirty=1;cell_draw(0,0,vx,vy,0,ox,oy,colour);}acc_mouse(&mx,&my,&buttons);}while(buttons&held);draw_filename(0,23,78);key=0;continue;}if((mb&1)&&mx==78&&my<22){if(my==0&&oy>0)oy--;else if(my==21&&oy<CH-22)oy++;else if(my>0&&my<21)oy=(my-1)*(CH-22)/19;if(cy<oy)cy=oy;if(cy>=oy+22)cy=oy+21;draw_full(cx,cy,ox,oy,colour);key=0;continue;}if((mb&1)&&my==22&&mx<78){if(mx==0&&ox>0)ox--;else if(mx==77&&ox<CW-39)ox++;else if(mx>0&&mx<77)ox=(mx-1)*(CW-39)/75;if(cx<ox)cx=ox;if(cx>=ox+39)cx=ox+38;draw_full(cx,cy,ox,oy,colour);key=0;continue;}if((mb&1)&&my>=23&&my<=24&&mx<40){colour=((my-23)*8)+mx/5;draw_full(cx,cy,ox,oy,colour);key=0;continue;}if(key==256+75&&cx>0)cx--;else if(key==256+77&&cx<CW-1)cx++;else if(key==256+72&&cy>0)cy--;else if(key==256+80&&cy<CH-1)cy++;else if(key==' '){pixels[cy*CW+cx]=(unsigned char)colour;draw_dirty=1;}else if(key!=27&&key!=256+0x85&&key!=256+0x57)key=0;if(cx<ox)ox=cx;else if(cx>=ox+39)ox=cx-38;if(cy<oy)oy=cy;else if(cy>=oy+22)oy=cy-21;if(key!=27&&key!=256+0x85&&key!=256+0x57)draw_full(cx,cy,ox,oy,colour);}*pcx=cx;*pcy=cy;*pox=ox;*poy=oy;*pcolour=colour;acc_modal_end();acc_close_target_restore(old_close_x,old_close_y);acc_restore_text_screen();acc_mouse_display(1);}
static int draw_confirm_close(void){int w=48,h=8,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,f=-1;unsigned b=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Unsaved Changes",1);acc_text(x+3,y+2,"You have unsaved changes!",ACC_LABEL,25);acc_button(x+3,y+5," Save ",f==0);acc_button(x+11,y+5," Discard ",f==1);acc_button(x+38,y+5," Close ",f==2);acc_wait(&k,&mx,&my,&b);if((b&1)&&my==y+5){if(mx>=x+3&&mx<x+9){f=0;k=13;}else if(mx>=x+11&&mx<x+20){f=1;k=13;}else if(mx>=x+38&&mx<x+44){f=2;k=13;}}if(k==27){acc_modal_end();return 0;}if(k==9||k==271){if(f<0)f=k==271?2:0;else f=k==271?(f+2)%3:(f+1)%3;k=0;continue;}if(k==13&&f>=0){acc_modal_end();return f==0?2:(f==1?1:0);}}}
static int draw_save_dialog(char *out)
{int w=50,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=(int)strlen(out),f=0;unsigned mb=0;acc_modal_begin();for(;;){acc_subbox(x,y,w,h,"Save Bitmap",1);acc_text(x+3,y+2,"Filename:",ACC_LABEL,10);acc_fill(x+13,y+2,32,1,' ',f==0?ACC_SELECT:ACC_CONTROL);acc_text(x+13,y+2,out,f==0?ACC_SELECT:ACC_CONTROL,31);if(f==0)acc_caret_set(x+13+(pos<31?pos:30),y+2);else acc_caret_hide();acc_button(x+3,y+6," Save ",f==1);acc_button(x+11,y+6," Cancel ",f==2);acc_wait(&k,&mx,&my,&mb);if(k==27){acc_modal_end();return 0;}if((mb&1)&&my==y+2){f=0;k=0;}else if((mb&1)&&my==y+6&&mx>=x+3&&mx<x+9){f=1;k=13;}else if((mb&1)&&my==y+6&&mx>=x+11&&mx<x+19){f=2;k=13;}if(k==9||k==271){f=k==271?(f+2)%3:(f+1)%3;k=0;continue;}if(k==13&&f==2){acc_modal_end();return 0;}if(k==13&&(f==0||f==1)){if(pos){if(!strrchr(out,'.')&&strlen(out)<ACC_PATH-4)strcat(out,".BMP");acc_modal_end();return 1;}k=0;continue;}if(f==0){int n=(int)strlen(out);if(k==256+71)pos=0;else if(k==256+79)pos=n;else if(k==8&&pos){memmove(out+pos-1,out+pos,n-pos+1);pos--;}else if(k>=32&&k<127&&n<ACC_PATH-1){memmove(out+pos+1,out+pos,n-pos+1);out[pos++]=(char)k;}}k=0;}}
static int draw_save_prompt(void)
{
  char name[ACC_PATH],full[ACC_PATH],msg[ACC_PATH+24];
  int x0,y0,x1,y1;

  crop(&x0,&y0,&x1,&y1);
  /* Save overwrites an opened/already-named bitmap.  New drawings use Save As. */
  if(draw_external&&draw_path[0]){
    if(!write_bmp(draw_path,x0,y0,x1-x0+1,y1-y0+1,1)){
      acc_notice("Save Error","Unable to save drawing.");
      return 0;
    }
    draw_dirty=0;
    sprintf(msg,"Drawing saved as a BMP to\n%s",draw_path);
    acc_notice("Save",msg);
    return 1;
  }

  strcpy(name,"DRAWING.BMP");
  if(!draw_save_dialog(name))return 0;
  if(strchr(name,'\\')||strchr(name,'/')||strchr(name,':'))strcpy(full,name);
  else acc_path(full,"EXPORT",name);
  if(!write_bmp(full,x0,y0,x1-x0+1,y1-y0+1,1)){
    acc_notice("Save Error","Unable to save drawing.");
    return 0;
  }
  strncpy(draw_path,full,ACC_PATH-1);
  draw_path[ACC_PATH-1]=0;
  draw_external=1;
  draw_dirty=0;
  sprintf(msg,"Drawing saved as a BMP to\n%s",full);
  acc_notice("Save",msg);
  return 1;
}
static int draw_save(char *small,char *big){int x0,y0,x1,y1;if(draw_external){crop(&x0,&y0,&x1,&y1);if(!write_bmp(draw_path,x0,y0,x1-x0+1,y1-y0+1,1))return 0;}else{if(!export_bmps(small,big))return 0;strncpy(draw_path,small,ACC_PATH-1);draw_path[ACC_PATH-1]=0;draw_external=1;}draw_dirty=0;return 1;}
static void keep_visible(int *ox,int *oy,int cx,int cy){if(cx<*ox)*ox=cx;else if(cx>=*ox+DW)*ox=cx-DW+1;if(cy<*oy)*oy=cy;else if(cy>=*oy+DH)*oy=cy-DH+1;}
int main(int argc,char **argv){char *small=draw_small,*big=draw_big,*message=draw_message,*openname=draw_openname,*openfull=draw_openfull,*bn,*openfile=0;int bx,by,x,y,cx=0,cy=0,ox=0,oy=0,mx=0,my=0,colour=15,key=0,focus=-1,vx,vy,i,done=0,discarded=0,choice;unsigned mb=0;if(acc_help(argc,argv,"!DRAW","A scrollable 96 by 96 colour pixel editor with 16-colour BMP import/export."))return 0;if(!acc_begin(argv[0],"Pixel Draw",0))return 1;draw_path[0]=0;memset(pixels,0,sizeof(pixels));draw_dirty=0;for(i=1;i<argc;i++)if(argv[i][0]!='/'&&argv[i][0]!='-')openfile=argv[i];if(openfile){if(!draw_is_bmp(openfile))acc_notice("Error","Unsupported file type! Bitmap images only.");else if(!load_bmp(openfile))acc_notice("Open Bitmap",draw_bmp_error==1?"The specified file does not exist.":"Unable to import this BMP image.");else{strncpy(draw_path,openfile,ACC_PATH-1);draw_path[ACC_PATH-1]=0;draw_external=1;}}bx=(acc_cols-66)/2;by=(acc_rows-21)/2;x=bx+3;y=by+2;acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);
 while(!done){acc_button(bx+3,y+DH+4," Open ",focus==3);acc_button(bx+10,y+DH+4,"  Save  ",focus==4);acc_put(bx+17,y+DH+4,179,ACC_BORDER);acc_button(bx+19,y+DH+4,"  Clear  ",focus==1);acc_button(bx+29,y+DH+4,"  Grid  ",focus==2);acc_button(bx+38,y+DH+4,"  Show  ",focus==5);acc_button(bx+57,y+DH+4,"  Exit  ",focus==6);acc_wait(&key,&mx,&my,&mb);
  if(key==7){grid_on=!grid_on;draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;} /* Ctrl+G */
  if(key==256+0x3F){show_picture();acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);acc_mouse_reapply_cursor();key=0;continue;} /* F5 Show */
  if(key==19){draw_save_prompt();acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if((key==256+0x85||key==256+0x57)||((mb&1)&&my==by&&mx>=bx+58&&mx<bx+60)){draw_maximize(&cx,&cy,&ox,&oy,&colour);acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if(key==14){memset(pixels,0,sizeof(pixels));draw_path[0]=0;draw_external=0;draw_dirty=1;cx=cy=ox=oy=0;draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if(key==23){if(draw_dirty){choice=draw_confirm_close();if(choice==0){acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}if(choice==2&&!draw_save_prompt()){key=0;continue;}}memset(pixels,0,sizeof(pixels));draw_path[0]=0;draw_external=0;draw_dirty=0;cx=cy=ox=oy=0;draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if(key==27){if(!draw_dirty){done=1;break;}choice=draw_confirm_close();if(choice==1){discarded=1;done=1;break;}if(choice==2){if(draw_save_prompt()){done=1;break;}}acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if((mb&3)&&my>=y&&my<y+DH&&mx>=x&&mx<x+DW*2){int held=(mb&2)?2:1,buttons=held;focus=0;acc_mouse_display(1);do{if(my>=y&&my<y+DH&&mx>=x&&mx<x+DW*2){vx=(mx-x)/2;vy=my-y;cx=ox+vx;cy=oy+vy;pixels[cy*CW+cx]=(held==2)?0:(unsigned char)colour;draw_dirty=1;cell_draw(x,y,vx,vy,0,ox,oy,colour);}acc_mouse(&mx,&my,&buttons);}while(buttons&held);draw_filename(x,y+DH+1,DW*2);key=0;continue;}
  if((mb&1)&&mx==x+DW*2&&my>=y&&my<y+DH){focus=0;if(my==y&&oy>0)oy--;else if(my==y+DH-1&&oy<CH-DH)oy++;else if(my>y&&my<y+DH-1)oy=(my-y-1)*(CH-DH)/(DH-3);if(cy<oy)cy=oy;if(cy>=oy+DH)cy=oy+DH-1;draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if((mb&1)&&my==y+DH&&mx>=x&&mx<x+DW*2){focus=0;if(mx==x&&ox>0)ox--;else if(mx==x+DW*2-1&&ox<CW-DW)ox++;else if(mx>x&&mx<x+DW*2-1)ox=(mx-x-1)*(CW-DW)/(DW*2-3);if(cx<ox)cx=ox;if(cx>=ox+DW)cx=ox+DW-1;draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if((mb&1)&&my>=y+DH+1&&my<=y+DH+2&&mx>=x&&mx<x+40){colour=((my-y-DH-1)*8)+(mx-x)/5;palette_draw(x,y+DH+1,colour);key=0;continue;}
  if((mb&1)&&my==y+DH+4){if(mx>=bx+3&&mx<bx+9)focus=3;else if(mx>=bx+10&&mx<bx+18)focus=4;else if(mx>=bx+19&&mx<bx+28)focus=1;else if(mx>=bx+29&&mx<bx+37)focus=2;else if(mx>=bx+38&&mx<bx+46)focus=5;else if(mx>=bx+57&&mx<bx+63)focus=6;else{key=0;continue;}key=13;}
  if(key==9||key==271){if(focus<0)focus=(key==271)?6:7;else if(key==271){if(focus==7)focus=6;else if(focus==0)focus=7;else focus--;}else{if(focus==7)focus=0;else if(focus==6)focus=7;else focus++;}draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}if(focus==7){if(key==256+75&&colour%8>0)colour--;else if(key==256+77&&colour%8<7)colour++;else if(key==256+72&&colour>=8)colour-=8;else if(key==256+80&&colour<8)colour+=8;palette_draw(x,y+DH+1,colour);key=0;continue;}
  if(key==13&&focus){if(focus==1){memset(pixels,0,sizeof(pixels));cx=cy=ox=oy=0;draw_dirty=1;draw_all(x,y,cx,cy,ox,oy,colour,focus);}else if(focus==2){grid_on=!grid_on;draw_all(x,y,cx,cy,ox,oy,colour,focus);}else if(focus==3){if(draw_open_dialog(openname)){draw_resolve_path(openname,openfull);if(!acc_exists(openfull)){sprintf(message,"File %s doesn't exist!",openname);acc_notice("Error",message);}else if(!draw_is_bmp(openfull))acc_notice("Error","Unsupported file type! Bitmap images only.");else if(!load_bmp(openfull))acc_notice("Open Bitmap",draw_bmp_error==3?"The image is too large to import with available DOS memory.":"Unable to import this BMP image.");else{strncpy(draw_path,openfull,ACC_PATH-1);draw_path[ACC_PATH-1]=0;draw_external=1;draw_dirty=0;cx=cy=ox=oy=0;}}acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);}else if(focus==4){draw_save_prompt();acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);}else if(focus==5){show_picture();acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);acc_mouse_reapply_cursor();}else if(focus==6){if(!draw_dirty)done=1;else{choice=draw_confirm_close();if(choice==1){discarded=1;done=1;}else if(choice==2&&draw_save_prompt()){done=1;}else{acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);}}}key=0;continue;}if(focus!=0){key=0;continue;}
  if(key==256+75&&cx>0)cx--;else if(key==256+77&&cx<CW-1)cx++;else if(key==256+72&&cy>0)cy--;else if(key==256+80&&cy<CH-1)cy++;else if(key==' '){pixels[cy*CW+cx]=(unsigned char)colour;draw_dirty=1;}else key=0;keep_visible(&ox,&oy,cx,cy);draw_all(x,y,cx,cy,ox,oy,colour,focus);
 }acc_end();return 0;}
