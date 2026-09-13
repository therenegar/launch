/* Launch! Pixel Draw accessory: persistent 28 x 12 colour cells. */
#include <stdio.h>
#include <string.h>
#include "ACCLIB.H"
#define DW 28
#define DH 12
#define SCALE 20
#define BW (DW*SCALE)
#define BH (DH*SCALE)
static unsigned char pixels[DW*DH],oldglyph[2][32];
static const unsigned char gridglyph[2][32]={
 {0x92,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80},
 {0x92,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};
static int grid_on=1;
static int grid_font(int install){int i;if(!install){for(i=0;i<2;i++)acc_glyph_write(201+i,oldglyph[i]);return 1;}for(i=0;i<2;i++){acc_glyph_read(201+i,oldglyph[i]);acc_glyph_write(201+i,gridglyph[i]);}return 1;}
static void save_pixels(void){char p[ACC_PATH];FILE *f;acc_path(p,"DATA","PIXELS.DAT");f=fopen(p,"wb");if(f){fwrite(pixels,1,sizeof(pixels),f);fclose(f);}}
static void load_pixels(void){char p[ACC_PATH];FILE *f;acc_path(p,"DATA","PIXELS.DAT");f=fopen(p,"rb");if(f){fread(pixels,1,sizeof(pixels),f);fclose(f);}}
static void put32(unsigned char *p,unsigned long v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
static int export_bmp(char *p){char name[20];FILE *f;unsigned char h[54],bgr[3],c;unsigned long size=54UL+(unsigned long)BW*3UL*BH;int x,y,num;for(num=1;num<10000;num++){sprintf(name,"PIXEL%d.BMP",num);acc_path(p,"EXPORT",name);if(!acc_exists(p))break;}if(num==10000)return 0;memset(h,0,sizeof(h));h[0]='B';h[1]='M';put32(h+2,size);h[10]=54;h[14]=40;put32(h+18,BW);put32(h+22,BH);h[26]=1;h[28]=24;f=fopen(p,"wb");if(!f)return 0;fwrite(h,1,54,f);for(y=BH-1;y>=0;y--)for(x=0;x<BW;x++){c=pixels[(y/SCALE)*DW+x/SCALE];bgr[2]=(c&4?170:0)+(c&8?85:0);bgr[1]=(c&2?170:0)+(c&8?85:0);bgr[0]=(c&1?170:0)+(c&8?85:0);fwrite(bgr,1,3,f);}fclose(f);return 1;}
static void cell_draw(int ox,int oy,int x,int y,int active){int c=pixels[y*DW+x],attr=ACC_ATTR(c,active?15:7),left=grid_on?201:' ',right=grid_on?202:' ';acc_put(ox+x*2,oy+y,left,attr);acc_put(ox+x*2+1,oy+y,right,attr);}
static void palette_draw(int x,int y,int selected){int i,j,attr,px=x+8;for(i=0;i<16;i++){int xx=px+(i%8)*5,yy=y+i/8;attr=ACC_ATTR(i,i==0?15:0);for(j=0;j<5;j++)acc_put(xx+j,yy,' ',attr);if(i==selected)acc_put(xx+2,yy,4,ACC_ATTR(i,i<8?15:0));}}
static void draw_all(int x,int y,int cx,int cy,int selected,int focus){int i,j;for(j=0;j<DH;j++)for(i=0;i<DW;i++)cell_draw(x,y,i,j,focus==0&&i==cx&&j==cy);palette_draw(x,y+DH+1,selected);}
int main(int argc,char **argv){char exported[ACC_PATH],message[ACC_PATH+32];int bx,by,x,y,cx=0,cy=0,mx=0,my=0,colour=15,key=0,focus=0;unsigned mb=0;if(acc_help(argc,argv,"!DRAW","A persistent 16-colour text-mode pixel editor with BMP export."))return 0;if(!acc_begin(argv[0],"Pixel Draw",0))return 1;grid_font(1);load_pixels();bx=(acc_cols-66)/2;by=(acc_rows-21)/2;x=bx+5;y=by+2;acc_box(bx,by,66,21,"Pixel Draw");draw_all(x,y,cx,cy,colour,focus);
 while(key!=27){acc_button(x,y+DH+4,"  Clear  ",focus==1);acc_button(x+11,y+DH+4,"  Grid  ",focus==2);acc_button(x+21,y+DH+4,"  Export  ",focus==3);acc_button(x+47,y+DH+4,"  Close  ",focus==4);acc_wait(&key,&mx,&my,&mb);
  if((mb&3)&&my>=y&&my<y+DH&&mx>=x&&mx<x+DW*2){cx=(mx-x)/2;cy=my-y;pixels[cy*DW+cx]=(mb&2)?0:(unsigned char)colour;cell_draw(x,y,cx,cy,1);save_pixels();key=0;continue;}if((mb&1)&&my>=y+DH+1&&my<=y+DH+2&&mx>=x+8&&mx<x+48){colour=((my-y-DH-1)*8)+(mx-x-8)/5;palette_draw(x,y+DH+1,colour);key=0;continue;}if((mb&1)&&my==y+DH+4){if(mx<x+10)focus=1;else if(mx>=x+11&&mx<x+20)focus=2;else if(mx>=x+21&&mx<x+32)focus=3;else if(mx>=x+47)focus=4;key=13;}
  if(key==9){focus=(focus+1)%6;draw_all(x,y,cx,cy,colour,focus);key=0;continue;}if(focus==5){if(key==27)break;if(key==256+75&&colour%8>0)colour--;else if(key==256+77&&colour%8<7)colour++;else if(key==256+72&&colour>=8)colour-=8;else if(key==256+80&&colour<8)colour+=8;palette_draw(x,y+DH+1,colour);key=0;continue;}if(key==13&&focus){if(focus==1){memset(pixels,0,sizeof(pixels));draw_all(x,y,cx,cy,colour,focus);save_pixels();}else if(focus==2){grid_on=!grid_on;draw_all(x,y,cx,cy,colour,focus);}else if(focus==3){if(!export_bmp(exported))acc_notice("Export","Unable to export pixel drawing.");else{sprintf(message,"Pixel drawing exported to\n%s",exported);acc_notice("Export",message);}}else key=27;if(key!=27)key=0;continue;}if(focus)focus=0;
  cell_draw(x,y,cx,cy,0);if(key==256+75&&cx>0)cx--;else if(key==256+77&&cx<DW-1)cx++;else if(key==256+72&&cy>0)cy--;else if(key==256+80&&cy<DH-1)cy++;else if(key==' '){pixels[cy*DW+cx]=(unsigned char)colour;save_pixels();}else if(key!=27)key=0;cell_draw(x,y,cx,cy,1);
 }save_pixels();grid_font(0);acc_end();return 0;}
