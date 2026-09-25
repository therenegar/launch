/* Launch! 3.71 build-time source generator.
   Keeps the large Release 3.71 canonical sources while generating Release 3.71
   build intermediates for core/installer changes and intelligent editor word
   wrapping.  Generated files are build intermediates, not distribution files.
   Microsoft C/C++ 7.0, DOS small model. */
#include <stdio.h>
#include <string.h>

#define GEN_LINE 8192
static char gen_line[GEN_LINE],gen_temp[GEN_LINE];

static int replace_once(char *line,int cap,const char *from,const char *to)
{
  char *temp=gen_temp,*hit;int before;
  hit=strstr(line,from);if(!hit)return 0;
  before=(int)(hit-line);
  if(before+(int)strlen(to)+(int)strlen(hit+strlen(from))+1>=cap)return -1;
  memcpy(temp,line,before);temp[before]=0;strcat(temp,to);strcat(temp,hit+strlen(from));
  strcpy(line,temp);return 1;
}

static void replace_all(char *line,int cap,const char *from,const char *to)
{
  int r;do{r=replace_once(line,cap,from,to);}while(r>0);
}

static void emit_startup_batch_helpers(FILE *out)
{
  fputs("static int startup_contains_ci(const char *s,const char *n)\n{\n",out);
  fputs("  int i;if(!s||!n||!*n)return 0;for(;*s;s++){for(i=0;n[i]&&s[i]&&toupper((unsigned char)s[i])==toupper((unsigned char)n[i]);i++);if(!n[i])return 1;}return 0;\n}\n",out);
  fputs("static int startup_is_freedos(void)\n{\n",out);
  fputs("  union REGS inr,outr;char *os,*comspec;memset(&inr,0,sizeof(inr));inr.h.ah=0x30;inr.h.al=0;intdos(&inr,&outr);\n",out);
  fputs("  if(outr.h.bh==0xFD)return 1;os=getenv(\"OS\");comspec=getenv(\"COMSPEC\");\n",out);
  fputs("  return startup_contains_ci(os,\"FREEDOS\")||startup_contains_ci(comspec,\"FREECOM\")||getenv(\"FREEDOS\")!=0;\n}\n",out);
  fputs("static const char *startup_batch_suffix(void)\n{\n",out);
  fputs("  return startup_is_freedos()?\":\\\\FDAUTO.BAT\":\":\\\\AUTOEXEC.BAT\";\n}\n",out);
}

