/* Launch! 3.72 installer - Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <direct.h>
#include <io.h>
#include <process.h>

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
  for(i=0;i<indent;i++)putchar(' ');
  colour_text(block,colour);
  colour_text(mark,(colour<<4)|15);
  colour_text(block,colour);
  putchar(' ');
}
static void question_icon(int indent){status_icon(indent,1,'?');}

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
static int write_initial_font_config(const char *install,int font_id){char path[PATH_SIZE];FILE*f;sprintf(path,"%s\\LAUNCH.CFG",install);f=fopen(path,"wt");if(!f)return 0;fprintf(f,"TITLEBAR_FG=15\nTITLEBAR_BG=7\nFONT_ID=%d\n",font_id);return fclose(f)==0;}
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


static int loadhigh_supported(const char *probe_dir)
{
  char bat[PATH_SIZE],ok[PATH_SIZE],cmd[PATH_SIZE*3],*comspec;FILE *f;int found;
  comspec=getenv("COMSPEC");if(!comspec||!*comspec||!probe_dir||!*probe_dir)return 0;
  sprintf(bat,"%s\\LHTEST.BAT",probe_dir);sprintf(ok,"%s\\LHTEST.$$$",probe_dir);
  remove(bat);remove(ok);f=fopen(bat,"wt");if(!f)return 0;
  fprintf(f,"@ECHO OFF\nECHO Y>%s\n",ok);if(fclose(f)!=0){remove(bat);return 0;}
  sprintf(cmd,"LOADHIGH %s /C %s >NUL",comspec,bat);system(cmd);remove(bat);
  f=fopen(ok,"rb");found=f!=0;if(f)fclose(f);remove(ok);return found;
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
  for(i=0;i<3;i++){
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

static int apply_component_config(const char *install,int screensavers,int fonts)
{
  char path[PATH_SIZE];FILE *f;int ok=1;
  if(screensavers&&fonts)return 1;
  sprintf(path,"%s\\LAUNCH.CFG",install);
  f=fopen(path,"at");if(!f)return 0;
  /* Last value wins in Launch!'s config parser.  Appending avoids rewriting
     a user's existing configuration while still making deselection safe on
     both fresh installs and upgrades. */
  if(!screensavers&&fputs("SCREENSAVER=0\n",f)==EOF)ok=0;
  if(!fonts){
    if(fputs("FONT_ID=0\n",f)==EOF)ok=0;
    if(fputs("FONT_PERSIST=0\n",f)==EOF)ok=0;
  }
  if(fclose(f)!=0)ok=0;return ok;
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
  colour_text(line,11);
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

static DAT_ENTRY dat_entry[64];

static int component_member(const char *name,int component)
{
  static const char *accessories[]={
    "CAL.ICS","!CAL.EXE","!CALC.EXE","!DRAW.EXE","!JOURNAL.EXE","!MKDOWN.EXE",
    "!NOTE.EXE","!STACK.EXE","!DFETCH.EXE","!TODOS.EXE",0};
  static const char *games[]={
    "!TYPO.EXE","TYPO.LVL","!BOXES.EXE","BOXES.LVL","!FCELL.EXE","!PLUMB.EXE",
    "!POP.EXE","!SNAKE.EXE","!SOL.EXE","!WORDZ.EXE","WORDZ.LVL",0};
  const char **files;int i;
  files=component==1?accessories:games;
  for(i=0;files[i];i++)if(!stricmp(name,files[i]))return 1;
  return 0;
}

