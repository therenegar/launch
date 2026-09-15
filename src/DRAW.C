/* Launch! Pixel Draw: scrollable persistent 96 x 96 colour canvas. */
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include "ACCLIB.H"
#define DW 30
#define DH 12
#define CW 96
#define CH 96
static unsigned char pixels[CW*CH],oldglyph[2][32];
static const unsigned char gridglyph[2][32]={
 {0x92,0,0,0x80,0,0,0x80,0,0,0x80,0,0,0x80,0,0,0x80},
 {0x92,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
static int grid_on=1;
static void grid_font(int install){int i;for(i=0;i<2;i++){if(install)acc_glyph_read(201+i,oldglyph[i]);acc_glyph_write(201+i,install?gridglyph[i]:oldglyph[i]);}}
static void save_pixels(void){char p[ACC_PATH];FILE *f;acc_path(p,"DATA","PIXELS.DAT");f=fopen(p,"wb");if(f){fwrite(pixels,1,sizeof(pixels),f);fclose(f);}}
static void load_pixels(void){char p[ACC_PATH];FILE *f;long size;int x,y;unsigned char old[DW*DH];memset(pixels,0,sizeof(pixels));acc_path(p,"DATA","PIXELS.DAT");f=fopen(p,"rb");if(!f)return;fseek(f,0L,SEEK_END);size=ftell(f);rewind(f);if(size==(long)sizeof(old)&&fread(old,1,sizeof(old),f)==sizeof(old)){for(y=0;y<DH;y++)for(x=0;x<DW;x++)pixels[y*CW+x]=old[y*DW+x];}else fread(pixels,1,sizeof(pixels),f);fclose(f);}
static void put32(unsigned char *p,unsigned long v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
static int write_bmp(const char *path,int x0,int y0,int w,int h,int scale){FILE *f;unsigned char hd[54],bgr[3],c;unsigned long row=(unsigned long)((w*scale*3+3)&~3),size=54UL+row*(unsigned long)(h*scale);int x,y,pad;memset(hd,0,sizeof(hd));hd[0]='B';hd[1]='M';put32(hd+2,size);hd[10]=54;hd[14]=40;put32(hd+18,(unsigned long)(w*scale));put32(hd+22,(unsigned long)(h*scale));hd[26]=1;hd[28]=24;f=fopen(path,"wb");if(!f)return 0;if(fwrite(hd,1,54,f)!=54){fclose(f);return 0;}for(y=h*scale-1;y>=0;y--){for(x=0;x<w*scale;x++){c=pixels[(y0+y/scale)*CW+x0+x/scale];if(c==6){bgr[2]=170;bgr[1]=85;bgr[0]=0;}else{bgr[2]=(c&4?170:0)+(c&8?85:0);bgr[1]=(c&2?170:0)+(c&8?85:0);bgr[0]=(c&1?170:0)+(c&8?85:0);}if(fwrite(bgr,1,3,f)!=3){fclose(f);return 0;}}for(pad=w*scale*3;pad<(int)row;pad++)fputc(0,f);}return fclose(f)==0;}
static int export_bmps(char *small,char *big){char n1[20],n2[20];int x,y,x0=CW,y0=CH,x1=-1,y1=-1,num;for(y=0;y<CH;y++)for(x=0;x<CW;x++)if(pixels[y*CW+x]){if(x<x0)x0=x;if(x>x1)x1=x;if(y<y0)y0=y;if(y>y1)y1=y;}if(x1<0){x0=y0=x1=y1=0;}for(num=1;num<10000;num++){sprintf(n1,"DRAW%d.BMP",num);sprintf(n2,"DRAW%dB.BMP",num);acc_path(small,"EXPORT",n1);acc_path(big,"EXPORT",n2);if(!acc_exists(small)&&!acc_exists(big))break;}if(num==10000)return 0;if(!write_bmp(small,x0,y0,x1-x0+1,y1-y0+1,1))return 0;if(!write_bmp(big,x0,y0,x1-x0+1,y1-y0+1,10)){remove(small);return 0;}return 1;}
static void crop(int *x0,int *y0,int *x1,int *y1){int x,y;*x0=CW;*y0=CH;*x1=*y1=-1;for(y=0;y<CH;y++)for(x=0;x<CW;x++)if(pixels[y*CW+x]){if(x<*x0)*x0=x;if(x>*x1)*x1=x;if(y<*y0)*y0=y;if(y>*y1)*y1=y;}if(*x1<0)*x0=*y0=*x1=*y1=0;}
static void graph_picture(int x0,int y0,int w,int h,int scale,int sx,int sy){unsigned char far *v=(unsigned char far *)((unsigned long)0xA000<<16);int plane,y,b,bit,px,py;unsigned char value,colour;outp(0x3CE,1);outp(0x3CF,0);outp(0x3CE,3);outp(0x3CF,0);outp(0x3CE,5);outp(0x3CF,0);outp(0x3CE,8);outp(0x3CF,255);for(plane=0;plane<4;plane++){outp(0x3C4,2);outp(0x3C5,1<<plane);for(y=sy;y<sy+h*scale;y++){py=y0+(y-sy)/scale;for(b=sx/8;b<=(sx+w*scale-1)/8;b++){value=0;for(bit=0;bit<8;bit++){px=b*8+bit;if(px>=sx&&px<sx+w*scale){colour=pixels[py*CW+x0+(px-sx)/scale];if(colour&(1<<plane))value|=(unsigned char)(0x80>>bit);}}v[y*80+b]=value;}}}outp(0x3C4,2);outp(0x3C5,15);}
static void show_picture(void){union REGS r;int x0,y0,x1,y1,w,h,scale,sx,sy;unsigned word;crop(&x0,&y0,&x1,&y1);w=x1-x0+1;h=y1-y0+1;scale=640/w;if(350/h<scale)scale=350/h;if(scale<1)scale=1;sx=(640-w*scale)/2;sy=(350-h*scale)/2;memset(&r,0,sizeof(r));r.h.ah=0;r.h.al=0x10;int86(0x10,&r,&r);graph_picture(x0,y0,w,h,scale,sx,sy);do{word=_bios_keybrd(_KEYBRD_READ);}while((word&255)!=27);acc_restore_screen();}
static void cell_draw(int x,int y,int vx,int vy,int active,int ox,int oy){int c=pixels[(oy+vy)*CW+ox+vx],attr=ACC_ATTR(c,active?15:7),l=grid_on?185:' ',r=grid_on?186:' ';acc_put(x+vx*2,y+vy,l,attr);acc_put(x+vx*2+1,y+vy,r,attr);}
static void palette_draw(int x,int y,int selected){int i,j,attr,px=x+8;for(i=0;i<16;i++){int xx=px+(i%8)*5,yy=y+i/8;attr=ACC_ATTR(i,i==0?15:0);for(j=0;j<5;j++)acc_put(xx+j,yy,' ',attr);if(i==selected)acc_put(xx+2,yy,4,ACC_ATTR(i,i<8?15:0));}}
static void hscroll(int x,int y,int pos){int i,track=DW*2-2,thumb=x+1+pos*(track-1)/(CW-DW);acc_put(x,y,17,ACC_CONTROL);for(i=1;i<DW*2-1;i++)acc_put(x+i,y,x+i==thumb?219:176,ACC_CONTROL);acc_put(x+DW*2-1,y,16,ACC_CONTROL);}
static void draw_all(int x,int y,int cx,int cy,int ox,int oy,int selected,int focus){int vx,vy;for(vy=0;vy<DH;vy++)for(vx=0;vx<DW;vx++)cell_draw(x,y,vx,vy,focus==0&&ox+vx==cx&&oy+vy==cy,ox,oy);hscroll(x,y+DH,ox);acc_scrollbar(x+DW*2,y,DH,oy,CH,DH);palette_draw(x,y+DH+1,selected);}
static void keep_visible(int *ox,int *oy,int cx,int cy){if(cx<*ox)*ox=cx;else if(cx>=*ox+DW)*ox=cx-DW+1;if(cy<*oy)*oy=cy;else if(cy>=*oy+DH)*oy=cy-DH+1;}
int main(int argc,char **argv){char small[ACC_PATH],big[ACC_PATH],message[ACC_PATH*2+48],*bn;int bx,by,x,y,cx=0,cy=0,ox=0,oy=0,mx=0,my=0,colour=15,key=0,focus=0,vx,vy;unsigned mb=0;if(acc_help(argc,argv,"!DRAW","A persistent scrollable 96 by 96 colour pixel editor with cropped BMP export."))return 0;if(!acc_begin(argv[0],"Pixel Draw",0))return 1;load_pixels();bx=(acc_cols-66)/2;by=(acc_rows-21)/2;x=bx+3;y=by+2;acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);
 while(key!=27){acc_button(x,y+DH+4,"  Clear  ",focus==1);acc_button(x+11,y+DH+4,"  Grid  ",focus==2);acc_button(x+21,y+DH+4,"  Export  ",focus==3);acc_button(x+29,y+DH+4,"  Show  ",focus==4);acc_button(bx+56,y+DH+4,"  Close  ",focus==5);acc_wait(&key,&mx,&my,&mb);
  if((mb&3)&&my>=y&&my<y+DH&&mx>=x&&mx<x+DW*2){vx=(mx-x)/2;vy=my-y;focus=0;cx=ox+vx;cy=oy+vy;pixels[cy*CW+cx]=(mb&2)?0:(unsigned char)colour;cell_draw(x,y,vx,vy,1,ox,oy);key=0;continue;}
  if((mb&1)&&mx==x+DW*2&&my>=y&&my<y+DH){focus=0;if(my==y&&oy>0)oy--;else if(my==y+DH-1&&oy<CH-DH)oy++;else if(my>y&&my<y+DH-1)oy=(my-y-1)*(CH-DH)/(DH-3);if(cy<oy)cy=oy;if(cy>=oy+DH)cy=oy+DH-1;draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if((mb&1)&&my==y+DH&&mx>=x&&mx<x+DW*2){focus=0;if(mx==x&&ox>0)ox--;else if(mx==x+DW*2-1&&ox<CW-DW)ox++;else if(mx>x&&mx<x+DW*2-1)ox=(mx-x-1)*(CW-DW)/(DW*2-3);if(cx<ox)cx=ox;if(cx>=ox+DW)cx=ox+DW-1;draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}
  if((mb&1)&&my>=y+DH+1&&my<=y+DH+2&&mx>=x+8&&mx<x+48){colour=((my-y-DH-1)*8)+(mx-x-8)/5;palette_draw(x,y+DH+1,colour);key=0;continue;}
  if((mb&1)&&my==y+DH+4){if(mx<x+10)focus=1;else if(mx>=x+11&&mx<x+20)focus=2;else if(mx>=x+21&&mx<x+32)focus=3;else if(mx>=x+29&&mx<x+39)focus=4;else if(mx>=bx+56&&mx<bx+62)focus=5;key=13;}
  if(key==9){focus=(focus+1)%7;draw_all(x,y,cx,cy,ox,oy,colour,focus);key=0;continue;}if(focus==6){if(key==27)break;if(key==256+75&&colour%8>0)colour--;else if(key==256+77&&colour%8<7)colour++;else if(key==256+72&&colour>=8)colour-=8;else if(key==256+80&&colour<8)colour+=8;palette_draw(x,y+DH+1,colour);key=0;continue;}
  if(key==13&&focus){if(focus==1){memset(pixels,0,sizeof(pixels));cx=cy=ox=oy=0;draw_all(x,y,cx,cy,ox,oy,colour,focus);save_pixels();}else if(focus==2){grid_on=!grid_on;draw_all(x,y,cx,cy,ox,oy,colour,focus);}else if(focus==3){if(!export_bmps(small,big))acc_notice("Export","Unable to export pixel drawings.");else{bn=strrchr(big,'\\');if(!bn)bn=big;else bn++;sprintf(message,"Drawings exported to\n%s + %s",small,bn);acc_notice("Export",message);}}else if(focus==4){show_picture();acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,ox,oy,colour,focus);}else key=27;if(key!=27)key=0;continue;}if(focus)focus=0;
  if(key==256+75&&cx>0)cx--;else if(key==256+77&&cx<CW-1)cx++;else if(key==256+72&&cy>0)cy--;else if(key==256+80&&cy<CH-1)cy++;else if(key==' '){pixels[cy*CW+cx]=(unsigned char)colour;}else if(key!=27)key=0;keep_visible(&ox,&oy,cx,cy);draw_all(x,y,cx,cy,ox,oy,colour,focus);
 }save_pixels();acc_end();return 0;}