static void emit_tooltip_ui(FILE *out)
{
  fputs("#define CORE_TOOLTIP_MAX 42\n",out);
  fputs("static int core_tt_active=0,core_tt_x=0,core_tt_y=0,core_tt_w=0;\n",out);
  fputs("static int core_tt_owner_x=-1,core_tt_owner_y=-1;\n",out);
  fputs("static unsigned short core_tt_line[CORE_TOOLTIP_MAX],core_tt_arrow;\n",out);
  fputs("static char core_tt_text[41];\n",out);
  fputs("static void core_tooltip_restore(void)\n{\n",out);
  fputs("  int i;if(!core_tt_active)return;\n",out);
  fputs("  for(i=0;i<core_tt_w;i++)video[core_tt_y*screen_cols+core_tt_x+i]=core_tt_line[i];\n",out);
  fputs("  video[(core_tt_y+1)*screen_cols+core_tt_x+2]=core_tt_arrow;\n",out);
  fputs("  core_tt_active=0;core_tt_owner_x=core_tt_owner_y=-1;\n}\n",out);
  fputs("static void core_tooltip_trim(const char *src,char *dst)\n{\n",out);
  fputs("  const char *a=src,*b;int n;while(*a==' ')a++;b=a+strlen(a);\n",out);
  fputs("  while(b>a&&b[-1]==' ')b--;n=(int)(b-a);if(n>40)n=40;\n",out);
  fputs("  memcpy(dst,a,n);dst[n]=0;if(!strcmp(dst,\"?\"))strcpy(dst,\"Help\");\n}\n",out);
  fputs("static void core_tooltip_show(int bx,int by,const char *label)\n{\n",out);
  fputs("  char clean[41];int i,w,ty=by-2,a=ATTR(appearance.controls_fg,appearance.controls_bg),aa;\n",out);
  fputs("  if(!appearance.show_tooltips){core_tooltip_restore();return;}\n",out);
  fputs("  core_tooltip_trim(label,clean);if(!*clean||ty<0)return;\n",out);
  fputs("  w=(int)strlen(clean)+2;if(w>CORE_TOOLTIP_MAX)w=CORE_TOOLTIP_MAX;\n",out);
  fputs("  if(bx+w>screen_cols)w=screen_cols-bx;if(w<3)return;\n",out);
  fputs("  if(core_tt_active&&core_tt_owner_x==bx&&core_tt_owner_y==by&&!strcmp(core_tt_text,clean))return;\n",out);
  fputs("  core_tooltip_restore();core_tt_x=bx;core_tt_y=ty;core_tt_w=w;\n",out);
  fputs("  core_tt_owner_x=bx;core_tt_owner_y=by;strncpy(core_tt_text,clean,40);core_tt_text[40]=0;\n",out);
  fputs("  for(i=0;i<w;i++)core_tt_line[i]=video[ty*screen_cols+bx+i];\n",out);
  fputs("  core_tt_arrow=video[(ty+1)*screen_cols+bx+2];\n",out);
  fputs("  for(i=0;i<w;i++)cell(bx+i,ty,' ',a);textout(bx+1,ty,core_tt_text,a,w-2);\n",out);
  fputs("  aa=(int)((core_tt_arrow>>8)&0xF0);cell(bx+2,ty+1,CORE_TOOLTIP_GLYPH,aa);core_tt_active=1;\n}\n",out);
  fputs("static int core_tooltip_icon(const char *label)\n{\n",out);
  fputs("  int a=0,b=0;return button_icon(label,&a,&b)==1;\n}\n",out);
}

static void emit_tooltip_glyph(FILE *out)
{
  fputs("static unsigned char far tooltip_old_glyph[32];static int tooltip_glyph_saved=0;\n",out);
  fputs("static const unsigned char tooltip_glyph16[32]={\n",out);
  fputs("  0xFF,0xFF,0x7E,0x7E,0x3C,0x3C,0x18,0x18,0,0,0,0,0,0,0,0,\n",out);
  fputs("  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};\n",out);
  fputs("static const unsigned char tooltip_glyph14[32]={\n",out);
  fputs("  0xFF,0x7E,0x7E,0x3C,0x3C,0x18,0x18,0,0,0,0,0,0,0,0,0,\n",out);
  fputs("  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};\n",out);
  fputs("static void tooltip_glyph_install(void)\n{\n",out);
  fputs("  FONT_REGS old;unsigned char far *font;const unsigned char *glyph;int i;\n",out);
  fputs("  unsigned char far *h=(unsigned char far *)MAKE_FP(0x40,0x85);\n",out);
  fputs("  glyph=(*h==14)?tooltip_glyph14:tooltip_glyph16;\n",out);
  fputs("  font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,CORE_TOOLTIP_GLYPH*32);\n",out);
  fputs("  if(!tooltip_glyph_saved){for(i=0;i<32;i++)tooltip_old_glyph[i]=font[i];tooltip_glyph_saved=1;}\n",out);
  fputs("  for(i=0;i<32;i++)font[i]=glyph[i];font_plane_close(&old);\n}\n",out);
  fputs("static void tooltip_glyph_restore(void)\n{\n",out);
  fputs("  FONT_REGS old;unsigned char far *font;int i;\n",out);
  fputs("  if(!tooltip_glyph_saved)return;\n",out);
  fputs("  font_plane_open(&old);font=(unsigned char far *)MAKE_FP(0xA000,CORE_TOOLTIP_GLYPH*32);\n",out);
  fputs("  for(i=0;i<32;i++)font[i]=tooltip_old_glyph[i];font_plane_close(&old);tooltip_glyph_saved=0;\n}\n",out);
}