static void remove_named(const char *install,const char *name)
{
  char p[PATH_SIZE];sprintf(p,"%s\\%s",install,name);remove(p);
}
static void remove_unselected_components(const char *install,int accessories,int games,int fonts,int menu_generator,int shortcut_key)
{
  static const char *acc[]={"CAL.ICS","!CAL.EXE","!CALC.EXE","!DRAW.EXE","!JOURNAL.EXE","!MKDOWN.EXE","!NOTE.EXE","!STACK.EXE","!DFETCH.EXE","!TODOS.EXE",0};
  static const char *gm[]={"!TYPO.EXE","TYPO.LVL","!BOXES.EXE","BOXES.LVL","!FCELL.EXE","!PLUMB.EXE","!POP.EXE","!SNAKE.EXE","!SOL.EXE","!WORDZ.EXE","WORDZ.LVL",0};
  int i;if(!accessories)for(i=0;acc[i];i++)remove_named(install,acc[i]);if(!games)for(i=0;gm[i];i++)remove_named(install,gm[i]);if(!fonts)remove_named(install,"FONT.DAT");if(!menu_generator){remove_named(install,"!MNUGEN.EXE");remove_named(install,"AUTOGEN.DAT");}if(!shortcut_key)remove_named(install,"!KEY.COM");
}

