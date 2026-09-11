/* AutoGen 2.2 - automatic Launch! menu generator for DOS.
 * Microsoft C/C++ 7.0, small model.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <io.h>

#define PATH_SIZE 128
#define MAX_FOUND 70
#define MAX_CATS 18
#define MAX_MATCHES 12
#define MAX_PRIMARY 760

typedef struct {
  char file[13],support1[13],support2[13];
  char title[19],category[17],company[34];
} DBREC;

typedef struct {
  char title[19],category[17],command[PATH_SIZE];
  unsigned char press_enter,change_directory;
} FOUND;

static FOUND found[MAX_FOUND];
static DBREC matches[MAX_MATCHES];
static char primary_files[MAX_PRIMARY][13];
static int primary_count;
static int found_count,dos_item_count,known_seen,unknown_seen,ambiguous_seen;
static char base_dir[PATH_SIZE],database_file[PATH_SIZE];
static char menu_file[PATH_SIZE],backup_file[PATH_SIZE],temp_file[PATH_SIZE];
static char copy_buffer[2048];

static void strip_line(char *s)
{
  int n=(int)strlen(s);
  while(n && (s[n-1]=='\r'||s[n-1]=='\n'||s[n-1]==' '||s[n-1]=='\t'))s[--n]=0;
}

static void upper_string(char *s)
{
  while(*s){*s=(char)toupper((unsigned char)*s);s++;}
}

static void menu_title(const char *source,char *destination)
{
  const char *tail;
  if(strlen(source)<=18){strcpy(destination,source);return;}
  if(!strnicmp(source,"Microsoft Windows ",18)){
    sprintf(destination,"MS Windows %.7s",source+18);return;
  }
  if(!strnicmp(source,"Dac Easy Accounting ",20)){
    sprintf(destination,"Dac Easy Acct %.4s",source+20);return;
  }
  if(!stricmp(source,"Logical Connection Plus")){
    strcpy(destination,"Logical Conn Plus");return;
  }
  if(!strnicmp(source,"Magnatype- ",11)){
    tail=strrchr(source,' ');if(tail&&tail[1])sprintf(destination,"Magnatype %.8s",tail+1);
    else {strncpy(destination,source,18);destination[18]=0;}return;
  }
  strncpy(destination,source,18);destination[18]=0;
}

static int exists(const char *name)
{
  FILE *f=fopen(name,"rb");if(!f)return 0;fclose(f);return 1;
}

static int command_available(const char *command)
{
  static const char *extensions[]={".COM",".EXE",".BAT"};
  char candidate[PATH_SIZE];const char *path,*end;int i,n;
  for(i=0;i<3;i++){sprintf(candidate,"%s%s",command,extensions[i]);if(exists(candidate))return 1;}
  path=getenv("PATH");
  while(path&&*path){
    end=strchr(path,';');n=end?(int)(end-path):(int)strlen(path);
    if(n>0&&n<PATH_SIZE-(int)strlen(command)-6){
      strncpy(candidate,path,n);candidate[n]=0;
      if(candidate[n-1]!='\\'&&candidate[n-1]!='/')strcat(candidate,"\\");
      for(i=0;i<3;i++){
        candidate[n=(int)strlen(candidate)]=0;
        strcat(candidate,command);strcat(candidate,extensions[i]);
        if(exists(candidate))return 1;
        candidate[n]=0;
      }
    }
    if(!end)break;
    path=end+1;
  }
  return 0;
}

static void print_program_name(const char *name)
{
  union REGS inregs,outregs;unsigned char page,row,column,columns,attribute;
  const char *p;
  if(!_isatty(_fileno(stdout))){fputs(name,stdout);return;}
  memset(&inregs,0,sizeof(inregs));inregs.h.ah=0x0F;int86(0x10,&inregs,&outregs);
  columns=outregs.h.ah;page=outregs.h.bh;
  memset(&inregs,0,sizeof(inregs));inregs.h.ah=0x03;inregs.h.bh=page;
  int86(0x10,&inregs,&outregs);row=outregs.h.dh;column=outregs.h.dl;
  memset(&inregs,0,sizeof(inregs));inregs.h.ah=0x08;inregs.h.bh=page;
  int86(0x10,&inregs,&outregs);attribute=(unsigned char)((outregs.h.ah&0xF0)|0x0A);
  for(p=name;*p;p++){
    memset(&inregs,0,sizeof(inregs));inregs.h.ah=0x09;inregs.h.al=(unsigned char)*p;
    inregs.h.bh=page;inregs.h.bl=attribute;inregs.x.cx=1;int86(0x10,&inregs,&outregs);
    column++;if(column>=columns){column=0;row++;}
    memset(&inregs,0,sizeof(inregs));inregs.h.ah=0x02;inregs.h.bh=page;
    inregs.h.dh=row;inregs.h.dl=column;int86(0x10,&inregs,&outregs);
  }
}

static void program_directory(const char *program,char *directory)
{
  char exe[PATH_SIZE],candidate[PATH_SIZE];const char *path,*end,*slash,*other;int n;
  strncpy(exe,program,PATH_SIZE-1);exe[PATH_SIZE-1]=0;
  if(!strchr(exe,'\\')&&!strchr(exe,'/')&&!strchr(exe,':')){
    path=getenv("PATH");
    while(path&&*path){
      end=strchr(path,';');n=end?(int)(end-path):(int)strlen(path);
      if(n>0&&n<PATH_SIZE-(int)strlen(exe)-2){
        strncpy(candidate,path,n);candidate[n]=0;
        if(candidate[n-1]!='\\'&&candidate[n-1]!='/')strcat(candidate,"\\");
        strcat(candidate,exe);if(exists(candidate)){strcpy(exe,candidate);break;}
      }
      if(!end)break;
      path=end+1;
    }
  }
  slash=strrchr(exe,'\\');other=strrchr(exe,'/');
  if(!slash||(other&&other>slash))slash=other;
  if(slash)n=(int)(slash-exe)+1;else if(exe[0]&&exe[1]==':')n=2;else n=0;
  strncpy(directory,exe,n);directory[n]=0;
}

static int copy_file(const char *source,const char *destination)
{
  FILE *in,*out;size_t n;int ok=1;
  in=fopen(source,"rb");if(!in)return 0;
  out=fopen(destination,"wb");if(!out){fclose(in);return 0;}
  while((n=fread(copy_buffer,1,sizeof(copy_buffer),in))!=0)
    if(fwrite(copy_buffer,1,n,out)!=n){ok=0;break;}
  if(ferror(in))ok=0;
  if(fclose(out)!=0)ok=0;
  fclose(in);return ok;
}

static int parse_database_line(char *line,DBREC *record)
{
  char *part[6],*p;int i;
  strip_line(line);p=line;while(*p==' '||*p=='\t')p++;
  if(!*p||*p==';'||*p=='#')return 0;
  part[0]=p;
  for(i=1;i<6;i++){p=strchr(p,'|');if(!p)return -1;*p++=0;part[i]=p;}
  if(!*part[0]||strlen(part[0])>12||strlen(part[1])>12||strlen(part[2])>12||
     !*part[3]||!*part[4]||strlen(part[4])>16)return -1;
  upper_string(part[0]);upper_string(part[1]);upper_string(part[2]);
  strcpy(record->file,part[0]);strcpy(record->support1,part[1]);
  strcpy(record->support2,part[2]);
  menu_title(part[3],record->title);
  strcpy(record->category,part[4]);
  strncpy(record->company,part[5],33);record->company[33]=0;
  return 1;
}

static int database_is_valid(void)
{
  FILE *f;char line[160];DBREC record;int parsed,count=0;
  f=fopen(database_file,"r");if(!f)return 0;
  while(fgets(line,sizeof(line),f)){
    parsed=parse_database_line(line,&record);
    if(parsed<0){fclose(f);return 0;}
    if(parsed>0){
      count++;
      if(primary_count<MAX_PRIMARY)strcpy(primary_files[primary_count++],record.file);
    }
  }
  fclose(f);return count>0&&count<=MAX_PRIMARY;
}

static int known_primary(const char *name)
{
  int i;for(i=0;i<primary_count;i++)if(!strcmp(primary_files[i],name))return 1;
  return 0;
}

static int already_found(const char *title)
{
  int i;for(i=0;i<found_count;i++)if(!stricmp(found[i].title,title))return 1;return 0;
}

static int is_dos_program(const char *path);

static int choose_match(const char *path,int count)
{
  char line[16];int i,n;
  if(count==1)return 0;
  ambiguous_seen++;
  printf("\nAmbiguous program: %s\n",path);
  for(i=0;i<count;i++)printf("  %d. %-18s  %-16s  %s\n",i+1,matches[i].title,
    matches[i].category,matches[i].company);
  printf("  0. Skip\nChoose [0]: ");
  if(!fgets(line,sizeof(line),stdin))return -1;
  n=atoi(line);if(n<1||n>count)return -1;return n-1;
}

static int marker_exists(const char *directory,const char *name)
{
  char path[PATH_SIZE];
  if(!*name)return 1;
  if(strlen(directory)+strlen(name)+2>=PATH_SIZE)return 0;
  strcpy(path,directory);if(path[strlen(path)-1]!='\\')strcat(path,"\\");
  strcat(path,name);return exists(path);
}

static void consider_file(const char *directory,const char *filename)
{
  FILE *f;char upper[13],full[PATH_SIZE],line[160];DBREC record;
  int parsed,count=0,choice;
  if(strlen(directory)+strlen(filename)+1>=PATH_SIZE)return;
  strcpy(full,directory);if(full[strlen(full)-1]!='\\')strcat(full,"\\");strcat(full,filename);
  strncpy(upper,filename,12);upper[12]=0;upper_string(upper);
  if(!known_primary(upper)){unknown_seen++;return;}
  if(!is_dos_program(full))return;
  f=fopen(database_file,"r");if(!f)return;
  while(fgets(line,sizeof(line),f)){
    parsed=parse_database_line(line,&record);
    if(parsed>0&&!strcmp(record.file,upper)&&
       marker_exists(directory,record.support1)&&marker_exists(directory,record.support2)){
      if(count<MAX_MATCHES)matches[count++]=record;
    }
  }
  fclose(f);
  if(count==0){unknown_seen++;return;}
  known_seen++;
  choice=choose_match(full,count);if(choice<0)return;
  if(already_found(matches[choice].title)||found_count>=MAX_FOUND)return;
  strcpy(found[found_count].title,matches[choice].title);
  strcpy(found[found_count].category,matches[choice].category);
  strcpy(found[found_count].command,full);
  found[found_count].press_enter=1;found[found_count].change_directory=1;
  found_count++;
  printf("Found: ");print_program_name(matches[choice].title);
  printf("%*s %s\n",18-(int)strlen(matches[choice].title),"",full);
}

static int executable_name(const char *name)
{
  const char *p=strrchr(name,'.');
  if(!p)return 1;
  return !stricmp(p,".EXE")||!stricmp(p,".COM")||!stricmp(p,".BAT")||!stricmp(p,".BTM");
}

static int is_dos_program(const char *path)
{
  FILE *f;unsigned char h[64],sig[2];long off;
  const char *ext=strrchr(path,'.');
  if(!ext||stricmp(ext,".EXE"))return 1;
  f=fopen(path,"rb");if(!f)return 0;
  if(fread(h,1,sizeof(h),f)<sizeof(h)||h[0]!='M'||h[1]!='Z'){fclose(f);return 1;}
  off=(long)h[0x3c]|((long)h[0x3d]<<8)|((long)h[0x3e]<<16)|((long)h[0x3f]<<24);
  if(off<64L||fseek(f,off,SEEK_SET)!=0||fread(sig,1,2,f)!=2){fclose(f);return 1;}
  fclose(f);
  if((sig[0]=='N'&&sig[1]=='E')||(sig[0]=='P'&&sig[1]=='E')||
     (sig[0]=='L'&&(sig[1]=='E'||sig[1]=='X')))return 0;
  return 1;
}

/* Directory paths are held in a disk-backed FIFO to keep stack and data use
 * fixed even on large drives. Each record is PATH_SIZE bytes. */
