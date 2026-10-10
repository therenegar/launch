/*
    __                           __    __
   / /   ____ ___  ______  _____/ /_  / /
  / /   / __ `/ / / / __ \/ ___/ __ \/ / 
 / /___/ /_/ / /_/ / / / /__/ / / /_/  
/_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
Launch! for DOS ---------------------
*/
/*
 * MAINTAINER NOTES - Launch! 3.79
 * File: INSTALL.C
 * Role: Canonical installer source
 * Build/ownership: Authoritative installer source; GENBUILD produces INSTBLD.C.
 * Maintainer contract: Installs Core/Accessories/Games/data files and updates DOS startup configuration without assuming COMMAND.COM.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
/* Launch! 3.79 installer - Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <direct.h>
#include <io.h>
#include <process.h>

static int startup_contains_ci(const char *s,const char *n)
{
  int i;if(!s||!n||!*n)return 0;for(;*s;s++){for(i=0;n[i]&&s[i]&&toupper((unsigned char)s[i])==toupper((unsigned char)n[i]);i++);if(!n[i])return 1;}return 0;
}
static int startup_is_freedos(void)
{
  union REGS inr,outr;char *os,*comspec;memset(&inr,0,sizeof(inr));inr.h.ah=0x30;inr.h.al=0;intdos(&inr,&outr);
  if(outr.h.bh==0xFD)return 1;os=getenv("OS");comspec=getenv("COMSPEC");
  return startup_contains_ci(os,"FREEDOS")||startup_contains_ci(comspec,"FREECOM")||getenv("FREEDOS")!=0;
}
static const char *startup_batch_suffix(void)
{
  return startup_is_freedos()?":\\FDAUTO.BAT":":\\AUTOEXEC.BAT";
}
#define PATH_SIZE 128
#define DAT_MAGIC "L361Z1\032"

static unsigned char copy_buffer[4096];
static void (interrupt far *old_int09)();
static volatile unsigned char capture_scan,capture_e0,capture_mods;
static volatile unsigned char capture_key,capture_key_mods;

#ifndef __GNUC__
static void interrupt far capture_int09()
{
  unsigned char scan,base,mask;
  _asm { in al, 60h }
  _asm { mov capture_scan, al }
  scan=capture_scan;
  if(scan==0xE0)capture_e0=1;
  else {
    base=(unsigned char)(scan&0x7F);mask=0;
    if(base==0x1D)mask=4;
    else if(base==0x38)mask=8;
    else if(base==0x2A || base==0x36)mask=1;
    if(mask){
      if(scan&0x80)capture_mods&=(unsigned char)~mask;
      else capture_mods|=mask;
    } else if(!(scan&0x80) && !capture_key){
      capture_key=base;capture_key_mods=capture_mods;
    }
    capture_e0=0;
  }
  _chain_intr(old_int09);
}
#else
static void capture_int09(void) {}
#endif

static int rom_contains_dosbox(unsigned segment,unsigned offset,unsigned length)
{
  const unsigned char far *rom;
  static const char name[]="DOSBOX";unsigned i,j;
  /* Build the 16:16 address directly.  Some Microsoft C 7 installations do
     not provide MK_FP as a macro and otherwise emit an unresolved _MK_FP. */
  rom=(const unsigned char far *)
      (((unsigned long)segment<<16)|(unsigned long)offset);
  for(i=0;i+sizeof(name)-1<=length;i++){
    for(j=0;j<sizeof(name)-1;j++)
      if(toupper(rom[i+j])!=name[j])break;
    if(j==sizeof(name)-1)return 1;
  }
  return 0;
}

static int running_in_dosbox(void)
{
  /* DOSBox and DOSBox-X identify themselves in the system or video BIOS. */
  return rom_contains_dosbox(0xF000,0xE000,0x2000)||
         rom_contains_dosbox(0xC000,0x0000,0x8000);
}

static int exists(const char *name)
{
  FILE *f=fopen(name,"rb");
  if(!f)return 0;
  fclose(f);return 1;
}

static void source_directory(const char *program,char *directory)
{
  static char exe[PATH_SIZE],candidate[PATH_SIZE];
  const char *path,*end,*slash,*other;int n;
  strncpy(exe,program,PATH_SIZE-1);exe[PATH_SIZE-1]=0;
  if(!strchr(exe,'\\') && !strchr(exe,'/') && !strchr(exe,':')){
    path=getenv("PATH");
    while(path && *path){
      end=strchr(path,';');n=end?(int)(end-path):(int)strlen(path);
      if(n>0 && n<PATH_SIZE-(int)strlen(exe)-2){
        strncpy(candidate,path,n);candidate[n]=0;
        if(candidate[n-1]!='\\' && candidate[n-1]!='/')strcat(candidate,"\\");
        strcat(candidate,exe);
        if(exists(candidate)){strcpy(exe,candidate);break;}
      }
      if(!end)break;
      path=end+1;
    }
  }
  slash=strrchr(exe,'\\');other=strrchr(exe,'/');
  if(!slash || (other && other>slash))slash=other;
  if(slash)n=(int)(slash-exe)+1;
  else if(exe[0] && exe[1]==':')n=2;
  else n=0;
  strncpy(directory,exe,n);directory[n]=0;
}

static int make_directories(char *path)
{
  static char work[PATH_SIZE];int i,start;
  strcpy(work,path);start=(work[1]==':')?3:1;
  for(i=start;work[i];i++)if(work[i]=='\\' || work[i]=='/'){
    char saved=work[i];work[i]=0;_mkdir(work);work[i]=saved;
  }
  _mkdir(work);
  return _access(work,0)==0;
}

static unsigned read_u16(FILE *f)
{unsigned a=(unsigned)fgetc(f),b=(unsigned)fgetc(f);return a|(b<<8);}

static unsigned long read_u32(FILE *f)
{unsigned long a=(unsigned char)fgetc(f),b=(unsigned char)fgetc(f),c=(unsigned char)fgetc(f),d=(unsigned char)fgetc(f);return a|(b<<8)|(c<<16)|(d<<24);}

static int decompress_file(FILE *in,FILE *out,unsigned long csize,unsigned long usize)
{
  unsigned long produced=0,pos=0;unsigned flags,bit,b1,b2,dist,len,j;int c;
  memset(copy_buffer,0,sizeof(copy_buffer));
  while(produced<usize){
    if(!csize)return 0;c=fgetc(in);if(c==EOF)return 0;flags=(unsigned char)c;csize--;
    for(bit=0;bit<8&&produced<usize;bit++){
      if(flags&(1U<<bit)){
        if(!csize)return 0;c=fgetc(in);if(c==EOF)return 0;csize--;
        copy_buffer[(unsigned)(pos&4095UL)]=(unsigned char)c;
        if(fputc(c,out)==EOF)return 0;pos++;produced++;
      }else{
        if(csize<2)return 0;b1=(unsigned char)fgetc(in);b2=(unsigned char)fgetc(in);csize-=2;
        dist=b1|((b2>>4)<<8);len=(b2&15)+3;
        if(!dist||dist>4095U||(unsigned long)dist>produced)return 0;
        for(j=0;j<len&&produced<usize;j++){
          c=copy_buffer[(unsigned)((pos-(unsigned long)dist)&4095UL)];
          copy_buffer[(unsigned)(pos&4095UL)]=(unsigned char)c;
          if(fputc(c,out)==EOF)return 0;pos++;produced++;
        }
      }
    }
  }
  return produced==usize&&csize==0&&!ferror(in)&&!ferror(out);
}

static int extract_file(const char *archive,const char *wanted,const char *destination)
{
  FILE *in,*out;char magic[8],name[13];unsigned count,i;
  unsigned long offset,csize,usize;int ok;
  in=fopen(archive,"rb");if(!in)return 0;
  if(fread(magic,1,8,in)!=8||memcmp(magic,DAT_MAGIC,8)){fclose(in);return 0;}
  count=read_u16(in);
  for(i=0;i<count;i++){
    if(fread(name,1,13,in)!=13){fclose(in);return 0;}name[12]=0;
    offset=read_u32(in);csize=read_u32(in);usize=read_u32(in);
    if(!stricmp(name,wanted)){
      if(fseek(in,(long)offset,SEEK_SET)){fclose(in);return 0;}
      out=fopen(destination,"wb");if(!out){fclose(in);return 0;}
      ok=decompress_file(in,out,csize,usize);
      if(fclose(out)!=0)ok=0;
      if(!ok)remove(destination);
      fclose(in);return ok;
    }
  }
  fclose(in);return 0;
}

static void strip_line(char *text)
{
  int n=(int)strlen(text);
  while(n && (text[n-1]=='\r' || text[n-1]=='\n'))text[--n]=0;
}

static int contains_icase(const char *haystack,const char *needle)
{
  unsigned i,j,n=(unsigned)strlen(needle);
  if(!n)return 1;
  for(i=0;haystack[i];i++){
    for(j=0;j<n&&haystack[i+j]&&toupper((unsigned char)haystack[i+j])==toupper((unsigned char)needle[j]);j++);
    if(j==n)return 1;
  }
  return 0;
}

static int contains_line(const char *filename,const char *wanted)
{
  FILE *f=fopen(filename,"r");static char line[256];
  if(!f)return 0;
  while(fgets(line,sizeof(line),f)){strip_line(line);if(!stricmp(line,wanted)){fclose(f);return 1;}}
  fclose(f);return 0;
}

/* Print through DOS first, then recolour only the cells that were printed.
   This is deliberately different from changing the active console attribute:
   DOS/BIOS remains in its normal white-on-black state, so CR/LF and scrolling
   cannot inherit the coloured background.  Only the requested screen cells
   are changed after normal console output has already completed. */
static void colour_text(const char *s,int attr)
{
  union REGS r;
  unsigned char mode,page;
  unsigned cols,start_row,start_col;
  unsigned row,col;
  unsigned short far *video;
  unsigned seg;
  const char *p;

  if(!_isatty(_fileno(stdout))){fputs(s,stdout);return;}

  /* Record where ordinary DOS output will begin. */
  fflush(stdout);
  memset(&r,0,sizeof(r));
  r.h.ah=0x0F;
  int86(0x10,&r,&r);
  mode=r.h.al;
  cols=r.h.ah?r.h.ah:80;
  page=r.h.bh;

  r.h.ah=3;
  r.h.bh=page;
  int86(0x10,&r,&r);
  start_row=r.h.dh;
  start_col=r.h.dl;

  /* Let DOS render and advance the cursor using its normal attribute. */
  fputs(s,stdout);
  fflush(stdout);

  /* Recolour only those already-rendered cells.  None of this changes the
     DOS/BIOS attribute used by subsequent output or screen scrolling. */
  seg=(mode==7)?0xB000:0xB800;
  video=(unsigned short far *)((unsigned long)seg<<16);
  row=start_row;
  col=start_col;
  for(p=s;*p;p++){
    if(*p=='\r'){col=0;continue;}
    if(*p=='\n'){row++;continue;}
    video[row*cols+col]=(video[row*cols+col]&0x00FF)|
                        ((unsigned short)(unsigned char)attr<<8);
    if(++col>=cols){col=0;row++;}
  }
}

static void normalise_current_output_row(void)
{
  union REGS r;
  unsigned char mode,page;
  unsigned cols,row,col,seg;
  unsigned short far *video;

  if(!_isatty(_fileno(stdout)))return;
  fflush(stdout);

  memset(&r,0,sizeof(r));
  r.h.ah=0x0F;
  int86(0x10,&r,&r);
  mode=r.h.al;
  cols=r.h.ah?r.h.ah:80;
  page=r.h.bh;

  r.h.ah=3;
  r.h.bh=page;
  int86(0x10,&r,&r);
  row=r.h.dh;

  seg=(mode==7)?0xB000:0xB800;
  video=(unsigned short far *)((unsigned long)seg<<16);
  for(col=0;col<cols;col++)
    video[row*cols+col]=(video[row*cols+col]&0x00FF)|0x0700;
}

