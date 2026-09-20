/* Launch! 3.86 POC - lightweight 8088/8086 command menu.
 * Microsoft C/C++ 7.0, small model. 8086 instruction set only.
 * Display: MDA/CGA/EGA/VGA 80-column text modes. No custom glyphs.
 */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <malloc.h>
#include <process.h>

#define MAX_NODES 96
#define MAX_TITLE 24
#define MAX_CMD 128
#define MAX_DEPTH 4
#define MENU_WIDTH 20
#define MENU_CAPACITY 20
#define FPTR(seg,off) ((void far *)MK_FP((seg),(off)))

typedef struct {
 char title[MAX_TITLE]; char command[MAX_CMD]; int parent;
 unsigned char folder,separator,active,order,press_enter,change_dir;
} NODE;

static NODE nodes[MAX_NODES];
static int node_count,cols=80,rows=25,mono=0,run_node=-1;
static unsigned short far *video;
static unsigned short far *saved;
static unsigned char old_cs,old_ce,old_cx,old_cy;
static char cfg[128];

#define ATTR(bg,fg) ((unsigned char)(((bg)<<4)|(fg)))
static unsigned char bg=1,border=11,title=14,folder=15,item=10,sel_fg=15,sel_bg=3;
static unsigned char a_bg(void){return mono?0x07:ATTR(bg,bg);}
static unsigned char a_border(void){return mono?0x07:ATTR(bg,border);}
static unsigned char a_title(void){return mono?0x0F:ATTR(bg,title);}
static unsigned char a_folder(void){return mono?0x0F:ATTR(bg,folder);}
static unsigned char a_item(void){return mono?0x07:ATTR(bg,item);}
static unsigned char a_sel(void){return mono?0x70:ATTR(sel_bg,sel_fg);}