static int scan_drives(const char *drives)
{
  FILE *q;struct find_t info;char queue_name[PATH_SIZE],dir[PATH_SIZE],mask[PATH_SIZE],child[PATH_SIZE];
  long read_at=0;int i,rc;
  sprintf(queue_name,"%sAUTODIR.$$$",base_dir);q=fopen(queue_name,"w+b");if(!q)return 0;
  for(i=0;drives[i];i++)if(isalpha((unsigned char)drives[i])){
    memset(dir,0,sizeof(dir));dir[0]=(char)toupper(drives[i]);dir[1]=':';dir[2]='\\';
    fwrite(dir,1,sizeof(dir),q);
  }
  fflush(q);
  for(;;){
    fseek(q,read_at,SEEK_SET);if(fread(dir,1,sizeof(dir),q)!=sizeof(dir))break;
    read_at+=sizeof(dir);printf("Scanning %s\r",dir);fflush(stdout);
    if(strlen(dir)+4>=PATH_SIZE)continue;
    strcpy(mask,dir);if(mask[strlen(mask)-1]!='\\')strcat(mask,"\\");strcat(mask,"*.*");
    rc=_dos_findfirst(mask,_A_NORMAL|_A_RDONLY|_A_HIDDEN|_A_SYSTEM|_A_SUBDIR,&info);
    while(!rc){
      if(info.attrib&_A_SUBDIR){
        if(strcmp(info.name,".")&&strcmp(info.name,"..")&&strlen(dir)+strlen(info.name)+2<PATH_SIZE){
          strcpy(child,dir);if(child[strlen(child)-1]!='\\')strcat(child,"\\");strcat(child,info.name);
          memset(child+strlen(child),0,sizeof(child)-strlen(child));
          fseek(q,0L,SEEK_END);fwrite(child,1,sizeof(child),q);fflush(q);
        }
      }else if(executable_name(info.name))consider_file(dir,info.name);
      rc=_dos_findnext(&info);
    }
  }
  fclose(q);remove(queue_name);puts("\nScan complete.");return 1;
}

