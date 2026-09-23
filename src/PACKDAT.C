/* Builds the compressed Launch! 3.65 INSTALL.DAT distribution archive.
   Per-file LZSS compression: 4K history window, 3..18 byte matches.
   Microsoft C/C++ 7.0 / DOS small model. */
#include <stdio.h>
#include <string.h>

#define MAGIC "L361Z1\032"
#define WINDOW 4096
#define WMASK  4095
#define HASHES 4096
#define MAX_MATCH 18
#define MIN_MATCH 3
#define MAX_CHAIN 256

static const char *files[]={
  "!.EXE","!KEY.COM","!KEYDB.COM","!KEY286.COM","!MNUGEN.EXE","AUTOGEN.DAT",
  "PWROFF.BMP","FONT.DAT","!SANS.FNT","PROMPTS.CFG","CAL.ICS","PROFONT.FNT","PROFONTB.FNT","PROFONTI.FNT","!CAL.EXE","!CALC.EXE",
  "!DRAW.EXE","!JOURNAL.EXE","!MKDOWN.EXE","!NOTE.EXE","!STACK.EXE","!SYSINFO.EXE",
  "!TODOS.EXE","!TYPO.EXE","TYPO.LVL","!BOXES.EXE","BOXES.LVL","!FCELL.EXE","!PLUMB.EXE","!POP.EXE",
  "!SNAKE.EXE","!SOL.EXE","!WORDZ.EXE","WORDZ.LVL",0
};

static unsigned char window_buf[WINDOW];
static unsigned short hash_head[HASHES],hash_prev[WINDOW];
static unsigned long slot_pos[WINDOW];
static unsigned char look[MAX_MATCH];
static int look_count;

static void write_u16(FILE *f,unsigned v)
{fputc(v&255,f);fputc((v>>8)&255,f);}

static void write_u32(FILE *f,unsigned long v)
{fputc((int)(v&255),f);fputc((int)((v>>8)&255),f);fputc((int)((v>>16)&255),f);fputc((int)((v>>24)&255),f);}

static unsigned hash3(const unsigned char *p)
{
  return (unsigned)((((unsigned)p[0]*251U)^((unsigned)p[1]*31U)^(unsigned)p[2])&4095U);
}

static void init_lz(void)
{
  int i;memset(hash_head,0,sizeof(hash_head));memset(hash_prev,0,sizeof(hash_prev));
  for(i=0;i<WINDOW;i++)slot_pos[i]=0xFFFFFFFFUL;
}

static void insert_pos(unsigned long pos)
{
  unsigned h,slot;
  if(look_count<3)return;
  h=hash3(look);slot=(unsigned)(pos&WMASK);
  hash_prev[slot]=hash_head[h];
  hash_head[h]=(unsigned short)(slot+1);
  slot_pos[slot]=pos;
}

static int find_match(unsigned long pos,int *best_dist)
{
  unsigned h,node,slot;unsigned long cpos,dist;int best=0,tries=0,j;unsigned char src;
  *best_dist=0;if(look_count<MIN_MATCH)return 0;
  h=hash3(look);node=hash_head[h];
  while(node&&tries++<MAX_CHAIN){
    slot=(unsigned)(node-1);cpos=slot_pos[slot];
    if(cpos==0xFFFFFFFFUL||cpos>=pos){node=hash_prev[slot];continue;}
    dist=pos-cpos;if(dist>WMASK){node=hash_prev[slot];continue;}
    for(j=0;j<look_count&&j<MAX_MATCH;j++){
      if((unsigned long)j<dist)src=window_buf[(unsigned)((cpos+(unsigned long)j)&WMASK)];
      else src=look[j-(int)dist];
      if(src!=look[j])break;
    }
    if(j>best){best=j;*best_dist=(int)dist;if(best==look_count||best==MAX_MATCH)break;}
    node=hash_prev[slot];
  }
  return best;
}