static int selected_member(const char *name,int shortcut_build,int accessories,
                           int games,int fonts,int menu_generator,const char **dest_name)
{
  *dest_name=name;
  if(!stricmp(name,"!.EXE")||!stricmp(name,"PROMPTS.CFG")||!stricmp(name,"PWROFF.BMP"))return 1;
  if(fonts&&!stricmp(name,"FONT.DAT"))return 1;
  if(menu_generator&&(!stricmp(name,"!MNUGEN.EXE")||!stricmp(name,"AUTOGEN.DAT")))return 1;
  if(accessories&&component_member(name,1))return 1;
  if(games&&component_member(name,2))return 1;
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
                                 int accessories,int games,int fonts,int menu_generator)
{
  FILE *in,*out;char magic[8],destination[PATH_SIZE];const char *dest_name;
  unsigned count,i;int selected,done=0,ok;
  in=fopen(archive,"rb");if(!in)return 0;
  if(fread(magic,1,8,in)!=8||memcmp(magic,DAT_MAGIC,8)){fclose(in);return 0;}
  count=read_u16(in);if(!count||count>64){fclose(in);return 0;}
  for(i=0;i<count;i++){
    if(fread(dat_entry[i].name,1,13,in)!=13){fclose(in);return 0;}
    dat_entry[i].name[12]=0;
    dat_entry[i].offset=read_u32(in);
    dat_entry[i].csize=read_u32(in);
    dat_entry[i].usize=read_u32(in);
  }
  extract_progress_total=0;
  for(i=0;i<count;i++)
    if(selected_member(dat_entry[i].name,shortcut_build,accessories,games,fonts,menu_generator,&dest_name))
      extract_progress_total++;
  if(!extract_progress_total)extract_progress_total=1;
  extract_progress_done=0;draw_extract_progress();
  if(fseek(in,(long)dat_entry[0].offset,SEEK_SET)){fclose(in);return 0;}

  for(i=0;i<count;i++){
    selected=selected_member(dat_entry[i].name,shortcut_build,accessories,games,fonts,menu_generator,&dest_name);
    if(selected){
      sprintf(destination,"%s\\%s",install,dest_name);
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
  sprintf(destination,"%s\\!SYSINFO.EXE",install);remove(destination);
  return 1;
}


int main(int argc,char **argv)
{
  static char install[PATH_SIZE],source_dir[PATH_SIZE],archive[PATH_SIZE];
  static char destination[PATH_SIZE],launch_exe[PATH_SIZE],autoexec[16],key_spec[64];
  char *comspec;
  int n,dosbox_detected,shortcut_build=-1,is286,update_autoexec=0,upgrade=0;
  int accessories=0,games=0,screensavers=0,fonts=0,menu_generator=0,shortcut_key=0;
  int cpu_ok,display_ok,vga_display,menu_result;
  const char *display_name;
  int add_path=0,add_shortcut=0,show_menu=0,autoexec_changed=0;
  (void)argc;
  installer_clear_screen();
  puts("\n");
  puts("Launch! 3.72 Installation");
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
  sprintf(destination,"%s\\LAUNCH.MNU",install);upgrade=exists(destination);
  if(!upgrade){sprintf(destination,"%s\\LAUNCH.CFG",install);upgrade=exists(destination);}
  if(upgrade)puts("\nExisting Launch! installation detected.\nAn upgrade will be performed, and existing configuration retained.");
  {
    char choose[32];int i,first_omit=1;
    int *flags[6];
    const char *labels[6]={"Accessories","Games","Screensavers","Fonts","Menu Generator","Keyboard Shortcut"};
    flags[0]=&accessories;flags[1]=&games;flags[2]=&screensavers;flags[3]=&fonts;flags[4]=&menu_generator;flags[5]=&shortcut_key;
    accessories=games=screensavers=fonts=menu_generator=shortcut_key=1;

    puts("\nThe following components will be installed:\n");
    for(i=0;i<6;i++){
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
      if(choose[i]>='1'&&choose[i]<='6')
        *flags[choose[i]-'1']=0;
      else if(choose[i]!=' '&&choose[i]!=','&&choose[i]!=';'){
        error_icon(0);puts("Invalid component selection. Use only numbers 1 through 6.");return 1;
      }
    }

    if(!choose[0]){
      puts("\nAll components selected for install.");
    }else{
      fputs("\nNot installing: ",stdout);
      for(i=0;i<6;i++)if(!*flags[i]){
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
  puts("\n Please wait while files are extracted and copied...");fflush(stdout);
  source_directory(argv[0],source_dir);sprintf(archive,"%sINSTALL.DAT",source_dir);
  if(!exists(archive)){error_icon(0);printf("Cannot find %s\n",archive);return 1;}
  sprintf(launch_exe,"%s\\!.EXE",install);
  if(!extract_install_files(archive,install,shortcut_build,accessories,games,fonts,menu_generator))return 1;
  remove_unselected_components(install,accessories,games,fonts,menu_generator,shortcut_key);
  if(!upgrade&&!write_initial_font_config(install,(fonts&&vga_display)?1:0)){error_icon(0);puts("Files were copied, but the initial font configuration could not be created.");return 1;}
  if(!apply_component_config(install,screensavers,fonts)){
    error_icon(0);puts("Files were copied, but the selected component configuration could not be applied.");return 1;
  }
  menu_result=spawnl(P_WAIT,launch_exe,"!.EXE","/INITMENU",NULL);
  if(menu_result!=0){error_icon(0);puts("Files were copied, but the standard Launch! menu could not be created or updated.");return 1;}
  comspec=getenv("COMSPEC");
  autoexec[0]=(comspec && comspec[1]==':')?(char)toupper(comspec[0]):'C';
  strcpy(autoexec+1,":\\AUTOEXEC.BAT");
  key_spec[0]=0;
  printf("\n");
  update_autoexec=!upgrade&&ask_yes("Do you want to update your AUTOEXEC.BAT file?",1,0);
  if(update_autoexec){
    puts("");add_path=ask_yes("Add Launch! to PATH?",1,5);
    puts("");add_shortcut=shortcut_key?ask_yes("Enable keyboard shortcut?",1,5):0;
    if(add_shortcut){
      printf("\n     The keyboard shortcut is set to ");colour_text("CTRL+ALT+.",10);puts("");
      puts("");if(ask_yes("Change the shortcut key/s?",0,5))capture_shortcut(key_spec);
    }
    puts("");show_menu=ask_yes("Show menu after startup?",0,5);
    if(add_path || add_shortcut || show_menu){
      if(!append_autoexec(autoexec,install,add_path,add_shortcut,key_spec,show_menu)){
        error_icon(0);printf("Files copied, but %s could not be updated.\n",autoexec);return 1;
      }
      autoexec_changed=1;
    }
  }
  printf("\nInstalled Launch! to %s\n\n",install);
  success_icon(0);
  if(autoexec_changed)puts("Install is complete. Reboot to activate the selected startup options.");
  else puts("Install is complete.");
  fputs("\nPress ",stdout);putchar(17);putchar(217);puts(" to exit.");
  getchar();return 0;
}
