/* DOS Fetch accessory. */
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ACCLIB.H"

extern unsigned _psp;

#define MAX_DRIVES 8
#define LOG_LINES 24

static char report_lines[LOG_LINES][80];
static int report_count=0;

static const char *computer_type(void)
{
  unsigned char t=*(unsigned char far *)(((unsigned long)0xFFFF<<16)|0x000E);
  switch(t){
    case 0xFF:return "IBM PC";
    case 0xFE:return "IBM PC/XT";
    case 0xFC:return "IBM PC/AT compatible";
    case 0xFA:return "IBM PS/2 Model 30";
    case 0xF8:return "IBM PS/2 Model 80";
    default:return "IBM PC compatible";
  }
}

static const char *processor(void)
{
  unsigned a,b;
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
  a=b=0;return "80286 or later";
#endif
}

#define DF_DOS_MS    1
#define DF_DOS_PC    2
#define DF_DOS_DR    3
#define DF_DOS_FREE  4

typedef struct {
  int family;
  int major;
  int minor;
  unsigned oem;
  unsigned revision;
} DF_DOS_INFO;

static int df_contains_ci(const char *text,const char *wanted)
{
  char copy[96];int i;
  if(!text)return 0;
  strncpy(copy,text,sizeof(copy)-1);copy[sizeof(copy)-1]=0;
  for(i=0;copy[i];i++)copy[i]=(char)toupper((unsigned char)copy[i]);
  return strstr(copy,wanted)!=0;
}

static int df_dr_version_id(void)
{
  unsigned result=0,found=0;
#ifndef __GNUC__
  _asm {
    mov ax,4452h
    stc
    int 21h
    jc df_dr_done
    mov result,ax
    mov found,1
  df_dr_done:
  }
#else
  (void)result;
#endif
  return found?(int)(result&0x00FFU):-1;
}

static void df_detect_dos(DF_DOS_INFO *info)
{
  union REGS inregs,outregs;char *os,*comspec;
  int dr_id,is_dr,is_free,is_pc;
  memset(&inregs,0,sizeof(inregs));inregs.h.ah=0x30;inregs.h.al=0;
  intdos(&inregs,&outregs);
  info->major=outregs.h.al;info->minor=outregs.h.ah;
  info->oem=outregs.h.bh;info->revision=outregs.h.bl;
  os=getenv("OS");comspec=getenv("COMSPEC");

  is_free=(info->oem==0xFD)||df_contains_ci(os,"FREEDOS")||
          df_contains_ci(comspec,"FREECOM")||getenv("FREEDOS")!=0;
  dr_id=df_dr_version_id();
  is_dr=(dr_id>=0||info->oem==0xEE||info->oem==0xEF);
  is_pc=(info->oem==0);

  if(!is_dr && info->major>=5){
    memset(&inregs,0,sizeof(inregs));inregs.x.ax=0x3306;intdos(&inregs,&outregs);
    if(outregs.h.bl>=5 && outregs.h.bl<100){
      info->major=outregs.h.bl;info->minor=outregs.h.bh;
    }
  }

  if(is_dr && dr_id>=0){
    if(dr_id==0x65){info->major=5;info->minor=0;}
    else if(dr_id>=0x66&&dr_id<=0x71){info->major=6;info->minor=0;}
    else if(dr_id>=0x72){info->major=7;info->minor=0;}
    else {info->major=3;info->minor=0;}
  }

  if(is_free)info->family=DF_DOS_FREE;
  else if(is_dr)info->family=DF_DOS_DR;
  else if(is_pc)info->family=DF_DOS_PC;
  else info->family=DF_DOS_MS;
}

static const char *dos_vendor(int family)
{
  if(family==DF_DOS_PC)return "PC DOS";
  if(family==DF_DOS_DR)return "DR-DOS";
  if(family==DF_DOS_FREE)return "FreeDOS";
  return "MS-DOS";
}

static const char *basename_dos(const char *s)
{
  const char *p,*last=s;
  if(!s||!*s)return "Unknown";
  for(p=s;*p;p++)if(*p=='\\'||*p=='/')last=p+1;
  return last;
}