static void status_icon(int indent,int colour,int symbol)
{
  char block[2],mark[2];int i;
  block[0]=(char)219;block[1]=0;mark[0]=(char)symbol;mark[1]=0;
  for(i=0;i<indent;i++)putchar(' ');colour_text(" ",7);
  colour_text(block,colour);
  colour_text(mark,(colour<<4)|15);
  colour_text(block,colour);
  colour_text(" ",7);
}
static void question_icon(int indent){status_icon(indent,1,'?');}
static void info_icon(int indent){status_icon(indent,1,'i');}
/* Windows 3.x installer prompt mark: two CP437 lower-half blocks. */
static void windows_icon(int indent)
{
  char b[2];int i;b[0]=(char)220;b[1]=0;
  for(i=0;i<indent;i++)putchar(' ');
  colour_text(b,0x49); /* bright blue foreground on red */
  colour_text(b,0x2E); /* bright yellow foreground on green */
  putchar(' ');
}

static int read_key_immediate(void)
{
  union REGS r;memset(&r,0,sizeof(r));r.h.ah=0;int86(0x16,&r,&r);return r.h.al?r.h.al:(256+r.h.ah);
}

static void cursor_pos(unsigned *row,unsigned *col)
{
  union REGS r;unsigned page;
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);page=r.h.bh;
  memset(&r,0,sizeof(r));r.h.ah=3;r.h.bh=(unsigned char)page;int86(0x10,&r,&r);
  *row=r.h.dh;*col=r.h.dl;
}
static void component_icon(int digit)
{
  char b[4];b[0]=' ';b[1]=(char)('0'+digit);b[2]=' ';b[3]=0;putchar(' ');colour_text(b,0x3F);putchar(' ');
}
static void screen_cursor_at(unsigned row,unsigned col)
{
  union REGS r;unsigned page;
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);page=r.h.bh;
  memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=(unsigned char)page;
  r.h.dh=(unsigned char)row;r.h.dl=(unsigned char)col;int86(0x10,&r,&r);
}
static void screen_text_attr_at(unsigned row,unsigned col,const char *text,unsigned attr)
{
  union REGS r;unsigned page,oldrow,oldcol,i;
  if(!_isatty(_fileno(stdout)))return;
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);page=r.h.bh;
  memset(&r,0,sizeof(r));r.h.ah=3;r.h.bh=(unsigned char)page;int86(0x10,&r,&r);
  oldrow=r.h.dh;oldcol=r.h.dl;
  for(i=0;text[i];i++){
    memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=(unsigned char)page;
    r.h.dh=(unsigned char)row;r.h.dl=(unsigned char)(col+i);int86(0x10,&r,&r);
    memset(&r,0,sizeof(r));r.h.ah=9;r.h.al=(unsigned char)text[i];
    r.h.bh=(unsigned char)page;r.h.bl=(unsigned char)attr;r.x.cx=1;int86(0x10,&r,&r);
  }
  memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=(unsigned char)page;
  r.h.dh=(unsigned char)oldrow;r.h.dl=(unsigned char)oldcol;int86(0x10,&r,&r);
}
static void screen_text_at(unsigned row,unsigned col,const char *text)
{
  screen_text_attr_at(row,col,text,7);
}

static unsigned screen_columns(void)
{
  union REGS r;
  if(!_isatty(_fileno(stdout)))return 80;
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);
  return r.h.ah?r.h.ah:80;
}

static unsigned screen_rows(void)
{
  union REGS r;
  if(!_isatty(_fileno(stdout)))return 25;
  memset(&r,0,sizeof(r));r.x.ax=0x1130;r.h.bh=0;int86(0x10,&r,&r);
  if(r.h.dl>=24 && r.h.dl<100)return (unsigned)r.h.dl+1;
  return 25;
}

static void installer_clear_screen(void)
{
  union REGS r;unsigned cols,rows,page;
  if(!_isatty(_fileno(stdout)))return;
  cols=screen_columns();rows=screen_rows();
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);page=r.h.bh;
  memset(&r,0,sizeof(r));r.h.ah=0x06;r.h.al=0;r.h.bh=7;r.x.cx=0;
  r.h.dh=(unsigned char)(rows-1);r.h.dl=(unsigned char)(cols-1);int86(0x10,&r,&r);
  memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=(unsigned char)page;r.h.dh=0;r.h.dl=0;int86(0x10,&r,&r);
}

static void installer_title_rule(void)
{
  union REGS r;unsigned cols,row,col,page;
  if(!_isatty(_fileno(stdout))){puts("--------------------------------------------------------------------------------");return;}
  cols=screen_columns();
  memset(&r,0,sizeof(r));r.h.ah=0x0F;int86(0x10,&r,&r);page=r.h.bh;
  memset(&r,0,sizeof(r));r.h.ah=3;r.h.bh=(unsigned char)page;int86(0x10,&r,&r);row=r.h.dh;
  for(col=0;col<cols;col++){
    memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=(unsigned char)page;r.h.dh=(unsigned char)row;r.h.dl=(unsigned char)col;int86(0x10,&r,&r);
    memset(&r,0,sizeof(r));r.h.ah=9;r.h.al=196;r.h.bh=(unsigned char)page;r.h.bl=7;r.x.cx=1;int86(0x10,&r,&r);
  }
  memset(&r,0,sizeof(r));r.h.ah=2;r.h.bh=(unsigned char)page;r.h.dh=(unsigned char)(row+1);r.h.dl=0;int86(0x10,&r,&r);
}
static void error_icon(int indent){status_icon(indent,4,'!');}
static void success_icon(int indent){status_icon(indent,2,3);}
static void choice_default(int default_yes)
{putchar('[');if(default_yes){colour_text("Y",10);fputs("/n",stdout);}else{fputs("y/",stdout);colour_text("N",10);}fputs("]: ",stdout);}

static int ask_yes(const char *prompt,int default_yes,int indent)
{
  char answer[16];
  question_icon(indent);printf("%s ",prompt);choice_default(default_yes);
  if(!fgets(answer,sizeof(answer),stdin))return default_yes;
  if(answer[0]=='\r' || answer[0]=='\n' || !answer[0])return default_yes;
  return toupper(answer[0])=='Y';
}
static int ask_windows_yes(const char *prompt,int default_yes,int indent)
{
  char answer[16];
  windows_icon(indent);printf("%s ",prompt);choice_default(default_yes);
  if(!fgets(answer,sizeof(answer),stdin))return default_yes;
  if(answer[0]=='\r' || answer[0]=='\n' || !answer[0])return default_yes;
  return toupper(answer[0])=='Y';
}

static int cpu_at_least_286(void){unsigned before,after;
#ifndef __GNUC__
 _asm {
  pushf
  pop ax
  mov before,ax
  and ax,0fffh
  push ax
  popf
  pushf
  pop ax
  mov after,ax
  mov ax,before
  push ax
  popf
 }
 return (after&0xF000)!=0xF000;
#else
 before=after=0;return 1;
#endif
}
static int cpu_is_286(void){unsigned before,after;
#ifndef __GNUC__
 _asm {
  pushf
  pop ax
  mov before,ax
  xor ax,7000h
  push ax
  popf
  pushf
  pop ax
  mov after,ax
  mov ax,before
  push ax
  popf
 }
 return ((before^after)&0x7000)==0;
#else
 before=after=0;return 0;
#endif
}
static const char *display_adapter(int *suitable){union REGS r;memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);if(r.h.al==0x1A){*suitable=1;return"VGA or compatible";}memset(&r,0,sizeof(r));r.h.ah=0x12;r.h.bl=0x10;int86(0x10,&r,&r);if(r.h.bl!=0x10){*suitable=1;return"EGA or compatible";}*suitable=0;return"CGA/MDA compatible";}
static int write_initial_font_config(const char *install,int font_id){char path[PATH_SIZE];FILE*f;int persist=font_id?1:0;sprintf(path,"%s\\LAUNCH.CFG",install);f=fopen(path,"wt");if(!f)return 0;fprintf(f,"suiteTitle=Launch!\nSCREEN_COLUMNS=80\nflashing=true\nTITLEBAR_FG=15\nTITLEBAR_BG=7\nFONT_ID=%d\nFONT_PERSIST=%d\nfontPersist=%d\nPROMPT_STYLE=0\nPROMPT_SET=0\nshortcutEnabled=1\nshortcutCtrl=1\nshortcutAlt=1\nshortcutShift=0\nshortcutKey=\\\n",font_id,persist,persist);return fclose(f)==0;}

/* Seed the standalone font service with the factory Launch! font.  !FONT.COM
   deliberately consumes FONT.CUR rather than parsing FONT.DAT itself, so a
   clean install must create the same cache that Configuration would create. */
static int write_initial_font_cache(const char *install,int vga_display,int font_id)
{
  char src[PATH_SIZE],dst[PATH_SIZE];FILE *in,*out;unsigned size,height,total=0,want;unsigned char header[12];
  if(font_id<=0)return 1;
  size=vga_display?4096:3584;height=vga_display?16:14;
  sprintf(src,"%s\\%s",install,vga_display?"FONT.DAT":"FONT14.DAT");
  sprintf(dst,"%s\\FONT.CUR",install);
  in=fopen(src,"rb");if(!in)return 0;
  if(fseek(in,(long)(font_id-1)*(long)size,SEEK_SET)!=0){fclose(in);return 0;}
  out=fopen(dst,"wb");if(!out){fclose(in);return 0;}
  memcpy(header,"LFCUR100",8);header[8]=(unsigned char)height;header[9]=(unsigned char)font_id;
  header[10]=(unsigned char)(size&0xFF);header[11]=(unsigned char)(size>>8);
  if(fwrite(header,1,sizeof(header),out)!=sizeof(header)){fclose(in);fclose(out);remove(dst);return 0;}
  while(total<size){
    want=size-total;if(want>sizeof(copy_buffer))want=sizeof(copy_buffer);
    if(fread(copy_buffer,1,want,in)!=want || fwrite(copy_buffer,1,want,out)!=want){fclose(in);fclose(out);remove(dst);return 0;}
    total+=want;
  }
  if(fwrite("Launch!",1,8,out)!=8){fclose(in);fclose(out);remove(dst);return 0;}
  fclose(in);if(fclose(out)!=0){remove(dst);return 0;}return 1;
}
static int hardware_warning(void){char answer[16];error_icon(0);fputs("This system's hardware doesn't meet minimum recommended requirements. Proceed ",stdout);choice_default(0);if(!fgets(answer,sizeof(answer),stdin))return 0;return toupper(answer[0])=='Y';}

static unsigned char far *bios_byte(unsigned offset)
{
  return (unsigned char far *)(((unsigned long)0x40<<16)|offset);
}

static unsigned short far *bios_word(unsigned offset)
{
  return (unsigned short far *)(((unsigned long)0x40<<16)|offset);
}

static int keyboard_ready(void)
{
  return *bios_word(0x1A)!=*bios_word(0x1C);
}

static unsigned read_keyboard(unsigned char *shift)
{
  unsigned short far *head=bios_word(0x1A);unsigned pos,next,word;
  _disable();pos=*head;word=*bios_word(pos);
  next=pos+2;if(next>=0x3E)next=0x1E;*head=next;
  *shift=*bios_byte(0x17);_enable();return word;
}

