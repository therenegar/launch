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
 * File: SBMMOD.C
 * Role: Built-in SysBar module helper/program
 * Build/ownership: Builds the initial SBM.EXE used inside default *.SBM packages.
 * Maintainer contract: Keep module output single-line/plain text so !SYSBAR can safely capture and clip it.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* Launch! SysBar Module helper - Release 3.73 initial module set. */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#define MAKE_FP(seg,off) ((void far *)((((unsigned long)(seg))<<16)|(unsigned short)(off)))
static unsigned first_mcb(void){union REGS r;struct SREGS s;memset(&r,0,sizeof(r));memset(&s,0,sizeof(s));r.h.ah=0x52;int86x(0x21,&r,&r,&s);return *(unsigned short far*)MAKE_FP(s.es,r.x.bx-2);}
static unsigned largest_kb(void){unsigned seg=first_mcb(),owner,size;unsigned char type;unsigned long run=0,best=0;int guard=0;if(!seg)return 0;while(guard++<256){type=*(unsigned char far*)MAKE_FP(seg,0);owner=*(unsigned short far*)MAKE_FP(seg,1);size=*(unsigned short far*)MAKE_FP(seg,3);if(owner==0||owner==_psp){run+=(unsigned long)size+1UL;if(run>best)best=run;}else run=0;if(type=='Z'||type!='M')break;seg=(unsigned)(seg+size+1);}return(unsigned)(best/64UL);}
static unsigned env_free_kb(void){unsigned parent=*(unsigned short far*)MAKE_FP(_psp,0x16),eseg,paras;unsigned long bytes,used=0;char far*p;if(!parent)parent=_psp;eseg=*(unsigned short far*)MAKE_FP(parent,0x2C);if(!eseg)return 0;paras=*(unsigned short far*)MAKE_FP(eseg-1,3);bytes=(unsigned long)paras*16UL;p=(char far*)MAKE_FP(eseg,0);while(used+1<bytes){if(p[used]==0&&p[used+1]==0){used+=2;break;}used++;}return bytes>used?(unsigned)((bytes-used+1023UL)/1024UL):0;}
static int packet_driver(void){unsigned v,off,seg,i;static const char sig[]="PKT DRVR";for(v=0x60;v<=0x80;v++){off=*(unsigned short far*)MAKE_FP(0,v*4);seg=*(unsigned short far*)MAKE_FP(0,v*4+2);if(!seg)continue;for(i=0;i<8;i++)if(*(unsigned char far*)MAKE_FP(seg,off+3+i)!=(unsigned char)sig[i])break;if(i==8)return 1;}return 0;}
static void drive_free(char*out){char*cs=getenv("COMSPEC");unsigned drive;union REGS r;unsigned long bytes;if(cs&&cs[0]&&cs[1]==':')drive=(unsigned)(toupper((unsigned char)cs[0])-'A'+1);else{memset(&r,0,sizeof(r));r.h.ah=0x19;int86(0x21,&r,&r);drive=(unsigned)r.h.al+1;}memset(&r,0,sizeof(r));r.h.ah=0x36;r.h.dl=(unsigned char)drive;int86(0x21,&r,&r);if(r.x.ax==0xFFFF){sprintf(out,"%c: ?",(char)('A'+drive-1));return;}bytes=(unsigned long)r.x.ax*r.x.bx*r.x.cx;if(bytes>=1024UL*1024UL*1024UL)sprintf(out,"%c: %luGB",(char)('A'+drive-1),(bytes+(512UL*1024UL*1024UL))/(1024UL*1024UL*1024UL));else if(bytes>=1024UL*1024UL)sprintf(out,"%c: %luMB",(char)('A'+drive-1),(bytes+512UL*1024UL)/(1024UL*1024UL));else sprintf(out,"%c: %luKB",(char)('A'+drive-1),(bytes+512UL)/1024UL);}
static void module_id(char*out){char cwd[128],*p;getcwd(cwd,sizeof(cwd));p=strrchr(cwd,'\\');p=p?p+1:cwd;strncpy(out,p,15);out[15]=0;p=strchr(out,'.');if(p)*p=0;strupr(out);}
int main(void){char id[16],s[20],cc[8]="INT";union REGS r;unsigned locks,ctry=0;unsigned char country[34];module_id(id);if(!strcmp(id,"MEM")){printf("MEM %uKB",largest_kb());return 0;}if(!strcmp(id,"ENV")){printf("ENV %uKB",env_free_kb());return 0;}if(!strcmp(id,"DISK")){drive_free(s);printf("%s",s);return 0;}if(!strcmp(id,"LOCKS")){locks=*(unsigned char far*)MAKE_FP(0x40,0x17);printf("CL%c NL%c SL%c",(locks&0x40)?'+':'-',(locks&0x20)?'+':'-',(locks&0x10)?'+':'-');return 0;}if(!strcmp(id,"MOUSE")){memset(&r,0,sizeof(r));r.x.ax=0;int86(0x33,&r,&r);printf("MOUSE %s",r.x.ax?"ON":"OFF");return 0;}if(!strcmp(id,"VIDEO")){memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);printf("%s",r.h.al==0x1A?"VGA":"EGA");return 0;}if(!strcmp(id,"NET")){printf("NET %s",packet_driver()?"ON":"OFF");return 0;}if(!strcmp(id,"LOCALE")){memset(country,0,sizeof(country));memset(&r,0,sizeof(r));r.h.ah=0x38;r.h.al=0;r.x.dx=(unsigned)country;int86(0x21,&r,&r);ctry=r.x.bx;if(ctry==61)strcpy(cc,"AUS");else if(ctry==1)strcpy(cc,"USA");else if(ctry==44)strcpy(cc,"GBR");printf("%u-%s",ctry,cc);return 0;}return 0;}