static int fill_initial(FILE *in)
{
  int c;look_count=0;
  while(look_count<MAX_MATCH&&(c=fgetc(in))!=EOF)look[look_count++]=(unsigned char)c;
  return look_count;
}

static void consume_one(FILE *in,unsigned long pos)
{
  int i,c;unsigned slot=(unsigned)(pos&WMASK);
  insert_pos(pos);
  window_buf[slot]=look[0];
  for(i=1;i<look_count;i++)look[i-1]=look[i];
  look_count--;
  c=fgetc(in);if(c!=EOF)look[look_count++]=(unsigned char)c;
}

static int compress_file(FILE *in,FILE *out,unsigned long *usize,unsigned long *csize)
{
  unsigned char group[17],flags;int tokens,glen,mlen,dist,i;unsigned long pos=0,start;
  init_lz();fill_initial(in);start=(unsigned long)ftell(out);
  while(look_count>0){
    flags=0;tokens=0;glen=1;
    while(tokens<8&&look_count>0){
      mlen=find_match(pos,&dist);
      if(mlen>=MIN_MATCH){
        group[glen++]=(unsigned char)(dist&255);
        group[glen++]=(unsigned char)((((unsigned)dist>>8)<<4)|((unsigned)(mlen-MIN_MATCH)&15));
        for(i=0;i<mlen;i++){consume_one(in,pos);pos++;}
      }else{
        flags|=(unsigned char)(1U<<tokens);
        group[glen++]=look[0];consume_one(in,pos);pos++;
      }
      tokens++;
    }
    group[0]=flags;
    if(fwrite(group,1,glen,out)!=(size_t)glen)return 0;
  }
  *usize=pos;*csize=(unsigned long)ftell(out)-start;return !ferror(in)&&!ferror(out);
}

int main(void)
{
  FILE *in,*out;char name[13];unsigned long offsets[40],csizes[40],usizes[40],data_start,total_raw=0,total_cmp=0;
  int i,count=0,ok=1;
  while(files[count])count++;
  out=fopen("INSTALL.DAT","w+b");
  if(!out){puts("PACKDAT: cannot create INSTALL.DAT");return 1;}

  fwrite(MAGIC,1,8,out);write_u16(out,(unsigned)count);
  data_start=10UL+(unsigned long)count*25UL;
  for(i=0;i<count;i++){
    memset(name,0,sizeof(name));strncpy(name,files[i],12);fwrite(name,1,13,out);
    write_u32(out,0);write_u32(out,0);write_u32(out,0);
  }
  if((unsigned long)ftell(out)!=data_start){fclose(out);remove("INSTALL.DAT");puts("PACKDAT: header size error");return 1;}

  for(i=0;i<count&&ok;i++){
    in=fopen(files[i],"rb");
    if(!in){printf("PACKDAT: cannot open %s\n",files[i]);ok=0;break;}
    offsets[i]=(unsigned long)ftell(out);
    if(!compress_file(in,out,&usizes[i],&csizes[i]))ok=0;
    fclose(in);total_raw+=usizes[i];total_cmp+=csizes[i];
    if(ok)printf("Compressed %-12s %7lu -> %7lu\n",files[i],usizes[i],csizes[i]);
  }

  if(ok){
    for(i=0;i<count;i++){
      if(fseek(out,10L+(long)i*25L+13L,SEEK_SET)){ok=0;break;}
      write_u32(out,offsets[i]);write_u32(out,csizes[i]);write_u32(out,usizes[i]);
    }
  }
  if(fclose(out)!=0)ok=0;
  if(!ok){remove("INSTALL.DAT");puts("PACKDAT: archive creation failed");return 1;}
  printf("Built compressed INSTALL.DAT with %d files (%lu -> %lu bytes, %lu%%).\n",
         count,total_raw,total_cmp,total_raw?(100UL*total_cmp)/total_raw:0UL);
  return 0;
}
