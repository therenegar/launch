/* Launch! 3.65 accessory runtime extension.
   This translation unit wraps the 3.5 ACCLIB implementation to add the
   Release 3.72 shared accessory extensions without duplicating the shared UI runtime. */
#define acc_begin acc36_begin_base
#define acc_end acc36_end_base
#define acc_end_screen acc36_end_screen_base
#define acc_restore_screen acc36_restore_screen_base
#define acc_restore_text_screen acc36_restore_text_screen_base
#define acc_button acc36_button_base
#define acc_box acc36_box_base
#define acc_subbox acc36_subbox_base
#define acc_mouse acc36_mouse_base
#define acc_key_ready acc36_key_ready_base
#define acc_wait acc36_wait_base
#define acc_notice acc36_notice_base
#define acc_help acc36_help_base
#include "ACCLIB.C"
#undef acc_begin
#undef acc_end
#undef acc_end_screen
#undef acc_restore_screen
#undef acc_restore_text_screen
#undef acc_button
#undef acc_box
#undef acc_subbox
#undef acc_mouse
#undef acc_key_ready
#undef acc_wait
#undef acc_notice
#undef acc_help

#ifdef ACCLIB_MIN_GLYPHS
static const unsigned char stack_add_a16[32]={0x00,0x00,0x01,0x01,0x01,0x01,0x3F,0x3F,0x3F,0x01,0x1D,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_add_a14[32]={0x00,0x01,0x01,0x01,0x01,0x3F,0x3F,0x3F,0x01,0x1D,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_add_b16[32]={0x00,0x00,0x80,0x80,0x80,0x80,0xFC,0xFC,0xFC,0x80,0xBE,0xA0,0xA0,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_add_b14[32]={0x00,0x80,0x80,0x80,0x80,0xFC,0xFC,0xFC,0x80,0xBE,0xA0,0xA0,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_delete_a16[32]={0x00,0x00,0x03,0x1F,0x20,0x3F,0x10,0x12,0x16,0x14,0x14,0x12,0x10,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_delete_a14[32]={0x00,0x03,0x1F,0x20,0x3F,0x10,0x12,0x16,0x14,0x14,0x12,0x10,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_delete_b16[32]={0x00,0x00,0xC0,0xF8,0x0C,0xF8,0x08,0x48,0x68,0x28,0x28,0x48,0x08,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_delete_b14[32]={0x00,0xC0,0xF8,0x0C,0xF8,0x08,0x48,0x28,0x28,0x68,0x48,0x08,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_export_a16[32]={0x00,0x00,0xF8,0x94,0x92,0x92,0xF2,0x02,0xF2,0x02,0xF2,0x02,0xFE,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_export_a14[32]={0x00,0xF8,0x94,0x92,0x92,0xF2,0x02,0xF2,0x02,0xF2,0x02,0xFE,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_export_b16[32]={0x00,0x00,0x1F,0x23,0x23,0x23,0x23,0x20,0x27,0x20,0x27,0x20,0x3F,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char stack_export_b14[32]={0x00,0x1F,0x23,0x23,0x23,0x23,0x20,0x27,0x20,0x27,0x20,0x3F,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
#endif

static int acc_dialog_x=-1,acc_dialog_y=-1,acc_dialog_w=0,acc_dialog_h=0;
static int acc_focus_suppressed=0;
static int acc_hover_button=-1,acc_hover_x=-1,acc_hover_y=-1,acc_hover_valid=0;
static void glyph36_refresh(void);
static void game_digits_refresh(void);
static void (*acc_idle_hook)(void)=0;
static unsigned char maximize_old[2][32];static int maximize_saved=0;
static char acc_app[20];
void acc_set_idle_hook(void (*fn)(void)){acc_idle_hook=fn;}
static int acc_app_is(const char *name){return strstr(acc_app,name)!=0;}
static void maximize_install(void){
#ifndef ACCLIB_MIN_GLYPHS
 if(!maximize_saved){acc_glyph_read(ACC_MAXIMIZE_L,maximize_old[0]);acc_glyph_read(ACC_MAXIMIZE_R,maximize_old[1]);maximize_saved=1;}
 acc_glyph_library(59,ACC_MAXIMIZE_L);acc_glyph_library(60,ACC_MAXIMIZE_R);
#endif
}
static void maximize_restore(void){
#ifndef ACCLIB_MIN_GLYPHS
 if(maximize_saved){acc_glyph_write(ACC_MAXIMIZE_L,maximize_old[0]);acc_glyph_write(ACC_MAXIMIZE_R,maximize_old[1]);maximize_saved=0;}
#endif
}
/* Release 3.63 keeps the configured mouse pointer unchanged in !DRAW. */
void acc_canvas_cursor_region(int x,int y,int w,int h,int colour)
{(void)x;(void)y;(void)w;(void)h;(void)colour;}
static void canvas_cursor_sync(int mx,int my){(void)mx;(void)my;}
void acc_mouse_reapply_cursor(void)
{
  union REGS r;if(!acc_mouse_present)return;
  mouse_pointer_restore();mouse_pointer_install();
  r.x.ax=1;int86(0x33,&r,&r);mouse_visible=1;
}

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
  /* Floppy source halves are logical 10/26; logical 26 is the extending
     left half, logical 10 is the right half. */
  acc_glyph_write(204,h==14?stack_export_b14:stack_export_b16);
  acc_glyph_write(181,h==14?stack_export_a14:stack_export_a16);
#endif
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
  return acc_app_is("!BOXES")||acc_app_is("!SOL")||
         acc_app_is("!FCELL")||acc_app_is("!POP")||
         acc_app_is("!SNAKE")||acc_app_is("!PLUMB")||
         acc_app_is("!WORDZ");
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

/* Automatic hover hints were removed in 3.72. See TOOLTIP.TXT for the
   retained transient-message visual design. */
int acc_help(int argc,char **argv,const char *name,const char *description)
{
  if(argc>1&&(!stricmp(argv[1],"/?")||!stricmp(argv[1],"-?"))){
    printf("%s - Launch! 3.65 accessory\n\n%s\n\nThis accessory requires !.EXE in the same directory.\n",name,description);return 1;
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
/* Change only the mouse input boundary; used by true full-screen accessory
   views whose live controls intentionally extend beyond the normal dialog. */
void acc_input_bounds(int x,int y,int w,int h)
{
  acc_dialog_x=x;acc_dialog_y=y;acc_dialog_w=w;acc_dialog_h=h;
}

int acc_begin(const char *argv0,const char *title,int graphics)
{
  int ok;const char *p=argv0,*q;if((q=strrchr(argv0,'\\'))!=0)p=q+1;if((q=strrchr(p,'/'))!=0)p=q+1;strncpy(acc_app,p,sizeof(acc_app)-1);acc_app[sizeof(acc_app)-1]=0;strupr(acc_app);
  ok=acc36_begin_base(argv0,title,graphics);if(ok){glyph36_refresh();maximize_install();game_digits_saved=0;game_digits_installed=0;game_digits_install();acc_hover_button=-1;acc_hover_valid=0;acc_dialog_x=acc_dialog_y=-1;acc_dialog_w=acc_dialog_h=0;acc_focus_suppressed=0;}return ok;
}

void acc_end_screen(void)
{
  acc_caret_hide();

  acc36_end_screen_base();
}

void acc_restore_text_screen(void)
{
  acc_caret_hide();
  /* Text-only Focus/Maximize return path: preserve the active text mode and
     custom font/glyph state. Only restore the saved text backing store. */
  acc36_restore_text_screen_base();
  acc_hover_button=-1;acc_hover_valid=0;
}

void acc_restore_screen(void)
{
  acc_caret_hide();
  acc36_restore_screen_base();
  glyph36_refresh();
  maximize_install();
  game_digits_refresh();
}

void acc_end(void)
{
  acc_caret_hide();
  game_digits_restore();
  maximize_restore();
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
  acc_caret_hide();
  acc_button_count=0;acc_hover_button=-1;acc_hover_valid=0;
}

void acc_modal_end(void)
{
  /* A modal may close while its last focused control is a text field.
     Kill the BIOS caret before the backing UI is exposed again so the
     underscore cannot survive at the old field coordinates. */
  acc_caret_hide();
  acc_button_count=0;acc_hover_button=-1;acc_hover_valid=0;
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
  if((*buttons&1)&&acc_dialog_w>0&&(*x<acc_dialog_x||*x>=acc_dialog_x+acc_dialog_w||*y<acc_dialog_y||*y>=acc_dialog_y+acc_dialog_h))*buttons|=ACC_MOUSE_OUTSIDE;
  return r;
}

int acc_key_ready(void){return acc36_key_ready_base();}

void acc_wait(int *key,int *x,int *y,unsigned *buttons)
{
  union REGS r;int i,w,shown_here=0;
  *key=0;*buttons=0;
  if(acc_mouse_present&&!mouse_visible){
    r.x.ax=1;int86(0x33,&r,&r);mouse_visible=1;shown_here=1;
  }
  for(;;){
    if(acc_idle_hook)acc_idle_hook();
    if(acc_key_ready()){
      *key=acc_key();acc_caret_hide();if(*key==9||*key==271)acc_focus_suppressed=0;
      if(*key==17||*key==256+0x6B){*key=27;break;} /* Ctrl+Q / Alt+F4 */
      {const char *want=0;
       if(*key==15)want="Open";else if(*key==5)want="Edit";else if(*key==19)want="Save";
       else if(*key==4)want="Delete";else if(*key==16)want="Print";
       else if(*key==256+0x93)want="Clr";else if(*key==256+0x92)want="\001";
       if(want){for(i=0;i<acc_button_count;i++)if(strstr(acc_buttons[i].text,want)){
         *x=acc_buttons[i].x;*y=acc_buttons[i].y;*buttons=1;acc_focus_suppressed=0;
         acc_press_button(acc_buttons[i].x,acc_buttons[i].y,acc_buttons[i].text);*key=0;break;}
         if(*buttons)break;
       }
      }
      break;
    }
    if(acc_mouse_present){
      int nr=-1;
      r.x.ax=3;int86(0x33,&r,&r);mouse_raw_x=r.x.cx;mouse_raw_y=r.x.dx;*x=r.x.cx/8;*y=r.x.dx/8;*buttons=(unsigned)(r.x.bx&~mouse_last_buttons);mouse_last_buttons=r.x.bx;
      canvas_cursor_sync(*x,*y);
      acc_hover_sync(*x,*y);
      if((*buttons&1)&&acc_dialog_w>0&&(*x<acc_dialog_x||*x>=acc_dialog_x+acc_dialog_w||*y<acc_dialog_y||*y>=acc_dialog_y+acc_dialog_h)){*buttons=0;*key=27;break;}
      if(*buttons){
        acc_caret_hide();
        if(*buttons&1){
          int hit=0,at;
          for(i=0;i<acc_button_count;i++){w=acc_buttons[i].width;if(*y==acc_buttons[i].y&&*x>=acc_buttons[i].x&&*x<acc_buttons[i].x+w){hit=1;acc_focus_suppressed=0;acc_press_button(acc_buttons[i].x,acc_buttons[i].y,acc_buttons[i].text);break;}}
          if(!hit){at=(int)((*(unsigned short far *)MAKE_FP(saved_video_segment,((*y*acc_cols+*x)*2))>>8)&255);if(at==ACC_BG||at==ACC_LABEL){acc_focus_suppressed=1;if(acc_hover_button>=0&&acc_hover_button<acc_button_count)acc_button_draw_state(acc_buttons[acc_hover_button].x,acc_buttons[acc_hover_button].y,acc_buttons[acc_hover_button].text,0,1);acc_hover_button=-1;}else acc_focus_suppressed=0;}
        }
        break;
      }
    }
  }
  if(acc_hover_button>=0&&acc_hover_button<acc_button_count)
    acc_button_draw_state(acc_buttons[acc_hover_button].x,acc_buttons[acc_hover_button].y,
      acc_buttons[acc_hover_button].text,acc_buttons[acc_hover_button].selected,1);
  acc_hover_button=-1;acc_hover_valid=0;
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
  acc_modal_begin();
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
