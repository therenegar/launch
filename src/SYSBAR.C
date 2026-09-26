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
 * File: SYSBAR.C
 * Role: !SYSBAR modular system-bar accessory
 * Build/ownership: Discovers APPDATA\*.SBM packages, captures module stdout and composes the SysBar. /CONFIG manages installed module order.
 * Maintainer contract: SBM packages contain SBM.CFG plus SBM.COM/EXE/BAT. SysBar owns width clipping and separators; modules only emit plain console text.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* !SYSBAR - Launch! SysBar accessory / module host, Release 3.73. */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#include "ACCLIB.H"
#define MAXMOD 24
#define MODNAM 20
#define MODDIR 13
typedef struct{char dir[MODDIR],name[MODNAM];int width;} MOD;
static char root[ACC_PATH],appdata[ACC_PATH];static MOD mods[MAXMOD];static int modn=0;static char sysbar_copy_buffer[2048];
/* Locate the Launch! installation and APPDATA root from argv[0]. */
static void source_dir(const char *a){char t[ACC_PATH],*p;strncpy(t,a,ACC_PATH-1);t[ACC_PATH-1]=0;p=strrchr(t,'\\');if(!p)p=strrchr(t,'/');if(p)p[1]=0;else t[0]=0;strcpy(root,t);sprintf(appdata,"%sAPPDATA",root);}
static int exists(const char*p){FILE*f=fopen(p,"rb");if(!f)return 0;fclose(f);return 1;}
/* Parse one SBM.CFG. Unknown keys are ignored so the package format can grow. */
static void read_cfg(MOD*m){char p[ACC_PATH],line[64],*q;FILE*f;sprintf(p,"%s\\%s\\SBM.CFG",appdata,m->dir);f=fopen(p,"r");if(!f)return;while(fgets(line,sizeof(line),f)){q=strchr(line,'\r');if(q)*q=0;q=strchr(line,'\n');if(q)*q=0;if(!strnicmp(line,"ModuleName=",11)){strncpy(m->name,line+11,MODNAM-1);m->name[MODNAM-1]=0;}else if(!strnicmp(line,"Width=",6)){m->width=atoi(line+6);if(m->width<1)m->width=1;if(m->width>60)m->width=60;}}fclose(f);}
/* Discover APPDATA\*.SBM directories. Discovery does not imply installation/order. */
static void scan_modules(void){struct find_t f;char mask[ACC_PATH];unsigned e;modn=0;sprintf(mask,"%s\\*.SBM",appdata);e=_dos_findfirst(mask,_A_SUBDIR,&f);while(!e&&modn<MAXMOD){if((f.attrib&_A_SUBDIR)&&strcmp(f.name,".")&&strcmp(f.name,"..")){strncpy(mods[modn].dir,f.name,MODDIR-1);mods[modn].dir[MODDIR-1]=0;strcpy(mods[modn].name,f.name);mods[modn].width=8;read_cfg(&mods[modn]);modn++;}e=_dos_findnext(&f);}}
static void make_dir(const char*p){mkdir(p);}
static void copy_file(const char*a,const char*b){FILE*i=fopen(a,"rb"),*o;size_t n;if(!i)return;o=fopen(b,"wb");if(!o){fclose(i);return;}while((n=fread(sysbar_copy_buffer,1,sizeof(sysbar_copy_buffer),i))>0)fwrite(sysbar_copy_buffer,1,n,o);fclose(o);fclose(i);}
static void init_one(const char*dir,const char*name,int width){char d[ACC_PATH],cfg[ACC_PATH],dst[ACC_PATH],src[ACC_PATH];FILE*f;sprintf(d,"%s\\%s.SBM",appdata,dir);make_dir(appdata);make_dir(d);sprintf(cfg,"%s\\SBM.CFG",d);if(!exists(cfg)){f=fopen(cfg,"w");if(f){fprintf(f,"ModuleName=%s\nWidth=%d\n",name,width);fclose(f);}}sprintf(src,"%sSBM.EXE",root);sprintf(dst,"%s\\SBM.EXE",d);if(exists(src)&&!exists(dst))copy_file(src,dst);}
/* Materialize the built-in starter modules when they are not already present. */
static void init_defaults(void){init_one("MEM","Memory",10);init_one("ENV","Environment",9);init_one("DISK","Disk free",10);init_one("LOCKS","Keyboard locks",12);init_one("MOUSE","Mouse",10);init_one("VIDEO","Video",5);init_one("NET","Network",7);init_one("LOCALE","Locale",8);}
/* SYSBAR.CFG is the installed-module list; file order is on-screen order. */
static int order_load(char names[MAXMOD][MODDIR]){char p[ACC_PATH],line[32],*q;FILE*f;int n=0;sprintf(p,"%s\\SYSBAR.CFG",appdata);f=fopen(p,"r");if(!f)return 0;while(n<MAXMOD&&fgets(line,sizeof(line),f)){q=strchr(line,'\r');if(q)*q=0;q=strchr(line,'\n');if(q)*q=0;if(*line){strncpy(names[n],line,MODDIR-1);names[n][MODDIR-1]=0;n++;}}fclose(f);return n;}
static void order_save(char names[MAXMOD][MODDIR],int n){char p[ACC_PATH];FILE*f;int i;sprintf(p,"%s\\SYSBAR.CFG",appdata);f=fopen(p,"w");if(!f)return;for(i=0;i<n;i++)fprintf(f,"%s\n",names[i]);fclose(f);}
static int find_mod(const char*d){int i;for(i=0;i<modn;i++)if(!stricmp(mods[i].dir,d))return i;return -1;}
/* Run SBM.EXE/COM directly, inheriting a temporarily redirected STDOUT handle.
   This is deliberately not implemented with system(): system() starts COMSPEC for
   every module, which made a normal SysBar display visibly execute 4START.BAT (or
   COMMAND.COM) once per module.  BAT modules necessarily use COMSPEC /C, but EXE
   and COM modules remain shell-free.  Only the first output line is displayed. */
