/* Launch! 3.6 build-time source generator.
   Keeps the large Release 3.5 sources canonical while generating Release 3.6
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

static int write_launch36(void)
{
  FILE *in=fopen("LAUNCH.C","rt"),*out;char *line=gen_line;int in_close=0,startup_hits=0;
  if(!in){puts("GEN36: cannot open LAUNCH.C");return 0;}
  out=fopen("L36TMP.C","wt");if(!out){fclose(in);puts("GEN36: cannot create L36TMP.C");return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"strcpy(autoexec+1,\":\\\\AUTOEXEC.BAT\")","strcpy(autoexec+1,startup_batch_suffix())")>0)startup_hits++;
    replace_all(line,GEN_LINE,"AUTOEXEC.BAT file","startup batch file");
    replace_all(line,GEN_LINE,"AUTOEXEC.BAT","startup batch file");
    replace_once(line,GEN_LINE,
      "Usage: ! [/CONFIG | /EXPLORE | /OPEN | /BYE | /NOW | /USE=file.mnu | /OPENTO=folder | /?]",
      "Usage: ! [menu.mnu] [/CONFIG | /EXPLORE | /OPEN | /BYE | /NOW | /OPENTO=folder | /?]");
    replace_once(line,GEN_LINE,
      "  /USE=file.mnu Use another menu file beside !.EXE (or a full path)",
      "  menu.mnu      Use another menu file beside !.EXE (or a full path)");
    /* 3.6 tooltip preference is part of the persisted appearance.  It is
       appended to the structure so older LAUNCH.CFG files remain valid and
       default to disabled. */
    replace_once(line,GEN_LINE,
      "unsigned char font_id,font_persist,mouse_cursor,prompt_inactivity;",
      "unsigned char font_id,font_persist,mouse_cursor,prompt_inactivity,show_tooltips;");
    replace_once(line,GEN_LINE,
      "static const APPEARANCE default_appearance={1,11,15,7,12,14,15,10,15,3,0,7,7,0,1,1,1,1,0,1,10,0,1,0,0,0,0};",
      "static const APPEARANCE default_appearance={1,11,15,7,12,14,15,10,15,3,0,7,7,0,1,1,1,1,0,1,10,0,1,0,0,0,0,0};");
    replace_once(line,GEN_LINE,
      "static APPEARANCE appearance={1,11,15,7,12,14,15,10,15,3,0,7,7,0,1,1,1,1,0,1,10,0,1,0,0,0,0};",
      "static APPEARANCE appearance={1,11,15,7,12,14,15,10,15,3,0,7,7,0,1,1,1,1,0,1,10,0,1,0,0,0,0,0};");
    replace_once(line,GEN_LINE,
      "else if(!stricmp(key,\"SHOW_SYSBAR\")){field=&a->show_sysbar;limit=1;}",
      "else if(!stricmp(key,\"SHOW_SYSBAR\")){field=&a->show_sysbar;limit=1;}\n  else if(!stricmp(key,\"SHOW_TOOLTIPS\")){field=&a->show_tooltips;limit=1;}");
    replace_once(line,GEN_LINE,
      "\"SHOW_POWER=%u\\nSHOW_TIME=%u\\nSHOW_SYSBAR=%u\\nFONT_ID=%u\\nFONT_PERSIST=%u\\n\"",
      "\"SHOW_POWER=%u\\nSHOW_TIME=%u\\nSHOW_SYSBAR=%u\\nSHOW_TOOLTIPS=%u\\nFONT_ID=%u\\nFONT_PERSIST=%u\\n\"");
    replace_once(line,GEN_LINE,
      "appearance.show_time,appearance.show_sysbar,appearance.font_id,appearance.font_persist,",
      "appearance.show_time,appearance.show_sysbar,appearance.show_tooltips,appearance.font_id,appearance.font_persist,");
    replace_once(line,GEN_LINE,"if(tab==0)return 8; /* Menu */","if(tab==0)return 9; /* Menu */");
    replace_once(line,GEN_LINE,
      "if(item==7){*limit=2;return &appearance.mouse_cursor;}",
      "if(item==7){*limit=2;return &appearance.mouse_cursor;}\n    if(item==8){*limit=1;return &appearance.show_tooltips;}");

    /* Menu-tab relayout from CONFIG.ASC. */
    if(strstr(line,"check_line(x+25,y+9,\"Show the time\"")){
      fputs("    check_line(x+25,y+9,\"Show the time\",appearance.show_time,f==4||h==4);\n",out);
      fputs("    cycle_control(x+49,y+9,appearance.hour_12?\"12-hour\":\"24-hour\",f==6||h==6);\n",out);continue;
    }
    if(strstr(line,"textout(x+5,y+10,\"Time format:\""))continue;
    if(strstr(line,"cycle_control(x+25,y+10,appearance.hour_12"))continue;
    if(strstr(line,"textout(x+5,y+12,\"Mouse cursor:\"")){fputs("    textout(x+5,y+11,\"Mouse cursor:\",C_INPUT_LABEL,18);\n",out);continue;}
    if(strstr(line,"cycle_control(x+25,y+12,mouse_cursor_names")){fputs("    cycle_control(x+25,y+11,mouse_cursor_names[appearance.mouse_cursor],f==7||h==7);\n",out);continue;}
    if(strstr(line,"textout(x+5,y+14,\"SysBar:\"")){
      fputs("    textout(x+5,y+13,\"Help tooltips:\",C_INPUT_LABEL,18);\n",out);
      fputs("    check_line(x+25,y+13,\"Display contextual help\",appearance.show_tooltips,f==8||h==8);\n",out);
      fputs("    textout(x+5,y+15,\"SysBar:\",C_INPUT_LABEL,18);\n",out);continue;
    }
    if(strstr(line,"check_line(x+25,y+14,\"Show on menu open\"")){fputs("    check_line(x+25,y+15,\"Show on menu open\",appearance.show_sysbar,f==5||h==5);\n",out);continue;}

    /* Menu-tab mouse hit map follows the same rows. */
    if(strstr(line,"if(my==y+14&&mx>=x+25&&mx<x+63)return CONFIG_CONTROL_BASE+5;")){fputs("    if(my==y+15&&mx>=x+25&&mx<x+63)return CONFIG_CONTROL_BASE+5;\n",out);continue;}
    if(strstr(line,"if(my==y+10&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE+6;")){fputs("    if(my==y+9&&mx>=x+49&&mx<x+64)return CONFIG_CONTROL_BASE+6;\n",out);continue;}
    if(strstr(line,"if(my==y+12&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE+7;")){
      fputs("    if(my==y+11&&mx>=x+25&&mx<x+40)return CONFIG_CONTROL_BASE+7;\n",out);
      fputs("    if(my==y+13&&mx>=x+25&&mx<x+63)return CONFIG_CONTROL_BASE+8;\n",out);continue;
    }
    if(strstr(line,"/* Launch! 3.5 - modal command menu for DOS")){
      fputs("/* Launch! 3.65 - modal command menu for DOS\n",out);continue;
    }
    if(strstr(line,"#define MAX_NODES 96")){
      emit_startup_batch_helpers(out);
      fputs("#define CORE_TOOLTIP_GLYPH 126\n",out);
      fputs(line,out);continue;
    }
    if(strstr(line,"static void launchui_restore(void);")){
      fputs(line,out);
      fputs("static void tooltip_glyph_install(void);\n",out);
      fputs("static void tooltip_glyph_restore(void);\n",out);
      fputs("static void core_tooltip_restore(void);\n",out);
      continue;
    }
    if(strstr(line,"static void close_menu(void)"))in_close=1;
    if(in_close&&strstr(line,"  launchui_restore();")){
      fputs("  core_tooltip_restore();\n",out);
      fputs("  tooltip_glyph_restore();\n",out);fputs(line,out);continue;
    }
    if(in_close&&!strcmp(line,"}\n")){fputs(line,out);in_close=0;continue;}
    if(strstr(line,"  launchui_install();")){
      fputs(line,out);fputs("  tooltip_glyph_install();\n",out);continue;
    }
    if(strstr(line,"  else launchui_rebase();")){
      fputs(line,out);fputs("  tooltip_glyph_install();\n",out);continue;
    }
    if(strstr(line,"if(tab==4 && item==0){mouse_pointer_restore();font_preview(appearance.font_id);mouse_pointer_install();}")){
      fputs("  if(tab==4 && item==0){mouse_pointer_restore();font_preview(appearance.font_id);tooltip_glyph_install();mouse_pointer_install();}\n",out);continue;
    }
    if(strstr(line,"  box(x,y,w,h,title,C_TITLE,appearance.titlebar_bg,appearance.titlebar_fg);")){
      fputs("  core_tooltip_restore();\n",out);fputs(line,out);continue;
    }
    if(strstr(line,"static void sort_default_folder_alpha(int parent,int *changed)")){
      fputs("static int children(int parent,int *list);\n",out);
      fputs(line,out);continue;
    }
    if(strstr(line,"static void draw_button_state(int x,int y,const char *label,int width,int focused,int enabled)")){
      emit_tooltip_ui(out);fputs(line,out);continue;
    }
    if(strstr(line,"static void draw_button(int x,int y,const char *label,int width,int focused){draw_button_state")){
      fputs("static void draw_button(int x,int y,const char *label,int width,int focused)\n{\n",out);
      fputs("  draw_button_state(x,y,label,width,focused,1);\n",out);
      fputs("  if(appearance.show_tooltips&&focused&&core_tooltip_icon(label))core_tooltip_show(x,y,label);\n",out);
      fputs("  else if(core_tt_active&&core_tt_owner_x==x&&core_tt_owner_y==y)core_tooltip_restore();\n",out);
      fputs("}\n",out);
      fputs("static void draw_button_tip(int x,int y,const char *label,int width,int focused,const char *tip)\n{\n",out);
      fputs("  draw_button_state(x,y,label,width,focused,1);if(appearance.show_tooltips&&focused)core_tooltip_show(x,y,tip);\n",out);
      fputs("  else if(core_tt_active&&core_tt_owner_x==x&&core_tt_owner_y==y)core_tooltip_restore();\n}\n",out);
      fputs("static void core_tooltip_region(int x,int y,int active,const char *tip)\n{\n",out);
      fputs("  if(appearance.show_tooltips&&active)core_tooltip_show(x,y,tip);else if(core_tt_active&&core_tt_owner_x==x&&core_tt_owner_y==y)core_tooltip_restore();\n}\n",out);
      continue;
    }
    if(strstr(line,"static const unsigned char launchui_codes[41]=")){
      emit_tooltip_glyph(out);fputs(line,out);continue;
    }
    if(strstr(line,"     sibling_exists(\"!SYSINFO.EXE\")||sibling_exists(\"!TODOS.EXE\")){")){
      fputs("     sibling_exists(\"!SYSINFO.EXE\")||sibling_exists(\"!TODOS.EXE\")||sibling_exists(\"!TYPO.EXE\")){\n",out);continue;
    }
    if(strstr(line,"     sibling_exists(\"!SNAKE.EXE\")||sibling_exists(\"!SOL.EXE\")){")){
      fputs("     sibling_exists(\"!SNAKE.EXE\")||sibling_exists(\"!SOL.EXE\")||sibling_exists(\"!PLUMB.EXE\")||sibling_exists(\"!WORDZ.EXE\")){\n",out);continue;
    }

    replace_all(line,GEN_LINE,
      "int fa=ATTR(appearance.controls_bg,(appearance.controls_fg&7)|8)",
      "int fa=ATTR(appearance.controls_bg,appearance.controls_fg)");

    /* Context-specific core tooltips. */
    replace_all(line,GEN_LINE,
      "draw_button(x+58,y+17,\"  ?  \",5,focus==23||hover==23)",
      "draw_button_tip(x+58,y+17,\"  ?  \",5,focus==23||hover==23,\"About Launch!\")");
    replace_once(line,GEN_LINE,
      "check_line(x+25,y+6,\"Persist\",appearance.font_persist,f==1||h==1);draw_character_preview",
      "check_line(x+25,y+6,\"Persist\",appearance.font_persist,f==1||h==1);core_tooltip_region(x+25,y+6,f==1||h==1,\"Make it stick no matter what\");draw_character_preview");
    replace_all(line,GEN_LINE,
      "draw_button(x+2,y+2,\"  Add  \",8,focus==10||hover_control==10)",
      "draw_button_tip(x+2,y+2,\"  Add  \",8,focus==10||hover_control==10,\"Create association\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+3,y+18,\"  Open  \",9,focus==1||hover_control==1)",
      "draw_button_tip(x+3,y+18,\"  Open  \",9,focus==1||hover_control==1,\"Open selected file\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+14,y+18,\"  Params  \",10,focus==2||hover_control==2)",
      "draw_button_tip(x+14,y+18,\"  Params  \",10,focus==2||hover_control==2,\"Open file with parameters\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+26,y+18,\"  Locate  \",10,focus==3||hover_control==3)",
      "draw_button_tip(x+26,y+18,\"  Locate  \",10,focus==3||hover_control==3,\"Navigate to file\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+3,y+18,\"  Run  \",9,focus==1||hover==1)",
      "draw_button_tip(x+3,y+18,\"  Run  \",9,focus==1||hover==1,\"Run selected executable\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+14,y+18,\"  Params  \",10,focus==2||hover==2)",
      "draw_button_tip(x+14,y+18,\"  Params  \",10,focus==2||hover==2,\"Run with parameters\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+3,y+6,\"  Run  \",9,focus==1||hover==1)",
      "draw_button_tip(x+3,y+6,\"  Run  \",9,focus==1||hover==1,\"Run now\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+29,y+7,\"  Choose  \",10,focus==A_CHOOSE||hover==A_CHOOSE)",
      "draw_button_tip(x+29,y+7,\"  Choose  \",10,focus==A_CHOOSE||hover==A_CHOOSE,\"Select launcher to use\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+13,y+11,\"  Cancel  \",10,focus==A_CANCEL||hover==A_CANCEL)",
      "draw_button(x+12,y+11,\"  Cancel  \",10,focus==A_CANCEL||hover==A_CANCEL)");
    replace_all(line,GEN_LINE,"x+13,y+11,\"  Cancel  \",10","x+12,y+11,\"  Cancel  \",10");
    replace_all(line,GEN_LINE,"my==y+11&&mx>=x+13&&mx<x+23","my==y+11&&mx>=x+12&&mx<x+22");
    replace_all(line,GEN_LINE,
      "draw_button(x+3,y+5,\"  Power Off  \",13,choice==0)",
      "draw_button_tip(x+3,y+5,\"  Power Off  \",13,choice==0,\"Attempt APM power off\")");
    replace_all(line,GEN_LINE,
      "draw_button(x+18,y+5,\"  Reboot  \",10,choice==1)",
      "draw_button_tip(x+18,y+5,\"  Reboot  \",10,choice==1,\"Perform cold boot\")");
    replace_once(line,GEN_LINE,
      "cell(x+57,y+6,' ',C_MENU_BACKGROUND);",
      "cell(x+57,y+6,' ',C_MENU_BACKGROUND);core_tooltip_region(x+47,y+6,focus==3||hover==3,\"Ask each launch\");");
    fputs(line,out);
    if(strstr(line,"if(sibling_exists(\"!TODOS.EXE\")&&!ensure_launcher(folder,\"To-Dos\",\"!TODOS\",1,changed))return 0;"))
      fputs("    if(sibling_exists(\"!TYPO.EXE\")&&!ensure_launcher(folder,\"Typo\",\"!TYPO\",1,changed))return 0;\n",out);
    if(strstr(line,"if(sibling_exists(\"!FCELL.EXE\")&&!ensure_launcher(folder,\"FreeCell\",\"!FCELL\",1,changed))return 0;"))
      fputs("    if(sibling_exists(\"!PLUMB.EXE\")&&!ensure_launcher(folder,\"Plumb\",\"!PLUMB\",1,changed))return 0;\n",out);
    if(strstr(line,"if(sibling_exists(\"!SOL.EXE\")&&!ensure_launcher(folder,\"Solitaire\",\"!SOL\",1,changed))return 0;"))
      fputs("    if(sibling_exists(\"!WORDZ.EXE\")&&!ensure_launcher(folder,\"Wordz\",\"!WORDZ\",1,changed))return 0;\n",out);
  }
  if(startup_hits<4){fclose(out);fclose(in);remove("L36TMP.C");puts("GEN36: expected startup batch references were not found in LAUNCH.C");return 0;}
  if(ferror(in)||fclose(out)!=0){fclose(in);remove("L36TMP.C");puts("GEN36: failed writing L36TMP.C");return 0;}
  fclose(in);return 1;
}