static void key_label(unsigned scan,char *label)
{
  static const unsigned char letters[26]={
    0x1E,0x30,0x2E,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,
    0x31,0x18,0x19,0x10,0x13,0x1F,0x14,0x16,0x2F,0x11,0x2D,0x15,0x2C
  };
  static const char digits[]="1234567890";int i;
  for(i=0;i<26;i++)if(scan==letters[i]){label[0]=(char)('A'+i);label[1]=0;return;}
  if(scan>=2 && scan<=11){label[0]=digits[scan-2];label[1]=0;return;}
  if(scan>=0x3B && scan<=0x44){sprintf(label,"F%d",scan-0x3A);return;}
  if(scan==0x57){strcpy(label,"F11");return;}
  if(scan==0x58){strcpy(label,"F12");return;}
  switch(scan){
    case 0x0C:strcpy(label,"-");return;case 0x0D:strcpy(label,"=");return;
    case 0x1A:strcpy(label,"[");return;case 0x1B:strcpy(label,"]");return;
    case 0x27:strcpy(label,";");return;case 0x29:strcpy(label,"`");return;
    case 0x2B:strcpy(label,"\\");return;case 0x33:strcpy(label,",");return;
    case 0x34:strcpy(label,".");return;case 0x35:strcpy(label,"/");return;
    case 0x39:strcpy(label,"SPACE");return;case 0x48:strcpy(label,"UP");return;
    case 0x50:strcpy(label,"DOWN");return;case 0x4B:strcpy(label,"LEFT");return;
    case 0x4D:strcpy(label,"RIGHT");return;case 0x5B:strcpy(label,"LWIN");return;
    case 0x5C:strcpy(label,"RWIN");return;case 0x5D:strcpy(label,"MENU");return;
  }
  sprintf(label,"SCAN %02X",scan);
}

static void append_key_display(char *text,const char *name)
{
  if(*text)strcat(text," + ");
  strcat(text,"[");strcat(text,name);strcat(text,"]");
}

static void show_keys(unsigned char shift,unsigned scan)
{
  static char text[80],label[16],padded[59];text[0]=0;
  if(shift&4)append_key_display(text,"CTRL");
  if(shift&8)append_key_display(text,"ALT");
  if(shift&3)append_key_display(text,"SHIFT");
  if(scan){key_label(scan,label);append_key_display(text,label);}
  printf("\rDetected: ");sprintf(padded,"%-58.58s",text);colour_text(padded,10);fflush(stdout);
}

static int capture_shortcut(char *spec)
{
  unsigned word,scan;unsigned char shift,captured_shift,last_shift=0xFF;
  puts("Press the key/s to use as a shortcut now (Esc cancels).");
  while(keyboard_ready()){shift=0;read_keyboard(&shift);}
  capture_scan=capture_e0=capture_mods=0;
  capture_key=capture_key_mods=0;
  old_int09=_dos_getvect(0x09);_dos_setvect(0x09,capture_int09);
  for(;;){
    shift=*bios_byte(0x17);
    if((shift&15)!=(last_shift&15)){show_keys(shift,0);last_shift=shift;}
    if(capture_key){
      _disable();scan=capture_key;captured_shift=capture_key_mods;
      capture_key=0;_enable();
      _dos_setvect(0x09,old_int09);
      while(keyboard_ready()){word=read_keyboard(&shift);(void)word;}
      shift=captured_shift;
      if(scan==1){puts("\nShortcut unchanged.");return 0;}
    } else if(keyboard_ready()){
      word=read_keyboard(&shift);scan=word>>8;
      if(scan==1){_dos_setvect(0x09,old_int09);
        puts("\nShortcut unchanged.");return 0;}
      if(!scan)continue;
      _dos_setvect(0x09,old_int09);
      /* Enhanced BIOSes return 85h-8Ch for F11/F12 with modifiers. */
      if(scan>=0x85 && scan<=0x8C){
        if(scan>=0x87 && scan<=0x88)shift|=1;
        else if(scan>=0x89 && scan<=0x8A)shift|=4;
        else if(scan>=0x8B)shift|=8;
        scan=(scan&1)?0x57:0x58;
      }
    } else continue;
    show_keys(shift,scan);puts("");
    spec[0]=0;
    if(shift&4)strcat(spec,"1D+");
    if(shift&8)strcat(spec,"38+");
    if(shift&3)strcat(spec,"2A+");
    if(scan==0x5B)strcat(spec,"LWIN");
    else if(scan==0x5C)strcat(spec,"RWIN");
    else if(scan==0x5D)strcat(spec,"MENU");
    else sprintf(spec+strlen(spec),"%02X",scan);
    while((*bios_byte(0x17)&15)!=0) ;
    return 1;
  }
}


static int file_contains_ci(const char *filename,const char *needle)
{
  FILE *f;int c;unsigned matched=0,nlen;
  if(!filename||!*filename||!needle||!*needle)return 0;
  nlen=(unsigned)strlen(needle);f=fopen(filename,"rb");if(!f)return 0;
  while((c=fgetc(f))!=EOF){
    unsigned char ch=(unsigned char)toupper((unsigned char)c);
    unsigned char want=(unsigned char)toupper((unsigned char)needle[matched]);
    if(ch==want){
      matched++;
      if(matched==nlen){fclose(f);return 1;}
    }else matched=(ch==(unsigned char)toupper((unsigned char)needle[0]))?1:0;
  }
  fclose(f);return 0;
}

static int startup_uses_loadhigh(const char *filename)
{
  char line[300],*p;FILE *f;
  if(!filename||!*filename)return 0;
  f=fopen(filename,"rt");if(!f)return 0;
  while(fgets(line,sizeof(line),f)){
    p=line;while(*p==' '||*p=='\t'||*p=='@')p++;
    if(!strnicmp(p,"REM",3)&&(p[3]==0||isspace((unsigned char)p[3])))continue;
    if(!strncmp(p,"::",2))continue;
    if(!strnicmp(p,"LOADHIGH",8)&&(p[8]==0||isspace((unsigned char)p[8]))){fclose(f);return 1;}
  }
  fclose(f);return 0;
}

/* LOADHIGH is a command-processor feature, so testing it by calling system()
   necessarily starts another command interpreter.  That is especially wrong
   for 4DOS/NDOS, where startup scripts such as 4START.BAT run in the child
   shell.  Detect support without executing anything: first honour an existing
   LOADHIGH use in the actual startup batch, then inspect the active COMSPEC
   binary for its command name.  The test is deliberately conservative; if
   support cannot be established, !START simply loads the TSRs normally. */
static int loadhigh_supported(const char *startup_file)
{
  char *comspec;
  if(startup_uses_loadhigh(startup_file))return 1;
  comspec=getenv("COMSPEC");
  if(comspec&&*comspec&&file_contains_ci(comspec,"LOADHIGH"))return 1;
  return 0;
}

static int append_autoexec(const char *filename,const char *path,int add_path,
                           int add_shortcut,const char *key_spec,int show_menu)
{
  FILE *f;static char path_line[256],load_line[256],menu_line[256];long size;
  static const char *lines[3];static int wanted[3];int i,last=0,missing=0;
  sprintf(path_line,"PATH %%PATH%%;%s",path);
  sprintf(load_line,"%s\\!KEY.COM",path);
  if(add_shortcut && *key_spec){strcat(load_line," /KEY=");strcat(load_line,key_spec);}
  sprintf(menu_line,"%s\\!.EXE",path);
  lines[0]=path_line;lines[1]=load_line;lines[2]=menu_line;
  wanted[0]=add_path;wanted[1]=add_shortcut;wanted[2]=show_menu;
  for(i=0;i<4;i++){
    wanted[i]=wanted[i] && !contains_line(filename,lines[i]);
    if(wanted[i])missing=1;
  }
  if(!missing)return 1;
  f=fopen(filename,"a+b");if(!f)return 0;
  fseek(f,0L,SEEK_END);size=ftell(f);
  if(size>0){fseek(f,-1L,SEEK_END);last=fgetc(f);fseek(f,0L,SEEK_END);}
  if(size>0 && last!='\n')fputs("\r\n",f);
  for(i=0;i<3;i++)if(wanted[i])fprintf(f,"%s\r\n",lines[i]);
  if(fclose(f)!=0)return 0;
  return 1;
}

static int apply_component_config(const char *install,int screensavers,int fonts,int shortcut)
{
  char path[PATH_SIZE];FILE *f;int ok=1;
  sprintf(path,"%s\\LAUNCH.CFG",install);
  f=fopen(path,"at");if(!f)return 0;
  /* Last value wins in Launch!'s config parser.  Record component presence as
     well as safe fallback values so Config can replace an omitted component's
     controls with an explicit not-installed message. */
  if(fprintf(f,"SAVERS_INSTALLED=%d\nFONTS_INSTALLED=%d\nSHORTCUT_INSTALLED=%d\n",screensavers?1:0,fonts?1:0,shortcut?1:0)<0)ok=0;
  if(!screensavers&&fputs("SCREENSAVER=0\n",f)==EOF)ok=0;
  if(!fonts){
    if(fputs("FONT_ID=0\n",f)==EOF)ok=0;
    if(fputs("FONT_PERSIST=0\nfontPersist=0\n",f)==EOF)ok=0;
  }
  if(!shortcut&&fputs("shortcutEnabled=0\n",f)==EOF)ok=0;
  if(fclose(f)!=0)ok=0;return ok;
}

static int append_startup_service_config(const char *install,int shortcut,const char *spec,int open_menu,int font_persist)
{
  static char path[PATH_SIZE],copy[64],key[16];
  char *tok,*end;FILE *f;int ctrl=1,alt=1,shift=0;unsigned long scan;
  strcpy(key,"\\");
  if(spec&&*spec){ctrl=alt=shift=0;strncpy(copy,spec,sizeof(copy)-1);copy[sizeof(copy)-1]=0;tok=strtok(copy,"+");while(tok){if(!stricmp(tok,"1D")||!stricmp(tok,"CTRL"))ctrl=1;else if(!stricmp(tok,"38")||!stricmp(tok,"ALT"))alt=1;else if(!stricmp(tok,"2A")||!stricmp(tok,"36")||!stricmp(tok,"SHIFT"))shift=1;else{scan=strtoul(tok,&end,16);if(*tok&&!*end&&scan<=255)key_label((unsigned)scan,key);else{strncpy(key,tok,sizeof(key)-1);key[sizeof(key)-1]=0;}}tok=strtok(NULL,"+");}}
  sprintf(path,"%s\\LAUNCH.CFG",install);f=fopen(path,"at");if(!f)return 0;
  if(fprintf(f,"shortcutEnabled=%d\nshortcutCtrl=%d\nshortcutAlt=%d\nshortcutShift=%d\nshortcutKey=%s\nOPEN_MENU_BOOT=%d\nFONT_PERSIST=%d\nfontPersist=%d\n",shortcut?1:0,ctrl,alt,shift,key,open_menu?1:0,font_persist?1:0,font_persist?1:0)<0){fclose(f);return 0;}
  return fclose(f)==0;
}

static int startup_launch_line(const char *line,const char *install,const char *token)
{
  char uline[300],uinst[PATH_SIZE],*p;int i;
  strncpy(uline,line,sizeof(uline)-1);uline[sizeof(uline)-1]=0;strncpy(uinst,install,sizeof(uinst)-1);uinst[sizeof(uinst)-1]=0;
  for(i=0;uline[i];i++)uline[i]=(char)toupper((unsigned char)uline[i]);for(i=0;uinst[i];i++)uinst[i]=(char)toupper((unsigned char)uinst[i]);
  p=uline;while(*p==' '||*p=='\t'||*p=='@')p++;if(!strncmp(p,"REM",3)||!strncmp(p,"::",2))return 0;
  return strstr(uline,uinst)!=0 && strstr(uline,token)!=0;
}

/* Read the final value of a simple key from LAUNCH.CFG.  The file may
   contain duplicate settings after upgrades; Launch! itself uses last value
   wins, so the installer follows the same rule. */