static int copy_generated_source(const char *src,const char *dst);

static int write_launch36(void)
{
  /* 3.71 keeps the final 3.7 core source authoritative.  The historical
     transform pass is no longer needed and could reapply already-merged UI
     patches, so regeneration copies it verbatim. */
  if(!copy_generated_source("LAUNCH.C","COREBLD.C")){puts("GENBUILD: failed copying LAUNCH.C to COREBLD.C");return 0;}
  return 1;
}

static int write_install36(void)
{
  FILE *in=fopen("INSTALL.C","rt"),*out;char *line=gen_line;int in_status=0,startup_hits=0,icon_lead=0,icon_tail=0;
  if(!in){puts("GENBUILD: cannot open INSTALL.C");return 0;}
  out=fopen("INSTBLD.C","wt");if(!out){fclose(in);puts("GENBUILD: cannot create INSTBLD.C");return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"strcpy(autoexec+1,\":\\\\AUTOEXEC.BAT\")","strcpy(autoexec+1,startup_batch_suffix())")>0)startup_hits++;
    replace_all(line,GEN_LINE,"AUTOEXEC.BAT file","startup batch file");
    replace_all(line,GEN_LINE,"AUTOEXEC.BAT","startup batch file");
    if(strstr(line,"/* Launch! 3.5 installer")){
      fputs("/* Launch! 3.71 installer - Microsoft C/C++ 7.0, DOS small model. */\n",out);continue;
    }
    if(strstr(line,"#define PATH_SIZE 128")){
      emit_startup_batch_helpers(out);fputs(line,out);continue;
    }
    if(strstr(line,"static void status_icon(int indent,int colour,int symbol)"))in_status=1;
    if(in_status&&strstr(line,"for(i=0;i<indent;i++)putchar(' ');")){
      fputs("  for(i=0;i<indent;i++)putchar(' ');colour_text(\" \",7);\n",out);icon_lead=1;continue;
    }
    if(in_status&&strstr(line,"  putchar(' ');")){
      fputs("  colour_text(\" \",7);\n",out);icon_tail=1;continue;
    }

    if(strstr(line,"puts(\"Launch! 3.5 Installation\");")){
      fputs("  puts(\"Launch! 3.65 Installation\");\n",out);continue;
    }
    fputs(line,out);
    if(in_status&&!strcmp(line,"}\n"))in_status=0;
  }
  if(startup_hits<1||!icon_lead||!icon_tail){fclose(out);fclose(in);remove("INSTBLD.C");puts("GENBUILD: expected installer patch points were not found");return 0;}
  if(ferror(in)||fclose(out)!=0){fclose(in);remove("INSTBLD.C");puts("GENBUILD: failed writing INSTBLD.C");return 0;}
  fclose(in);return 1;
}


static void emit_acc_tooltip_decl(FILE *out);

static void emit_note_wrap(FILE *out)
{
  fputs("static void note_word_wrap(int *pcx,int *pcy)\n{\n",out);
  fputs("  int cy=*pcy,start=EW-1,len,i,can=1;if(cy>=NL-1)return;\n",out);
  fputs("  if(note[cy*NW+EW-1]==' '){*pcy=cy+1;*pcx=0;softwrap[cy+1]=1;return;}\n",out);
  fputs("  while(start>0&&note[cy*NW+start-1]!=' ')start--;\n",out);
  fputs("  if(start<=0){*pcy=cy+1;*pcx=0;softwrap[cy+1]=1;return;}\n",out);
  fputs("  len=EW-start;for(i=EW-len;i<EW;i++)if(note[(cy+1)*NW+i]!=' ')can=0;\n",out);
  fputs("  if(!can){*pcy=cy+1;*pcx=0;softwrap[cy+1]=1;return;}\n",out);
  fputs("  memmove(note+(cy+1)*NW+len,note+(cy+1)*NW,EW-len);\n",out);
  fputs("  memcpy(note+(cy+1)*NW,note+cy*NW+start,len);memset(note+cy*NW+start,' ',EW-start);\n",out);
  fputs("  softwrap[cy+1]=1;*pcy=cy+1;*pcx=len;\n}\n",out);
}

