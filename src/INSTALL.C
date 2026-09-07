/* Launch! 1.0 installer - Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <io.h>

#define PATH_SIZE 128

static unsigned char copy_buffer[4096];

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
  char destination[PATH_SIZE],autoexec[16],*comspec;int n;
  (void)argc;
  puts("Launch! 1.0 Installation\n");
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
  source_directory(argv[0],source_dir);
  sprintf(source,"%s!.EXE",source_dir);sprintf(destination,"%s\\!.EXE",install);
  if(!copy_file(source,destination)){printf("Cannot copy %s\n",source);return 1;}
  sprintf(source,"%sSHORTCUT.COM",source_dir);sprintf(destination,"%s\\SHORTCUT.COM",install);
  if(!copy_file(source,destination)){printf("Cannot copy %s\n",source);return 1;}
  comspec=getenv("COMSPEC");
  autoexec[0]=(comspec && comspec[1]==':')?(char)toupper(comspec[0]):'C';
  strcpy(autoexec+1,":\\AUTOEXEC.BAT");
  if(!append_autoexec(autoexec,install)){
    printf("Files copied, but %s could not be updated.\n",autoexec);return 1;
  }
  printf("\nInstalled !.EXE and SHORTCUT.COM in %s\n",install);
  printf("Updated %s with PATH and LOADHIGH commands.\n",autoexec);
  puts("\nPlease reboot the computer to complete the installation.");
  puts("Press Enter to exit.");
  getchar();return 0;
}