static int category_index(char cats[MAX_CATS][17],int *count,const char *name)
{
  int i;for(i=0;i<*count;i++)if(!stricmp(cats[i],name))return i;
  if(*count>=MAX_CATS)return -1;
  strcpy(cats[*count],name);return (*count)++;
}

static void add_dos_item(const char *title,const char *category,const char *command,int enter)
{
  if(found_count>=MAX_FOUND||already_found(title))return;
  strncpy(found[found_count].title,title,18);found[found_count].title[18]=0;
  strcpy(found[found_count].category,category);
  strcpy(found[found_count].command,command);
  found[found_count].press_enter=(unsigned char)enter;
  found[found_count].change_directory=0;
  found_count++;dos_item_count++;
}

static void add_available_dos_item(const char *title,const char *category,
  const char *command,int enter)
{
  if(command_available(command))add_dos_item(title,category,command,enter);
}

static int contains_text(const char *text,const char *wanted)
{
  char copy[PATH_SIZE];if(!text)return 0;
  strncpy(copy,text,PATH_SIZE-1);copy[PATH_SIZE-1]=0;upper_string(copy);
  return strstr(copy,wanted)!=NULL;
}

static void add_dos_utilities(void)
{
  union REGS inregs,outregs;char category[17],*os,*comspec,*ver;
  int major,minor,is_dr=0,is_free=0,is_pc=0;
  memset(&inregs,0,sizeof(inregs));inregs.h.ah=0x30;inregs.h.al=1;
  intdos(&inregs,&outregs);major=outregs.h.al;minor=outregs.h.ah;
  is_pc=(outregs.h.bh==0);
  if(major>=5){
    memset(&inregs,0,sizeof(inregs));inregs.x.ax=0x3306;intdos(&inregs,&outregs);
    if(outregs.h.bl>=5&&outregs.h.bl<100){major=outregs.h.bl;minor=outregs.h.bh;}
  }
  os=getenv("OS");comspec=getenv("COMSPEC");
  is_free=contains_text(os,"FREEDOS")||contains_text(comspec,"FREECOM")||getenv("FREEDOS")!=NULL;
  memset(&inregs,0,sizeof(inregs));inregs.x.ax=0x4452;intdos(&inregs,&outregs);
  if(!outregs.x.cflag)is_dr=1;
  ver=getenv("VER");
  if(is_free)strcpy(category,"FreeDOS");
  else if(is_dr&&ver&&*ver){strncpy(category,ver,16);category[16]=0;}
  else if(is_dr)sprintf(category,"DR DOS %d.%d",major,minor);
  else if(is_pc)sprintf(category,"PC DOS %d.%d",major,minor);
  else sprintf(category,"MS-DOS %d.%d",major,minor);
  add_dos_item("Check Disk",category,"CHKDSK",1);
  add_dos_item("Format Disk",category,"FORMAT",0);
  add_dos_item("Partition Disk",category,"FDISK",1);
  add_dos_item("Sys Install",category,"SYS",0);
  if(major>=4||is_free)add_dos_item("Memory Info",category,"MEM",1);
  if(major>=5||is_free){
    add_dos_item(is_pc?"E Editor":"Text Editor",category,is_pc?"E":"EDIT",1);
    add_dos_item("Help",category,"HELP",1);
  }
  if(major>=6||is_free){
    add_dos_item("Defragment Disk",category,"DEFRAG",1);
    add_dos_item("Undelete Files",category,"UNDELETE",1);
  }
  if((major>6||(major==6&&minor>=20))||is_free||is_pc)
    add_dos_item("Scan Disk",category,"SCANDISK",1);
  add_available_dos_item("File Attributes",category,"ATTRIB",0);
  if(major>=6||is_free)add_available_dos_item("Delete Tree",category,"DELTREE",0);
  add_available_dos_item("Find File",category,"FIND",0);
  if(major>3||(major==3&&minor>=20)||is_free)
    add_available_dos_item("XCopy",category,"XCOPY",0);
  if(major>=4||is_free)add_available_dos_item("DOS Shell",category,"DOSSHELL",1);
  add_available_dos_item("Sort Input",category,"SORT",0);
}