static int startup_cfg_bool(const char *install,const char *key,int defvalue)
{
  static char path[PATH_SIZE],line[256];char *p,*eq,*valuep;FILE *f;int value=defvalue;
  sprintf(path,"%s\\LAUNCH.CFG",install);f=fopen(path,"rt");if(!f)return value;
  while(fgets(line,sizeof(line),f)){
    p=line;while(*p==' '||*p=='\t')p++;if(*p==';'||*p=='#'||!*p)continue;
    eq=strchr(p,'=');if(!eq)continue;valuep=eq+1;*eq=0;while(eq>p&&(eq[-1]==' '||eq[-1]=='\t'))*--eq=0;
    if(!stricmp(p,key)){p=valuep;while(*p==' '||*p=='\t')p++;value=(*p=='1'||toupper((unsigned char)*p)=='Y')?1:0;}
  }
  fclose(f);return value;
}

static int startup_cfg_light(const char *install)
{
  static char path[PATH_SIZE],line[256];char *p,*eq,*valuep;FILE *f;int light=0;
  sprintf(path,"%s\\LAUNCH.CFG",install);f=fopen(path,"rt");if(!f)return 0;
  while(fgets(line,sizeof(line),f)){
    p=line;while(*p==' '||*p=='\t')p++;eq=strchr(p,'=');if(!eq)continue;valuep=eq+1;*eq=0;
    if(!stricmp(p,"ShortcutTarget")){p=valuep;while(*p==' '||*p=='\t')p++;light=!strnicmp(p,"LIGHT",5);}
  }
  fclose(f);return light;
}

/* Resident TSRs are loaded directly by the user's command processor.  Do not
   place a transient loader below them: when that loader exits it leaves a
   conventional-memory hole, which Windows 3.0 reports as fragmentation. */
static int write_start_batch(const char *startup_file,const char *install,int add_key,const char *key_spec,int add_font,int light_target,int show_menu)
{
  static char path[PATH_SIZE],line[300],prompt_line[300];FILE *f,*oldf;const char *keyprog=light_target?"!TKEY.COM":"!KEY.COM";int loadhigh;
  /* Preserve a prompt already managed by Launch! across an upgrade.  Clean
     installs have no PROMPT line until Configuration > Prompt > Set is used. */
  loadhigh=loadhigh_supported(startup_file);
  prompt_line[0]=0;sprintf(path,"%s\\!START.BAT",install);oldf=fopen(path,"rt");
  if(oldf){while(fgets(line,sizeof(line),oldf)){char *q=line;while(*q==' '||*q=='\t')q++;if(!strnicmp(q,"PROMPT ",7)){q[strcspn(q,"\r\n")]=0;strncpy(prompt_line,q,sizeof(prompt_line)-1);prompt_line[sizeof(prompt_line)-1]=0;}}fclose(oldf);}
  f=fopen(path,"wt");if(!f)return 0;
  if(fputs("@ECHO OFF\n",f)==EOF){fclose(f);return 0;}
  if(fprintf(f,"PATH %%PATH%%;%s\n",install)<0){fclose(f);return 0;}
  if(fprintf(f,"%s\\!CONFIG.EXE /DISPLAY\n",install)<0){fclose(f);return 0;}
  if(add_font)if(fprintf(f,"%s%s\\!FONT.COM\n",loadhigh?"LOADHIGH ":"",install)<0){fclose(f);return 0;}
  if(add_key){
    if(fprintf(f,"%s%s\\%s",loadhigh?"LOADHIGH ":"",install,keyprog)<0){fclose(f);return 0;}
    if(key_spec&&*key_spec)if(fprintf(f," /KEY=%s",key_spec)<0){fclose(f);return 0;}
    if(fputc('\n',f)==EOF){fclose(f);return 0;}
  }
  if(prompt_line[0])if(fprintf(f,"%s\n",prompt_line)<0){fclose(f);return 0;}
  if(show_menu)if(fprintf(f,"%s\\!.EXE\n",install)<0){fclose(f);return 0;}
  return fclose(f)==0;
}

static int startup_is_start_line(const char *upper)
{
  const char *p=upper,*hit;
  while(*p==' '||*p=='\t'||*p=='@')p++;
  if(!strncmp(p,"::",2))return 0;
  if(!strncmp(p,"REM",3)&&(p[3]==0||isspace((unsigned char)p[3])))return 0;
  hit=strstr(p,"!START");
  if(!hit)return 0;
  if(hit>p && hit[-1]!='\\' && hit[-1]!='/' && !isspace((unsigned char)hit[-1]))return 0;
  return hit[6]==0||hit[6]=='\r'||hit[6]=='\n'||hit[6]=='.'||isspace((unsigned char)hit[6]);
}

static int migrate_launch_startup(const char *filename,const char *install,int add_key,const char *key_spec,int add_font,int light_target,int show_menu,int preserve_menu)
{
  /* AUTOEXEC.BAT/FDAUTO.BAT is edited as a file.  Never invoke a command
     processor to change startup configuration.  Replace the first existing
     !START line in place; remove duplicate/legacy Launch! startup lines; append
     one !START line only when no existing Launch! startup position was found. */
  static char temp[PATH_SIZE],old[PATH_SIZE],line[300],up[300],uinst[PATH_SIZE];
  char *dot,*q;FILE *in,*out;int ok=1,had_menu=0,i,last=1,inserted=0,legacy=0;
  strcpy(temp,filename);dot=strrchr(temp,'.');if(dot)strcpy(dot,".$L$");else strcat(temp,".$L$");
  strcpy(old,filename);dot=strrchr(old,'.');if(dot)strcpy(dot,".L!$");else strcat(old,".L!$");
  in=fopen(filename,"rt");out=fopen(temp,"wt");if(!out){if(in)fclose(in);return 0;}
  if(in){while(ok&&fgets(line,sizeof(line),in)){
    strncpy(up,line,sizeof(up)-1);up[sizeof(up)-1]=0;
    strncpy(uinst,install,sizeof(uinst)-1);uinst[sizeof(uinst)-1]=0;
    for(i=0;up[i];i++)up[i]=(char)toupper((unsigned char)up[i]);
    for(i=0;uinst[i];i++)uinst[i]=(char)toupper((unsigned char)uinst[i]);

    if(startup_is_start_line(up)){
      if(!inserted){if(fprintf(out,"%s\\!START\n",install)<0)ok=0;else{inserted=1;last=1;}}
      continue;
    }

    legacy=0;
    if(strstr(up,uinst)){
      if(strstr(up,"!HELPER")||strstr(up,"!APPLY")||strstr(up,"!FONT")||
         strstr(up,"!KEY")||strstr(up,"!TKEY"))legacy=1;
      if(strstr(up,"PATH")&&strstr(up,uinst))legacy=1;
      if(strstr(up,"!.EXE")){had_menu=1;legacy=1;}
    }
    q=up;while(*q==' '||*q=='\t'||*q=='@')q++;
    if(*q=='!'&&(q[1]==0||q[1]=='\r'||q[1]=='\n')){had_menu=1;legacy=1;}

    if(legacy){
      if(!inserted){if(fprintf(out,"%s\\!START\n",install)<0)ok=0;else{inserted=1;last=1;}}
      continue;
    }

    if(fputs(line,out)==EOF){ok=0;break;}
    last=(line[0]&&line[strlen(line)-1]=='\n');
  }
  if(ferror(in))ok=0;if(fclose(in)!=0)ok=0;}

  if(ok&&!inserted){
    if(!last)if(fputs("\n",out)==EOF)ok=0;
    if(ok&&fprintf(out,"%s\\!START\n",install)<0)ok=0;
  }
  if(fclose(out)!=0)ok=0;
  if(!ok){remove(temp);return 0;}

  /* Build !START.BAT from the original startup environment before replacing
     the startup file, so LOADHIGH capability detection can inspect it. */
  if(!write_start_batch(filename,install,add_key,key_spec,add_font,light_target,
                        show_menu||(preserve_menu&&had_menu))){remove(temp);return 0;}

  remove(old);
  if(in&&rename(filename,old)!=0){remove(temp);return 0;}
  if(rename(temp,filename)!=0){if(in)rename(old,filename);remove(temp);return 0;}
  if(in)remove(old);
  return 1;
}

/* Prepare one post-install activation batch in the installed Launch! directory
   and queue only its short command name.  The command is consumed by COMMAND.COM after
   INSTALL.EXE has terminated, so resident services become direct children of
   the shell and do not fragment conventional memory beneath themselves. */
static int installer_queue_bios_short_text(const char *text)
{
  unsigned short far *head=bios_word(0x1A);
  unsigned short far *tail=bios_word(0x1C);
  unsigned short far *buf;unsigned h,t,next,need=0,i;const char *p;
  for(p=text;*p;p++)need++;
  h=*head;t=*tail;next=t;
  for(i=0;i<need;i++){next+=2;if(next>=0x3E)next=0x1E;if(next==h)return 0;}
  _disable();t=*tail;
  for(p=text;*p;p++){
    unsigned short word=(unsigned char)*p;if(*p=='\r')word|=0x1C00;
    buf=bios_word(t);*buf=word;t+=2;if(t>=0x3E)t=0x1E;
  }
  *tail=(unsigned short)t;_enable();return 1;
}

static int prepare_postinstall_start(const char *install,int add_key,const char *key_spec,int add_font,int light_target,char *command)
{
  char batch[PATH_SIZE];FILE *f;const char *keyprog=light_target?"!TKEY.COM":"!KEY.COM";
  /* !WELCOME is the friendly one-shot hand-off shown immediately after
     INSTALL exits.  It is generated in the installed directory so the text
     and commands always use the actual Launch! path.  Do not self-delete an
     executing DOS batch file: real COMMAND.COM versions reopen it between
     lines and can otherwise report "Batch file missing". */
  if(!add_key && !add_font){command[0]=0;return 0;}
  sprintf(batch,"%s\\!WELCOME.BAT",install);f=fopen(batch,"wt");if(!f)return 0;
  if(fputs("@ECHO OFF\n\n",f)==EOF){fclose(f);return 0;}
  if(fputs("ECHO.\nECHO Great! You made it. Welcome to your new DOS experience.\n\n",f)==EOF){fclose(f);return 0;}
  if(fprintf(f,"PATH %%PATH%%;%s\n",install)<0){fclose(f);return 0;}
  /* The welcome flow intentionally hides !FONT's normal status line. */
  if(add_font)if(fprintf(f,"%s\\!FONT.COM >NUL\n",install)<0){fclose(f);return 0;}
  if(add_key){
    if(fprintf(f,"%s\\%s",install,keyprog)<0){fclose(f);return 0;}
    if(key_spec&&*key_spec)if(fprintf(f," /KEY=%s",key_spec)<0){fclose(f);return 0;}
    if(fputc('\n',f)==EOF){fclose(f);return 0;}
  }
  if(fputs("\nECHO.\nECHO Run !CONFIG to configure everything in one place.\nECHO.\n\n",f)==EOF){fclose(f);return 0;}
  if(fclose(f)!=0)return 0;
  strcpy(command,"!WELCOME\r");return 1;
}

static int extract_progress_done=0,extract_progress_total=1;

static void draw_extract_progress(void)
{
  char line[64];int i,percent,filled,pos=0;
  percent=(extract_progress_done*100)/extract_progress_total;
  if(percent>100)percent=100;
  filled=(extract_progress_done*30)/extract_progress_total;
  line[pos++]='\r';line[pos++]=' ';
  line[pos++]='[';
  for(i=0;i<30;i++)line[pos++]=(char)(i<filled?219:176);
  line[pos++]=']';line[pos++]=' ';
  sprintf(line+pos,"%3d%%",percent);pos+=(int)strlen(line+pos);line[pos]=0;
  colour_text(line,3);
  fflush(stdout);
  if(extract_progress_done>=extract_progress_total){
    putchar('\n');
    /* If the newline scrolls the screen, DOS/BIOS can create the new bottom
       row using the cyan attribute from the progress line.  Restore the whole
       active row to the installer's normal grey before the next prompt. */
    normalise_current_output_row();
  }
}