static int copy_generated_source(const char *src,const char *dst)
{
  FILE *in=fopen(src,"rt"),*out;char *line=gen_line;
  if(!in)return 0;
  out=fopen(dst,"wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in))fputs(line,out);
  if(ferror(in)||fclose(out)!=0){fclose(in);remove(dst);return 0;}
  fclose(in);return 1;
}

static int write_note36(void)
{
  if(!copy_generated_source("NOTE.C","NOTEBLD.C")){puts("GENBUILD: failed copying NOTE.C to NOTEBLD.C");return 0;}
  return 1;
}

static void emit_journal_wrap(FILE *out)
{
  fputs("static void journal_word_wrap(int *pcx,int *pcy)\n{\n",out);
  fputs("  int cy=*pcy,start=NW-1,len,width=NW-4,i,can=1;if(cy>=NL-1)return;\n",out);
  fputs("  if(note[cy*NW+NW-1]==' '){*pcy=cy+1;*pcx=4;softwrap[cy+1]=1;return;}\n",out);
  fputs("  while(start>4&&note[cy*NW+start-1]!=' ')start--;\n",out);
  fputs("  if(start<=4){*pcy=cy+1;*pcx=4;softwrap[cy+1]=1;return;}\n",out);
  fputs("  len=NW-start;for(i=NW-len;i<NW;i++)if(note[(cy+1)*NW+i]!=' ')can=0;\n",out);
  fputs("  if(!can){*pcy=cy+1;*pcx=4;softwrap[cy+1]=1;return;}\n",out);
  fputs("  memmove(note+(cy+1)*NW+4+len,note+(cy+1)*NW+4,width-len);\n",out);
  fputs("  memcpy(note+(cy+1)*NW+4,note+cy*NW+start,len);memset(note+cy*NW+start,' ',NW-start);\n",out);
  fputs("  softwrap[cy+1]=1;*pcy=cy+1;*pcx=4+len;\n}\n",out);
}

static int write_journal36(void)
{
  if(!copy_generated_source("JOURNAL.C","JOURBLD.C")){puts("GENBUILD: failed copying JOURNAL.C to JOURBLD.C");return 0;}
  return 1;
}

static void emit_stack_wrap(FILE *out)
{
  fputs("static void stack_word_wrap(int *pcx,int *pcy)\n{\n",out);
  fputs("  int cy=*pcy,start=CW-1,len,i,can=1;char *text=cards[current].text;if(cy>=TEXT_LINES-1)return;\n",out);
  fputs("  if(text[cy*CW+CW-1]==' '){*pcy=cy+1;*pcx=0;return;}\n",out);
  fputs("  while(start>0&&text[cy*CW+start-1]!=' ')start--;\n",out);
  fputs("  if(start<=0){*pcy=cy+1;*pcx=0;return;}\n",out);
  fputs("  len=CW-start;for(i=CW-len;i<CW;i++)if(text[(cy+1)*CW+i]!=' ')can=0;\n",out);
  fputs("  if(!can){*pcy=cy+1;*pcx=0;return;}\n",out);
  fputs("  memmove(text+(cy+1)*CW+len,text+(cy+1)*CW,CW-len);\n",out);
  fputs("  memcpy(text+(cy+1)*CW,text+cy*CW+start,len);memset(text+cy*CW+start,' ',CW-start);\n",out);
  fputs("  *pcy=cy+1;*pcx=len;\n}\n",out);
}

static int write_stack36(void)
{
  if(!copy_generated_source("STACK.C","STACKBLD.C")){puts("GENBUILD: failed copying STACK.C to STACKBLD.C");return 0;}
  return 1;
}


static void emit_acc_tooltip_decl(FILE *out)
{
  fputs("void acc_tooltip_region(int x,int y,int w,const char *text,int active);\n",out);
  fputs("void acc_tooltip_clear_regions(void);\n",out);
}

static int write_cal36(void)
{
  FILE *in=fopen("CAL.C","rt"),*out;char *line=gen_line;int c=0;
  if(!in){puts("GENBUILD: cannot open CAL.C");return 0;}out=fopen("CALBLD.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"focus=4,dirty=1","focus=-1,dirty=1")>0)c++;
    if(replace_once(line,GEN_LINE,"acc_subbox(x,y,w,h,\"Day\",1)","acc_subbox(x,y,w,h,\"Day\",0)")>0)c++;
    replace_all(line,GEN_LINE,"acc_text(x+6+tx+4,yy+1,day_list[i].location,ACC_TEXT,35)","acc_text(x+6+tx+4,yy+1,day_list[i].location,ACC_LABEL,35)");
    replace_all(line,GEN_LINE,"i<top+5","i<top+6");
    replace_all(line,GEN_LINE,"if(top+5<n)acc_put(x+w-3,y+13,31,ACC_BORDER);","if(top+6<n)acc_put(x+w-3,y+15,31,ACC_BORDER);");
    replace_all(line,GEN_LINE,"acc_button(x+(w-6)/2,y+h-3,\"  OK  \",1);","");
    replace_all(line,GEN_LINE,"if(key==27||key==13||key==' '||((mb&1)&&my==y+h-3))return;","if(key==27)return;");
    replace_all(line,GEN_LINE,"top-=5","top-=6");replace_all(line,GEN_LINE,"top+=5","top+=6");
    replace_all(line,GEN_LINE,"top+5>n","top+6>n");replace_all(line,GEN_LINE,"n>5?n-5:0","n>6?n-6:0");
    fputs(line,out);
  }
  /* TC17 already carries these Calendar fixes in canonical CAL.C. Zero
     matches therefore means no transform is required; one match indicates a
     genuinely incomplete source and remains an error. */
  if(c==1){fclose(out);fclose(in);remove("CALBLD.C");puts("GEN37: incomplete CAL.C patch state");return 0;}
  fclose(in);if(fclose(out)!=0)return 0;return 1;
}

static int write_dfetch36(void)
{
  FILE *in=fopen("DFETCH.C","rt"),*out;char *line=gen_line;
  if(!in){puts("GENBUILD: cannot open DFETCH.C");return 0;}
  out=fopen("FETCHBLD.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in))fputs(line,out);
  fclose(in);return fclose(out)==0;
}

static int write_snake36(void)
{
  FILE *in=fopen("SNAKE.C","rt"),*out;char *line=gen_line;int c=0;
  if(!in){puts("GENBUILD: cannot open SNAKE.C");return 0;}out=fopen("SNAKEBLD.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"acc_button(x+10,y+h-3,\" Next \",focus==2);","acc_button(x+12,y+h-3,\" Next \",focus==2);")>0)c++;
    replace_all(line,GEN_LINE,"acc_button(x+12,y+h-3,\" Next \",focus==2);","acc_button(x+12,y+h-3,\" Next \",focus==2);{char ls[16];sprintf(ls,\"Level %d/%d\",level+1,LEVELS);acc_text(x+19,y+h-3,ls,ACC_HEADING,12);}");
    replace_all(line,GEN_LINE,"mx>=x+12&&mx<x+18","mx>=x+14&&mx<x+20");
    fputs(line,out);
  }
  if(!c){fclose(out);fclose(in);remove("SNAKEBLD.C");puts("GENBUILD: SNAKE.C patch point not found");return 0;}fclose(in);return fclose(out)==0;
}

static int write_pop36(void)
{
  FILE *in=fopen("POP.C","rt"),*out;char *line=gen_line;int c=0;
  if(!in){puts("GENBUILD: cannot open POP.C");return 0;}out=fopen("POPBLD.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"acc_button(x+11,y+18,\"  Prev  \",focus==2);acc_button(x+18,y+18,\"  Next  \",focus==3);",
      "if(level>1)acc_button(x+11,y+18,\"  Prev  \",focus==2);else acc_button_disabled(x+11,y+18,\"  Prev  \");if(level<10)acc_button(x+18,y+18,\"  Next  \",focus==3);else acc_button_disabled(x+18,y+18,\"  Next  \");")>0)c++;
    replace_all(line,GEN_LINE,"else if(my==y+18&&mx>=x+11&&mx<x+16){acc_press_button(x+11,y+18,\"  Prev  \");if(--level<1)level=10;new_game();full=1;}",
      "else if(level>1&&my==y+18&&mx>=x+11&&mx<x+16){acc_press_button(x+11,y+18,\"  Prev  \");level--;new_game();full=1;}");
    replace_all(line,GEN_LINE,"else if(my==y+18&&mx>=x+18&&mx<x+23){acc_press_button(x+18,y+18,\"  Next  \");if(++level>10)level=1;new_game();full=1;}",
      "else if(level<10&&my==y+18&&mx>=x+18&&mx<x+23){acc_press_button(x+18,y+18,\"  Next  \");level++;new_game();full=1;}");
    replace_all(line,GEN_LINE,"else if(my==y+18&&mx>=x+11&&mx<x+16)hover=2;else if(my==y+18&&mx>=x+18&&mx<x+23)hover=3;",
      "else if(level>1&&my==y+18&&mx>=x+11&&mx<x+16)hover=2;else if(level<10&&my==y+18&&mx>=x+18&&mx<x+23)hover=3;");
    replace_all(line,GEN_LINE,"acc_button(x+11,y+18,\"  Prev  \",focus==2||hover==2);acc_button(x+18,y+18,\"  Next  \",focus==3||hover==3);",
      "if(level>1)acc_button(x+11,y+18,\"  Prev  \",focus==2||hover==2);else acc_button_disabled(x+11,y+18,\"  Prev  \");if(level<10)acc_button(x+18,y+18,\"  Next  \",focus==3||hover==3);else acc_button_disabled(x+18,y+18,\"  Next  \");");
    replace_all(line,GEN_LINE,"if(key==271){focus--;if(focus<0)focus=4;}else{focus++;if(focus>4)focus=0;}full=1;",
      "if(key==271){do{focus--;if(focus<0)focus=4;}while((focus==2&&level==1)||(focus==3&&level==10));}else{do{focus++;if(focus>4)focus=0;}while((focus==2&&level==1)||(focus==3&&level==10));}full=1;");
    replace_all(line,GEN_LINE,"else if((key==13||key==' ')&&focus==2){if(--level<1)level=10;new_game();full=1;}",
      "else if((key==13||key==' ')&&focus==2&&level>1){level--;new_game();full=1;}");
    replace_all(line,GEN_LINE,"else if((key==13||key==' ')&&focus==3){if(++level>10)level=1;new_game();full=1;}",
      "else if((key==13||key==' ')&&focus==3&&level<10){level++;new_game();full=1;}");
    fputs(line,out);
  }
  if(!c){fclose(out);fclose(in);remove("POPBLD.C");puts("GENBUILD: POP.C patch point not found");return 0;}fclose(in);return fclose(out)==0;
}