static int found_compare(const void *aa,const void *bb)
{
  const FOUND *a=(const FOUND *)aa,*b=(const FOUND *)bb;int n=stricmp(a->category,b->category);
  return n?n:stricmp(a->title,b->title);
}

static void write_category(FILE *f,const char *cat,int start,int count)
{
  char section[80];int level=0,take,i,remain=count,pos=start;
  strcpy(section,"Launcher\\");strcat(section,cat);
  while(remain>0&&level<3){
    fprintf(f,"\n[%s]\n",section);take=remain>20?19:remain;
    for(i=0;i<take;i++)fprintf(f,"ITEM=%s|%s|%d|%d|0\n",found[pos+i].title,found[pos+i].command,
      found[pos+i].press_enter,found[pos+i].change_directory);
    pos+=take;remain-=take;
    if(remain){fprintf(f,"FOLDER=More\n");strcat(section,"\\More");}
    level++;
  }
  if(remain>0){
    fprintf(f,"\n[%s]\n",section);
    for(i=0;i<remain&&i<20;i++)fprintf(f,"ITEM=%s|%s|%d|%d|0\n",found[pos+i].title,found[pos+i].command,
      found[pos+i].press_enter,found[pos+i].change_directory);
  }
}

static int write_menu(void)
{
  FILE *f;char cats[MAX_CATS][17];int cat_count=0,i,j,start,count,had_menu;
  qsort(found,found_count,sizeof(found[0]),found_compare);
  for(i=0;i<found_count;i++)category_index(cats,&cat_count,found[i].category);
  f=fopen(temp_file,"w");if(!f)return 0;
  fputs("; Launch! 2.2 menu generated by AutoGen\n\n[Launcher]\n",f);
  for(i=0;i<cat_count;i++)fprintf(f,"FOLDER=%s\n",cats[i]);
  for(i=0;i<cat_count;i++){
    start=-1;count=0;
    for(j=0;j<found_count;j++)if(!stricmp(found[j].category,cats[i])){if(start<0)start=j;count++;}
    if(start>=0)write_category(f,cats[i],start,count);
  }
  if(fclose(f)!=0){remove(temp_file);return 0;}
  had_menu=exists(menu_file);
  if(had_menu){
    remove(backup_file);if(!copy_file(menu_file,backup_file)){remove(temp_file);return 0;}
    remove(menu_file);
  }
  if(rename(temp_file,menu_file)!=0){
    if(exists(backup_file))copy_file(backup_file,menu_file);
    return 0;
  }
  if(!had_menu){
    remove(backup_file);
    if(!copy_file(menu_file,backup_file))return 0;
  }
  return 1;
}