static void advance_extract_progress(void)
{
  if(extract_progress_done<extract_progress_total)extract_progress_done++;
  draw_extract_progress();
}

typedef struct {
  char name[13];
  unsigned long offset,csize,usize;
} DAT_ENTRY;

static DAT_ENTRY dat_entry[128];

static int component_member(const char *name,int component)
{
  static const char *accessories[]={
    "CAL.ICS","!CAL.EXE","!CALC.EXE","!UNITS.EXE","!DRAW.EXE","!JOURNAL.EXE","!MD.EXE",
    "!NOTE.EXE","!STACK.EXE","!DFETCH.EXE","!TODOS.EXE",0};
  static const char *games[]={
    "!TYPO.EXE","TYPO.LVL","!BOXES.EXE","BOXES.LVL","!FCELL.EXE","!PLUMB.EXE",
    "!POP.EXE","!SNAKE.EXE","!SOL.EXE","!WORDZ.EXE","WORDZ.LVL","!METRO.EXE","!JELLOH.EXE","JELLOH.LVL",0};
  const char **files;int i;
  files=component==1?accessories:games;
  for(i=0;files[i];i++)if(!stricmp(name,files[i]))return 1;
  return 0;
}

static void remove_named(const char *install,const char *name)
{
  char p[PATH_SIZE];const char *dot=strrchr(name,'.');
  sprintf(p,"%s\\%s",install,name);remove(p);
  if(dot&&!stricmp(dot,".LVL")){sprintf(p,"%s\\GAMERES\\%s",install,name);remove(p);}
}
static int write_help_batch(const char *install)
{
  char p[PATH_SIZE];FILE *f;
  sprintf(p,"%s\\!HELP.BAT",install);f=fopen(p,"w");if(!f)return 0;
  fprintf(f,"@ECHO OFF\n%s\\!MDVIEW /HELP %s\\HELP\\README.MD\n",install,install);
  return fclose(f)==0;
}

static void remove_sample_documents(const char *install)
{
  char p[PATH_SIZE];int i;const char *name;
  for(i=0;i<128&&dat_entry[i].name[0];i++){
    name=dat_entry[i].name;
    if(strlen(name)>4&&!stricmp(name+strlen(name)-4,".BMP")&&stricmp(name,"PWROFF.BMP")){sprintf(p,"%s\\SAMPLES\\%s",install,name);remove(p);}
    else if(strlen(name)>3&&!stricmp(name+strlen(name)-3,".MD")&&stricmp(name,"README.MD")){sprintf(p,"%s\\SAMPLES\\%s",install,name);remove(p);}
    else if(strlen(name)>3&&!stricmp(name+strlen(name)-3,".DB")){sprintf(p,"%s\\SAMPLES\\%s",install,name);remove(p);}
  }
  sprintf(p,"%s\\SAMPLES",install);rmdir(p);
  sprintf(p,"%s\\!HELP.BAT",install);remove(p);
  {static const char *hd[]={"README.MD","MENU.MD","WININT.MD","ACCESS.MD","GAMES.MD","TSHOOT.MD",0};for(i=0;hd[i];i++){sprintf(p,"%s\\HELP\\%s",install,hd[i]);remove(p);}}
  sprintf(p,"%s\\HELP",install);rmdir(p);
}

static void remove_unselected_components(const char *install,int accessories,int games,int fonts,int menu_generator,int shortcut_key,int sample_docs,int vga_display)
{
  static const char *acc[]={"CAL.ICS","!CAL.EXE","!CALC.EXE","!UNITS.EXE","!DRAW.EXE","!JOURNAL.EXE","!MD.EXE","!NOTE.EXE","!STACK.EXE","!DFETCH.EXE","!TODOS.EXE",0};
  static const char *gm[]={"!TYPO.EXE","TYPO.LVL","!BOXES.EXE","BOXES.LVL","!FCELL.EXE","!PLUMB.EXE","!POP.EXE","!SNAKE.EXE","!SOL.EXE","!WORDZ.EXE","WORDZ.LVL","!METRO.EXE","!JELLOH.EXE","JELLOH.LVL",0};
  int i;
  if(!accessories)for(i=0;acc[i];i++)remove_named(install,acc[i]);
  if(!games)for(i=0;gm[i];i++)remove_named(install,gm[i]);
  if(!sample_docs)remove_sample_documents(install);
  if(!accessories&&!sample_docs)remove_named(install,"!MDVIEW.EXE");
  if(!fonts)remove_named(install,"!FONT.COM");
  if(!fonts&&!accessories&&!sample_docs){remove_named(install,"FONT.DAT");remove_named(install,"FONT14.DAT");}
  else if(vga_display)remove_named(install,"FONT14.DAT");else remove_named(install,"FONT.DAT");
  if(!menu_generator){remove_named(install,"!MNUGEN.EXE");remove_named(install,"AUTOGEN.DAT");}
  if(!shortcut_key){remove_named(install,"!KEY.COM");remove_named(install,"!TKEY.COM");remove_named(install,"!KEY86.COM");}
}

static int is_help_document(const char *name)
{
  return !stricmp(name,"README.MD")||!stricmp(name,"MENU.MD")||!stricmp(name,"WININT.MD")||
         !stricmp(name,"ACCESS.MD")||!stricmp(name,"GAMES.MD")||!stricmp(name,"TSHOOT.MD");
}

static int selected_member(const char *name,int shortcut_build,int accessories,
                           int games,int fonts,int menu_generator,int sample_docs,
                           int vga_display,int windows_integration,int win_version,const char **dest_name)
{
  *dest_name=name;
  if(!stricmp(name,"!.EXE")||!stricmp(name,"!86.EXE")||!stricmp(name,"!CONFIG.EXE")||!stricmp(name,"PROMPTS.CFG")||!stricmp(name,"COLORS.CFG")||!stricmp(name,"PWROFF.BMP"))return 1;
  /* Help rides with component 7.  The read-only Markdown viewer is shared by
     Help and Accessories, so a minimal install that excludes both gets neither
     the viewer nor its font catalogue dependency. */
  if(sample_docs&&is_help_document(name))return 1;
  if((accessories||sample_docs)&&!stricmp(name,"!MDVIEW.EXE"))return 1;
  /* Windows integration is optional.  A DOS-only install does not extract
     the Win16 companion, icons or Program Manager group template. */
  if(windows_integration&&(!stricmp(name,"!W30.EXE")||!stricmp(name,"!W31.EXE")||!stricmp(name,"LAUNCH.GRP")))return 1;
  /* Windows 3.0 deliberately has no standalone Menu Manager. */
  if(windows_integration&&win_version>=31&&!stricmp(name,"!MNUMAN.EXE"))return 1;
  if((fonts||accessories||sample_docs)&&vga_display&&!stricmp(name,"FONT.DAT"))return 1;
  if((fonts||accessories||sample_docs)&&!vga_display&&!stricmp(name,"FONT14.DAT"))return 1;
  if(fonts&&!stricmp(name,"!FONT.COM"))return 1;
  if(menu_generator&&(!stricmp(name,"!MNUGEN.EXE")||!stricmp(name,"AUTOGEN.DAT")))return 1;
  if(accessories&&component_member(name,1))return 1;
  if(games&&component_member(name,2))return 1;
  if(sample_docs&&((strlen(name)>4&&!stricmp(name+strlen(name)-4,".BMP")&&stricmp(name,"PWROFF.BMP"))||(strlen(name)>3&&!stricmp(name+strlen(name)-3,".MD"))||(strlen(name)>3&&!stricmp(name+strlen(name)-3,".DB"))))return 1;
  if(shortcut_build<0)return 0;
  if(!stricmp(name,"!KEY.COM")){
    if(shortcut_build!=0)return 0;*dest_name="!KEY.COM";return 1;
  }
  if(!stricmp(name,"!KEYDB.COM")){
    if(shortcut_build!=1)return 0;*dest_name="!KEY.COM";return 1;
  }
  if(!stricmp(name,"!KEY286.COM")){
    if(shortcut_build!=2)return 0;*dest_name="!KEY.COM";return 1;
  }
  if(!stricmp(name,"!TKEY.COM")){
    if(shortcut_build!=0)return 0;*dest_name="!TKEY.COM";return 1;
  }
  if(!stricmp(name,"!TKEYDB.COM")){
    if(shortcut_build!=1)return 0;*dest_name="!TKEY.COM";return 1;
  }
  if(!stricmp(name,"!TKEY286.COM")){
    if(shortcut_build!=2)return 0;*dest_name="!TKEY.COM";return 1;
  }
  return 0;
}

static int skip_compressed(FILE *in,unsigned long size)
{
  size_t n,want;
  while(size){
    want=size>sizeof(copy_buffer)?sizeof(copy_buffer):(size_t)size;
    n=fread(copy_buffer,1,want,in);if(n!=want)return 0;size-=n;
  }
  return 1;
}

/* Read INSTALL.DAT once, then walk its payloads sequentially.  This avoids
   repeatedly reopening and seeking around a floppy for every installed file. */