static int write_sol36(void)
{
  FILE *in=fopen("SOL.C","rt"),*out;char *line=gen_line;int c=0;
  if(!in){puts("GENBUILD: cannot open SOL.C");return 0;}out=fopen("SOLBLD.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"n=en-st,off=4","n=en-st,off=5")>0)c++;
    replace_all(line,GEN_LINE,"if(row==4)return-2;off=5;","if(row==4)return-2;");
    replace_all(line,GEN_LINE,"else if(row==14)return-2;","else if(row==15)return-2;");
    replace_all(line,GEN_LINE,"for(i=4;i<=15;i++)","for(i=5;i<=16;i++)");
    replace_all(line,GEN_LINE,"acc_fill(x+2+c*10,y+4,1,12","acc_fill(x+2+c*10,y+5,1,12");
    replace_all(line,GEN_LINE,"acc_fill(x+8+c*10,y+4,1,12","acc_fill(x+8+c*10,y+5,1,12");
    replace_all(line,GEN_LINE,"st=sol_vstart(c);en=sol_vend(c);row=4;","st=sol_vstart(c);en=sol_vend(c);row=5;");
    replace_all(line,GEN_LINE,"draw_empty(x+3+c*10,y+4,0)","draw_empty(x+3+c*10,y+5,0)");
    /* Hidden marker deliberately remains at y+4, the clear top tableau row. */
    replace_all(line,GEN_LINE,"hidden_mark(x+3+c*10,y+14)","hidden_mark(x+3+c*10,y+15)");
    replace_all(line,GEN_LINE,"sy=4+(tn[c]-first_face(c)>11&&!reveal[c]?1:0)+(selidx-st)","sy=5+(selidx-st)");
    replace_all(line,GEN_LINE,"sy=4+(tn[c]-first_face(c)>11&&!reveal[c]?1:0)+(cursor[c]-st)","sy=5+(cursor[c]-st)");
    replace_all(line,GEN_LINE,"int dr=(tn[c]-first_face(c)>11)?15:4+(tn[c]-first_face(c));","int dr=(tn[c]-first_face(c)>11)?16:5+(tn[c]-first_face(c));");
    replace_all(line,GEN_LINE,"*py=(tn[c]-first_face(c)>11)?15:4+(tn[c]-first_face(c));","*py=(tn[c]-first_face(c)>11)?16:5+(tn[c]-first_face(c));");
    fputs(line,out);
  }
  if(!c){fclose(out);fclose(in);remove("SOLBLD.C");puts("GENBUILD: SOL.C patch point not found");return 0;}fclose(in);return fclose(out)==0;
}