static int write_install36(void)
{
  FILE *in=fopen("INSTALL.C","rt"),*out;char *line=gen_line;int in_status=0,startup_hits=0,icon_lead=0,icon_tail=0;
  if(!in){puts("GEN36: cannot open INSTALL.C");return 0;}
  out=fopen("I36TMP.C","wt");if(!out){fclose(in);puts("GEN36: cannot create I36TMP.C");return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"strcpy(autoexec+1,\":\\\\AUTOEXEC.BAT\")","strcpy(autoexec+1,startup_batch_suffix())")>0)startup_hits++;
    replace_all(line,GEN_LINE,"AUTOEXEC.BAT file","startup batch file");
    replace_all(line,GEN_LINE,"AUTOEXEC.BAT","startup batch file");
    if(strstr(line,"/* Launch! 3.5 installer")){
      fputs("/* Launch! 3.65 installer - Microsoft C/C++ 7.0, DOS small model. */\n",out);continue;
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

    if(strstr(line,"    \"!NOTE.EXE\",\"!STACK.EXE\",\"!SYSINFO.EXE\",\"!TODOS.EXE\",")){
      fputs("    \"!NOTE.EXE\",\"!STACK.EXE\",\"!SYSINFO.EXE\",\"!TODOS.EXE\",\"!TYPO.EXE\",\n",out);continue;
    }
    if(strstr(line,"    \"!BOXES.EXE\",\"!FCELL.EXE\",\"!POP.EXE\",\"!SNAKE.EXE\",\"!SOL.EXE\",0};")){
      fputs("    \"!BOXES.EXE\",\"!FCELL.EXE\",\"!PLUMB.EXE\",\"!POP.EXE\",\"!SNAKE.EXE\",\"!SOL.EXE\",\"!WORDZ.EXE\",0};\n",out);continue;
    }
    if(strstr(line,"puts(\"Launch! 3.5 Installation\");")){
      fputs("  puts(\"Launch! 3.65 Installation\");\n",out);continue;
    }
    fputs(line,out);
    if(in_status&&!strcmp(line,"}\n"))in_status=0;
  }
  if(startup_hits<1||!icon_lead||!icon_tail){fclose(out);fclose(in);remove("I36TMP.C");puts("GEN36: expected installer patch points were not found");return 0;}
  if(ferror(in)||fclose(out)!=0){fclose(in);remove("I36TMP.C");puts("GEN36: failed writing I36TMP.C");return 0;}
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
  if(!copy_generated_source("NOTE.C","N36TMP.C")){puts("GEN36: failed copying NOTE.C to N36TMP.C");return 0;}
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
  if(!copy_generated_source("JOURNAL.C","J36TMP.C")){puts("GEN36: failed copying JOURNAL.C to J36TMP.C");return 0;}
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
  if(!copy_generated_source("STACK.C","S36TMP.C")){puts("GEN36: failed copying STACK.C to S36TMP.C");return 0;}
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
  if(!in){puts("GEN36: cannot open CAL.C");return 0;}out=fopen("C36TMP.C","wt");if(!out){fclose(in);return 0;}
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
  if(c<2){fclose(out);fclose(in);remove("C36TMP.C");puts("GEN36: CAL.C patch points not found");return 0;}
  fclose(in);if(fclose(out)!=0)return 0;return 1;
}