static int extract_install_files(const char *archive,const char *install,int shortcut_build,
                                 int accessories,int games,int fonts,int menu_generator,int sample_docs,
                                 int vga_display,int windows_integration,int win_version)
{
  FILE *in,*out;char magic[8],destination[PATH_SIZE];const char *dest_name;
  unsigned count,i;int selected,done=0,ok;
  if(sample_docs){sprintf(destination,"%s\\SAMPLES",install);if(!make_directories(destination))return 0;}
  in=fopen(archive,"rb");if(!in)return 0;
  if(fread(magic,1,8,in)!=8||memcmp(magic,DAT_MAGIC,8)){fclose(in);return 0;}
  count=read_u16(in);if(!count||count>128){fclose(in);return 0;}
  for(i=0;i<count;i++){
    if(fread(dat_entry[i].name,1,13,in)!=13){fclose(in);return 0;}
    dat_entry[i].name[12]=0;
    dat_entry[i].offset=read_u32(in);
    dat_entry[i].csize=read_u32(in);
    dat_entry[i].usize=read_u32(in);
  }
  extract_progress_total=0;
  for(i=0;i<count;i++)
    if(selected_member(dat_entry[i].name,shortcut_build,accessories,games,fonts,menu_generator,sample_docs,vga_display,windows_integration,win_version,&dest_name))
      extract_progress_total++;
  if(!extract_progress_total)extract_progress_total=1;
  extract_progress_done=0;draw_extract_progress();
  if(fseek(in,(long)dat_entry[0].offset,SEEK_SET)){fclose(in);return 0;}

  for(i=0;i<count;i++){
    selected=selected_member(dat_entry[i].name,shortcut_build,accessories,games,fonts,menu_generator,sample_docs,vga_display,windows_integration,win_version,&dest_name);
    if(selected){
      if(is_help_document(dest_name)){sprintf(destination,"%s\\HELP",install);if(!make_directories(destination)){fclose(in);return 0;}sprintf(destination,"%s\\HELP\\%s",install,dest_name);}
      else if(sample_docs&&strlen(dest_name)>4&&!stricmp(dest_name+strlen(dest_name)-4,".BMP")&&stricmp(dest_name,"PWROFF.BMP")){sprintf(destination,"%s\\SAMPLES\\%s",install,dest_name);}
      else if(sample_docs&&strlen(dest_name)>3&&!stricmp(dest_name+strlen(dest_name)-3,".MD")){sprintf(destination,"%s\\SAMPLES\\%s",install,dest_name);}
      else if(sample_docs&&strlen(dest_name)>3&&!stricmp(dest_name+strlen(dest_name)-3,".DB")){sprintf(destination,"%s\\SAMPLES\\%s",install,dest_name);}
      else if(strlen(dest_name)>4&&!stricmp(dest_name+strlen(dest_name)-4,".LVL")){sprintf(destination,"%s\\GAMERES",install);if(!make_directories(destination)){fclose(in);return 0;}sprintf(destination,"%s\\GAMERES\\%s",install,dest_name);}
      else sprintf(destination,"%s\\%s",install,dest_name);
      out=fopen(destination,"wb");
      if(!out){fclose(in);error_icon(0);printf("Cannot create %s\n",destination);return 0;}
      ok=decompress_file(in,out,dat_entry[i].csize,dat_entry[i].usize);
      if(fclose(out)!=0)ok=0;
      if(!ok){remove(destination);fclose(in);error_icon(0);printf("Cannot extract %s\n",dat_entry[i].name);return 0;}
      done++;advance_extract_progress();
      if(done>=extract_progress_total)break;
    }else{
      if(!skip_compressed(in,dat_entry[i].csize)){fclose(in);return 0;}
    }
  }
  fclose(in);
  if(done!=extract_progress_total)return 0;
  if(accessories||games){
    sprintf(destination,"%s\\DATA",install);if(!make_directories(destination))return 0;
    sprintf(destination,"%s\\EXPORT",install);if(!make_directories(destination))return 0;
  }
  /* 3.65 consolidates all runtime display fonts into FONT.DAT.  Remove the
     obsolete loose font files from upgrades so the installed directory also
     reflects the new dependency model. */
  sprintf(destination,"%s\\!SANS.FNT",install);remove(destination);
  sprintf(destination,"%s\\PROFONT.FNT",install);remove(destination);
  sprintf(destination,"%s\\PROFONTB.FNT",install);remove(destination);
  sprintf(destination,"%s\\PROFONTI.FNT",install);remove(destination);
  /* 3.77 keeps game level packs together under GAMERES.  Remove legacy
     root-level copies during upgrade after the new copies have been extracted. */
  sprintf(destination,"%s\\TYPO.LVL",install);remove(destination);
  sprintf(destination,"%s\\BOXES.LVL",install);remove(destination);
  sprintf(destination,"%s\\WORDZ.LVL",install);remove(destination);
  sprintf(destination,"%s\\JELLY.LVL",install);remove(destination);
  sprintf(destination,"%s\\GAMERES\\JELLY.LVL",install);remove(destination);
  sprintf(destination,"%s\\!MKDOWN.EXE",install);remove(destination);
  sprintf(destination,"%s\\!WIN16.EXE",install);remove(destination);
  sprintf(destination,"%s\\!MGR16.EXE",install);remove(destination);
  sprintf(destination,"%s\\!SYSINFO.EXE",install);remove(destination);
  sprintf(destination,"%s\\!SYSBAR.EXE",install);remove(destination);
  sprintf(destination,"%s\\SBM.EXE",install);remove(destination);
  if(sample_docs){
    if(!write_help_batch(install)){error_icon(0);printf("Cannot create %s\\!HELP.BAT\n",install);return 0;}
  }else{
    sprintf(destination,"%s\\!HELP.BAT",install);remove(destination);
    sprintf(destination,"%s\\HELP\\README.MD",install);remove(destination);
    sprintf(destination,"%s\\HELP",install);rmdir(destination);
  }
  return 1;
}



/* Launch! 3.77 Windows 3.x integration.  The DOS installer records the
   Windows location and creates a conservative legacy PIF which Windows 3.x
   can open directly.  Program Manager registration uses a prepared GRP
   template and does not require a one-shot Win16 setup helper. */
static int write_windows_config(const char *install,int enabled,const char *winpath)
{
  char path[PATH_SIZE];FILE *f;
  sprintf(path,"%s\\LAUNCH.CFG",install);
  f=fopen(path,"at");if(!f)return 0;
  fprintf(f,"WinInst=%d\n",enabled?1:0);
  fprintf(f,"WinPath=%s\n",winpath&&*winpath?winpath:"C:\\WINDOWS");
  return fclose(f)==0;
}

static int valid_windows3_path(const char *path)
{
  char p[PATH_SIZE];
  sprintf(p,"%s\\WIN.COM",path);if(!exists(p))return 0;
  sprintf(p,"%s\\PROGMAN.EXE",path);if(!exists(p))return 0;
  return 1;
}

static void pif_put_word(unsigned char *p,unsigned off,unsigned v)
{ p[off]=(unsigned char)(v&255);p[off+1]=(unsigned char)((v>>8)&255); }

static int write_launch_pif(const char *install)
{
  static unsigned char pif[0x171];static char path[PATH_SIZE],exe[64];unsigned i,sum=0;FILE *f;
  memset(pif,0,sizeof(pif));
  strncpy((char*)pif+2,"Launch!",29);
  pif_put_word(pif,0x20,640);pif_put_word(pif,0x22,128);
  sprintf(exe,"%s\\!.EXE",install);strncpy((char*)pif+0x24,exe,62);
  /* PIF header flags: 0x10 = Close Window on Exit. */
  pif[0x63]=0x10;
  if(install[0]&&install[1]==':')pif[0x64]=(unsigned char)(toupper(install[0])-'A');
  strncpy((char*)pif+0x65,install,63);
  pif[0xE5]=0;pif[0xE6]=1;pif[0xE7]=0;pif[0xE8]=0xFF;pif[0xE9]=25;pif[0xEA]=80;
  /* Classic PIF checksum: byte 1 chosen so the complete 0x171-byte sum is 0. */
  pif[1]=0;for(i=0;i<sizeof(pif);i++)sum=(sum+pif[i])&255;pif[1]=(unsigned char)((256-sum)&255);
  sprintf(path,"%s\\LAUNCH.PIF",install);f=fopen(path,"wb");if(!f)return 0;
  if(fwrite(pif,1,sizeof(pif),f)!=sizeof(pif)){fclose(f);return 0;}return fclose(f)==0;
}

/* Windows 3.1 introduced the system common-dialog library.  Launch! keeps
   separate Win16 companions so the installed Program Manager icon can point
   at the implementation native to the user's Windows generation.  3.11 is
   deliberately treated as 3.1. */
static int detect_windows_version(const char *winpath)
{
  char p[PATH_SIZE];
  sprintf(p,"%s\\SYSTEM\\COMMDLG.DLL",winpath);
  return exists(p)?31:30;
}

static unsigned grp_word(const unsigned char *p,unsigned off)
{ return (unsigned)p[off]|((unsigned)p[off+1]<<8); }
static void grp_set_word(unsigned char *p,unsigned off,unsigned v)
{ p[off]=(unsigned char)(v&255);p[off+1]=(unsigned char)((v>>8)&255); }

/* Translate an offset from the supplied C:\\LAUNCH template to the rebuilt
   group after all twelve installation-path strings have been resized. */
static unsigned grp_map_offset(unsigned oldoff,const unsigned *pos,const int *delta,int count)
{
  int i;long n=(long)oldoff;
  for(i=0;i<count;i++)if(pos[i]<oldoff)n+=delta[i];
  return (unsigned)n;
}

/* Install the real Program Manager group supplied with Launch!.
   LAUNCH.GRP is now the clean four-icon Windows 3.1 template supplied for
   Release 3.79.  Its visual coordinates are preserved exactly.  The item
   directory in a GRP file is not necessarily stored in screen order, so find
   each item by title rather than assuming fixed record positions.  We then
   normalize the logical order to DOS, Configuration, Launch! 16, Menu Manager
   (the first two are the Windows 3.0 subset), relocate all C:\LAUNCH paths,
   repair item/tag offsets and regenerate the 16-bit GRP checksum. */
static int install_launch_group(const char *install,const char *winpath,int win_version)
{
  static char src[PATH_SIZE],dst[PATH_SIZE];FILE *f;unsigned char *in,*out;
  static const char *roles[4]={"Launch! for DOS","Configuration","Launch! 16","Menu Manager"};
  long sz;unsigned old_cb,new_cb,old_items[4],old_role[4],new_role[4];
  unsigned old_ord[4],pos[16];int old_to_new[4],delta[16],reps=0;unsigned i,j,k,newsz;int dl;
  const char *old="C:\\LAUNCH";unsigned oldlen=9,newlen=(unsigned)strlen(install);
  unsigned fields[6]={12,14,16,18,20,22};unsigned sum,words,patched=0;const char *winexe;
  if(newlen<3||newlen>63)return 0;
  sprintf(src,"%s\\LAUNCH.GRP",install);f=fopen(src,"rb");if(!f)return 0;
  fseek(f,0,SEEK_END);sz=ftell(f);fseek(f,0,SEEK_SET);
  if(sz<100||sz>12000){fclose(f);return 0;}
  in=(unsigned char*)malloc((unsigned)sz);out=(unsigned char*)malloc((unsigned)sz+12*(newlen+8));
  if(!in||!out){if(in)free(in);if(out)free(out);fclose(f);return 0;}
  if(fread(in,1,(unsigned)sz,f)!=(unsigned)sz){fclose(f);free(in);free(out);return 0;}fclose(f);
  if(memcmp(in,"PMCC",4)){free(in);free(out);return 0;}
  old_cb=grp_word(in,6);if(grp_word(in,32)!=4){free(in);free(out);return 0;}
  for(i=0;i<4;i++)old_items[i]=grp_word(in,34+i*2);
  /* Identify the four records from their titles.  This deliberately accepts
     the clean template's non-sequential internal item table. */
  for(j=0;j<4;j++){
    int found=-1;
    for(i=0;i<4;i++){
      unsigned oi=old_items[i],to;
      if(oi+24>(unsigned)sz)continue;
      to=grp_word(in,oi+18);if(to>=(unsigned)sz)continue;
      if(!strcmp((char *)(in+to),roles[j])){found=(int)i;break;}
    }
    if(found<0){free(in);free(out);return 0;}
    old_role[j]=old_items[found];old_ord[j]=(unsigned)found;
  }
  /* Locate every template installation path: two per item plus one working
     directory tag per item = twelve occurrences in the supplied GRP. */
  for(i=0;i+oldlen<=(unsigned)sz;i++)if(!memcmp(in+i,old,oldlen)){
    if(reps>=16){free(in);free(out);return 0;}pos[reps]=i;delta[reps]=(int)newlen-(int)oldlen;reps++;i+=oldlen-1;
  }
  if(reps!=12){free(in);free(out);return 0;}
  /* Rebuild with resized strings. */
  i=j=k=0;while(i<(unsigned)sz){
    if(k<(unsigned)reps&&i==pos[k]){memcpy(out+j,install,newlen);j+=newlen;i+=oldlen;k++;}
    else out[j++]=in[i++];
  }
  newsz=j;new_cb=grp_map_offset(old_cb,pos,delta,reps);grp_set_word(out,6,new_cb);
  grp_set_word(out,22,grp_map_offset(grp_word(in,22),pos,delta,reps));
  /* Repair every item record, not just the visible Windows 3.0 subset. */
  for(i=0;i<4;i++){
    unsigned oi=old_items[i],ni=grp_map_offset(oi,pos,delta,reps);
    grp_set_word(out,34+i*2,ni);
    for(j=0;j<6;j++)grp_set_word(out,ni+fields[j],grp_map_offset(grp_word(in,oi+fields[j]),pos,delta,reps));
  }
  for(i=0;i<4;i++)new_role[i]=grp_map_offset(old_role[i],pos,delta,reps);
  /* Normalize logical order while retaining the template record coordinates:
     DOS, Configuration, Launch! 16, Menu Manager. */
  for(i=0;i<4;i++){grp_set_word(out,34+i*2,new_role[i]);old_to_new[old_ord[i]]=(int)i;}
  /* Windows 3.1 extension tags carry an item ordinal at +2.  Renumber it to
     match the normalized directory.  Tag 0x8101 also carries the working
     directory and therefore grows/shrinks with the installation path. */
  dl=(int)newlen-(int)oldlen;
  for(i=new_cb;i+6<newsz;){
    unsigned id=grp_word(out,i),idx,cb,nextcb;
    if(id==0xFFFF)break;
    cb=grp_word(out,i+4);if(cb<6||i+cb>newsz)break;
    if(id==0x8101||id==0x8102){idx=grp_word(out,i+2);if(idx<4)grp_set_word(out,i+2,(unsigned)old_to_new[idx]);}
    nextcb=cb;
    if(id==0x8101){nextcb=(unsigned)((int)cb+dl);grp_set_word(out,i+4,nextcb);}
    i+=nextcb;
  }
  /* The clean template already names !W31.EXE and !MNUMAN.EXE.  Four W31
     references exist: Launch! 16's command plus the DOS/Configuration/Win16
     icon sources.  Windows 3.0 swaps all four to the same-size !W30.EXE name;
     Windows 3.1/3.11 leaves them as !W31.EXE. */
  winexe=(win_version>=31)?"!W31.EXE":"!W30.EXE";
  for(i=0;i+8<=newsz;i++)if(!memcmp(out+i,"!W31.EXE",8)){
    memcpy(out+i,winexe,8);patched++;i+=7;
  }
  if(patched!=4){free(in);free(out);return 0;}
  /* Windows 3.0 exposes only the first two normalized items: DOS launcher and
     Configuration.  The attached four-item template remains intact for 3.1. */
  if(win_version<31)grp_set_word(out,32,2);
  /* Checksum is the negative 16-bit sum of all little-endian WORDs. */
  grp_set_word(out,4,0);sum=0;words=(newsz+1)/2;
  for(i=0;i<words;i++){unsigned lo=out[i*2],hi=(i*2+1<newsz)?out[i*2+1]:0;sum=(sum+lo+(hi<<8))&0xFFFF;}
  grp_set_word(out,4,(unsigned)((0x10000UL-sum)&0xFFFF));
  sprintf(dst,"%s\\LAUNCH.GRP",winpath);f=fopen(dst,"wb");if(!f){free(in);free(out);return 0;}
  if(fwrite(out,1,newsz,f)!=newsz){fclose(f);free(in);free(out);return 0;}if(fclose(f)){free(in);free(out);return 0;}
  free(in);free(out);remove(src);return 1;
}