static void run_module(MOD*m,char*out)
{
  char cwd[ACC_PATH],tmp[ACC_PATH],mdir[ACC_PATH],exe[16],path[ACC_PATH],*comspec;
  FILE*f,*capture=0;int c,n=0,oldout=-1,rc=-1,isbat=0;
  getcwd(cwd,sizeof(cwd));sprintf(tmp,"%s\\SBM.$$$",appdata);sprintf(mdir,"%s\\%s",appdata,m->dir);
  strcpy(exe,"SBM.EXE");sprintf(path,"%s\\%s",mdir,exe);
  if(!exists(path)){strcpy(exe,"SBM.COM");sprintf(path,"%s\\%s",mdir,exe);}
  if(!exists(path)){strcpy(exe,"SBM.BAT");sprintf(path,"%s\\%s",mdir,exe);isbat=1;}
  if(exists(path)){
    /* Open the capture file before changing directory.  APPDATA may be a path
       relative to the Launch! directory when !SYSBAR itself was invoked by a
       relative name. */
    capture=fopen(tmp,"wb");oldout=_dup(1);
    if(capture&&oldout>=0&&_dup2(_fileno(capture),1)==0){
      if(!chdir(mdir)){
        if(isbat){comspec=getenv("COMSPEC");if(comspec&&*comspec)rc=spawnl(P_WAIT,comspec,comspec,"/C",exe,NULL);}
        else rc=spawnl(P_WAIT,exe,exe,NULL);
        chdir(cwd);
      }
      fflush(stdout);_dup2(oldout,1);
    }
    if(oldout>=0)_close(oldout);if(capture)fclose(capture);
  }
  f=fopen(tmp,"rb");if(f){while(n<m->width&&(c=fgetc(f))!=EOF&&c!='\r'&&c!='\n')out[n++]=(char)c;fclose(f);remove(tmp);}
  (void)rc;while(n<m->width)out[n++]=' ';out[n]=0;
}
static void load_colors(int *bg,int *fg){char p[ACC_PATH],line[64];FILE*f;*bg=0;*fg=7;sprintf(p,"%sLAUNCH.CFG",root);f=fopen(p,"r");if(!f)return;while(fgets(line,sizeof(line),f)){if(!strnicmp(line,"CONTROLS_BG=",12))*bg=atoi(line+12);else if(!strnicmp(line,"CONTROLS_FG=",12))*fg=atoi(line+12);}fclose(f);}
/* Compose the bar from right to left so fixed-width modules and separators fit
   the existing top-line SysBar presentation used by Launch! Core. */
