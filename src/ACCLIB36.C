/* Launch! 3.63 accessory runtime extension.
   This translation unit wraps the 3.5 ACCLIB implementation to add the
   Release 3.63 tooltip component without duplicating the shared UI runtime. */
#define acc_begin acc36_begin_base
#define acc_end acc36_end_base
#define acc_end_screen acc36_end_screen_base
#define acc_restore_screen acc36_restore_screen_base
#define acc_button acc36_button_base
#define acc_box acc36_box_base
#define acc_subbox acc36_subbox_base
#define acc_mouse acc36_mouse_base
#define acc_wait acc36_wait_base
#define acc_notice acc36_notice_base
#define acc_help acc36_help_base
#include "ACCLIB.C"
#undef acc_begin
#undef acc_end
#undef acc_end_screen
#undef acc_restore_screen
#undef acc_button
#undef acc_box
#undef acc_subbox
#undef acc_mouse
#undef acc_wait
#undef acc_notice
#undef acc_help

#ifdef ACCLIB_MIN_GLYPHS
static const unsigned char stack_add_a16[32]={0x00,0x00,0x01,0x01,0x01,0x01,0x3F,0x3F,0x3F,0x01,0x1D,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_add_a14[32]={0x00,0x01,0x01,0x01,0x01,0x3F,0x3F,0x3F,0x01,0x1D,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_add_b16[32]={0x00,0x00,0x80,0x80,0x80,0x80,0xFC,0xFC,0xFC,0x80,0xBE,0xA0,0xA0,0xA0,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_add_b14[32]={0x00,0x80,0x80,0x80,0x80,0xFC,0xFC,0xFC,0x80,0xBE,0xA0,0xA0,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_delete_a16[32]={0x00,0x00,0x03,0x1F,0x20,0x3F,0x10,0x16,0x14,0x14,0x14,0x12,0x10,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_delete_a14[32]={0x00,0x03,0x1F,0x20,0x3F,0x10,0x12,0x16,0x14,0x14,0x12,0x10,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_delete_b16[32]={0x00,0x00,0xC0,0xF8,0x0C,0xFC,0x08,0x68,0x28,0x28,0x28,0x48,0x08,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_delete_b14[32]={0x00,0xC0,0xF8,0x0C,0xF8,0x08,0x48,0x28,0x28,0x68,0x48,0x08,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_export_a16[32]={0x00,0x00,0x1F,0x21,0x21,0x21,0x21,0x20,0x27,0x20,0x27,0x20,0x3F,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_export_a14[32]={0x00,0x1F,0x21,0x21,0x21,0x21,0x20,0x27,0x20,0x27,0x20,0x3F,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_export_b16[32]={0x00,0x00,0xF8,0x94,0x92,0x92,0xF2,0x02,0xE2,0x02,0xE2,0x02,0xFE,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_export_b14[32]={0x00,0xF8,0x94,0x92,0x92,0xF2,0x02,0xF2,0x02,0xF2,0x02,0xFE,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
#endif

#define TOOLTIP_ARROW_GLYPH 126
#define TOOLTIP_LEFT_GLYPH 203
#define TOOLTIP_RIGHT_GLYPH 226
#define TOOLTIP_MAX 42
static unsigned char tooltip_old_glyph[2][32];
static int tooltip_glyph_saved=0;
static int tooltip_active=0,tooltip_x=0,tooltip_y=0,tooltip_w=0;
static int tooltip_owner_x=-1,tooltip_owner_y=-1,tooltip_owner_w=0;
static int tooltip_dismissed=0,tooltip_dismiss_x=-1,tooltip_dismiss_y=-1,tooltip_dismiss_w=0;
static int tooltip_suspended=0,tooltip_enabled=0;
static int acc_dialog_x=-1,acc_dialog_y=-1,acc_dialog_w=0,acc_dialog_h=0;
static int acc_focus_suppressed=0;
static int note_hover_region=-1;
static int acc_hover_button=-1,acc_hover_x=-1,acc_hover_y=-1,acc_hover_valid=0;
static void (*acc_idle_hook)(void)=0;
void acc_set_idle_hook(void (*fn)(void)){acc_idle_hook=fn;}
/* Release 3.63 keeps the configured mouse pointer unchanged in !DRAW.
   The canvas itself now supplies hover feedback, so this legacy API is a no-op
   retained only for source compatibility with older 3.6x accessory sources. */
void acc_canvas_cursor_region(int x,int y,int w,int h,int colour)
{(void)x;(void)y;(void)w;(void)h;(void)colour;}
static void canvas_cursor_sync(int mx,int my){(void)mx;(void)my;}
void acc_mouse_reapply_cursor(void)
{
  union REGS r;if(!acc_mouse_present)return;
  /* A BIOS graphics-mode round trip can make the driver's text cursor vanish
     while our visibility flag still says it is shown.  Reinstall the user's
     configured cursor from a clean saved-glyph state, then force it visible. */
  mouse_pointer_restore();mouse_pointer_install();
  r.x.ax=1;int86(0x33,&r,&r);mouse_visible=1;
}
static char tooltip_app[20];
#define TOOLTIP_REGIONS 12
typedef struct {int x,y,w;const char *text;int active;} TTREGION;
static TTREGION tooltip_regions[TOOLTIP_REGIONS];static int tooltip_region_count=0;
static unsigned short tooltip_line[TOOLTIP_MAX];
static unsigned short tooltip_arrow_cell;
static char tooltip_text[41];
/* Arrow is the existing tooltip pointer.  The rounded caps are taken from
   LAUNCHUI.FNT/F14 source chars 203 (left) and 87 (right), but are installed
   into non-cell-extension runtime slots so they cannot collide with button,
   game or VGA ninth-column glyphs. */
static const unsigned char tooltip_arrow16[32]={0xFF,0xFF,0x7E,0x7E,0x3C,0x3C,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char tooltip_arrow14[32]={0xFF,0x7E,0x7E,0x3C,0x3C,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char tooltip_left16[32]={0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char tooltip_left14[32]={0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char tooltip_right16[32]={0xF0,0xFC,0xFC,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFC,0xF0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char tooltip_right14[32]={0xF0,0xFC,0xFC,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFC,0xF0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


static void glyph36_refresh(void)
{
#ifndef ACCLIB_MIN_GLYPHS
  return;
#else
  /* !STACK omits the full glyph table to preserve DGROUP.  Refresh the
     current 3.63 Add/Delete/Export artwork explicitly. */
  int h=acc_font_height();
  acc_glyph_write(208,h==14?stack_add_a14:stack_add_a16);
  acc_glyph_write(187,h==14?stack_add_b14:stack_add_b16);
  acc_glyph_write(209,h==14?stack_delete_a14:stack_delete_a16);
  acc_glyph_write(188,h==14?stack_delete_b14:stack_delete_b16);
  acc_glyph_write(204,h==14?stack_export_a14:stack_export_a16);
  acc_glyph_write(181,h==14?stack_export_b14:stack_export_b16);
#endif
}


static void tooltip_load_setting(void)
{
  char path[ACC_PATH],line[96],*eq;FILE *f;size_t n;
  tooltip_enabled=0;
  strcpy(path,acc_directory);n=strlen(path);
  if(n&&path[n-1]!='\\'&&path[n-1]!='/')strcat(path,"\\");
  strcat(path,"LAUNCH.CFG");f=fopen(path,"rt");if(!f)return;
  while(fgets(line,sizeof(line),f)){
    eq=strchr(line,'=');if(!eq)continue;*eq++=0;
    if(!stricmp(line,"SHOW_TOOLTIPS")){tooltip_enabled=(unsigned char)(atoi(eq)!=0);break;}
  }
  fclose(f);
}

static void tooltip_trim(const char *src,char *dst)
{
  const char *a=src,*b;int n;
  while(*a==' ')a++;b=a+strlen(a);while(b>a&&b[-1]==' ')b--;
  n=(int)(b-a);if(n>40)n=40;memcpy(dst,a,n);dst[n]=0;
}

static int tooltip_is_icon(const char *text)
{
  int a=0,b=0;return acc_button_icon(text,&a,&b)==1;
}

static int tooltip_app_is(const char *name)
{
  return strstr(tooltip_app,name)!=0;
}

/* Release 3.63 game-number artwork.
   Games replace ASCII 0..9 only.  Punctuation and letters remain ordinary
   CP437 except for the card-only rank/suit glyphs owned by SOL/FCELL. */
static const unsigned char game_glyph_code[10]={'0','1','2','3','4','5','6','7','8','9'};
static const unsigned char game_glyph_logical[10]={64,65,66,67,68,69,70,71,72,73};
static unsigned char game_digit_old[10][32];
static int game_digits_installed=0,game_digits_saved=0;
static int game_digit_app(void)
{
  return tooltip_app_is("!BOXES")||tooltip_app_is("!SOL")||
         tooltip_app_is("!FCELL")||tooltip_app_is("!POP")||
         tooltip_app_is("!SNAKE")||tooltip_app_is("!PLUMB")||
         tooltip_app_is("!WORDZ");
}
static void game_digits_install(void)
{
  int i;if(game_digits_installed||!game_digit_app())return;
  /* Snapshot the user's real digit glyphs only once. A mode reset or a
     temporary suspend/resume must never replace that snapshot with the
     Launch! game-number artwork, otherwise the large pixel digits can leak
     back to DOS after a game exits. */
  if(!game_digits_saved){for(i=0;i<10;i++)acc_glyph_read(game_glyph_code[i],game_digit_old[i]);game_digits_saved=1;}
  for(i=0;i<10;i++)acc_glyph_library(game_glyph_logical[i],game_glyph_code[i]);
  game_digits_installed=1;
}
static void game_digits_restore(void)
{
  int i;if(!game_digits_installed)return;
  for(i=0;i<10;i++)acc_glyph_write(game_glyph_code[i],game_digit_old[i]);
  game_digits_installed=0;
}
static void game_digits_refresh(void)
{
  int i;if(!game_digits_installed)return;
  for(i=0;i<10;i++)acc_glyph_library(game_glyph_logical[i],game_glyph_code[i]);
}
void acc_game_glyphs_suspend(void){game_digits_restore();}
void acc_game_glyphs_resume(void){game_digits_install();}

static int tooltip_button_label(const char *text,char *out)
{
  int ia=0,ib=0,icon=tooltip_is_icon(text);acc_button_icon(text,&ia,&ib);tooltip_trim(text,out);
  if(tooltip_app_is("!CAL")){
    if(strstr(text,"Prev")||ia==17||((unsigned char)out[0]==17&&!out[1])){strcpy(out,"Previous month");return 1;}
    if(strstr(text,"Next")||ia==16||((unsigned char)out[0]==16&&!out[1])){strcpy(out,"Next month");return 1;}
    if(!stricmp(out,"Today")){strcpy(out,"Jump to current month");return 1;}
  }
  if(tooltip_app_is("!DRAW")){if(!stricmp(out,"Show")){strcpy(out,"Fullscreen graphics mode");return 1;}if(!stricmp(out,"Clear")){strcpy(out,"Erase entire canvas");return 1;}if(!stricmp(out,"Grid")){strcpy(out,"Show/hide grid");return 1;}}
  if(tooltip_app_is("!JOURNAL")){
    if(strstr(text,"Prev")||ia==17||((unsigned char)out[0]==17&&!out[1])){strcpy(out,"Previous day");return 1;}
    if(strstr(text,"Next")||ia==16||((unsigned char)out[0]==16&&!out[1])){strcpy(out,"Next day");return 1;}
    if(!stricmp(out,"Go To")){strcpy(out,"Go to specific day");return 1;}
    if(!stricmp(out,"Chars")){strcpy(out,"Insert extended character");return 1;}
  }
  if(tooltip_app_is("!NOTE")){
    if(!stricmp(out,"Add")){strcpy(out,"Add new tab");return 1;}
    if(!stricmp(out,"Clr")){strcpy(out,"Clear current page");return 1;}
    if(!stricmp(out,"Chars")){strcpy(out,"Insert extended character");return 1;}
  }
  if(tooltip_app_is("!STACK")){
    if(!stricmp(out,"Add")){strcpy(out,"Add new card");return 1;}
    if(strstr(text,"Prev")||ia==17){strcpy(out,"Previous card");return 1;}
    if(strstr(text,"Next")||ia==16){strcpy(out,"Next card");return 1;}
  }
  if(tooltip_app_is("!TODOS")){
    if(!stricmp(out,"Add")){strcpy(out,"Add task or group");return 1;}
    if(!stricmp(out,"Edit")){strcpy(out,"Edit selected task");return 1;}
    if(!stricmp(out,"Delete")){strcpy(out,"Delete selected task");return 1;}
    if(!stricmp(out,"Sort")){strcpy(out,"Toggle sort order");return 1;}
    if(!stricmp(out,"OK")){strcpy(out,"Save");return 1;}
    if(!stricmp(out,"Cancel")){strcpy(out,"Discard");return 1;}
    if(!stricmp(out,"Show")||!stricmp(out,"Hide")||(out[0]&&(unsigned char)out[0]>=128)){strcpy(out,"Show/hide completed tasks");return 1;}
  }
  if(tooltip_app_is("!SOL")&&!stricmp(out,"Refresh")){strcpy(out,"Deal a new game");return 1;}
  if(tooltip_app_is("!FCELL")&&!stricmp(out,"Refresh")){strcpy(out,"Deal a new game");return 1;}
  if(tooltip_app_is("!SNAKE")){
    if(!stricmp(out,"Refresh")){strcpy(out,"Try again");return 1;}
    if(strstr(text,"Next")||ia==16){strcpy(out,"Next level");return 1;}
  }
  if(tooltip_app_is("!POP")){
    if(!stricmp(out,"Refresh")||!stricmp(out,"Retry")){strcpy(out,"Try again");return 1;}
    if(strstr(text,"Prev")||ia==17){strcpy(out,"Previous level");return 1;}
    if(strstr(text,"Next")||ia==16){strcpy(out,"Next level");return 1;}
  }
  if(tooltip_app_is("!PLUMB")){
    if(!stricmp(out,"Refresh")||!stricmp(out,"Retry")){strcpy(out,"Try again");return 1;}
    if(!stricmp(out,"Flow!")){strcpy(out,"Make it flow!");return 1;}
    if(!stricmp(out,"Pause")||!stricmp(out,"Resume")){strcpy(out,"Pause game");return 1;}
  }
  if(tooltip_app_is("!TYPO")){
    if(!stricmp(out,"Refresh")||!stricmp(out,"Retry")){strcpy(out,"Try again");return 1;}
    if(strstr(text,"Prev")||ia==17){strcpy(out,"Previous level");return 1;}
    if(strstr(text,"Next")||ia==16){strcpy(out,"Next level");return 1;}
    if(!stricmp(out,"Records")){strcpy(out,"Top 10 scores");return 1;}
  }
  if(tooltip_app_is("!BOXES")){
    if(!stricmp(out,"Retry")||!stricmp(out,"Refresh")){strcpy(out,"Try again");return 1;}
    if(!stricmp(out,"Solve")){strcpy(out,"Play it out for me");return 1;}
    if(strstr(text,"Prev")||ia==17){strcpy(out,"Previous level");return 1;}
    if(strstr(text,"Next")||ia==16){strcpy(out,"Next level");return 1;}
    if(!stricmp(out,"Go To")){strcpy(out,"Go to level #");return 1;}
  }
  if(icon){if(!strcmp(out,"?"))strcpy(out,"Help");return 1;}
  return 0;
}

void acc_tooltip_region(int x,int y,int w,const char *text,int active)
{
  int i;if(w<=0||!text)return;
  for(i=0;i<tooltip_region_count;i++)if(tooltip_regions[i].x==x&&tooltip_regions[i].y==y&&tooltip_regions[i].w==w){tooltip_regions[i].text=text;tooltip_regions[i].active=active;return;}
  if(tooltip_region_count<TOOLTIP_REGIONS){i=tooltip_region_count++;tooltip_regions[i].x=x;tooltip_regions[i].y=y;tooltip_regions[i].w=w;tooltip_regions[i].text=text;tooltip_regions[i].active=active;}
}

void acc_tooltip_clear_regions(void)
{
  tooltip_region_count=0;
}

static int tooltip_mouse_hide(void)
{
  union REGS r;if(!acc_mouse_present||!mouse_visible)return 0;
  r.x.ax=2;int86(0x33,&r,&r);return 1;
}

static void tooltip_mouse_show(int hidden)
{
  union REGS r;if(!hidden||!acc_mouse_present||!mouse_visible)return;
  r.x.ax=1;int86(0x33,&r,&r);
}

static void tooltip_restore(void)
{
  int i,hidden;if(!tooltip_active)return;hidden=tooltip_mouse_hide();
  for(i=0;i<tooltip_w;i++)*(unsigned short far *)MAKE_FP(saved_video_segment,((tooltip_y*acc_cols+tooltip_x+i)*2))=tooltip_line[i];
  *(unsigned short far *)MAKE_FP(saved_video_segment,(((tooltip_y+1)*acc_cols+tooltip_x+2)*2))=tooltip_arrow_cell;
  tooltip_active=0;tooltip_owner_x=tooltip_owner_y=-1;tooltip_owner_w=0;
  tooltip_mouse_show(hidden);
}

static int tooltip_refresh_backing(void)
{
  int i,changed=0,a=ACC_ATTR(0,acc_appearance.titles),attr,arrow_attr;
  unsigned short cur,expected;int ch;
  if(!tooltip_active)return 0;
  for(i=0;i<tooltip_w;i++){
    if(i==0){ch=TOOLTIP_LEFT_GLYPH;attr=(int)((tooltip_line[i]>>8)&0xF0);}
    else if(i==tooltip_w-1){ch=TOOLTIP_RIGHT_GLYPH;attr=(int)((tooltip_line[i]>>8)&0xF0);}
    else {ch=(i-1<(int)strlen(tooltip_text))?(unsigned char)tooltip_text[i-1]:' ';attr=a;}
    expected=(unsigned short)(((unsigned short)attr<<8)|(unsigned char)ch);
    cur=*(unsigned short far *)MAKE_FP(saved_video_segment,((tooltip_y*acc_cols+tooltip_x+i)*2));
    if(cur!=expected){tooltip_line[i]=cur;changed=1;}
  }
  arrow_attr=(int)((tooltip_arrow_cell>>8)&0xF0);
  expected=(unsigned short)(((unsigned short)arrow_attr<<8)|(unsigned char)TOOLTIP_ARROW_GLYPH);
  cur=*(unsigned short far *)MAKE_FP(saved_video_segment,(((tooltip_y+1)*acc_cols+tooltip_x+2)*2));
  if(cur!=expected){tooltip_arrow_cell=cur;changed=1;}
  return changed;
}

static void tooltip_paint(void)
{
  int a=ACC_ATTR(0,acc_appearance.titles),left_attr,right_attr,arrow_attr,hidden;
  if(!tooltip_active)return;hidden=tooltip_mouse_hide();
  left_attr=(int)((tooltip_line[0]>>8)&0xF0);
  right_attr=(int)((tooltip_line[tooltip_w-1]>>8)&0xF0);
  acc_put(tooltip_x,tooltip_y,TOOLTIP_LEFT_GLYPH,left_attr);
  acc_fill(tooltip_x+1,tooltip_y,tooltip_w-2,1,' ',a);
  acc_text(tooltip_x+1,tooltip_y,tooltip_text,a,tooltip_w-2);
  acc_put(tooltip_x+tooltip_w-1,tooltip_y,TOOLTIP_RIGHT_GLYPH,right_attr);
  /* Pointer and rounded caps use black foreground over the exact background
     colour already present underneath their cells. */
  arrow_attr=(int)((tooltip_arrow_cell>>8)&0xF0);
  acc_put(tooltip_x+2,tooltip_y+1,TOOLTIP_ARROW_GLYPH,arrow_attr);
  tooltip_mouse_show(hidden);
}

static int tooltip_owner_same(int x,int y,int w)
{return tooltip_dismissed&&x==tooltip_dismiss_x&&y==tooltip_dismiss_y&&w==tooltip_dismiss_w;}

static void tooltip_show(int bx,int by,int bw,const char *text)
{
  char clean[41];int i,w,ty=by-2;
  tooltip_trim(text,clean);if(!*clean)return;
  if(tooltip_app_is("!NOTE")&&!stricmp(clean,"Add page")){bx-=2;if(bx<0)bx=0;}
  ty=by-2;if(ty<0)return;
  w=(int)strlen(clean)+2;if(w>TOOLTIP_MAX)w=TOOLTIP_MAX;if(bx+w>acc_cols)w=acc_cols-bx;
  if(w<3)return;
  if(tooltip_owner_same(bx,by,bw)){tooltip_restore();return;}
  if(tooltip_active&&tooltip_x==bx&&tooltip_y==ty&&!strcmp(tooltip_text,clean)){tooltip_owner_x=bx;tooltip_owner_y=by;tooltip_owner_w=bw;if(tooltip_refresh_backing())tooltip_paint();return;}
  tooltip_restore();tooltip_x=bx;tooltip_y=ty;tooltip_w=w;tooltip_owner_x=bx;tooltip_owner_y=by;tooltip_owner_w=bw;strncpy(tooltip_text,clean,40);tooltip_text[40]=0;
  for(i=0;i<w;i++)tooltip_line[i]=*(unsigned short far *)MAKE_FP(saved_video_segment,((ty*acc_cols+bx+i)*2));
  tooltip_arrow_cell=*(unsigned short far *)MAKE_FP(saved_video_segment,(((ty+1)*acc_cols+bx+2)*2));
  tooltip_active=1;tooltip_paint();
}

static void tooltip_dismiss_current(void)
{
  if(tooltip_active){tooltip_dismissed=1;tooltip_dismiss_x=tooltip_owner_x;tooltip_dismiss_y=tooltip_owner_y;tooltip_dismiss_w=tooltip_owner_w;}
  tooltip_restore();
}

static int tooltip_hit(int mx,int my)
{
  if(!tooltip_active)return 0;
  if(my==tooltip_y&&mx>=tooltip_x&&mx<tooltip_x+tooltip_w)return 1;
  return my==tooltip_y+1&&mx==tooltip_x+2;
}

static void tooltip_sync(int mx,int my,int mouse_valid)
{
  int i,target=-1,region=-1,ox=-1,oy=-1,ow=0;char tip[41];
  if(tooltip_suspended||!tooltip_enabled){tooltip_restore();return;}
  if(mouse_valid){
    for(i=tooltip_region_count-1;i>=0;i--)if(tooltip_regions[i].active&&my==tooltip_regions[i].y&&mx>=tooltip_regions[i].x&&mx<tooltip_regions[i].x+tooltip_regions[i].w){region=i;break;}
    if(region<0)for(i=acc_button_count-1;i>=0;i--){if(my==acc_buttons[i].y&&mx>=acc_buttons[i].x&&mx<acc_buttons[i].x+acc_buttons[i].width&&tooltip_button_label(acc_buttons[i].text,tip)){target=i;break;}}
  }
  if(region>=0){ox=tooltip_regions[region].x;oy=tooltip_regions[region].y;ow=tooltip_regions[region].w;if(!tooltip_owner_same(ox,oy,ow))tooltip_dismissed=0;tooltip_show(ox,oy,ow,tooltip_regions[region].text);return;}
  if(target<0&&!acc_focus_suppressed)for(i=acc_button_count-1;i>=0;i--)if(acc_buttons[i].selected&&tooltip_button_label(acc_buttons[i].text,tip)){target=i;break;}
  if(target>=0){ox=acc_buttons[target].x;oy=acc_buttons[target].y;ow=acc_buttons[target].width;if(!tooltip_owner_same(ox,oy,ow))tooltip_dismissed=0;tooltip_button_label(acc_buttons[target].text,tip);tooltip_show(ox,oy,ow,tip);}
  else {tooltip_restore();tooltip_dismissed=0;}
}

static void tooltip_install(void)
{
  int h=acc_font_height();if(!tooltip_enabled||tooltip_glyph_saved)return;
  acc_glyph_read(TOOLTIP_ARROW_GLYPH,tooltip_old_glyph[0]);
  acc_glyph_read(TOOLTIP_RIGHT_GLYPH,tooltip_old_glyph[1]);
  acc_glyph_write(TOOLTIP_ARROW_GLYPH,h==14?tooltip_arrow14:tooltip_arrow16);
  /* Install the rounded left cap in char 203 itself.  Its C0-DF position
     deliberately enables VGA ninth-column line-graphics extension. */
  acc_glyph_write(TOOLTIP_LEFT_GLYPH,h==14?tooltip_left14:tooltip_left16);
  acc_glyph_write(TOOLTIP_RIGHT_GLYPH,h==14?tooltip_right14:tooltip_right16);
  tooltip_glyph_saved=1;
}

static void tooltip_refresh_glyphs(void)
{
  int h;if(!tooltip_glyph_saved)return;
  h=acc_font_height();
  acc_glyph_write(TOOLTIP_ARROW_GLYPH,h==14?tooltip_arrow14:tooltip_arrow16);
  acc_glyph_write(TOOLTIP_LEFT_GLYPH,h==14?tooltip_left14:tooltip_left16);
  acc_glyph_write(TOOLTIP_RIGHT_GLYPH,h==14?tooltip_right14:tooltip_right16);
}

static void tooltip_uninstall(void)
{
  if(!tooltip_glyph_saved)return;
  acc_glyph_write(TOOLTIP_ARROW_GLYPH,tooltip_old_glyph[0]);
  acc_glyph_write(TOOLTIP_RIGHT_GLYPH,tooltip_old_glyph[1]);
  tooltip_glyph_saved=0;
}


int acc_help(int argc,char **argv,const char *name,const char *description)
{
  if(argc>1&&(!stricmp(argv[1],"/?")||!stricmp(argv[1],"-?"))){
    printf("%s - Launch! 3.63 accessory\n\n%s\n\nThis accessory requires !.EXE in the same directory.\n",name,description);return 1;
  }
  return 0;
}

void acc_box(int x,int y,int w,int h,const char *title)
{
  acc36_box_base(x,y,w,h,title);acc_dialog_x=x;acc_dialog_y=y;acc_dialog_w=w;acc_dialog_h=h;
}
void acc_subbox(int x,int y,int w,int h,const char *title,int toolbar)
{
  acc36_subbox_base(x,y,w,h,title,toolbar);acc_dialog_x=x;acc_dialog_y=y;acc_dialog_w=w;acc_dialog_h=h;
}

int acc_begin(const char *argv0,const char *title,int graphics)
{
  int ok;const char *p=argv0,*q;if((q=strrchr(argv0,'\\'))!=0)p=q+1;if((q=strrchr(p,'/'))!=0)p=q+1;strncpy(tooltip_app,p,sizeof(tooltip_app)-1);tooltip_app[sizeof(tooltip_app)-1]=0;strupr(tooltip_app);
  ok=acc36_begin_base(argv0,title,graphics);if(ok){glyph36_refresh();game_digits_saved=0;game_digits_installed=0;game_digits_install();tooltip_load_setting();tooltip_install();tooltip_active=0;tooltip_region_count=0;acc_hover_button=-1;acc_hover_valid=0;acc_dialog_x=acc_dialog_y=-1;acc_dialog_w=acc_dialog_h=0;note_hover_region=-1;acc_focus_suppressed=0;tooltip_dismissed=0;}return ok;
}

void acc_end_screen(void)
{
  /* Clear the live overlay before the canonical backing-store restore.
     Without this, callers that restore the screen explicitly and later call
     acc_end() leave tooltip cells resurrected on the DOS prompt. */
  tooltip_restore();tooltip_active=0;
  acc36_end_screen_base();
}

void acc_restore_screen(void)
{
  tooltip_restore();tooltip_active=0;
  acc36_restore_screen_base();
  glyph36_refresh();
  game_digits_refresh();
  tooltip_refresh_glyphs();
}

void acc_end(void)
{
  tooltip_restore();tooltip_active=0;acc_hover_button=-1;acc_hover_valid=0;
  tooltip_uninstall();
  game_digits_restore();
  acc36_end_base();
}

void acc_button(int x,int y,const char *text,int selected)
{
  int i,a=0,b=0,w,found=-1,hovered;
  w=(acc_button_icon(text,&a,&b)==2)?9:(acc_button_icon(text,&a,&b)?((b<0)?5:6):(int)strlen(text));
  hovered=acc_hover_valid&&acc_hover_y==y&&acc_hover_x>=x&&acc_hover_x<x+w;
  acc_button_draw_state(x,y,text,hovered||(selected&&!acc_focus_suppressed),1);
  for(i=0;i<acc_button_count;i++)if(acc_buttons[i].x==x&&acc_buttons[i].y==y){found=i;break;}
  if(found<0&&acc_button_count<ACC_MAX_BUTTONS)found=acc_button_count++;
  if(found>=0){acc_buttons[found].x=x;acc_buttons[found].y=y;acc_buttons[found].text=text;acc_buttons[found].selected=selected;acc_buttons[found].width=w;if(hovered)acc_hover_button=found;}
}

void acc_modal_begin(void)
{
  tooltip_restore();
  tooltip_suspended=1;
  acc_button_count=0;tooltip_region_count=0;acc_hover_button=-1;acc_hover_valid=0;
}

void acc_modal_end(void)
{
  tooltip_restore();
  tooltip_suspended=0;
  acc_button_count=0;tooltip_region_count=0;acc_hover_button=-1;acc_hover_valid=0;
}

void acc_mouse_display(int show)
{
  union REGS r;
  if(!acc_mouse_present)return;
  if(show){
    if(!mouse_visible){r.x.ax=1;int86(0x33,&r,&r);mouse_visible=1;}
  } else {
    if(mouse_visible){r.x.ax=2;int86(0x33,&r,&r);mouse_visible=0;}
  }
}

static void acc_hover_sync(int mx,int my)
{
  int i,w,newhover=-1;
  acc_hover_x=mx;acc_hover_y=my;acc_hover_valid=1;
  if(acc_hover_button>=acc_button_count)acc_hover_button=-1;
  for(i=0;i<acc_button_count;i++){
    w=acc_buttons[i].width;
    if(my==acc_buttons[i].y&&mx>=acc_buttons[i].x&&mx<acc_buttons[i].x+w){newhover=i;break;}
  }
  if(newhover!=acc_hover_button){
    if(acc_hover_button>=0&&acc_hover_button<acc_button_count)
      acc_button_draw_state(acc_buttons[acc_hover_button].x,acc_buttons[acc_hover_button].y,
        acc_buttons[acc_hover_button].text,acc_buttons[acc_hover_button].selected&&!acc_focus_suppressed,1);
    acc_hover_button=newhover;
    if(acc_hover_button>=0&&acc_hover_button<acc_button_count)
      acc_button_draw_state(acc_buttons[acc_hover_button].x,acc_buttons[acc_hover_button].y,
        acc_buttons[acc_hover_button].text,1,1);
  }
}

int acc_mouse(int *x,int *y,int *buttons)
{
  int r=acc36_mouse_base(x,y,buttons);
  if(acc_mouse_present){canvas_cursor_sync(*x,*y);acc_hover_sync(*x,*y);}
  tooltip_sync(*x,*y,acc_mouse_present);
  if((*buttons&1)&&tooltip_hit(*x,*y)){tooltip_dismiss_current();*buttons=ACC_MOUSE_MOVED;return r;}
  if((*buttons&1)&&tooltip_active)tooltip_dismiss_current();
  if((*buttons&1)&&acc_dialog_w>0&&(*x<acc_dialog_x||*x>=acc_dialog_x+acc_dialog_w||*y<acc_dialog_y||*y>=acc_dialog_y+acc_dialog_h))*buttons|=ACC_MOUSE_OUTSIDE;
  return r;
}

void acc_wait(int *key,int *x,int *y,unsigned *buttons)
{
  union REGS r;int i,w,shown_here=0;
  *key=0;*buttons=0;
  if(acc_mouse_present&&!mouse_visible){
    r.x.ax=1;int86(0x33,&r,&r);mouse_visible=1;shown_here=1;
  }
  if(!acc_mouse_present)tooltip_sync(0,0,0);
  for(;;){
    if(acc_idle_hook)acc_idle_hook();
    if(_bios_keybrd(_KEYBRD_READY)){*key=acc_key();if(*key==9||*key==271)acc_focus_suppressed=0;break;}
    if(acc_mouse_present){
      int nr=-1;
      r.x.ax=3;int86(0x33,&r,&r);mouse_raw_x=r.x.cx;mouse_raw_y=r.x.dx;*x=r.x.cx/8;*y=r.x.dx/8;*buttons=(unsigned)(r.x.bx&~mouse_last_buttons);mouse_last_buttons=r.x.bx;
      canvas_cursor_sync(*x,*y);
      acc_hover_sync(*x,*y);
      tooltip_sync(*x,*y,1);
      if((*buttons&1)&&tooltip_hit(*x,*y)){tooltip_dismiss_current();*buttons=ACC_MOUSE_MOVED;break;}
      if((*buttons&1)&&tooltip_active)tooltip_dismiss_current();
      if(!*buttons&&tooltip_app_is("!NOTE")){for(i=tooltip_region_count-1;i>=0;i--)if(tooltip_regions[i].active&&!strcmp(tooltip_regions[i].text,"Right click to rename")&&*y==tooltip_regions[i].y&&*x>=tooltip_regions[i].x&&*x<tooltip_regions[i].x+tooltip_regions[i].w){nr=i;break;}if(nr!=note_hover_region){note_hover_region=nr;*buttons=ACC_MOUSE_MOVED;break;}}
      if((*buttons&1)&&acc_dialog_w>0&&(*x<acc_dialog_x||*x>=acc_dialog_x+acc_dialog_w||*y<acc_dialog_y||*y>=acc_dialog_y+acc_dialog_h)){tooltip_restore();*buttons=0;*key=27;break;}
      if(*buttons){
        if(*buttons&1){
          int hit=0,at;
          for(i=0;i<acc_button_count;i++){w=acc_buttons[i].width;if(*y==acc_buttons[i].y&&*x>=acc_buttons[i].x&&*x<acc_buttons[i].x+w){hit=1;acc_focus_suppressed=0;tooltip_restore();acc_press_button(acc_buttons[i].x,acc_buttons[i].y,acc_buttons[i].text);break;}}
          if(!hit){at=(int)((*(unsigned short far *)MAKE_FP(saved_video_segment,((*y*acc_cols+*x)*2))>>8)&255);if(at==ACC_BG||at==ACC_LABEL){acc_focus_suppressed=1;tooltip_restore();if(acc_hover_button>=0&&acc_hover_button<acc_button_count)acc_button_draw_state(acc_buttons[acc_hover_button].x,acc_buttons[acc_hover_button].y,acc_buttons[acc_hover_button].text,0,1);acc_hover_button=-1;}else acc_focus_suppressed=0;}
        }
        break;
      }
    }
  }
  if(acc_hover_button>=0&&acc_hover_button<acc_button_count)
    acc_button_draw_state(acc_buttons[acc_hover_button].x,acc_buttons[acc_hover_button].y,
      acc_buttons[acc_hover_button].text,acc_buttons[acc_hover_button].selected,1);
  acc_hover_button=-1;acc_hover_valid=0;
  if(!(*buttons&ACC_MOUSE_MOVED))tooltip_restore();
  if(acc_mouse_present&&shown_here&&mouse_visible){
    r.x.ax=2;int86(0x33,&r,&r);mouse_visible=0;
  }
  acc_button_count=0;
  if((*buttons&1)&&*y==close_y&&(*x==close_x||*x==close_x+1)){*buttons=0;*key=27;}
}

/* Release 3.6 notice dialog.  Keep the canonical layout, but follow the
   suite-wide keyboard model: a modal opens with no focused control and the
   first Tab selects OK. */
void acc_notice(const char *title,const char *message)
{
  const char *p1=message,*p2=0,*p3=0,*n1=strchr(message,'\n'),*n2=0;
  int l1,l2=0,l3=0,lines=1,maxlen,w,h,x,y,k=0,mx=0,my=0,i,j,bx,focus=-1;
  int keep=mouse_visible;
  int old_close_x=close_x,old_close_y=close_y,old_tbx=toolbar_box_x,old_tby=toolbar_box_y,old_tbw=toolbar_box_w,old_tbh=toolbar_box_h;
  int old_dx=acc_dialog_x,old_dy=acc_dialog_y,old_dw=acc_dialog_w,old_dh=acc_dialog_h;
  unsigned mb=0;
  tooltip_restore();acc_modal_begin();
  if(n1){lines=2;p2=n1+1;n2=strchr(p2,'\n');if(n2){lines=3;p3=n2+1;}}
  l1=n1?(int)(n1-p1):(int)strlen(p1);
  if(p2)l2=n2?(int)(n2-p2):(int)strlen(p2);if(p3)l3=(int)strlen(p3);
  maxlen=l1;if(l2>maxlen)maxlen=l2;if(l3>maxlen)maxlen=l3;
  w=maxlen+10;h=6+lines;if(w<54)w=54;if(w>acc_cols-4)w=acc_cols-4;
  x=(acc_cols-w)/2;y=(acc_rows-h)/2;
  for(j=0;j<=h;j++)for(i=0;i<=w;i++)notice_screen[j*(w+1)+i]=*(unsigned short far *)MAKE_FP(saved_video_segment,((y+j)*acc_cols+x+i)*2);
  acc_subbox(x,y,w,h,title,1);acc_message_icon(x+3,y+2,acc_message_type(title,message));
  acc_text(x+7,y+2,p1,ACC_LABEL,l1);if(p2)acc_text(x+7,y+3,p2,ACC_LABEL,l2);if(p3)acc_text(x+7,y+4,p3,ACC_LABEL,l3);
  bx=x+(w-6)/2;
  while(k!=27){
    acc_button(bx,y+h-3,"  OK  ",focus==0);acc_wait(&k,&mx,&my,&mb);
    if((mb&1)&&my==y+h-3&&mx>=bx&&mx<bx+6){focus=0;k=13;}
    if(k==9||k==271){focus=0;k=0;continue;}
    if(k==13&&focus==0)break;
    if(k!=27)k=0;
  }
  for(j=0;j<=h;j++)for(i=0;i<=w;i++)*(unsigned short far *)MAKE_FP(saved_video_segment,((y+j)*acc_cols+x+i)*2)=notice_screen[j*(w+1)+i];
  close_x=old_close_x;close_y=old_close_y;toolbar_box_x=old_tbx;toolbar_box_y=old_tby;toolbar_box_w=old_tbw;toolbar_box_h=old_tbh;
  acc_dialog_x=old_dx;acc_dialog_y=old_dy;acc_dialog_w=old_dw;acc_dialog_h=old_dh;
  acc_modal_end();if(acc_mouse_present&&keep)acc_mouse_display(1);
}
