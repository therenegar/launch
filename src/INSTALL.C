/* Launch! 1.5 installer - Microsoft C/C++ 7.0, DOS small model. */
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
  char exe[PATH_SIZE],candidate[PATH_SIZE];
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
  char work[PATH_SIZE];int i,start;
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
  FILE *f=fopen(filename,"r");char line[256];
  if(!f)return 0;
  while(fgets(line,sizeof(line),f)){strip_line(line);if(!stricmp(line,wanted)){fclose(f);return 1;}}
  fclose(f);return 0;
}

static int append_autoexec(const char *filename,const char *path)
{
  FILE *f;char path_line[256],load_line[256];long size;
  int last=0,has_path,has_load;
  sprintf(path_line,"SET PATH=%s;%%PATH%%",path);
  sprintf(load_line,"LOADHIGH %s\\SHORTCUT.COM",path);
  has_path=contains_line(filename,path_line);
  has_load=contains_line(filename,load_line);
  if(has_path && has_load)return 1;
  f=fopen(filename,"a+b");if(!f)return 0;
  fseek(f,0L,SEEK_END);size=ftell(f);
  if(size>0){fseek(f,-1L,SEEK_END);last=fgetc(f);fseek(f,0L,SEEK_END);}
  if(size>0 && last!='\n')fputs("\r\n",f);
  if(!has_path)fprintf(f,"%s\r\n",path_line);
  if(!has_load)fprintf(f,"%s\r\n",load_line);
  if(fclose(f)!=0)return 0;
  return 1;
}

int main(int argc,char **argv)
{
  char install[PATH_SIZE],source_dir[PATH_SIZE],source[PATH_SIZE];
  char destination[PATH_SIZE],autoexec[16],*comspec,answer[16];
  int n,dosbox_detected,use_dosbox;
  (void)argc;
  puts("Launch! 1.5 Installation\n");
  printf("Installation directory [C:\\LAUNCH]: ");
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
    printf("\nIt looks like you're running in DOSBox, is this correct? [Y/n]: ");
    if(!fgets(answer,sizeof(answer),stdin))return 1;
    use_dosbox=!answer[0]||answer[0]=='\r'||answer[0]=='\n'||toupper(answer[0])=='Y';
  }else{
    printf("\nAre you installing in DOSBox? [y/N]: ");
    if(!fgets(answer,sizeof(answer),stdin))return 1;
    use_dosbox=toupper(answer[0])=='Y';
  }
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
  comspec=getenv("COMSPEC");
  autoexec[0]=(comspec && comspec[1]==':')?(char)toupper(comspec[0]):'C';
  strcpy(autoexec+1,":\\AUTOEXEC.BAT");
  if(!append_autoexec(autoexec,install)){
    printf("Files copied, but %s could not be updated.\n",autoexec);return 1;
  }
  printf("\nInstalled Launch! in %s\n",install);
  printf("Shortcut version: %s\n",use_dosbox?"DOSBox":"Real/emulated PC BIOS");
  printf("Updated %s with PATH and LOADHIGH commands.\n\n\n",autoexec);
  printf("\nDo you want to scan drive C for recognized programs\n");
  printf("and build an initial Launch! menu? [y/N]: ");
  if(fgets(answer,sizeof(answer),stdin)&&toupper(answer[0])=='Y'){
    sprintf(destination,"%s\\AUTOGEN.EXE",install);
    if(spawnl(P_WAIT,destination,"AUTOGEN.EXE",NULL)==-1)
      puts("AutoGen could not be started. Run AUTOGEN manually after installation.");
  }
  puts("\nPlease reboot to activate the keyboard shortcut.");
  puts("Press Ù to exit.");
  getchar();return 0;
}