static void show_bar(void){char order[MAXMOD][MODDIR],out[64];int n,i,j,x=79,bg,fg,idx;unsigned short far*v=(unsigned short far*)0xB8000000L;union REGS r;load_colors(&bg,&fg);memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);if(r.h.al==7)v=(unsigned short far*)0xB0000000L;scan_modules();n=order_load(order);if(!n){static const char *def[]={"MEM.SBM","ENV.SBM","DISK.SBM","LOCKS.SBM","MOUSE.SBM","VIDEO.SBM","NET.SBM","LOCALE.SBM"};for(i=0;i<8;i++)if(find_mod(def[i])>=0)strcpy(order[n++],def[i]);}for(i=n-1;i>=0;i--){idx=find_mod(order[i]);if(idx<0)continue;run_module(&mods[idx],out);for(j=mods[idx].width-1;j>=0&&x>=0;j--)v[x--]=(unsigned short)(((bg<<4)|fg)<<8)|(unsigned char)out[j];if(i&&x>=0)v[x--]=(unsigned short)(((bg<<4)|0)<<8)|179;} }
/* /CONFIG: two-list module manager. Installed modules are on the left; available
   modules are on the right. Ctrl+Up/Down reorders the installed list. */
static void config_dialog(void){char installed[MAXMOD][MODDIR],avail[MAXMOD][MODDIR];int ni,na=0,i,j,li=0,ri=0,focus=0,k=0,mx=0,my=0;unsigned mb=0;scan_modules();ni=order_load(installed);if(!ni){static const char *def[]={"MEM.SBM","ENV.SBM","DISK.SBM","LOCKS.SBM","MOUSE.SBM","VIDEO.SBM","NET.SBM","LOCALE.SBM"};for(i=0;i<8;i++)if(find_mod(def[i])>=0)strcpy(installed[ni++],def[i]);}for(i=0;i<modn;i++){for(j=0;j<ni;j++)if(!stricmp(mods[i].dir,installed[j]))break;if(j==ni)strcpy(avail[na++],mods[i].dir);}acc_box(7,3,66,19,"SysBar Configuration");for(;;){acc_fill(10,6,23,11,' ',ACC_BG);acc_fill(47,6,23,11,' ',ACC_BG);acc_text(10,5,"Installed modules",ACC_LABEL,20);acc_text(47,5,"Available modules",ACC_LABEL,20);for(i=0;i<ni&&i<10;i++){j=find_mod(installed[i]);acc_text(10,6+i,j>=0?mods[j].name:installed[i],focus==0&&i==li?ACC_SELECT:ACC_TEXT,20);}for(i=0;i<na&&i<10;i++){j=find_mod(avail[i]);acc_text(47,6+i,j>=0?mods[j].name:avail[i],focus==1&&i==ri?ACC_SELECT:ACC_TEXT,20);}acc_button(36,9," \021 ",focus==2);acc_button(36,12," \020 ",focus==3);acc_button(10,19,"  OK  ",focus==4);acc_button(61,19," Cancel ",focus==5);acc_wait(&k,&mx,&my,&mb);if(k==27||((mb&1)&&my==19&&mx>=61)){break;}if(k==9||k==271){focus=k==271?(focus+5)%6:(focus+1)%6;k=0;continue;}if((focus==0||focus==1)&&(k==256+72||k==256+80)){int *sel=focus?&ri:&li,nn=focus?na:ni;if(k==256+72&&*sel>0)(*sel)--;else if(k==256+80&&*sel+1<nn)(*sel)++;continue;}if(focus==0&&(k==256+141||k==256+145)){int to=li+(k==256+141?-1:1);if(to>=0&&to<ni){char t[MODDIR];strcpy(t,installed[li]);strcpy(installed[li],installed[to]);strcpy(installed[to],t);li=to;}continue;}if((k==13||k==' ')&&focus==2&&na>0&&ni<MAXMOD){strcpy(installed[ni++],avail[ri]);for(i=ri;i<na-1;i++)strcpy(avail[i],avail[i+1]);na--;if(ri>=na)ri=na?na-1:0;continue;}if((k==13||k==' ')&&focus==3&&ni>0){strcpy(avail[na++],installed[li]);for(i=li;i<ni-1;i++)strcpy(installed[i],installed[i+1]);ni--;if(li>=ni)li=ni?ni-1:0;continue;}if((k==13||k==' ')&&focus==4){order_save(installed,ni);break;}}
}
/* Entry points: /INIT creates starter packages, /CONFIG edits module order, and
   no parameter renders the SysBar once then exits. */
int main(int argc,char**argv){source_dir(argv[0]);if(argc>1&&!stricmp(argv[1],"/INIT")){init_defaults();return 0;}if(argc>1&&!stricmp(argv[1],"/CONFIG")){if(!acc_begin(argv[0],"SysBar",0))return 1;init_defaults();config_dialog();acc_end();return 0;}init_defaults();show_bar();return 0;}