static void usage(void)
{
  puts("AutoGen 2.2 - an automatic menu generator for Launch!\n");
  puts("Usage: AUTOGEN [/LOOKIN=drives] [/?]");
  puts("\nDefault: /LOOKIN=C");
  puts("Examples: AUTOGEN /LOOKIN=D");
  puts("          AUTOGEN /LOOKIN=C,D,E\n");
  puts("Also adds useful commands for the detected DOS family and version.");
}

static int parse_drives(int argc,char **argv,char *drives)
{
  int i,j,n=0;strcpy(drives,"C");
  for(i=1;i<argc;i++){
    if(!stricmp(argv[i],"/?")||!stricmp(argv[i],"-?")){usage();return 0;}
    if(!strnicmp(argv[i],"/LOOKIN=",8)||!strnicmp(argv[i],"-LOOKIN=",8)){
      n=0;for(j=8;argv[i][j]&&n<25;j++)if(isalpha((unsigned char)argv[i][j]))drives[n++]=(char)toupper(argv[i][j]);
      drives[n]=0;if(!n)return -1;
    }else return -1;
  }
  return 1;
}

int main(int argc,char **argv)
{
  char drives[26],answer[16];int parsed;
  puts("AutoGen 2.2 - Launch! Program Scanner\n");
  parsed=parse_drives(argc,argv,drives);if(parsed==0)return 0;if(parsed<0){puts("Invalid option. Use AUTOGEN /?");return 1;}
  program_directory(argv[0],base_dir);
  if(strlen(base_dir)>PATH_SIZE-13){puts("AutoGen's installation path is too long.");return 1;}
  sprintf(database_file,"%sAUTOGEN.DAT",base_dir);sprintf(menu_file,"%sLAUNCH.MNU",base_dir);
  sprintf(backup_file,"%sLAUNCH.BAK",base_dir);sprintf(temp_file,"%sLAUNCH.$$$",base_dir);
  if(!database_is_valid()){printf("Cannot read a valid program database: %s\n",database_file);return 1;}
  if(exists(menu_file)){
    printf("LAUNCH.MNU already exists. Replace it with a generated menu? [y/N]: ");
    if(!fgets(answer,sizeof(answer),stdin)||toupper(answer[0])!='Y'){puts("Existing menu retained.");return 0;}
  }
  printf("Scanning drive%s %s for known DOS programs.\n",strlen(drives)>1?"s":"",drives);
  puts("This may take a few minutes. Press Ctrl+C to cancel.\n");
  add_dos_utilities();
  if(!scan_drives(drives)){puts("Unable to create the scan work file.");return 1;}
  if(!write_menu()){puts("Could not safely write LAUNCH.MNU.");return 1;}
  printf("\nCreated LAUNCH.MNU with %d menu item%s.\n",found_count,found_count==1?"":"s");
  printf("DOS utilities added: %d. Recognized applications added: %d.\n",
    dos_item_count,found_count-dos_item_count);
  printf("Known executable matches: %d; ambiguous files reviewed: %d.\n",known_seen,ambiguous_seen);
  if(found_count==MAX_FOUND)puts("The 70-item Launch! tree limit was reached; additional matches were omitted.");
  if(exists(backup_file))puts("The previous menu was preserved as LAUNCH.BAK.");
  return 0;
}