static void report_item(const char *label,const char *value)
{
  if(report_count>=LOG_LINES)return;
  sprintf(report_lines[report_count],"%-12s %s",label,value);
  report_lines[report_count][79]=0;
  report_count++;
}

static void fetch_item(int x,int y,const char *label,const char *value)
{
  char l[18];
  sprintf(l,"%s:",label);
  acc_text(x,y,l,ACC_TITLE,15);
  acc_text(x+14,y,value,ACC_TEXT,34);
  report_item(label,value);
}

static unsigned largest_program_kb(void)
{
  unsigned seg=_psp-1,next,size,owner;unsigned char type;unsigned long paras;
  size=*(unsigned short far *)(((unsigned long)seg<<16)|3);paras=size;
  type=*(unsigned char far *)((unsigned long)seg<<16);
  while(type!='Z'){
    next=seg+size+1;type=*(unsigned char far *)((unsigned long)next<<16);
    if(type!='M'&&type!='Z')break;
    owner=*(unsigned short far *)(((unsigned long)next<<16)|1);if(owner!=0)break;
    size=*(unsigned short far *)(((unsigned long)next<<16)|3);
    paras+=(unsigned long)size+1;seg=next;
  }
  return (unsigned)(paras/64UL);
}

static unsigned long physical_ram_kb(void)
{
  union REGS r;unsigned long kb;
  memset(&r,0,sizeof(r));r.x.ax=0xE801;int86(0x15,&r,&r);
  if(!r.x.cflag){
    if(!r.x.ax&&!r.x.bx){r.x.ax=r.x.cx;r.x.bx=r.x.dx;}
    kb=(unsigned long)_bios_memsize()+(unsigned long)r.x.ax+(unsigned long)r.x.bx*64UL;
    if(kb>=1024UL)return kb;
  }
  memset(&r,0,sizeof(r));r.h.ah=0x88;int86(0x15,&r,&r);
  if(!r.x.cflag&&r.x.ax)return (unsigned long)_bios_memsize()+(unsigned long)r.x.ax;
  return (unsigned long)_bios_memsize();
}

static int packet_driver_interrupt(void)
{
  int i,j;unsigned char far *p;static const char sig[]="PKT DRVR";
  for(i=0x60;i<=0x80;i++){
    p=(unsigned char far *)_dos_getvect(i);if(!p)continue;
    p+=3;
    for(j=0;j<8;j++)if(p[j]!=(unsigned char)sig[j])break;
    if(j==8)return i;
  }
  return -1;
}

static const char *video_name(void)
{
  static char s[32];union REGS r;unsigned eq;
  memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);
  if(r.h.al==0x1A){sprintf(s,"VGA %dx%d text",acc_cols,acc_rows);return s;}
  eq=_bios_equiplist();
  switch((eq>>4)&3){
    case 3:sprintf(s,"MDA %dx%d text",acc_cols,acc_rows);break;
    case 1:sprintf(s,"CGA 40x25 text");break;
    case 2:sprintf(s,"CGA %dx%d text",acc_cols,acc_rows);break;
    default:sprintf(s,"EGA %dx%d text",acc_cols,acc_rows);break;
  }
  return s;
}

/* DOS Fetch uses the three CP437 block forms in LOGOS.TXT.  Launch!'s
   UI font deliberately repurposes some CP437 positions, so restore these
   three shapes privately while DFETCH is active and put the user's glyphs
   back before returning to Launch!. */