static void emit_boxes_goto(FILE *out)
{
  fputs("static int boxes_goto_dialog(void)\n{\n",out);
  fputs("  int w=38,h=9,x=(acc_cols-w)/2,y=(acc_rows-h)/2,k=0,mx=0,my=0,pos=0,focus=0,v;unsigned mb=0;char s[5];s[0]=0;\n",out);
  fputs("  for(;;){acc_subbox(x,y,w,h,\"Go To\",1);acc_text(x+3,y+2,\"Level number:\",ACC_LABEL,13);acc_fill(x+17,y+2,5,1,' ',focus==0?ACC_SELECT:ACC_CONTROL);acc_text(x+17,y+2,s,focus==0?ACC_SELECT:ACC_CONTROL,4);acc_button(x+3,y+6,\"  Go  \",focus==1);acc_button(x+11,y+6,\"  Cancel  \",focus==2);acc_wait(&k,&mx,&my,&mb);\n",out);
  fputs("    if((mb&1)&&my==y+2&&mx>=x+17&&mx<x+22){focus=0;k=0;continue;}if((mb&1)&&my==y+6){if(mx>=x+3&&mx<x+9){focus=1;k=13;}else if(mx>=x+11&&mx<x+21){focus=2;k=13;}}\n",out);
  fputs("    if(k==27)return-1;if(k==9||k==271){focus=(k==271)?(focus+2)%3:(focus+1)%3;k=0;continue;}if((k==13||k==' ')&&focus==2)return-1;if((k==13||k==' ')&&focus==1){v=atoi(s);if(v>=1&&v<=LEVELS)return v-1;k=0;continue;}\n",out);
  fputs("    if(focus==0){if(k==8&&pos){s[--pos]=0;}else if(k>='0'&&k<='9'&&pos<3){s[pos++]=(char)k;s[pos]=0;}}k=0;}\n}\n",out);
}

static int write_boxes36(void)
{
  FILE *in=fopen("BOXES.C","rt"),*out;char *line=gen_line;
  if(!in){puts("GENBUILD: cannot open BOXES.C");return 0;}
  out=fopen("BOXBLD.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in))fputs(line,out);
  fclose(in);return fclose(out)==0;
}

int main(void)
{
  /* Release 3.71 changes only generated Core, Installer and Journal sources.
     Leave the already-qualified 3.7 intermediates for all other components
     untouched: several older transformation routines are historical and are
     not idempotent once their fixes have already been merged. */
  if(!write_launch36()||!write_install36()||!write_journal36())return 1;
  puts("Generated Launch! 3.71 Core, Installer and Journal build sources.");return 0;
}
