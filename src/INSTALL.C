/* Launch! 2.5 installer - Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <direct.h>
#include <io.h>
#include <process.h>

#define PATH_SIZE 128

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

static int copy_file(const char *source,const char *destination)
{
  FILE *in,*out;size_t n;int ok=1;
  if(!stricmp(source,destination))return 1;
  in=fopen(source,"rb");if(!in)return 0;
  out=fopen(destination,"wb");if(!out){fclose(in);return 0;}
  while((n=fread(copy_buffer,1,sizeof(copy_buffer),in))!=0)
    if(fwrite(copy_buffer,1,n,out)!=n){ok=0;break;}
  if(ferror(in))ok=0;
  if(fclose(out)!=0)ok=0;
  fclose(in);return ok;
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

static int ask_yes(const char *prompt,int default_yes)
{
  char answer[16];
  printf("%s [%s]: ",prompt,default_yes?"Y/n":"y/N");
  if(!fgets(answer,sizeof(answer),stdin))return default_yes;
  if(answer[0]=='\r' || answer[0]=='\n' || !answer[0])return default_yes;
  return toupper(answer[0])=='Y';
}

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
  static char text[80],label[16];text[0]=0;
  if(shift&4)append_key_display(text,"CTRL");
  if(shift&8)append_key_display(text,"ALT");
  if(shift&3)append_key_display(text,"SHIFT");
  if(scan){key_label(scan,label);append_key_display(text,label);}
  printf("\rDetected: %-58s",text);fflush(stdout);
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

static int append_autoexec(const char *filename,const char *path,int add_path,
                           int add_shortcut,const char *key_spec,int show_menu)
{
  FILE *f;static char path_line[256],load_line[256],menu_line[256];long size;
  static const char *lines[3];static int wanted[3];int i,last=0,missing=0;
  sprintf(path_line,"PATH %%PATH%%;%s",path);
  sprintf(load_line,"LOADHIGH %s\\SHORTCUT.COM",path);
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

int main(int argc,char **argv)
{
  static char install[PATH_SIZE],source_dir[PATH_SIZE],source[PATH_SIZE];
  static char destination[PATH_SIZE],autoexec[16],answer[16],key_spec[64];
  char *comspec;
  int n,dosbox_detected,use_dosbox,update_autoexec=0;
  int add_path=0,add_shortcut=0,show_menu=0,autoexec_changed=0;
  (void)argc;
  puts("Launch! 2.5 Installation");
  puts("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n\n");
  printf("Install to directory [C:\\LAUNCH]: ");
  if(!fgets(install,sizeof(install),stdin))return 1;
  strip_line(install);
  if(!*install)strcpy(install,"C:\\LAUNCH");
  n=(int)strlen(install);
  if(n<3 || n>PATH_SIZE-20 || install[1]!=':' || strchr(install,';') || strchr(install,'"')){
    puts("Invalid DOS installation path.");return 1;
  }
  while(n>3 && (install[n-1]=='\\' || install[n-1]=='/'))install[--n]=0;
  if(!make_directories(install)){printf("Cannot create or access %s\n",install);return 1;}
  dosbox_detected=running_in_dosbox();
  if(dosbox_detected){
    printf("\nIt looks like you're running in DOSBox, is that correct? [Y/n]: ");
    if(!fgets(answer,sizeof(answer),stdin))return 1;
    use_dosbox=!answer[0]||answer[0]=='\r'||answer[0]=='\n'||toupper(answer[0])=='Y';
  }else{
    printf("\nAre you installing in DOSBox? [y/N]: ");
    if(!fgets(answer,sizeof(answer),stdin))return 1;
    use_dosbox=toupper(answer[0])=='Y';
  }
  puts("\nCopying files...");fflush(stdout);
  source_directory(argv[0],source_dir);
  sprintf(source,"%s!.EXE",source_dir);sprintf(destination,"%s\\!.EXE",install);
  if(!copy_file(source,destination)){printf("Cannot copy %s\n",source);return 1;}
  sprintf(source,"%s%s",source_dir,use_dosbox?"SHORTCDB.COM":"SHORTCUT.COM");
  sprintf(destination,"%s\\SHORTCUT.COM",install);
  if(!copy_file(source,destination)){printf("Cannot copy %s\n",source);return 1;}
  sprintf(source,"%sAUTOGEN.EXE",source_dir);sprintf(destination,"%s\\AUTOGEN.EXE",install);
  if(!copy_file(source,destination)){printf("Cannot copy %s\n",source);return 1;}
  sprintf(source,"%sAUTOGEN.DAT",source_dir);sprintf(destination,"%s\\AUTOGEN.DAT",install);
  if(!copy_file(source,destination)){printf("Cannot copy %s\n",source);return 1;}
  sprintf(source,"%sPWROFF.BMP",source_dir);sprintf(destination,"%s\\PWROFF.BMP",install);
  if(!copy_file(source,destination)){printf("Cannot copy %s\n",source);return 1;}
  sprintf(source,"%sFONT.DAT",source_dir);sprintf(destination,"%s\\FONT.DAT",install);
  if(!copy_file(source,destination)){printf("Cannot copy %s\n",source);return 1;}
  comspec=getenv("COMSPEC");
  autoexec[0]=(comspec && comspec[1]==':')?(char)toupper(comspec[0]):'C';
  strcpy(autoexec+1,":\\AUTOEXEC.BAT");
  key_spec[0]=0;
  printf("\n");
  update_autoexec=ask_yes("Do you want to update your AUTOEXEC.BAT file?",1);
  if(update_autoexec){
    add_path=ask_yes("Add Launch! to PATH?",1);
    add_shortcut=ask_yes("Enable keyboard shortcut?",1);
    if(add_shortcut){
      puts("\nThe keyboard shortcut is set to CTRL+ALT+.");
      if(ask_yes("Change the shortcut key/s?",0))capture_shortcut(key_spec);
    }
    show_menu=ask_yes("Show menu after startup?",0);
    if(add_path || add_shortcut || show_menu){
      if(!append_autoexec(autoexec,install,add_path,add_shortcut,key_spec,show_menu)){
        printf("Files copied, but %s could not be updated.\n",autoexec);return 1;
      }
      autoexec_changed=1;
    }
  }
  printf("\n- Installed LAUNCH! to %s\n",install);
  printf("- SHORTCUT 2.5 build: %s\n",use_dosbox?"DOSBox":"real/emulated BIOS");
  if(autoexec_changed)printf("- Updated %s with the selected startup options.\n",autoexec);
  else printf("- %s was not changed.\n",autoexec);
  printf("\nScan the C drive now for recognized programs\n");
  printf("and build an initial Launch! menu? [y/N]: ");
  if(fgets(answer,sizeof(answer),stdin)&&toupper(answer[0])=='Y'){
    sprintf(destination,"%s\\AUTOGEN.EXE",install);
    if(spawnl(P_WAIT,destination,"AUTOGEN.EXE",NULL)==-1)
      puts("AutoGen could not be started. Run AUTOGEN manually after installation.");
  }
  if(autoexec_changed)
    puts("\nInstall is complete. Reboot to activate the selected startup options.");
  else puts("\nInstall is complete.");
  puts("Press Ù to exit.");
  getchar();return 0;
}