/* Register LAUNCH.GRP for the next Program Manager start without DDE.
   Group numbers in PROGMAN.INI must remain contiguous on Windows 3.0. */
static void cleanup_old_windows_integration(const char *install,const char *winpath)
{
  static const char *junk[]={"WINSETUP.EXE","WINSETUP.CFG","LAUNCH.MNU","LAUNCH.CFG","LAUNCH.PIF","LAUNCH.ICO","LAUNCH16.ICO","CONFIG.ICO","MENUMGR.ICO","!WIN16.EXE","!W30.EXE","!W31.EXE","!MGR16.EXE","!MNUMAN.EXE",0};
  static char p[PATH_SIZE],ini[PATH_SIZE],tmp[PATH_SIZE],line[512],copy[512],self[PATH_SIZE];FILE *in,*out;int i;
  /* Remove files that older 3.75 test installers may have left in WINDOWS.
     LAUNCH.GRP is intentionally excluded. */
  for(i=0;junk[i];i++){sprintf(p,"%s\\%s",winpath,junk[i]);remove(p);}
  /* Also remove the obsolete one-shot WINSETUP token from [windows] run=. */
  sprintf(ini,"%s\\WIN.INI",winpath);sprintf(tmp,"%s\\WIN.LT$",winpath);sprintf(self,"%s\\WINSETUP.EXE",install);
  in=fopen(ini,"rt");if(!in)return;out=fopen(tmp,"wt");if(!out){fclose(in);return;}
  while(fgets(line,sizeof(line),in)){
    char *q=line,*hit;while(*q==' '||*q=='\\t')q++;
    if(!strnicmp(q,"run=",4)){
      strcpy(copy,line);hit=copy;
      while((hit=strstr(hit,"WINSETUP.EXE"))!=0){
        char *a=hit,*b=hit+12;while(a>copy&&a[-1]!='='&&a[-1]!=' '&&a[-1]!='\\t')a--;
        while(*b&&*b!=' '&&*b!='\\t'&&*b!='\\r'&&*b!='\\n')b++;
        while(*b==' '||*b=='\\t')b++;memmove(a,b,strlen(b)+1);hit=a;
      }
      fputs(copy,out);
    }else fputs(line,out);
  }
  fclose(in);if(!fclose(out)){remove(ini);rename(tmp,ini);}else remove(tmp);
}

/* Put the resident Win16 Launch! companion on WIN.INI [windows] load=.
   Existing load= programs are preserved.  Launch! uses semicolons between
   entries and replaces an older !W30/!W31 entry during an upgrade. */
static int launch_load_entry(const char *entry)
{
  const char *p=entry,*base;char name[32];int n;
  while(*p==' '||*p=='\t')p++;base=p;
  while(*p&&*p!=';'&&*p!='\r'&&*p!='\n'){if(*p=='\\'||*p=='/')base=p+1;p++;}
  n=(int)(p-base);while(n>0&&(base[n-1]==' '||base[n-1]=='\t'))n--;
  if(n<=0||n>=(int)sizeof(name))return 0;
  memcpy(name,base,n);name[n]=0;
  return !stricmp(name,"!W30.EXE")||!stricmp(name,"!W31.EXE");
}

static int append_windows_load_text(char *out,int outsize,const char *text)
{
  int have=(int)strlen(out),need=(int)strlen(text);
  if(have+need>=outsize)return 0;
  memcpy(out+have,text,need+1);return 1;
}

static int rebuild_windows_load_value(const char *oldvalue,const char *launchprog,char *out,int outsize)
{
  const char *p=oldvalue,*q;int first=1,n;char token[256];out[0]=0;
  while(*p){
    q=strchr(p,';');if(!q)q=p+strlen(p);n=(int)(q-p);if(n>=(int)sizeof(token))n=(int)sizeof(token)-1;
    memcpy(token,p,n);token[n]=0;
    {char *a=token,*b;while(*a==' '||*a=='\t')a++;b=a+strlen(a);while(b>a&&(b[-1]==' '||b[-1]=='\t'))*--b=0;
      if(*a&&!launch_load_entry(a)){
        if(!first&&!append_windows_load_text(out,outsize,";"))return 0;
        if(!append_windows_load_text(out,outsize,a))return 0;first=0;
      }
    }
    p=*q?q+1:q;
  }
  if(!first&&!append_windows_load_text(out,outsize,";"))return 0;
  return append_windows_load_text(out,outsize,launchprog);
}

static int register_windows_load(const char *install,const char *winpath,int win_version)
{
  static char ini[PATH_SIZE],tmp[PATH_SIZE],line[512],value[512],launchprog[PATH_SIZE];
  FILE *in,*out;char *p,*q;int in_windows=0,saw_windows=0,saw_load=0;
  sprintf(ini,"%s\\WIN.INI",winpath);sprintf(tmp,"%s\\WIN.LD$",winpath);
  sprintf(launchprog,"%s\\%s",install,win_version>=31?"!W31.EXE":"!W30.EXE");
  in=fopen(ini,"rt");if(!in)return 0;out=fopen(tmp,"wt");if(!out){fclose(in);return 0;}
  while(fgets(line,sizeof(line),in)){
    p=line;while(*p==' '||*p=='\t')p++;
    if(*p=='['){
      if(in_windows&&!saw_load){fprintf(out,"load=%s\n",launchprog);saw_load=1;}
      in_windows=!strnicmp(p,"[windows]",9);if(in_windows)saw_windows=1;
    }
    if(in_windows){
      q=p;if(!strnicmp(q,"load",4)){q+=4;while(*q==' '||*q=='\t')q++;if(*q=='='){
        q++;q[strcspn(q,"\r\n")]=0;if(!rebuild_windows_load_value(q,launchprog,value,sizeof(value))){fclose(in);fclose(out);remove(tmp);return 0;}
        fprintf(out,"load=%s\n",value);saw_load=1;continue;
      }}
    }
    fputs(line,out);
  }
  if(in_windows&&!saw_load){fprintf(out,"load=%s\n",launchprog);saw_load=1;}
  if(!saw_windows){fprintf(out,"\n[windows]\nload=%s\n",launchprog);saw_load=1;}
  fclose(in);if(fclose(out)){remove(tmp);return 0;}
  remove(ini);if(rename(tmp,ini)){remove(tmp);return 0;}return saw_load;
}

static int register_launch_group(const char *winpath)
{
  static char ini[PATH_SIZE],tmp[PATH_SIZE],grp[PATH_SIZE],line[512];FILE *in,*out;
  int in_groups=0,saw_groups=0,inserted=0,maxgroup=0;char *p;int n;
  sprintf(ini,"%s\\PROGMAN.INI",winpath);sprintf(tmp,"%s\\PROGMAN.$$$",winpath);sprintf(grp,"%s\\LAUNCH.GRP",winpath);
  in=fopen(ini,"rt");if(!in)return 0;out=fopen(tmp,"wt");if(!out){fclose(in);return 0;}
  while(fgets(line,sizeof(line),in)){
    p=line;while(*p==' '||*p=='\\t')p++;
    if(*p=='['){
      if(in_groups&&!inserted){fprintf(out,"Group%d=%s\n",maxgroup+1,grp);inserted=1;}
      in_groups=!strnicmp(p,"[Groups]",8);if(in_groups)saw_groups=1;
    }
    if(in_groups&&!strnicmp(p,"Group",5)){
      n=atoi(p+5);if(n>maxgroup)maxgroup=n;
      if(contains_icase(p,grp)){inserted=1;}
    }
    fputs(line,out);
  }
  if(in_groups&&!inserted){fprintf(out,"Group%d=%s\n",maxgroup+1,grp);inserted=1;}
  if(!saw_groups){fprintf(out,"\n[Groups]\nGroup1=%s\n",grp);inserted=1;}
  fclose(in);if(fclose(out)){remove(tmp);return 0;}
  remove(ini);if(rename(tmp,ini)){remove(tmp);return 0;}return inserted;
}

