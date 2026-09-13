/* Launch! System Info accessory. */
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ACCLIB.H"
extern unsigned _psp;
static const char *computer_type(void){unsigned char t=*(unsigned char far *)(((unsigned long)0xFFFF<<16)|0x000E);switch(t){case 0xFF:return"IBM PC";case 0xFE:return"IBM PC/XT";case 0xFC:return"IBM PC/AT compatible";case 0xFA:return"IBM PS/2 Model 30";case 0xF8:return"IBM PS/2 Model 80";default:return"IBM PC compatible";}}
static const char *processor(void){unsigned a,b;
#ifndef __GNUC__
 _asm {
  pushf
  pop ax
  mov a,ax
  xor ax,4000h
  push ax
  popf
  pushf
  pop ax
  mov b,ax
  mov ax,a
  push ax
  popf
 }
 return ((a^b)&0x4000)?"80386 or later":"80286";
#else
 a=b=0;return"80286 or later";
#endif
}
static const char *dos_vendor(unsigned oem){if(oem==0)return"PC DOS";if(oem==0xFD)return"FreeDOS";if(oem==0xEE)return"DR-DOS";return"MS-DOS";}
static char sys_lines[20][80];static int sys_line_count=0;
static void log_item(const char *label,const char *value){int n,end=78;if(sys_line_count>=20)return;memset(sys_lines[sys_line_count],' ',79);sys_lines[sys_line_count][79]=0;n=(int)strlen(label);if(n>27)n=27;memcpy(sys_lines[sys_line_count],label,n);n=(int)strlen(value);if(n>52)n=52;memcpy(sys_lines[sys_line_count]+27,value,n);while(end>=0&&sys_lines[sys_line_count][end]==' ')sys_lines[sys_line_count][end--]=0;sys_line_count++;}
static void item(int x,int y,const char *label,const char *value){acc_text(x,y,label,ACC_LABEL,23);acc_text(x+27,y,value,ACC_TEXT,37);log_item(label,value);}
static void drive_item(int x,int y,const char *label,char drive,unsigned percent){char s[40],bar[11];int i,used=(percent+5)/10,fg=percent>=90?acc_appearance.main_title:acc_appearance.controls_fg,attr=ACC_ATTR(acc_appearance.controls_bg,fg);if(used>10)used=10;acc_text(x,y,label,ACC_LABEL,23);acc_put(x+27,y,drive,ACC_TEXT);acc_put(x+28,y,':',ACC_TEXT);for(i=0;i<10;i++){acc_put(x+31+i,y,i<used?219:176,attr);bar[i]=(char)(i<used?'#':'.');}bar[10]=0;sprintf(s,"%u%% used",percent);acc_text(x+42,y,s,ACC_TEXT,9);sprintf(s,"%c: %s %u%% used",drive,bar,percent);log_item(label,s);}
static int print_info(void){FILE *f;int i;f=fopen("LPT1","wb");if(!f)return 0;fputs("Launch! System Information\r\n",f);fputs("==========================\r\n\r\n",f);for(i=0;i<sys_line_count;i++){fputs(sys_lines[i],f);fputs("\r\n",f);}fputc('\f',f);fclose(f);return 1;}
static unsigned largest_program_kb(void){unsigned seg=_psp-1,next,size,owner;unsigned char type;unsigned long paras;size=*(unsigned short far *)(((unsigned long)seg<<16)|3);paras=size;type=*(unsigned char far *)((unsigned long)seg<<16);while(type!='Z'){next=seg+size+1;type=*(unsigned char far *)((unsigned long)next<<16);if(type!='M'&&type!='Z')break;owner=*(unsigned short far *)(((unsigned long)next<<16)|1);if(owner!=0)break;size=*(unsigned short far *)(((unsigned long)next<<16)|3);paras+=(unsigned long)size+1;seg=next;}return(unsigned)(paras/64UL);}
static int xms_info(unsigned *total,unsigned *largest){int ok=0;unsigned xt=0,xl=0;
#ifndef __GNUC__
 void (far *entry)(void);
 _asm {
  mov ax,4300h
  int 2fh
  cmp al,80h
  jne absent
  mov ax,4310h
  int 2fh
  mov word ptr entry,bx
  mov word ptr entry+2,es
  mov ah,08h
  call dword ptr entry
  mov xt,dx
  mov xl,ax
  mov ok,1
 absent:
 }
 *total=xt;*largest=xl;
#else
 (void)total;(void)largest;
#endif
 return ok;
}
int main(int argc,char **argv){union REGS r;char s[80],drive_letter[9];unsigned drive_percent[9],eq,total,avail,ems_total=0,ems_free=0;int x,y,h,row,maj,min,d,flops,drive_count=0,i,close_row,focus=1;unsigned long tk,fk;const char *cs;if(acc_help(argc,argv,"!SYSINFO","Reports essential DOS computer hardware and system details."))return 0;if(!acc_begin(argv[0],"System Info",0))return 1;sys_line_count=0;for(d=3;d<=26&&drive_count<9;d++){memset(&r,0,sizeof(r));r.h.ah=0x36;r.h.dl=(unsigned char)d;int86(0x21,&r,&r);if(r.x.ax!=0xFFFF){tk=(unsigned long)r.x.ax*r.x.cx*r.x.dx/1024UL;fk=(unsigned long)r.x.ax*r.x.cx*r.x.bx/1024UL;drive_letter[drive_count]=(char)('A'+d-1);drive_percent[drive_count]=tk?(unsigned)((tk-fk)*100UL/tk):0;drive_count++;}}h=16+(drive_count?drive_count:1);x=(acc_cols-72)/2;y=(acc_rows-h)/2;acc_box(x,y,72,h,"System Info");row=y+2;eq=_bios_equiplist();
 item(x+3,row++,"Computer type",computer_type());item(x+3,row++,"Processor",processor());memset(&r,0,sizeof(r));r.x.ax=0x3000;int86(0x21,&r,&r);maj=r.h.al;min=r.h.ah;sprintf(s,"%s %d.%02d REV %u",dos_vendor(r.h.bh),maj,min,(unsigned)r.h.bl);item(x+3,row++,"Operating System",s);cs=getenv("COMSPEC");item(x+3,row++,"Command Interpreter",cs?cs:"Unknown");
 total=_bios_memsize();avail=largest_program_kb();sprintf(s,"%u KB total, %u KB avail.",total,avail);item(x+3,row++,"Conventional memory",s);
 memset(&r,0,sizeof(r));r.h.ah=0x42;int86(0x67,&r,&r);if(r.h.ah==0){ems_free=r.x.bx*16U;ems_total=r.x.dx*16U;sprintf(s,"%u KB total, %u KB avail.",ems_total,ems_free);}else strcpy(s,"Not available");item(x+3,row++,"Expanded (EMS) memory",s);
 {unsigned xt,xl;if(xms_info(&xt,&xl))sprintf(s,"%u KB total, %u KB avail.",xt,xl);else strcpy(s,"Not available");}
 item(x+3,row++,"Extended (XMS) memory",s);
 if(drive_count)for(i=0;i<drive_count;i++)drive_item(x+3,row++,i?"":"Hard drives",drive_letter[i],drive_percent[i]);else item(x+3,row++,"Hard drives","None detected");
 flops=(eq&1)?((eq>>6)&3)+1:0;sprintf(s,"%d",flops);item(x+3,row++,"Floppy drives",s);memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);item(x+3,row++,"Video adapter",r.h.al==0x1A?"VGA or compatible":"EGA or compatible");item(x+3,row++,"Pointing device",acc_mouse_present?"Mouse installed":"Not detected");
 close_row=row+1;for(;;){int k=0,mx=0,my=0;unsigned mb=0;acc_button(x+3,close_row,"  Print  ",focus==0);acc_button(x+59,close_row,"  Close  ",focus==1);acc_wait(&k,&mx,&my,&mb);if(k==27)break;if((mb&1)&&my==close_row){if(mx>=x+3&&mx<x+12){focus=0;k=13;}else if(mx>=x+59&&mx<x+68){focus=1;k=13;}}if(k==9){focus=1-focus;continue;}if(k==13){if(focus==0){if(!print_info())acc_notice("Print","Unable to print system information to LPT1.");}else break;}}acc_end();return 0;}
