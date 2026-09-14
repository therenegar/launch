/* Builds the uncompressed Launch! 3.11 INSTALL.DAT distribution archive. */
#include <stdio.h>
#include <string.h>

#define MAGIC "L30DAT1\032"

static const char *files[]={
  "!.EXE","SHORTCUT.COM","SHORTCDB.COM","AUTOGEN.EXE","AUTOGEN.DAT",
  "PWROFF.BMP","FONT.DAT","LAUNCH.MNU","!CAL.EXE","!CALC.EXE",
  "!DRAW.EXE","!NOTE.EXE","!STACK.EXE","!SYSINFO.EXE","!BOXES.EXE",0
};

static unsigned char buffer[4096];

static void write_u16(FILE *f,unsigned v)
{fputc(v&255,f);fputc((v>>8)&255,f);}

static void write_u32(FILE *f,unsigned long v)
{fputc((int)(v&255),f);fputc((int)((v>>8)&255),f);fputc((int)((v>>16)&255),f);fputc((int)((v>>24)&255),f);}

int main(void)
{
  FILE *in,*out;char name[13];unsigned long sizes[32],offset;size_t n;
  int i,count=0,ok=1;
  while(files[count])count++;
  for(i=0;i<count;i++){
    in=fopen(files[i],"rb");
    if(!in){printf("PACKDAT: cannot open %s\n",files[i]);return 1;}
    fseek(in,0L,SEEK_END);sizes[i]=(unsigned long)ftell(in);fclose(in);
  }
  out=fopen("INSTALL.DAT","wb");
  if(!out){puts("PACKDAT: cannot create INSTALL.DAT");return 1;}
  fwrite(MAGIC,1,8,out);write_u16(out,(unsigned)count);
  offset=10UL+(unsigned long)count*21UL;
  for(i=0;i<count;i++){
    memset(name,0,sizeof(name));strncpy(name,files[i],12);fwrite(name,1,13,out);
    write_u32(out,offset);write_u32(out,sizes[i]);offset+=sizes[i];
  }
  for(i=0;i<count&&ok;i++){
    in=fopen(files[i],"rb");if(!in){ok=0;break;}
    while((n=fread(buffer,1,sizeof(buffer),in))!=0)
      if(fwrite(buffer,1,n,out)!=n){ok=0;break;}
    if(ferror(in))ok=0;
    fclose(in);
  }
  if(fclose(out)!=0)ok=0;
  if(!ok){remove("INSTALL.DAT");puts("PACKDAT: archive creation failed");return 1;}
  printf("Built INSTALL.DAT with %d files (%lu bytes).\n",count,offset);
  return 0;
}