int main(int argc,char **argv)
{
  static char install[PATH_SIZE],source_dir[PATH_SIZE],archive[PATH_SIZE];
  static char destination[PATH_SIZE],launch_exe[PATH_SIZE],autoexec[16],key_spec[64],post_apply_command[PATH_SIZE+16];
  static char child_comspec[PATH_SIZE+9],child_path[PATH_SIZE*2+6];
  char *child_env[3];
  char *comspec,*envpath;
  int n,dosbox_detected,shortcut_build=-1,is286,update_autoexec=0,upgrade=0;
  int accessories=0,games=0,screensavers=0,fonts=0,menu_generator=0,shortcut_key=0,sample_docs=0;
  int cpu_ok,display_ok,vga_display,menu_result;
  const char *display_name;
  int add_path=0,add_shortcut=0,show_menu=0,font_start=0,light_target=0,post_apply_ready=0;
  int win_inst=0,win_version=30;static char win_path[PATH_SIZE];
  (void)argc;
  installer_clear_screen();
  puts("\n");
  colour_text("Launch!",12);puts(" 3.79 Installation");
  installer_title_rule();
  puts("");
  cpu_ok=cpu_at_least_286();display_name=display_adapter(&display_ok);vga_display=!strncmp(display_name,"VGA",3);if((!cpu_ok||!display_ok)&&!hardware_warning())return 1;
  question_icon(0);printf("Install to directory [");colour_text("C:\\LAUNCH",10);printf("]: ");
  if(!fgets(install,sizeof(install),stdin))return 1;
  strip_line(install);
  if(!*install)strcpy(install,"C:\\LAUNCH");
  n=(int)strlen(install);
  if(n<3 || n>PATH_SIZE-20 || install[1]!=':' || strchr(install,';') || strchr(install,'"')){
    error_icon(0);puts("Invalid DOS installation path.");return 1;
  }
  while(n>3 && (install[n-1]=='\\' || install[n-1]=='/'))install[--n]=0;
  if(!make_directories(install)){error_icon(0);printf("Cannot create or access %s\n",install);return 1;}
  sprintf(destination,"%s\\!WELCOME.BAT",install);remove(destination);
  sprintf(destination,"%s\\LAUNCH.MNU",install);upgrade=exists(destination);
  if(!upgrade){sprintf(destination,"%s\\LAUNCH.CFG",install);upgrade=exists(destination);}
  if(upgrade)puts("\nExisting Launch! installation detected.\nAn upgrade will be performed, and existing configuration retained.");
  {
    char choose[32];int i,first_omit=1;
    int *flags[7];
    const char *labels[7]={"Accessories","Games","Screensavers","Fonts","Menu Generator","Keyboard Shortcut","Help and Sample Docs"};
    flags[0]=&accessories;flags[1]=&games;flags[2]=&screensavers;flags[3]=&fonts;flags[4]=&menu_generator;flags[5]=&shortcut_key;flags[6]=&sample_docs;
    accessories=games=screensavers=fonts=menu_generator=shortcut_key=sample_docs=1;

    puts("\nThe following components will be installed:\n");
    for(i=0;i<7;i++){
      component_icon(i+1);
      printf("%s\n",labels[i]);
    }

    fputs("\nPress ",stdout);putchar(17);putchar(217);
    fputs(" to continue install, or\n",stdout);
    fputs("type the number/s for components you want excluded, and press ",stdout);
    putchar(17);putchar(217);puts(".");
    puts("");
    question_icon(0);fputs("Exclude number/s: ",stdout);fflush(stdout);

    if(!fgets(choose,sizeof(choose),stdin)){
      error_icon(0);puts("Unable to read component selection.");return 1;
    }
    strip_line(choose);
    for(i=0;choose[i];i++){
      if(choose[i]>='1'&&choose[i]<='7')
        *flags[choose[i]-'1']=0;
      else if(choose[i]!=' '&&choose[i]!=','&&choose[i]!=';'){
        error_icon(0);puts("Invalid component selection. Use only numbers 1 through 7.");return 1;
      }
    }

    if(!choose[0]){
      puts("\nAll components selected for install.");
    }else{
      fputs("\nNot installing: ",stdout);
      for(i=0;i<7;i++)if(!*flags[i]){
        if(!first_omit)fputs(", ",stdout);
        fputs(labels[i],stdout);
        first_omit=0;
      }
      puts("");
    }
  }
  if(shortcut_key){
    is286=cpu_is_286();dosbox_detected=running_in_dosbox();
    if(is286){puts("\n80286-class CPU detected; the 286-safe shortcut will be installed.");shortcut_build=2;}
    else if(dosbox_detected)shortcut_build=1;
    else shortcut_build=0;
  }

  /* Windows integration is chosen before copying.  Once that question (and
     optional path) is complete, move to the dedicated copy/progress screen. */
  strcpy(win_path,"C:\\WINDOWS");
  puts("");win_inst=ask_windows_yes("Add Windows 3.x integration?",1,1);
  if(win_inst){
    char entered[PATH_SIZE];
    puts("");
    for(;;){
      question_icon(0);printf("Windows installation path [");colour_text("C:\\WINDOWS",10);printf("]: ");
      if(!fgets(entered,sizeof(entered),stdin))return 1;strip_line(entered);
      if(*entered)strncpy(win_path,entered,sizeof(win_path)-1);win_path[sizeof(win_path)-1]=0;
      n=(int)strlen(win_path);while(n>3&&(win_path[n-1]=='\\'||win_path[n-1]=='/'))win_path[--n]=0;
      if(valid_windows3_path(win_path))break;
      error_icon(0);printf("%s does not appear to contain Windows 3.x (WIN.COM/PROGMAN.EXE not found).\n",win_path);
    }
    win_version=detect_windows_version(win_path);
    /* Release 3.79 keeps !W30.EXE out of the first-floppy distribution while
       Windows 3.0 support remains under development.  Do not register a
       missing companion, WIN.INI load= entry, or Program Manager group. */
    if(win_version<31){
      puts("");
      info_icon(0);puts("Windows 3.0 integration is not included in Release 3.79.");
      puts("DOS installation will continue without Windows integration.");
      win_inst=0;
    }
  }

  installer_clear_screen();
  puts("\n");
  colour_text("Launch!",12);puts(" 3.79 Installation");
  installer_title_rule();
  puts("\n Please wait while files are extracted and copied...");fflush(stdout);
  source_directory(argv[0],source_dir);sprintf(archive,"%sINSTALL.DAT",source_dir);
  if(!exists(archive)){error_icon(0);printf("Cannot find %s\n",archive);return 1;}
  sprintf(launch_exe,"%s\\!.EXE",install);
  if(!extract_install_files(archive,install,shortcut_build,accessories,games,fonts,menu_generator,sample_docs,vga_display,win_inst,win_version))return 1;
  remove_unselected_components(install,accessories,games,fonts,menu_generator,shortcut_key,sample_docs,vga_display);
  if(!win_inst){
    static const char *winfiles[]={"!W30.EXE","!W31.EXE","!MNUMAN.EXE","LAUNCH.ICO","LAUNCH16.ICO","CONFIG.ICO","MENUMGR.ICO","LAUNCH.GRP",0};
    int wi;for(wi=0;winfiles[wi];wi++)remove_named(install,winfiles[wi]);
  }else if(win_version<31){
    remove_named(install,"!MNUMAN.EXE");remove_named(install,"MENUMGR.ICO");
  }
  /* A machine gets exactly one built-in font catalogue.  Also remove an
     opposite-format catalogue left behind by an earlier installation. */
  if(fonts){if(vga_display)remove_named(install,"FONT14.DAT");else remove_named(install,"FONT.DAT");}
  if(!upgrade&&!write_initial_font_config(install,(fonts&&display_ok)?6:0)){error_icon(0);puts("Files were copied, but the initial font configuration could not be created.");return 1;}
  if(!upgrade&&fonts&&display_ok&&!write_initial_font_cache(install,vga_display,6)){error_icon(0);puts("Files were copied, but the initial Launch! font cache could not be created.");return 1;}
  if(!apply_component_config(install,screensavers,fonts,shortcut_key)){
    error_icon(0);puts("Files were copied, but the selected component configuration could not be applied.");return 1;
  }
  if(win_inst){
    cleanup_old_windows_integration(install,win_path);
    if(!register_windows_load(install,win_path,win_version)){error_icon(0);puts("Windows integration selected, but WIN.INI load= could not be updated.");return 1;}
    if(!write_launch_pif(install)){error_icon(0);puts("Windows integration selected, but LAUNCH.PIF could not be created.");return 1;}
    if(!install_launch_group(install,win_path,win_version)){error_icon(0);puts("Windows integration selected, but LAUNCH.GRP could not be installed.");return 1;}
    if(!register_launch_group(win_path)){error_icon(0);puts("Windows integration selected, but LAUNCH.GRP could not be registered in PROGMAN.INI.");return 1;}
    remove_named(install,"LAUNCH.ICO");remove_named(install,"LAUNCH16.ICO");
  }
  if(!write_windows_config(install,win_inst,win_path)){error_icon(0);puts("Could not save Windows integration settings.");return 1;}

  /* Keep the /INITMENU helper environment deliberately tiny.  The Microsoft C
     startup code copies the inherited DOS environment into the child near heap;
     a large development AUTOEXEC environment can otherwise make the already-large
     core fail at startup with R6009 even though conventional memory is available. */
  comspec=getenv("COMSPEC");envpath=getenv("PATH");
  {int ce=0;child_env[0]=NULL;
    if(comspec && *comspec){sprintf(child_comspec,"COMSPEC=%s",comspec);child_env[ce++]=child_comspec;}
    /* /INITMENU must still see the user's DOS command search path.  Omitting
       PATH made the generated DOS Commands folder empty, while retaining only
       COMSPEC+PATH keeps the child environment small enough for the large core. */
    if(envpath && *envpath){strcpy(child_path,"PATH=");strncat(child_path,envpath,sizeof(child_path)-6);child_path[sizeof(child_path)-1]=0;child_env[ce++]=child_path;}
    child_env[ce]=NULL;
  }
  menu_result=spawnle(P_WAIT,launch_exe,"!.EXE","/INITMENU",NULL,child_env);
  if(menu_result!=0){error_icon(0);puts("Files were copied, but the standard Launch! menu could not be created or updated.");return 1;}
  comspec=getenv("COMSPEC");
  autoexec[0]=(comspec && comspec[1]==':')?(char)toupper(comspec[0]):'C';
  strcpy(autoexec+1,startup_batch_suffix());
  key_spec[0]=0;
  printf("\n");
  add_shortcut=shortcut_key?ask_yes("Enable keyboard shortcut?",1,0):0;
  if(add_shortcut){
    printf("\n The keyboard shortcut is set to ");colour_text("CTRL+ALT+\\",10);puts("");
    puts("");if(ask_yes("Change the shortcut key/s?",0,0))capture_shortcut(key_spec);
  }
  puts("");show_menu=!upgrade?ask_yes("Do you want to open the menu at startup?",0,0):startup_cfg_bool(install,"OPEN_MENU_BOOT",0);
  font_start=!upgrade?fonts:(fonts&&startup_cfg_bool(install,"fontPersist",startup_cfg_bool(install,"FONT_PERSIST",0)));
  if(!append_startup_service_config(install,add_shortcut,key_spec,show_menu,font_start)){error_icon(0);puts("Could not save startup service configuration.");return 1;}
  light_target=startup_cfg_light(install);
  /* The installer always owns the Launch! PATH/startup block.  PATH is needed
     even when optional resident services were not selected. */
  if(!migrate_launch_startup(autoexec,install,add_shortcut,key_spec,font_start,light_target,show_menu,upgrade)){
    printf("\n");error_icon(0);printf("Files copied, but %s could not be migrated.\n",autoexec);return 1;
  }
  post_apply_command[0]=0;
  post_apply_ready=prepare_postinstall_start(install,add_shortcut,key_spec,font_start,light_target,post_apply_command);
  printf("\nInstalled Launch! to %s\n\n",install);
  success_icon(0);
  puts("Install is complete.");
  if(!post_apply_ready){
    puts("Launch! startup settings will apply on the next DOS startup.");
  }
  fputs("\nPress ",stdout);putchar(17);putchar(217);puts(" to exit.");
  getchar();
  if(post_apply_ready&&post_apply_command[0]){
    /* The PC BIOS keyboard ring only has room for fifteen queued keystrokes.
       Put DOS in the installed Launch! directory first, then inject the short
       !WELCOME command.  The shell prompt therefore naturally resumes in the
       installed directory and executes C:\LAUNCH\!WELCOME.BAT without a
       long path overflowing the BIOS keyboard buffer. */
    if(install[1]==':')_chdrive(toupper((unsigned char)install[0])-'A'+1);
    chdir(install);
    installer_queue_bios_short_text(post_apply_command);
  }
  return 0;
}