static unsigned char fetch_old_blocks[4][32];
static int fetch_blocks_saved=0;
static const unsigned char fetch_full16[]={
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char fetch_upper16[]={
  255,255,255,255,255,255,255,255,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char fetch_lower16[]={
  0,0,0,0,0,0,0,0,255,255,255,255,255,255,255,255,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char fetch_shade16[]={
  0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char fetch_full14[]={
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char fetch_upper14[]={
  255,255,255,255,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char fetch_lower14[]={
  0,0,0,0,0,0,0,255,255,255,255,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char fetch_shade14[]={
  0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static void fetch_blocks_install(void)
{
  int h=acc_font_height();
  if(!fetch_blocks_saved){
    acc_glyph_read(176,fetch_old_blocks[0]);acc_glyph_read(219,fetch_old_blocks[1]);
    acc_glyph_read(220,fetch_old_blocks[2]);acc_glyph_read(223,fetch_old_blocks[3]);
    fetch_blocks_saved=1;
  }
  acc_glyph_write(176,h==14?fetch_shade14:fetch_shade16);
  acc_glyph_write(219,h==14?fetch_full14:fetch_full16);
  acc_glyph_write(220,h==14?fetch_lower14:fetch_lower16);
  acc_glyph_write(223,h==14?fetch_upper14:fetch_upper16);
}
static void fetch_blocks_restore(void)
{
  if(!fetch_blocks_saved)return;
  acc_glyph_write(176,fetch_old_blocks[0]);acc_glyph_write(219,fetch_old_blocks[1]);
  acc_glyph_write(220,fetch_old_blocks[2]);acc_glyph_write(223,fetch_old_blocks[3]);
  fetch_blocks_saved=0;
}

static int xms_present(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x4300;int86(0x2F,&r,&r);
  return r.h.al==0x80;
}
static int ems_present(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.h.ah=0x40;int86(0x67,&r,&r);
  return r.h.ah==0;
}
static int file_mentions(const char *path,const char *needle)
{
  FILE *f;char line[160],up[160];int i;
  f=fopen(path,"rt");if(!f)return 0;
  while(fgets(line,sizeof(line),f)){
    for(i=0;line[i]&&i<159;i++)up[i]=(char)toupper((unsigned char)line[i]);up[i]=0;
    if(strstr(up,needle)){fclose(f);return 1;}
  }
  fclose(f);return 0;
}
static const char *memory_manager(void)
{
  int xms=xms_present(),ems=ems_present();
  if(!xms&&!ems)return "None";
  /* Confirm presence first, then use the boot configuration only to identify
     which compatible manager supplied the live XMS/EMS interface. */
  if(file_mentions("C:\\CONFIG.SYS","386MAX.SYS"))return "386MAX";
  if(file_mentions("C:\\CONFIG.SYS","QEMM386.SYS"))return "QEMM386";
  if(file_mentions("C:\\CONFIG.SYS","QMAX.SYS"))return "QMAX";
  if(file_mentions("C:\\CONFIG.SYS","JEMM386"))return "JEMM386";
  if(file_mentions("C:\\CONFIG.SYS","EMM386.EXE"))return "EMM386";
  if(file_mentions("C:\\CONFIG.SYS","HIMEMX"))return ems?"HIMEMX + EMS":"HIMEMX";
  if(file_mentions("C:\\CONFIG.SYS","HIMEM.SYS"))return ems?"HIMEM + EMS":"HIMEM";
  if(ems&&xms)return "EMS/XMS manager";
  if(ems)return "EMS manager";
  return "XMS manager";
}

static void logo_text(int x,int y,const char *p,int fg)
{
  int i,a=ACC_ATTR(acc_appearance.background,fg);
  for(i=0;p[i];i++)if(p[i]!=' ')acc_put(x+i,y,(unsigned char)p[i],a);
}

/* LOGOS.TXT artwork is embedded here so DOS Fetch has no runtime resource. */
static const char *logo_ms[]={
 " \333\334     \334\333 \334\337\337\337\337\334",
 " \333 \333   \333 \333  \337\337\334\334",
 " \333  \337\334\337  \333 \337\334\334\334\334\337",
 "\333\337\337\337\334 \334\337\337\337\337\334\334\337\337\337\337\334",
 "\333    \333\333    \333\337\334\334    ",
 "\333    \333\333    \333   \337\337\334 ",
 "\333\334\334\334\337 \337\334\334\334\334\337\337\334\334\334\334\337"};
static const char *logo_pc[]={
 "    \333\337\337\334 \334\337\337\337\334",
 "    \333\334\334\337 \333",
 "    \333    \337\334\334\334\337",
 "\333\337\337\337\334  \334\334\334\334 \334\337\337\337\337\334",
 "\333    \333\333    \333\337\334\334    ",
 "\333    \333\333    \333   \337\337\334 ",
 "\333\334\334\334\337 \333    \333\337\334\334\334\334\337 ",
 "       \337\337\337\337"};
static const char *logo_dr[]={
 "\333\337\337\337\337\337\333  \333\337\337\334 \333\337\337\334",
 "\333 \333 \260 \333  \333  \333 \333\334\334\337 ",
 "\333\334\334\334\334\334\333  \333\334\334\337 \333  \333",
 "",
 "\333\337\337\337\334 \334\337\337\337\337\334\334\337\337\337\337\334",
 "\333    \333\333    \333\337\334\334    ",
 "\333    \333\333    \333   \337\337\334 ",
 "\333\334\334\334\337 \337\334\334\334\334\337\337\334\334\334\334\337"};
static const char *logo_free[]={
 "\333\337\337\337 \333\334\337 \334\337\337\334 \334\337\337\334",
 "\333\337\337  \333   \333\337\337  \333\337\337",
 "\333    \333   \337\334\334\334 \337\334\334\334",
 "",
 "\333\337\337\337\334 \334\337\337\337\337\334\334\337\337\337\337\334",
 "\333    \333\333    \333\337\334\334    ",
 "\333    \333\333    \333   \337\337\334 ",
 "\333\334\334\334\337 \337\334\334\334\334\337\337\334\334\334\334\337"};

static void draw_dos_word(int x,int y,const char **l,int first_row)
{
  int r;
  /* D=0..5, O=6..11, S=12..17 in the supplied artwork. */
  for(r=first_row;r<first_row+4;r++){
    char a[7],b[7],c[8];
    strncpy(a,l[r],6);a[6]=0;
    strncpy(b,l[r]+6,6);b[6]=0;
    strncpy(c,l[r]+12,7);c[7]=0;
    logo_text(x,y+r,a,12);       /* D: Bright Red */
    logo_text(x+6,y+r,b,14);     /* O: Bright Yellow, overridden by caller where needed */
    logo_text(x+12,y+r,c,12);    /* S: Bright Red */
  }
}

static void draw_logo(int x,int y,int family)
{
  int r;
  if(family==DF_DOS_PC){ /* IBM PC DOS */
    for(r=0;r<3;r++)logo_text(x,y+r,logo_pc[r],9);
    for(r=3;r<7;r++){
      char d[7],o[7],ss[8];strncpy(d,logo_pc[r],6);d[6]=0;strncpy(o,logo_pc[r]+6,6);o[6]=0;strncpy(ss,logo_pc[r]+12,7);ss[7]=0;
      logo_text(x,y+r,d,11);logo_text(x+6,y+r,o,14);logo_text(x+12,y+r,ss,12);
    }
    logo_text(x,y+7,logo_pc[7],14);
  } else if(family==DF_DOS_DR){ /* DR-DOS */
    for(r=0;r<3;r++){logo_text(x,y+r,logo_dr[r],9);logo_text(x+9,y+r,logo_dr[r]+9,12);}
    for(r=4;r<8;r++)logo_text(x,y+r,logo_dr[r],12);
  } else if(family==DF_DOS_FREE){ /* FreeDOS */
    for(r=0;r<3;r++)logo_text(x,y+r,logo_free[r],2);
    for(r=4;r<8;r++){char d[7],o[7],ss[8];strncpy(d,logo_free[r],6);d[6]=0;strncpy(o,logo_free[r]+6,6);o[6]=0;strncpy(ss,logo_free[r]+12,7);ss[7]=0;logo_text(x,y+r,d,10);logo_text(x+6,y+r,o,10);logo_text(x+12,y+r,ss,10);}
  } else { /* MS-DOS */
    for(r=0;r<3;r++)logo_text(x,y+r,logo_ms[r],7);
    for(r=3;r<7;r++){
      char d[7],o[7],ss[8];strncpy(d,logo_ms[r],6);d[6]=0;strncpy(o,logo_ms[r]+6,6);o[6]=0;strncpy(ss,logo_ms[r]+12,7);ss[7]=0;
      logo_text(x,y+r,d,12);logo_text(x+6,y+r,o,5);logo_text(x+12,y+r,ss,14);
    }
  }
}

static void draw_palette(int x,int y)
{
  int i;
  for(i=0;i<8;i++)acc_fill(x+i*4,y,4,1,219,ACC_ATTR(acc_appearance.background,i));
  for(i=0;i<8;i++)acc_fill(x+i*4,y+1,4,1,219,ACC_ATTR(acc_appearance.background,i+8));
}

static int print_info(void)
{
  FILE *f;int i;
  f=fopen("LPT1","wb");if(!f)return 0;
  fputs("DOS Fetch - Launch! 3.71\r\n",f);
  fputs("========================\r\n\r\n",f);
  for(i=0;i<report_count;i++){fputs(report_lines[i],f);fputs("\r\n",f);}
  fputc('\f',f);fclose(f);return 1;
}

static const char *kernel_name(int family)
{
  if(family==DF_DOS_PC)return "IBMBIO.COM";
  if(family==DF_DOS_FREE)return "KERNEL.SYS";
  if(family==DF_DOS_DR)return "DRBIO.SYS";
  return "IO.SYS";
}

static const char *country_name(unsigned c)
{
  switch(c){case 1:return "USA";case 44:return "UK";case 49:return "Germany";case 61:return "Australia";case 64:return "New Zealand";case 81:return "Japan";default:return "";}
}

static unsigned dos_country(void)
{
  union REGS r;struct SREGS sr;static unsigned char info[40];
  memset(&r,0,sizeof(r));segread(&sr);r.x.ax=0x3800;sr.ds=FP_SEG(info);r.x.dx=FP_OFF(info);int86x(0x21,&r,&r,&sr);
  if(r.x.cflag)return 0;return r.x.bx;
}

static int cdrom_first_drive(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1500;int86(0x2F,&r,&r);if(r.x.bx)return r.x.cx;return -1;
}

static void draw_disk_bar(int x,int y,char drive,unsigned pct)
{
  int i,used=(int)((pct*20UL+50UL)/100UL);char b[16];
  acc_put(x,y,drive,ACC_TEXT);acc_put(x+1,y,':',ACC_TEXT);
  for(i=0;i<20;i++)acc_put(x+2+i,y,i<used?219:176,
    i<used?(pct>95?ACC_ATTR(acc_appearance.background,4):ACC_TEXT):ACC_LABEL);
  sprintf(b," (%u%% used)",pct);acc_text(x+22,y,b,ACC_TEXT,12);
}

static void drive_list(char *out,int floppy,int cd)
{
  int i,p=0;out[0]=0;for(i=0;i<floppy;i++)p+=sprintf(out+p,"%c: ",'A'+i);if(!p)strcpy(out,"None");
}

int main(int argc,char **argv)
{
  union REGS r;char s[80],floppy_s[24],cd_s[16];
  char drive_letter[MAX_DRIVES];unsigned drive_percent[MAX_DRIVES];
  unsigned long drive_total[MAX_DRIVES],drive_free[MAX_DRIVES];
  unsigned eq,total,avail,country;unsigned long physical,tk,fk;DF_DOS_INFO dos;
  int x=2,y=1,w=75,h=22,row,d,drive_count=0,i;
  int serial,parallel,game,netint,flops,cd;
  const char *cs,*blaster;

  if(acc_help(argc,argv,"!DFETCH","DOS Fetch - a FastFetch-style view of DOS hardware and system information."))return 0;
  if(!acc_begin(argv[0],"DOS Fetch",0))return 1;
  fetch_blocks_install();
  if(acc_cols<80){x=0;w=acc_cols;}if(acc_rows<25){y=0;h=acc_rows;}

  report_count=0;
  for(d=3;d<=26&&drive_count<MAX_DRIVES;d++){
    memset(&r,0,sizeof(r));r.h.ah=0x36;r.h.dl=(unsigned char)d;int86(0x21,&r,&r);
    if(r.x.ax!=0xFFFF){
      tk=(unsigned long)r.x.ax*r.x.cx*r.x.dx/1024UL;fk=(unsigned long)r.x.ax*r.x.cx*r.x.bx/1024UL;
      drive_letter[drive_count]=(char)('A'+d-1);drive_total[drive_count]=tk;drive_free[drive_count]=fk;
      drive_percent[drive_count]=tk?(unsigned)((tk-fk)*100UL/tk):0;drive_count++;
    }
  }

  df_detect_dos(&dos);

  acc_box(x,y,w,h,"DOS Fetch");
  /* DFETCH has no toolbar: remove acc_box()'s standard toolbar divider. */
  acc_fill(x+1,y+h-4,w-2,1,' ',ACC_BG);
  acc_put(x,y+h-4,179,ACC_BORDER);acc_put(x+w-1,y+h-4,179,ACC_BORDER);
  draw_logo(x+3,y+6,dos.family);
  row=y+2;
  if(dos.family==DF_DOS_DR)
    sprintf(s,"%s %d.%02d",dos_vendor(dos.family),dos.major,dos.minor);
  else
    sprintf(s,"%s %d.%02d REV %u",dos_vendor(dos.family),dos.major,dos.minor,dos.revision);
  fetch_item(x+24,row++,"OS",s);
  fetch_item(x+24,row++,"Host",computer_type());
  fetch_item(x+24,row++,"Kernel",kernel_name(dos.family));
  cs=getenv("COMSPEC");fetch_item(x+24,row++,"Shell",basename_dos(cs));
  fetch_item(x+24,row++,"Display",video_name());
  fetch_item(x+24,row++,"CPU",processor());

  total=_bios_memsize();avail=largest_program_kb();physical=physical_ram_kb();
  if(physical>=1024UL)sprintf(s,"%lu MB RAM, %u KB DOS free",(physical+512UL)/1024UL,avail);else sprintf(s,"%lu KB RAM, %u KB DOS free",physical,avail);
  fetch_item(x+24,row++,"Memory",s);
  fetch_item(x+24,row++,"Manager",memory_manager());

  if(drive_count){char lab[18];sprintf(lab,"System Disk");report_item(lab,"");acc_text(x+24,row,"System Disk:",ACC_TITLE,15);draw_disk_bar(x+38,row,drive_letter[0],drive_percent[0]);row++;}
  else fetch_item(x+24,row++,"System Disk","None detected");

  eq=_bios_equiplist();flops=(eq&1)?((eq>>6)&3)+1:0;drive_list(floppy_s,flops,0);fetch_item(x+24,row++,"Floppy",floppy_s);
  cd=cdrom_first_drive();if(cd>=0){sprintf(cd_s,"%c:",'A'+cd);fetch_item(x+24,row++,"CD-ROM",cd_s);}else fetch_item(x+24,row++,"CD-ROM","None");
  fetch_item(x+24,row++,"Mouse",acc_mouse_present?"Installed":"Not detected");
  netint=packet_driver_interrupt();if(netint>=0){sprintf(s,"Packet driver at INT %02Xh",netint);fetch_item(x+24,row++,"Network",s);}else fetch_item(x+24,row++,"Network","Not detected");
  blaster=getenv("BLASTER");if(blaster&&*blaster){fetch_item(x+24,row++,"Audio","Sound Blaster or compatible");}else fetch_item(x+24,row++,"Audio","Not detected");
  serial=(eq>>9)&7;parallel=(eq>>14)&3;game=(eq&0x1000)?1:0;sprintf(s,"%d serial, %d parallel%s",serial,parallel,game?", 1 game":"");fetch_item(x+24,row++,"Ports",s);
  country=dos_country();if(country){sprintf(s,"%03u - %s",country,country_name(country));fetch_item(x+24,row++,"Locale",s);}else fetch_item(x+24,row++,"Locale","Unknown");

  draw_palette(x+24,y+19);
  for(;;){
    int k=0,mx=0,my=0;unsigned mb=0;
    acc_wait(&k,&mx,&my,&mb);
    if(k==27)break;
    if(k==16){if(!print_info())acc_notice("Print","Unable to print DOS Fetch information to LPT1.");}
  }
  fetch_blocks_restore();
  acc_end();return 0;
}