static int write_sysinfo36(void)
{
  FILE *in=fopen("SYSINFO.C","rt"),*out;char *line=gen_line;int c=0;
  if(!in){puts("GEN36: cannot open SYSINFO.C");return 0;}out=fopen("Y36TMP.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"close_row,focus=1,serial","close_row,focus=-1,serial")>0)c++;
    if(replace_once(line,GEN_LINE,"if(k==9){focus=1-focus;continue;}","if(k==9){if(focus<0)focus=0;else focus=1-focus;continue;}")>0)c++;
    replace_once(line,GEN_LINE,"if(k==13){if(focus==0)","if(k==13&&focus>=0){if(focus==0)");
    fputs(line,out);
  }
  if(c<2){fclose(out);fclose(in);remove("Y36TMP.C");puts("GEN36: SYSINFO.C patch points not found");return 0;}fclose(in);return fclose(out)==0;
}

static int write_snake36(void)
{
  FILE *in=fopen("SNAKE.C","rt"),*out;char *line=gen_line;int c=0;
  if(!in){puts("GEN36: cannot open SNAKE.C");return 0;}out=fopen("K36TMP.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in)){
    if(replace_once(line,GEN_LINE,"acc_button(x+10,y+h-3,\" Next \",focus==2);","acc_button(x+12,y+h-3,\" Next \",focus==2);")>0)c++;
    replace_all(line,GEN_LINE,"acc_button(x+12,y+h-3,\" Next \",focus==2);","acc_button(x+12,y+h-3,\" Next \",focus==2);{char ls[16];sprintf(ls,\"Level %d/%d\",level+1,LEVELS);acc_text(x+19,y+h-3,ls,ACC_HEADING,12);}");
    replace_all(line,GEN_LINE,"mx>=x+12&&mx<x+18","mx>=x+14&&mx<x+20");
    fputs(line,out);
  }
  if(!c){fclose(out);fclose(in);remove("K36TMP.C");puts("GEN36: SNAKE.C patch point not found");return 0;}fclose(in);return fclose(out)==0;
}