static void cell(int x,int y,int ch,unsigned char at){if(x>=0&&x<cols&&y>=0&&y<rows)video[y*cols+x]=(unsigned short)((at<<8)|(ch&255));}
static void textout(int x,int y,const char *s,unsigned char at,int width){int i,e=0;for(i=0;i<width;i++){if(!e&&!s[i])e=1;cell(x+i,y,e?' ':s[i],at);}}
static void cursor_hide(void){union REGS r;r.h.ah=3;r.h.bh=0;int86(0x10,&r,&r);old_cs=r.h.ch;old_ce=r.h.cl;old_cx=r.h.dl;old_cy=r.h.dh;r.h.ah=1;r.h.ch=0x20;r.h.cl=0;int86(0x10,&r,&r);}
static void cursor_restore(void){union REGS r;r.h.ah=1;r.h.ch=old_cs;r.h.cl=old_ce;int86(0x10,&r,&r);r.h.ah=2;r.h.bh=0;r.h.dh=old_cy;r.h.dl=old_cx;int86(0x10,&r,&r);}
static void video_init(void){unsigned char far *m=(unsigned char far *)FPTR(0x40,0x49);unsigned short far *c=(unsigned short far *)FPTR(0x40,0x4a);mono=(*m==7);cols=*c;if(cols<40||cols>80)cols=80;rows=25;video=(unsigned short far *)FPTR(mono?0xb000:0xb800,0);}
static int save_screen(void){unsigned i,n=(unsigned)(cols*rows);saved=(unsigned short far *)_fmalloc(n*2U);if(!saved)return 0;for(i=0;i<n;i++)saved[i]=video[i];return 1;}
static void restore_screen(void){unsigned i,n=(unsigned)(cols*rows);if(saved)for(i=0;i<n;i++)video[i]=saved[i];}
static void box(int x,int y,int w,int h,const char *t){int i,j,l;for(j=0;j<h;j++)for(i=0;i<w;i++)cell(x+i,y+j,' ',a_bg());cell(x,y,218,a_border());cell(x+w-1,y,191,a_border());cell(x,y+h-1,192,a_border());cell(x+w-1,y+h-1,217,a_border());for(i=1;i<w-1;i++){cell(x+i,y,196,a_border());cell(x+i,y+h-1,196,a_border());}for(j=1;j<h-1;j++){cell(x,y+j,179,a_border());cell(x+w-1,y+j,179,a_border());}if(t&&*t){l=strlen(t);if(l>w-4)l=w-4;textout(x+2,y,t,a_title(),l);}}
static int cmpnode(const void *aa,const void *bb){const NODE *a=(const NODE*)aa,*b=(const NODE*)bb;if(a->parent!=b->parent)return a->parent-b->parent;return (int)a->order-(int)b->order;}
static void defaults(void){node_count=0;memset(nodes,0,sizeof(nodes));strcpy(nodes[0].title,"DOS Commands");nodes[0].parent=-1;nodes[0].folder=1;nodes[0].active=1;nodes[0].order=1;node_count=1;}
static void make_cfg_path(const char *argv0){char *p;strncpy(cfg,argv0,sizeof(cfg)-1);cfg[sizeof(cfg)-1]=0;p=strrchr(cfg,'\\');if(p)p[1]=0;else cfg[0]=0;strcat(cfg,"LAUNCH.CFG");}
static void load_cfg(void){FILE *f;char line[300],*p;NODE n;defaults();f=fopen(cfg,"rt");if(!f)return;node_count=0;while(fgets(line,sizeof(line),f)&&node_count<MAX_NODES){if(line[0]=='#'||line[0]==';'||line[0]=='\r'||line[0]=='\n')continue;memset(&n,0,sizeof(n));p=strtok(line,"|\r\n");if(!p)continue;strncpy(n.title,p,MAX_TITLE-1);p=strtok(NULL,"|\r\n");if(p)strncpy(n.command,p,MAX_CMD-1);p=strtok(NULL,"|\r\n");n.parent=p?atoi(p):-1;p=strtok(NULL,"|\r\n");n.folder=p?(unsigned char)atoi(p):0;p=strtok(NULL,"|\r\n");n.separator=p?(unsigned char)atoi(p):0;p=strtok(NULL,"|\r\n");n.active=p?(unsigned char)atoi(p):1;p=strtok(NULL,"|\r\n");n.order=p?(unsigned char)atoi(p):(unsigned char)node_count;p=strtok(NULL,"|\r\n");n.press_enter=p?(unsigned char)atoi(p):1;p=strtok(NULL,"|\r\n");n.change_dir=p?(unsigned char)atoi(p):0;nodes[node_count++]=n;}fclose(f);if(!node_count)defaults();qsort(nodes,node_count,sizeof(NODE),cmpnode);}
static int children(int parent,int *list){int i,n=0;for(i=0;i<node_count&&n<MENU_CAPACITY;i++)if(nodes[i].active&&nodes[i].parent==parent)list[n++]=i;return n;}
static int menu(int parent,int x,int y,int depth){int list[MENU_CAPACITY],n,sel=0,k,i,h,idx;n=children(parent,list);h=n+3;if(h<5)h=5;if(y+h>rows)y=rows-h;if(x+MENU_WIDTH>cols)x=cols-MENU_WIDTH;box(x,y,MENU_WIDTH,h,parent<0?"Launch!":nodes[parent].title);for(;;){for(i=0;i<n;i++){idx=list[i];textout(x+2,y+1+i,nodes[idx].title,i==sel?a_sel():(nodes[idx].folder?a_folder():a_item()),MENU_WIDTH-4);if(nodes[idx].folder)cell(x+MENU_WIDTH-2,y+1+i,16,i==sel?a_sel():a_folder());}if(!n)textout(x+2,y+1,"Empty",a_border(),MENU_WIDTH-4);k=getch();if(k==0||k==0xe0){k=getch();if(k==72&&n){if(--sel<0)sel=n-1;}else if(k==80&&n){if(++sel>=n)sel=0;}else if(k==77&&n&&nodes[list[sel]].folder&&depth<MAX_DEPTH){idx=menu(list[sel],x+MENU_WIDTH-1,y+1+sel,depth+1);if(idx>=0)return idx;}else if(k==75&&depth)return -2;}else if(k==27)return -1;else if(k==13&&n){idx=list[sel];if(nodes[idx].folder){if(depth<MAX_DEPTH){idx=menu(idx,x+MENU_WIDTH-1,y+1+sel,depth+1);if(idx>=0)return idx;}}else return idx;}}
}
static void execute_node(int n){char cmd[MAX_CMD+16],dir[MAX_CMD],*p;if(n<0)return;strncpy(cmd,nodes[n].command,MAX_CMD-1);cmd[MAX_CMD-1]=0;if(nodes[n].change_dir){strncpy(dir,cmd,MAX_CMD-1);dir[MAX_CMD-1]=0;p=strrchr(dir,'\\');if(p){*p=0;chdir(dir);}}if(nodes[n].press_enter)system(cmd);else { /* Macro launch is supplied by SHORTC86.COM; direct invocation falls back to system(). */ system(cmd); }}
static void usage(void){puts("Launch! 3.86 8086 POC");puts("Usage: !86 [/?]");puts("8088/8086+, DOS, MDA/CGA/EGA/VGA text display.");}
int main(int argc,char **argv){int r;if(argc>1&&!strcmp(argv[1],"/?")){usage();return 0;}make_cfg_path(argv[0]);load_cfg();video_init();if(!save_screen()){fputs("Launch!: insufficient memory.\n",stderr);return 1;}cursor_hide();r=menu(-1,1,rows-2-MENU_CAPACITY,0);restore_screen();cursor_restore();_ffree(saved);saved=0;if(r>=0)execute_node(r);return 0;}