static int write_pop36(void)
{
  FILE *in=fopen("POP.C","rt"),*out;char *line=gen_line;int c=0;
  if(!in){puts("GEN36: cannot open POP.C");return 0;}out=fopen("P36TMP.C","wt");if(!out){fclose(in);return 0;}
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
  if(!c){fclose(out);fclose(in);remove("P36TMP.C");puts("GEN36: POP.C patch point not found");return 0;}fclose(in);return fclose(out)==0;
}

static int write_sol36(void)
{
  FILE *in=fopen("SOL.C","rt"),*out;char *line=gen_line;int c=0;
  if(!in){puts("GEN36: cannot open SOL.C");return 0;}out=fopen("O36TMP.C","wt");if(!out){fclose(in);return 0;}
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
  if(!c){fclose(out);fclose(in);remove("O36TMP.C");puts("GEN36: SOL.C patch point not found");return 0;}fclose(in);return fclose(out)==0;
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
  if(!in){puts("GEN36: cannot open BOXES.C");return 0;}
  out=fopen("B36TMP.C","wt");if(!out){fclose(in);return 0;}
  while(fgets(line,GEN_LINE,in))fputs(line,out);
  fclose(in);return fclose(out)==0;
}

int main(void)
{
  if(!write_launch36()||!write_install36()||!write_note36()||
     !write_journal36()||!write_stack36()||!write_cal36()||!write_sysinfo36()||
     !write_snake36()||!write_pop36()||!write_sol36()||!write_boxes36())return 1;
  puts("Generated Launch! 3.65 build sources.");return 0;
}
