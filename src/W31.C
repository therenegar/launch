/* Launch! 3.79 - Windows 3.1/3.11 menu companion.
   Target: Microsoft C/C++ 7.0 + Windows 3.0/3.1 SDK, medium model. */
#ifndef WINVER
#define WINVER 0x0300
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "WINCLOCK.H"
static int close_request=0;
#include <commdlg.h>
#include <dde.h>

#define MAX_NODES 128
#define MAX_SECTIONS 32  /* Keep Win16 medium-model DGROUP within the 64K limit. */
#define MAX_LINE 256
#define IDM_FIRST 100
#define IDM_RUN 899
#define IDM_EXPLORE 900
#define IDM_EXITWIN 901
#define IDD_RUN 1000
#define IDD_LAUNCHER 1001
#define IDD_FOLDER 1002
#define IDD_CONFIRM 1003
#define IDD_EXITWIN 1004
#define IDD_MESSAGE 1005
#define IDD_MANAGER 1006
#define CONFIRM_TEXT 70
#define CONFIRM_ICON 71
#define MESSAGE_TEXT 72
#define MESSAGE_ERROR_ICON 73
#define MESSAGE_SUCCESS_ICON 74
#define MESSAGE_CONFIRM_ICON 75
#define IDI_SLEEP 1100
#define IDI_CONFIRM 1101
#define IDI_ERROR 1102
#define IDI_SUCCESS 1103
#define IDI_WINPROG 1104
#define IDI_DOSPROG 1105
#define IDB_MANAGER_LOGO 1200
#define IDB_MENU_LOGO 1201
#define IDB_MENU_TILE 1202
#define IDB_MANAGER_TILE 1203
#define IDM_MENU_BANNER 880
#define RUN_EDIT 20
#define RUN_OK IDOK
#define RUN_CANCEL IDCANCEL
#define RUN_BROWSE 23
#define RUN_MIN 24
#define BTN_ID 1
#define RAISE_TIMER 1
#define WM_SHOW_LAUNCH_MENU (WM_USER+16)
#define WM_MANAGE_ACTION (WM_USER+17)
#define WM_MANAGE_SELECTED (WM_USER+18)
#define WM_BEGIN_BUTTON_DRAG (WM_USER+19)
#define WM_SHOW_MENU_MANAGER (WM_USER+20)
#define WM_MANAGER_ACTION (WM_USER+21)
#define WM_OPEN_PENDING_MENU (WM_USER+22)
#ifndef TPM_BOTTOMALIGN
#define TPM_BOTTOMALIGN 0x0020
#endif

/* Windows 3.0 requires TrackPopupMenu wFlags to be zero.  Windows 3.1
   introduced the popup alignment flags; use the bottom-alignment bit only
   when that later USER implementation is actually running. */
static int launch_win31_or_later(void)
{
  DWORD v=GetVersion();
  BYTE major=LOBYTE(LOWORD(v));
  BYTE minor=HIBYTE(LOWORD(v));
  return (major>3 || (major==3 && minor>=10));
}
#ifndef VK_OEM_5
#define VK_OEM_5 0xDC
#endif
#ifndef SC_HOTKEY
#define SC_HOTKEY 0xF150
#endif
#define IDM_MGMT_ADD_LAUNCHER 920
#define IDM_MGMT_ADD_FOLDER 921
#define IDM_MGMT_ADD_SEPARATOR 922
#define IDM_MGMT_EDIT 923
#define IDM_MGMT_REMOVE 924
#define IDM_MGMT_REORDER 925
#define IDM_EDIT_FIRST 1200
#define IDM_REMOVE_FIRST 1400

#define MANAGE_NONE 0
#define MANAGE_EDIT 1
#define MANAGE_REMOVE 2
#define MANAGE_MENU 3

#define MSG_ERROR 1
#define MSG_WARNING 2
#define MSG_INFO 3
#define MSG_SUCCESS 4

#define ITEM_NAME 40
#define ITEM_COMMAND 41
#define ITEM_PARAMS 42
#define ITEM_PROMPT 43
#define ITEM_ENTER 44
#define ITEM_CHDIR 45
#define ITEM_ADDPATH 46
#define ITEM_OK IDOK
#define ITEM_CANCEL IDCANCEL
#define ITEM_BROWSE 49
#define ITEM_DOSICON 50
#define ITEM_WINICON 51

#define MANAGER_PATH 80
#define MANAGER_BROWSE 81
#define MANAGER_TREE 82
#define MANAGER_ADD 83
#define MANAGER_EDIT 84
#define MANAGER_REMOVE 85
#define MANAGER_UP 86
#define MANAGER_DOWN 87
#define MANAGER_BANNER 88
#define MANAGER_CLOSE IDCANCEL
#define IDM_MANAGER_ADD_LAUNCHER 940
#define IDM_MANAGER_ADD_FOLDER 941
#define IDM_MANAGER_ADD_SEPARATOR 942

typedef struct {
  char title[48];
  char command[160];
  int section_index;
  int child_section_index;
  int is_folder;
  int enter_after;
  int change_dir;
  int prompt;
  int add_path;
  int windows_program;
} MENU_NODE;

typedef struct { char name[128]; HMENU menu; } SECTION;

typedef struct {
  int node_index;
  int depth;
  int is_windows;
  unsigned long branch_mask;
  int has_next;
} MANAGER_ROW;

static HINSTANCE gInst;
static HWND gWnd, gButton, runWnd, runEdit, runMin;
static HBRUSH gWindowBrush;
static HFONT gDialogFont,gButtonFont;
static MENU_NODE nodes[MAX_NODES];
static int node_count;
static SECTION sections[MAX_SECTIONS];
static int section_count;
static char base_dir[144];
static char menu_path[160];
static int menu_path_explicit;
static int start_menu_manager;
static char winrun_path[160];
static char launch_cfg_path[160];
static char suite_title[32]="Launch!";
static int menu_open_pending=0;
static int menu_open_delay=0;
static char win16_ini_path[160];
static HMENU root_menu;
static int menu_tracking;
static int win_accent_index=12;
static int win_launcher_index=10;
static int button_x=24;
static int button_edge=0; /* 0=top, 1=bottom */
static int manage_mode=MANAGE_NONE;
static HWND itemWnd,itemName,itemCommand,itemParams,itemPrompt,itemEnter,itemChdir,itemAddpath;
static int item_dialog_done,item_dialog_ok,item_dialog_folder,item_dialog_editing,item_dialog_node;
static int item_dialog_section;
static MENU_NODE item_work;
static char item_exe[160],item_params[160];
static char confirm_title[48],confirm_text[180];
static int confirm_dialog_id=IDD_CONFIRM;
static char message_title[48],message_text[220];
static int message_kind;
static HWND managerWnd,managerTree,managerPath,managerBanner;
static MANAGER_ROW manager_rows[MAX_NODES];
static int manager_row_count;
static unsigned char manager_expanded[MAX_NODES];
static char manager_original_path[160];
static FARPROC gManagerTreeProc;
static FARPROC gManagerBannerProc;
/* Win16 USER callbacks must carry the correct instance data segment,
   particularly on Windows 3.0.  Keep a MakeProcInstance thunk separate
   from the original control procedure returned by SetWindowLong. */
static FARPROC gManagerTreeInstProc;
static FARPROC gManagerBannerInstProc;
static HBITMAP gManagerLogoBmp,gManagerTileBmp;
static HBITMAP gMenuLogoBmp,gMenuTileBmp;
static HWND dde_server;
static int dde_initiating;
static int dde_waiting;
static int dde_request_ok;
static HGLOBAL dde_reply;
static FARPROC gButtonProc;
static FARPROC gButtonInstProc;
static FARPROC gHostInstProc;
static int gWin30;
static FILE *gDebugFile;

static void debug_open(void)
{
  char exe[160],path[180],*q;
  exe[0]=0;path[0]=0;
  if(GetModuleFileName(gInst,exe,sizeof(exe))){
    exe[sizeof(exe)-1]=0;q=strrchr(exe,'\\');
    if(q){*q=0;sprintf(path,"%s\\W31DBG.LOG",exe);}
  }
  if(!path[0])strcpy(path,"W31DBG.LOG");
  gDebugFile=fopen(path,"wt");
}

static void debug_msg(const char *s)
{
  if(gDebugFile){fprintf(gDebugFile,"%s\n",s);fflush(gDebugFile);}
}

static void debug_val(const char *s,unsigned long v)
{
  if(gDebugFile){fprintf(gDebugFile,"%s: %lu (0x%lX)\n",s,v,v);fflush(gDebugFile);}
}
static void resize_button_for_mode(void);
static int browse_for_program(HWND owner,char *file,int maxfile);
static int browse_for_menu(HWND owner,char *file,int maxfile);
static int show_confirm_dialog(const char *title,const char *text);
static int show_exit_windows_dialog(void);
static void show_message_dialog(HWND owner,const char *title,const char *text,int kind);
static int ensure_menu_header_bitmaps(void);
static int measure_menu_header(MEASUREITEMSTRUCT FAR *mis);
static int draw_menu_header(DRAWITEMSTRUCT FAR *dis);
static void transparent_stretch_bitmap(HDC dc,HBITMAP bmp,int x,int y,int w,int h,COLORREF key);
static void transparent_tile_bitmap(HDC dc,HBITMAP bmp,int x,int y,int w,COLORREF key);
static void opaque_stretch_bitmap(HDC dc,HBITMAP bmp,int x,int y,int w,int h);
static void opaque_tile_bitmap(HDC dc,HBITMAP bmp,int x,int y,int w);
static int ensure_menu_file(void);
static int copy_file_local(const char *src,const char *dst);
static LRESULT FAR PASCAL ButtonProc(HWND h,UINT msg,WPARAM wp,LPARAM lp);
static LRESULT FAR PASCAL ManagerTreeProc(HWND h,UINT msg,WPARAM wp,LPARAM lp);

static void trim(char *s)
{
  char *p=s; int n;
  while(*p==' '||*p=='\t')p++;
  if(p!=s)memmove(s,p,strlen(p)+1);
  n=strlen(s);while(n>0&&(s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\r'||s[n-1]=='\n'))s[--n]=0;
}

static int get_base_dir(void)
{
  char exe[160],*q;int n;
  exe[0]=0;n=GetModuleFileName(gInst,exe,sizeof(exe));
  if(!n || n>=(int)sizeof(exe)){
    #ifdef MNUMAN_BUILD
    MessageBox(NULL,"Windows could not determine the !MNUMAN.EXE path.","Menu Manager",MB_OK|MB_ICONSTOP);
#else
    MessageBox(NULL,"Windows could not determine the !W31.EXE path.","Launch!",MB_OK|MB_ICONSTOP);
#endif
    return 0;
  }
  exe[sizeof(exe)-1]=0;
  q=strrchr(exe,'\\');
  if(!q){
    #ifdef MNUMAN_BUILD
    MessageBox(NULL,"Windows returned !MNUMAN.EXE without a directory path.","Menu Manager",MB_OK|MB_ICONSTOP);
#else
    MessageBox(NULL,"Windows returned !W31.EXE without a directory path.","Launch!",MB_OK|MB_ICONSTOP);
#endif
    return 0;
  }
  *q=0;
  strcpy(base_dir,exe);
  sprintf(menu_path,"%s\\LAUNCH.MNU",base_dir);
  sprintf(winrun_path,"%s\\WINRUN.BAT",base_dir);
  sprintf(launch_cfg_path,"%s\\LAUNCH.CFG",base_dir);
  sprintf(win16_ini_path,"%s\\LAUNCH16.INI",base_dir);
  return 1;
}

static int is_menu_filename(const char *s)
{
  const char *dot=strrchr(s,'.');
  return dot && !stricmp(dot,".MNU");
}

static void apply_menu_argument(const char *arg)
{
  if(!arg||!arg[0]||!is_menu_filename(arg))return;
  menu_path_explicit=1;
  if(strchr(arg,':')||arg[0]=='\\')strncpy(menu_path,arg,sizeof(menu_path)-1);
  else sprintf(menu_path,"%s\\%s",base_dir,arg);
  menu_path[sizeof(menu_path)-1]=0;
}

static void set_menu_path_from_command_line(LPSTR cmd)
{
  char tok[160];const char far *p=cmd;int n;
  menu_path_explicit=0;start_menu_manager=0;
  while(p&&*p){
    while(*p==' '||*p=='\t')p++;
    if(!*p)break;
    n=0;while(*p&&*p!=' '&&*p!='\t'&&n<(int)sizeof(tok)-1)tok[n++]=*p++;
    tok[n]=0;trim(tok);if(!tok[0])continue;
    /* Win16 remains deliberately 8.3-only: no quote parsing is introduced. */
    if(!stricmp(tok,"/CLOCK")||!stricmp(tok,"-CLOCK")){clock_mode=1;clock_locale();clock_update();continue;}
    if(!stricmp(tok,"/CLOSE")||!stricmp(tok,"-CLOSE")){close_request=1;continue;}
    if(!stricmp(tok,"/MANAGE")||!stricmp(tok,"-MANAGE")||!stricmp(tok,"MANAGE")){start_menu_manager=1;continue;}
    if(!strnicmp(tok,"/USE=",5)||!strnicmp(tok,"-USE=",5)){apply_menu_argument(tok+5);continue;}
    apply_menu_argument(tok);
  }
}

static void menu_sidecar_path(char *out,const char *ext)
{
  char *slash,*dot;
  strncpy(out,menu_path,179);out[179]=0;
  slash=strrchr(out,'\\');dot=strrchr(out,'.');
  if(dot && (!slash||dot>slash))*dot=0;
  strncat(out,ext,179-strlen(out));
}

static void load_win_preferences(void)
{
  FILE *f;char line[96],*eq;int v;
  win_accent_index=12;win_launcher_index=10;strcpy(suite_title,"Launch!");
  f=fopen(launch_cfg_path,"rt");
  if(f){
    while(fgets(line,sizeof(line),f)){
      trim(line);
      if(!line[0]||line[0]==';'||line[0]=='#')continue;
      eq=strchr(line,'=');if(!eq)continue;*eq++=0;trim(line);trim(eq);
      if(!stricmp(line,"suiteTitle")){if(*eq){strncpy(suite_title,eq,sizeof(suite_title)-1);suite_title[sizeof(suite_title)-1]=0;}}
      else if(!stricmp(line,"MAIN_TITLE")){v=atoi(eq);if(v>=0&&v<=15)win_accent_index=v;}
      else if(!stricmp(line,"LAUNCHERS")){v=atoi(eq);if(v>=0&&v<=15)win_launcher_index=v;}
    }
    fclose(f);
  }
  button_x=(int)GetPrivateProfileInt("Button","X",24,win16_ini_path);
  button_edge=(int)GetPrivateProfileInt("Button","Edge",0,win16_ini_path)?1:0;
}

static void save_button_position(void)
{
  char value[16];
  sprintf(value,"%d",button_x);
  WritePrivateProfileString("Button","X",value,win16_ini_path);
  sprintf(value,"%d",button_edge);
  WritePrivateProfileString("Button","Edge",value,win16_ini_path);
}

static int find_section(const char *name)
{
  int i;for(i=0;i<section_count;i++)if(!stricmp(sections[i].name,name))return i;return -1;
}
static int ensure_section(const char *name)
{
  int i=find_section(name);if(i>=0)return i;
  if(section_count>=MAX_SECTIONS)return -1;
  strncpy(sections[section_count].name,name,sizeof(sections[0].name)-1);
  sections[section_count].name[sizeof(sections[0].name)-1]=0;
  sections[section_count].menu=NULL;
  return section_count++;
}

static int find_folder_node(int section_index,const char *title)
{
  int i;
  for(i=0;i<node_count;i++)
    if(nodes[i].section_index==section_index && nodes[i].is_folder &&
       !stricmp(nodes[i].title,title))return i;
  return -1;
}

static int ensure_folder_node(int section_index,const char *title,int child_section_index)
{
  int i=find_folder_node(section_index,title);MENU_NODE *n;
  if(i>=0){nodes[i].child_section_index=child_section_index;return i;}
  if(node_count>=MAX_NODES)return -1;
  n=&nodes[node_count++];memset(n,0,sizeof(*n));n->is_folder=1;
  strncpy(n->title,title,sizeof(n->title)-1);n->title[sizeof(n->title)-1]=0;trim(n->title);
  n->section_index=section_index;n->child_section_index=child_section_index;
  return node_count-1;
}

/* Match the DOS menu loader's useful tolerance for section-only folder trees.
   A long-lived LAUNCH.MNU may contain [Launcher\Folder] sections without a
   separate FOLDER=Folder record at the parent.  DOS reconstructs that parent
   automatically; Win16 must do the same or the native root menu appears empty. */
static int ensure_section_tree(const char *name)
{
  char work[128],full[128],*p,*q;int parent,child;
  strncpy(work,name,sizeof(work)-1);work[sizeof(work)-1]=0;trim(work);
  if(strnicmp(work,"Launcher",8) || (work[8] && work[8]!='\\'))return ensure_section(work);
  strcpy(full,"Launcher");parent=ensure_section(full);if(parent<0)return -1;
  p=work+8;if(*p=='\\')p++;
  while(*p){
    q=strchr(p,'\\');if(q)*q=0;trim(p);if(!*p)return -1;
    if(strlen(full)+strlen(p)+2>=sizeof(full))return -1;
    strcat(full,"\\");strcat(full,p);
    child=ensure_section(full);if(child<0)return -1;
    if(ensure_folder_node(parent,p,child)<0)return -1;
    parent=child;if(!q)break;p=q+1;
  }
  return parent;
}

static int split_item(char *s,char *f[],int maxf)
{
  int n=0;char *p=s; if(maxf<1)return 0;f[n++]=p;
  while(*p&&n<maxf){if(*p=='|'){*p=0;f[n++]=p+1;}p++;}return n;
}

static int load_launch_menu(void)
{
  FILE *f;char line[MAX_LINE],section[128]="Launcher";int section_index;
  node_count=0;section_count=0;
  f=fopen(menu_path,"rt");
  if(!f)return 0;
  section_index=ensure_section("Launcher");
  while(fgets(line,sizeof(line),f)){
    char *fields[7];int nf;MENU_NODE *n;
    trim(line);if(!line[0]||line[0]==';')continue;
    if(line[0]=='['){char *r=strchr(line,']');if(r){*r=0;strncpy(section,line+1,sizeof(section)-1);section[sizeof(section)-1]=0;trim(section);section_index=ensure_section_tree(section);}continue;}
    if(node_count>=MAX_NODES)continue;
    if(!strnicmp(line,"FOLDER=",7)){
      char title[48],child[128];int child_index;
      strncpy(title,line+7,sizeof(title)-1);title[sizeof(title)-1]=0;trim(title);
      if(!title[0] || section_index<0)continue;
      if(strlen(section)+strlen(title)+2>=sizeof(child))continue;
      strcpy(child,section);strcat(child,"\\");strcat(child,title);
      child_index=ensure_section(child);
      if(child_index>=0)ensure_folder_node(section_index,title,child_index);
    } else if(!strnicmp(line,"ITEM=",5)){
      n=&nodes[node_count++];memset(n,0,sizeof(*n));
      nf=split_item(line+5,fields,7);if(nf<2){node_count--;continue;}
      strncpy(n->title,fields[0],sizeof(n->title)-1);strncpy(n->command,fields[1],sizeof(n->command)-1);
      n->section_index=section_index;
      if(nf>2)n->enter_after=atoi(fields[2]);if(nf>3)n->change_dir=atoi(fields[3]);
      if(nf>4)n->prompt=atoi(fields[4]);if(nf>5)n->add_path=atoi(fields[5]);
      if(nf>6&&!stricmp(fields[6],"W"))n->windows_program=1;
    } else if(!strnicmp(line,"SEPARATOR=",10)||!stricmp(line,"SEPARATOR")){
      n=&nodes[node_count++];memset(n,0,sizeof(*n));strcpy(n->title,"-");n->section_index=section_index;
    }
  }
  fclose(f);return 1;
}

static HMENU menu_for_section(const char *name){int i=find_section(name);return i>=0?sections[i].menu:NULL;}
static int is_windows_exe(const char *cmd);

static int section_has_nodes(int section_index)
{
  int i;
  for(i=0;i<node_count;i++)if(nodes[i].section_index==section_index)return 1;
  return 0;
}

static int ensure_menu_header_bitmaps(void)
{
  if(!gMenuLogoBmp)gMenuLogoBmp=LoadBitmap(gInst,MAKEINTRESOURCE(IDB_MENU_LOGO));
  if(!gMenuTileBmp)gMenuTileBmp=LoadBitmap(gInst,MAKEINTRESOURCE(IDB_MENU_TILE));
  return gMenuLogoBmp&&gMenuTileBmp;
}

static int measure_menu_header(MEASUREITEMSTRUCT FAR *mis)
{
  BITMAP bm_logo,bm_tile;
  if(!mis || mis->CtlType!=ODT_MENU || mis->itemID!=IDM_MENU_BANNER)return 0;
  if(!ensure_menu_header_bitmaps())return 0;
  GetObject(gMenuLogoBmp,sizeof(bm_logo),&bm_logo);
  GetObject(gMenuTileBmp,sizeof(bm_tile),&bm_tile);
  /* MENULOGO.BMP establishes only the minimum menu width.  Normal menu
     contents remain free to make the popup wider; MENUTILE.BMP is repeated
     horizontally to the natural right edge. */
  mis->itemWidth=(UINT)bm_logo.bmWidth;
  mis->itemHeight=(UINT)(bm_logo.bmHeight>bm_tile.bmHeight?bm_logo.bmHeight:bm_tile.bmHeight);
  return 1;
}

static int draw_menu_header(DRAWITEMSTRUCT FAR *dis)
{
  BITMAP bm_logo,bm_tile;int x,y,remaining;
  if(!dis || dis->CtlType!=ODT_MENU || dis->itemID!=IDM_MENU_BANNER)return 0;
  if(!ensure_menu_header_bitmaps())return 1;
  GetObject(gMenuLogoBmp,sizeof(bm_logo),&bm_logo);
  GetObject(gMenuTileBmp,sizeof(bm_tile),&bm_tile);
  y=dis->rcItem.top;
  /* Menu artwork is intentionally opaque: preserve the grey background
     designed into MENULOGO.BMP / MENUTILE.BMP rather than colour-keying it. */
  opaque_stretch_bitmap(dis->hDC,gMenuLogoBmp,dis->rcItem.left,y,bm_logo.bmWidth,bm_logo.bmHeight);
  x=dis->rcItem.left+bm_logo.bmWidth;remaining=dis->rcItem.right-x;
  if(remaining>0)opaque_tile_bitmap(dis->hDC,gMenuTileBmp,x,y,remaining);
  return 1;
}

static void build_native_menu(void)
{
  int i;HMENU m,sub;
  /* Create a fresh native USER menu tree for each invocation. */
  for(i=0;i<section_count;i++)sections[i].menu=CreatePopupMenu();
  for(i=0;i<node_count;i++){
    m=(nodes[i].section_index>=0&&nodes[i].section_index<section_count)?sections[nodes[i].section_index].menu:NULL;if(!m)continue;
    if(!strcmp(nodes[i].title,"-")){
      if(manage_mode==MANAGE_REMOVE)AppendMenu(m,MF_STRING,IDM_REMOVE_FIRST+i,"[Separator]");
      else AppendMenu(m,MF_SEPARATOR,0,NULL);
      continue;
    }
    if(nodes[i].is_folder){
      sub=(nodes[i].child_section_index>=0&&nodes[i].child_section_index<section_count)?sections[nodes[i].child_section_index].menu:NULL;
      if(sub){
        if(manage_mode==MANAGE_EDIT){
          AppendMenu(sub,MF_STRING,IDM_EDIT_FIRST+i,"Edit this folder...");
          if(section_has_nodes(nodes[i].child_section_index))AppendMenu(sub,MF_SEPARATOR,0,NULL);
        } else if(manage_mode==MANAGE_REMOVE){
          AppendMenu(sub,MF_STRING,IDM_REMOVE_FIRST+i,"Remove this folder...");
          if(section_has_nodes(nodes[i].child_section_index))AppendMenu(sub,MF_SEPARATOR,0,NULL);
        }
        AppendMenu(m,MF_POPUP,(UINT)sub,nodes[i].title);
      }
    }
    else {
      char label[52];UINT id;
      strncpy(label,nodes[i].title,sizeof(label)-1);label[sizeof(label)-1]=0;
      id=(manage_mode==MANAGE_EDIT)?(IDM_EDIT_FIRST+i):
         (manage_mode==MANAGE_REMOVE)?(IDM_REMOVE_FIRST+i):(IDM_FIRST+i);
      AppendMenu(m,MF_STRING,id,label);
    }
  }
  root_menu=menu_for_section("Launcher");
  if(root_menu && ensure_menu_header_bitmaps())
    InsertMenu(root_menu,0,MF_BYPOSITION|MF_OWNERDRAW|MF_DISABLED,IDM_MENU_BANNER,NULL);
  if(root_menu && manage_mode==MANAGE_NONE){
    AppendMenu(root_menu,MF_SEPARATOR,0,NULL);
    AppendMenu(root_menu,MF_STRING,IDM_RUN,"&Run...");
    AppendMenu(root_menu,MF_STRING,IDM_EXPLORE,"&Explore");
    AppendMenu(root_menu,MF_STRING,IDM_EXITWIN,"E&xit Windows");
  }
}

static void first_token(const char *cmd,char *out,int max)
{
  int i=0;const char *p=cmd;while(*p==' '||*p=='\t')p++;
  while(*p&&*p!=' '&&*p!='\t'&&i<max-1)out[i++]=*p++;
  out[i]=0;
}

static int is_windows_exe(const char *cmd)
{
  char token[144],path[160],search[160];FILE *f;unsigned char sig[2],os;long neoff;unsigned char b[4];
  first_token(cmd,token,sizeof(token));if(!token[0])return 0;
  strcpy(path,token);f=fopen(path,"rb");
  if(!f && !strchr(path,'\\') && !strchr(path,':')){
    sprintf(path,"%s\\%s",base_dir,token);f=fopen(path,"rb");
    if(!f){search[0]=0;_searchenv(token,"PATH",search);if(search[0]){strcpy(path,search);f=fopen(path,"rb");}}
  }
  if(!f){char wdir[144];GetWindowsDirectory(wdir,sizeof(wdir));sprintf(path,"%s\\%s",wdir,token);f=fopen(path,"rb");}
  if(!f)return 0;
  if(fread(sig,1,2,f)!=2||sig[0]!='M'||sig[1]!='Z'){fclose(f);return 0;}
  if(fseek(f,0x3c,SEEK_SET)||fread(b,1,4,f)!=4){fclose(f);return 0;}
  neoff=(long)b[0]|((long)b[1]<<8)|((long)b[2]<<16)|((long)b[3]<<24);
  if(fseek(f,neoff,SEEK_SET)||fread(sig,1,2,f)!=2||sig[0]!='N'||sig[1]!='E'){fclose(f);return 0;}
  if(fseek(f,neoff+0x36L,SEEK_SET)||fread(&os,1,1,f)!=1){fclose(f);return 0;}fclose(f);
  return os==2||os==4;
}
static int item_command_needs_file_validation(const char *cmd)
{
  char token[144];char *dot;first_token(cmd,token,sizeof(token));dot=strrchr(token,'.');
  if(!dot)return 0;
  return !stricmp(dot,".EXE")||!stricmp(dot,".COM")||!stricmp(dot,".BAT");
}

static int item_command_file_exists(const char *cmd)
{
  char token[144],path[160],search[160];FILE *f;
  first_token(cmd,token,sizeof(token));if(!token[0])return 0;
  strcpy(path,token);f=fopen(path,"rb");
  if(!f && !strchr(path,'\\') && !strchr(path,':')){
    sprintf(path,"%s\\%s",base_dir,token);f=fopen(path,"rb");
    if(!f){search[0]=0;_searchenv(token,"PATH",search);if(search[0]){strcpy(path,search);f=fopen(path,"rb");}}
  }
  if(!f){char wdir[144];GetWindowsDirectory(wdir,sizeof(wdir));sprintf(path,"%s\\%s",wdir,token);f=fopen(path,"rb");}
  if(!f)return 0;fclose(f);return 1;
}

static int item_command_validation(const char *cmd)
{
  if(!item_command_needs_file_validation(cmd))return -1;
  return item_command_file_exists(cmd)?1:0;
}

static COLORREF launch_colourref(int index)
{
  static const unsigned char rgb[16][3]={
    {0,0,0},{0,0,170},{0,170,0},{0,170,170},{170,0,0},{170,0,170},{170,85,0},{170,170,170},
    {85,85,85},{85,85,255},{85,255,85},{85,255,255},{255,85,85},{255,85,255},{255,255,85},{255,255,255}
  };
  if(index<0||index>15)index=7;return RGB(rgb[index][0],rgb[index][1],rgb[index][2]);
}

static void run_dos(const char *cmd)
{
  FILE *f;char shell[220];
  f=fopen(winrun_path,"wt");if(!f){show_message_dialog(gWnd,"Launch!","Could not create WINRUN.BAT.",MSG_ERROR);return;}
  fprintf(f,"@ECHO OFF\n%s\nPAUSE\n",cmd);fclose(f);
  sprintf(shell,"COMMAND.COM /C %s",winrun_path);
  if(WinExec(shell,SW_SHOWNORMAL)<32)show_message_dialog(gWnd,"Launch!","Windows could not start the DOS command.",MSG_ERROR);
}

static int command_is_pif(const char *cmd)
{
  char token[144],*dot;first_token(cmd,token,sizeof(token));dot=strrchr(token,'.');
  return dot && !stricmp(dot,".PIF");
}

static void run_item(int idx)
{
  if(idx<0||idx>=node_count||nodes[idx].is_folder)return;
  if(nodes[idx].windows_program||command_is_pif(nodes[idx].command)){
    if(WinExec(nodes[idx].command,SW_SHOWNORMAL)<32)show_message_dialog(gWnd,"Launch!","Windows could not start this program.",MSG_ERROR);
  } else run_dos(nodes[idx].command);
}

static int wait_for_dde_flag(int *flag,DWORD timeout_ms)
{
  DWORD start=GetTickCount();MSG msg;
  while(*flag){
    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
      TranslateMessage(&msg);DispatchMessage(&msg);
      if(!*flag)break;
    }
    if(!*flag)break;
    if((DWORD)(GetTickCount()-start)>=timeout_ms)break;
    Yield();
  }
  return *flag?0:1;
}

static int dde_connect_progman(void)
{
  HWND progman;ATOM app,topic;
  if(dde_server && IsWindow(dde_server))return 1;
  dde_server=NULL;progman=FindWindow("Progman",NULL);if(!progman)return 0;
  app=GlobalAddAtom("PROGMAN");topic=GlobalAddAtom("PROGMAN");
  if(!app||!topic){if(app)GlobalDeleteAtom(app);if(topic)GlobalDeleteAtom(topic);return 0;}
  dde_initiating=1;
  SendMessage(progman,WM_DDE_INITIATE,(WPARAM)gWnd,MAKELONG(app,topic));
  GlobalDeleteAtom(app);GlobalDeleteAtom(topic);
  if(dde_initiating)wait_for_dde_flag(&dde_initiating,1500L);
  return dde_server?1:0;
}

static HGLOBAL dde_request_text(const char *item)
{
  ATOM atom;
  if(!dde_connect_progman())return NULL;
  if(dde_reply){GlobalFree(dde_reply);dde_reply=NULL;}
  dde_request_ok=0;dde_waiting=1;
  atom=GlobalAddAtom(item);if(!atom){dde_waiting=0;return NULL;}
  if(!PostMessage(dde_server,WM_DDE_REQUEST,(WPARAM)gWnd,MAKELONG(CF_TEXT,atom))){
    GlobalDeleteAtom(atom);dde_waiting=0;return NULL;
  }
  if(!wait_for_dde_flag(&dde_waiting,5000L)){dde_waiting=0;return NULL;}
  if(!dde_request_ok){if(dde_reply){GlobalFree(dde_reply);dde_reply=NULL;}return NULL;}
  return dde_reply;
}

static void dde_disconnect_progman(void)
{
  HWND server=dde_server;
  dde_server=NULL;dde_waiting=0;dde_initiating=0;
  if(server && IsWindow(server))PostMessage(server,WM_DDE_TERMINATE,(WPARAM)gWnd,0L);
}

static void clean_menu_text(char *s)
{
  char *p=s;
  while(*p){if(*p=='|'||*p=='\r'||*p=='\n')*p=' ';p++;}
  trim(s);
}

static char *next_line_text(char **cursor)
{
  char *p=*cursor,*line,*end;
  if(!p||!*p)return NULL;line=p;end=p;
  while(*end&&*end!='\r'&&*end!='\n')end++;
  if(*end){*end++=0;if(*end=='\n'||*end=='\r')end++;}
  *cursor=end;return line;
}

static int csv_next_field(char **cursor,char *out,int max)
{
  char *p=*cursor;int n=0,quoted=0;
  if(!p||!*p){out[0]=0;return 0;}
  while(*p==' '||*p=='\t')p++;
  if(*p=='\"'){quoted=1;p++;}
  while(*p){
    if(quoted){
      if(*p=='\"'){
        if(p[1]=='\"'){if(n<max-1)out[n++]='\"';p+=2;continue;}
        p++;while(*p==' '||*p=='\t')p++;break;
      }
    } else if(*p==',')break;
    if(n<max-1)out[n++]=*p;p++;
  }
  out[n]=0;if(*p==',')p++;*cursor=p;trim(out);return 1;
}

static int write_progman_group(FILE *f,const char *group)
{
  HGLOBAL h;LPSTR data;char *cursor,*line;char title[96],command[192];int first=1,count=0;
  h=dde_request_text(group);if(!h)return 0;
  data=(LPSTR)GlobalLock(h);if(!data){GlobalFree(h);dde_reply=NULL;return 0;}
  cursor=data;
  while((line=next_line_text(&cursor))!=NULL){
    char *fields=line;
    if(first){first=0;continue;}
    title[0]=command[0]=0;
    if(!csv_next_field(&fields,title,sizeof(title)))continue;
    if(!csv_next_field(&fields,command,sizeof(command)))continue;
    clean_menu_text(title);clean_menu_text(command);
    if(!title[0]||!command[0])continue;
    if(fprintf(f,"ITEM=%s|%s|0|0|0|0\n",title,command)<0){GlobalUnlock(h);GlobalFree(h);dde_reply=NULL;return -1;}
    count++;
  }
  GlobalUnlock(h);GlobalFree(h);dde_reply=NULL;return count;
}

static int ensure_menu_file(void)
{
  FILE *f=fopen(menu_path,"rt");
  char msg[220];
  if(f){fclose(f);return 1;}
  /* !W31 never manufactures or searches for a menu.  The default menu is
     exactly LAUNCH.MNU beside !W31.EXE; an explicit command-line menu is
     the only override. */
  sprintf(msg,"Launch! could not open the menu file:\n\n%s",menu_path);
  show_message_dialog(gWnd,"Launch!",msg,MSG_ERROR);
  return 0;
}

static void begin_popup_tracking(HWND *previous_active,HWND *previous_focus)
{
  *previous_active=GetActiveWindow();*previous_focus=GetFocus();
  /* TrackPopupMenu on Windows 3.x needs a genuinely active/focused owner.
     The Launch! button normally uses SW_SHOWNOACTIVATE so it never steals
     focus from applications.  Temporarily promote it only while USER owns
     the modal popup loop, and suspend the 100 ms keep-on-top timer so no
     launcher-window housekeeping competes with menu mouse tracking. */
  menu_tracking=1;KillTimer(gWnd,RAISE_TIMER);
  if(!IsWindowEnabled(gWnd))EnableWindow(gWnd,TRUE);
  BringWindowToTop(gWnd);SetActiveWindow(gWnd);SetFocus(gButton?gButton:gWnd);
}

static void end_popup_tracking(HWND previous_active,HWND previous_focus)
{
  menu_tracking=0;SetTimer(gWnd,RAISE_TIMER,100,NULL);
  if(previous_active && previous_active!=gWnd && IsWindow(previous_active)){
    SetActiveWindow(previous_active);
    if(previous_focus && IsWindow(previous_focus))SetFocus(previous_focus);
  }
  SetWindowPos(gWnd,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
  InvalidateRect(gWnd,NULL,FALSE);
}

/* TrackPopupMenu owns a private modal message loop.  Queue one navigation
   keystroke immediately before entering that loop so USER itself establishes
   the initial selection using normal menu rules (including skipping separators
   and disabled items).  Down selects the first item for a top-edge launcher;
   Up selects the last selectable item for a bottom-edge launcher. */
static void queue_initial_menu_focus(void)
{
  UINT key=button_edge?VK_UP:VK_DOWN;
  PostMessage(gWnd,WM_KEYDOWN,key,0L);
  PostMessage(gWnd,WM_KEYUP,key,0L);
}

static void show_launch_menu(void)
{
  POINT pt;RECT wr;UINT flags;int i;HWND previous_active,previous_focus;
  if(!ensure_menu_file())return;
  if(!load_launch_menu()){show_message_dialog(gWnd,"Launch!","The selected menu file could not be opened.",MSG_ERROR);return;}
  load_win_preferences();resize_button_for_mode();
  build_native_menu();if(!root_menu)return;
  begin_popup_tracking(&previous_active,&previous_focus);
  GetWindowRect(gWnd,&wr);pt.x=wr.left;
  flags=0;
  if(button_edge){pt.y=wr.top;if(launch_win31_or_later())flags|=TPM_BOTTOMALIGN;}
  else pt.y=wr.bottom;
  queue_initial_menu_focus();
  TrackPopupMenu(root_menu,flags,pt.x,pt.y,0,gWnd,NULL);
  DestroyMenu(root_menu);root_menu=NULL;
  for(i=0;i<section_count;i++)sections[i].menu=NULL;
  end_popup_tracking(previous_active,previous_focus);
}


static int copy_file_local(const char *src,const char *dst)
{
  FILE *in,*out;char buf[1024];size_t n;int ok=1;
  in=fopen(src,"rb");if(!in)return 0;
  out=fopen(dst,"wb");if(!out){fclose(in);return 0;}
  while((n=fread(buf,1,sizeof(buf),in))!=0){if(fwrite(buf,1,n,out)!=n){ok=0;break;}}
  if(ferror(in))ok=0;
  fclose(in);if(fclose(out)!=0)ok=0;
  if(!ok)remove(dst);
  return ok;
}

static int write_menu_section(FILE *f,int section_index)
{
  int i;
  if(section_index<0||section_index>=section_count)return 0;
  if(fprintf(f,"[%s]\n",sections[section_index].name)<0)return 0;
  for(i=0;i<node_count;i++)if(nodes[i].section_index==section_index){
    if(!strcmp(nodes[i].title,"-")){
      if(fputs("SEPARATOR=\n",f)==EOF)return 0;
    } else if(nodes[i].is_folder){
      if(fprintf(f,"FOLDER=%s\n",nodes[i].title)<0)return 0;
    } else {
      if(fprintf(f,"ITEM=%s|%s|%d|%d|%d|%d%s\n",nodes[i].title,nodes[i].command,
                 nodes[i].enter_after,nodes[i].change_dir,nodes[i].prompt,nodes[i].add_path,
                 nodes[i].windows_program?"|W":"")<0)return 0;
    }
  }
  if(fputc('\n',f)==EOF)return 0;
  for(i=0;i<node_count;i++)if(nodes[i].section_index==section_index && nodes[i].is_folder){
    if(!write_menu_section(f,nodes[i].child_section_index))return 0;
  }
  return 1;
}

static int save_launch_menu(void)
{
  char tmp[180],bak[180];FILE *f;int root,ok;
  menu_sidecar_path(tmp,".$$$");menu_sidecar_path(bak,".BAK");
  root=find_section("Launcher");if(root<0)return 0;
  remove(tmp);f=fopen(tmp,"wt");if(!f)return 0;
  ok=fputs("; Launch! 3.79 menu definition\n; ITEM=title|command and parameters|press Enter|change directory|prompt|add to PATH (0/1)|W (Windows only)\n; SEPARATOR= adds a movable horizontal separator\n\n",f)!=EOF;
  if(ok)ok=write_menu_section(f,root);
  if(fclose(f)!=0)ok=0;
  if(!ok){remove(tmp);return 0;}
  remove(bak);copy_file_local(menu_path,bak);
  if(remove(menu_path)!=0){remove(tmp);return 0;}
  if(rename(tmp,menu_path)!=0){copy_file_local(bak,menu_path);remove(tmp);return 0;}
  if(!load_launch_menu())return 0;
  return 1;
}

static void split_command_win(const char *command,char *exe,char *params)
{
  const char *p=command,*end;int n;
  while(*p==' '||*p=='\t')p++;
  end=p;while(*end&&*end!=' '&&*end!='\t')end++;
  n=(int)(end-p);if(n>158)n=158;
  strncpy(exe,p,n);exe[n]=0;
  while(*end==' '||*end=='\t')end++;
  strncpy(params,end,159);params[159]=0;
}

static int folder_name_exists(int section_index,const char *name,int except_node)
{
  int i;
  for(i=0;i<node_count;i++)if(i!=except_node && nodes[i].section_index==section_index && nodes[i].is_folder && !stricmp(nodes[i].title,name))return 1;
  return 0;
}

static void rename_folder_sections(int node_index,const char *new_title)
{
  int i,child;char old_prefix[128],new_prefix[128],tail[128];int old_len;
  if(node_index<0||node_index>=node_count||!nodes[node_index].is_folder)return;
  child=nodes[node_index].child_section_index;if(child<0||child>=section_count)return;
  strcpy(old_prefix,sections[child].name);old_len=strlen(old_prefix);
  sprintf(new_prefix,"%s\\%s",sections[nodes[node_index].section_index].name,new_title);
  for(i=0;i<section_count;i++){
    if(!stricmp(sections[i].name,old_prefix)){
      strncpy(sections[i].name,new_prefix,sizeof(sections[i].name)-1);sections[i].name[sizeof(sections[i].name)-1]=0;
    } else if(!strnicmp(sections[i].name,old_prefix,old_len) && sections[i].name[old_len]=='\\'){
      strncpy(tail,sections[i].name+old_len,sizeof(tail)-1);tail[sizeof(tail)-1]=0;
      strncpy(sections[i].name,new_prefix,sizeof(sections[i].name)-1);sections[i].name[sizeof(sections[i].name)-1]=0;
      strncat(sections[i].name,tail,sizeof(sections[i].name)-strlen(sections[i].name)-1);
    }
  }
}

static void center_dialog(HWND h)
{
  RECT r;int w,hgt,x,y;
  GetWindowRect(h,&r);w=r.right-r.left;hgt=r.bottom-r.top;
  x=(GetSystemMetrics(SM_CXSCREEN)-w)/2;y=(GetSystemMetrics(SM_CYSCREEN)-hgt)/2;
  if(x<0)x=0;if(y<0)y=0;
  SetWindowPos(h,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
}

static void update_item_program_type(HWND h)
{
  char exe[160];int valid,iswin=0;HWND dosicon,winicon;
  if(!h||item_dialog_folder||!itemCommand)return;
  GetWindowText(itemCommand,exe,sizeof(exe));trim(exe);
  valid=exe[0]?item_command_validation(exe):-1;
  if(valid>0)iswin=is_windows_exe(exe);
  dosicon=GetDlgItem(h,ITEM_DOSICON);winicon=GetDlgItem(h,ITEM_WINICON);
  if(dosicon)ShowWindow(dosicon,(valid>0&&!iswin)?SW_SHOW:SW_HIDE);
  if(winicon)ShowWindow(winicon,(valid>0&&iswin)?SW_SHOW:SW_HIDE);
  /* These four options affect only DOS sessions.  Preserve their checked
     state, but make the irrelevance obvious only for a valid Windows EXE. */
  EnableWindow(itemPrompt,!iswin);
  EnableWindow(itemEnter,!iswin);
  EnableWindow(itemChdir,!iswin);
  EnableWindow(itemAddpath,!iswin);
}

static BOOL FAR PASCAL ItemDlgProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  switch(msg){
    case WM_INITDIALOG:
      itemWnd=h;center_dialog(h);
      SetWindowText(h,item_dialog_folder?(item_dialog_editing?"Edit Folder":"Add Folder"):(item_dialog_editing?"Edit Launcher":"Add Launcher"));
      itemName=GetDlgItem(h,ITEM_NAME);
      itemCommand=item_dialog_folder?NULL:GetDlgItem(h,ITEM_COMMAND);
      itemParams=item_dialog_folder?NULL:GetDlgItem(h,ITEM_PARAMS);
      itemPrompt=item_dialog_folder?NULL:GetDlgItem(h,ITEM_PROMPT);
      itemEnter=item_dialog_folder?NULL:GetDlgItem(h,ITEM_ENTER);
      itemChdir=item_dialog_folder?NULL:GetDlgItem(h,ITEM_CHDIR);
      itemAddpath=item_dialog_folder?NULL:GetDlgItem(h,ITEM_ADDPATH);
      SendMessage(itemName,EM_LIMITTEXT,item_dialog_folder?16:18,0L);
      SetWindowText(itemName,item_work.title);
      if(!item_dialog_folder){
        SendMessage(itemCommand,EM_LIMITTEXT,158,0L);
        SendMessage(itemParams,EM_LIMITTEXT,158,0L);
        SetWindowText(itemCommand,item_exe);SetWindowText(itemParams,item_params);
        SendMessage(itemPrompt,BM_SETCHECK,item_work.prompt?1:0,0L);
        SendMessage(itemEnter,BM_SETCHECK,item_work.enter_after?1:0,0L);
        SendMessage(itemChdir,BM_SETCHECK,item_work.change_dir?1:0,0L);
        SendMessage(itemAddpath,BM_SETCHECK,item_work.add_path?1:0,0L);
        update_item_program_type(h);
      }
      SetFocus(itemName);return FALSE;
    case WM_COMMAND:
      if(wp==ITEM_COMMAND && HIWORD(lp)==EN_CHANGE){update_item_program_type(h);InvalidateRect(itemCommand,NULL,TRUE);UpdateWindow(itemCommand);return TRUE;}
      if(wp==ITEM_BROWSE){
        char file[160]="";
        if(browse_for_program(h,file,sizeof(file)))SetWindowText(itemCommand,file);
        return TRUE;
      }
      if(wp==IDOK){
        char name[48],exe[160],params[160],cmd[160];
        GetWindowText(itemName,name,sizeof(name));trim(name);
        if(!name[0]){show_message_dialog(h,item_dialog_folder?"Folder":"Launcher","Enter a name.",MSG_WARNING);SetFocus(itemName);return TRUE;}
        if(item_dialog_folder){
          if(folder_name_exists(item_dialog_section,name,item_dialog_node)){show_message_dialog(h,"Folder","That folder name already exists in this menu.",MSG_WARNING);return TRUE;}
          strncpy(item_work.title,name,sizeof(item_work.title)-1);item_work.title[sizeof(item_work.title)-1]=0;
        } else {
          GetWindowText(itemCommand,exe,sizeof(exe));trim(exe);GetWindowText(itemParams,params,sizeof(params));trim(params);
          if(!exe[0]){show_message_dialog(h,"Launcher","Enter a command.",MSG_WARNING);SetFocus(itemCommand);return TRUE;}
          strncpy(item_work.title,name,sizeof(item_work.title)-1);item_work.title[sizeof(item_work.title)-1]=0;
          strcpy(cmd,exe);if(params[0] && strlen(cmd)<sizeof(cmd)-2){strcat(cmd," ");strncat(cmd,params,sizeof(cmd)-strlen(cmd)-1);}
          strncpy(item_work.command,cmd,sizeof(item_work.command)-1);item_work.command[sizeof(item_work.command)-1]=0;
          item_work.prompt=SendMessage(itemPrompt,BM_GETCHECK,0,0L)?1:0;
          item_work.enter_after=SendMessage(itemEnter,BM_GETCHECK,0,0L)?1:0;
          item_work.change_dir=SendMessage(itemChdir,BM_GETCHECK,0,0L)?1:0;
          item_work.add_path=SendMessage(itemAddpath,BM_GETCHECK,0,0L)?1:0;
          item_work.windows_program=(item_command_validation(exe)>0&&is_windows_exe(exe))?1:0;
        }
        item_dialog_ok=1;EndDialog(h,IDOK);return TRUE;
      }
      if(wp==IDCANCEL){EndDialog(h,IDCANCEL);return TRUE;}
      break;
    case WM_CLOSE:EndDialog(h,IDCANCEL);return TRUE;
    case WM_DESTROY:itemWnd=NULL;return TRUE;
  }
  return FALSE;
}

static int run_item_dialog(int node_index,int folder,int editing,int section_index)
{
  FARPROC proc;int rc,old_tracking=menu_tracking;
  load_win_preferences();
  memset(&item_work,0,sizeof(item_work));item_exe[0]=item_params[0]=0;
  item_dialog_node=node_index;item_dialog_folder=folder;item_dialog_editing=editing;item_dialog_section=section_index;
  if(editing && node_index>=0 && node_index<node_count){item_work=nodes[node_index];if(!folder)split_command_win(item_work.command,item_exe,item_params);}
  else {item_work.enter_after=1;item_work.section_index=section_index;}
  item_dialog_ok=0;itemWnd=NULL;
  proc=MakeProcInstance((FARPROC)ItemDlgProc,gInst);if(!proc)return 0;
  menu_tracking=1;
  rc=DialogBox(gInst,MAKEINTRESOURCE(folder?IDD_FOLDER:IDD_LAUNCHER),managerWnd?managerWnd:gWnd,proc);
  menu_tracking=old_tracking;
  FreeProcInstance(proc);itemWnd=NULL;SetActiveWindow(managerWnd?managerWnd:gWnd);
  if(rc==-1){show_message_dialog(gWnd,"Launch!","Could not create the Launch! dialog.",MSG_ERROR);return 0;}
  return item_dialog_ok;
}

static int add_menu_item_to_section(int folder,int section_index)
{
  MENU_NODE *n;char child[128];
  if(section_index<0||section_index>=section_count)return 0;
  if(node_count>=MAX_NODES){show_message_dialog(gWnd,"Launch!","The menu is full.",MSG_WARNING);return 0;}
  if(!run_item_dialog(-1,folder,0,section_index))return 0;
  n=&nodes[node_count];*n=item_work;n->section_index=section_index;n->is_folder=folder;n->child_section_index=-1;
  if(folder){
    if(section_count>=MAX_SECTIONS){show_message_dialog(gWnd,"Launch!","The menu has too many folders.",MSG_WARNING);return 0;}
    if(strlen(sections[section_index].name)+strlen(n->title)+2>=sizeof(child)){show_message_dialog(gWnd,"Launch!","The folder path is too long.",MSG_WARNING);return 0;}
    sprintf(child,"%s\\%s",sections[section_index].name,n->title);n->child_section_index=ensure_section(child);
  }
  node_count++;
  if(!save_launch_menu()){show_message_dialog(managerWnd?managerWnd:gWnd,"Launch!","Could not update the selected menu file.",MSG_ERROR);return 0;}
  return 1;
}

static int add_menu_item(int folder)
{
  int root=find_section("Launcher");
  return add_menu_item_to_section(folder,root);
}

static int add_menu_separator_to_section(int section_index)
{
  MENU_NODE *n;
  if(section_index<0||section_index>=section_count||node_count>=MAX_NODES)return 0;
  n=&nodes[node_count++];memset(n,0,sizeof(*n));strcpy(n->title,"-");n->section_index=section_index;n->child_section_index=-1;
  if(!save_launch_menu()){show_message_dialog(managerWnd?managerWnd:gWnd,"Launch!","Could not update the selected menu file.",MSG_ERROR);return 0;}
  return 1;
}

static int add_menu_separator(void)
{
  return add_menu_separator_to_section(find_section("Launcher"));
}

static void edit_menu_node(int node_index)
{
  char old_title[48];
  if(node_index<0||node_index>=node_count)return;
  if(!strcmp(nodes[node_index].title,"-"))return;
  strcpy(old_title,nodes[node_index].title);
  if(!run_item_dialog(node_index,nodes[node_index].is_folder,1,nodes[node_index].section_index))return;
  if(nodes[node_index].is_folder && stricmp(old_title,item_work.title))rename_folder_sections(node_index,item_work.title);
  item_work.section_index=nodes[node_index].section_index;
  item_work.child_section_index=nodes[node_index].child_section_index;
  item_work.is_folder=nodes[node_index].is_folder;
  nodes[node_index]=item_work;
  if(!save_launch_menu())show_message_dialog(managerWnd?managerWnd:gWnd,"Launch!","Could not update the selected menu file.",MSG_ERROR);
}

static int section_is_under(int section_index,const char *prefix)
{
  int n;if(section_index<0||section_index>=section_count)return 0;n=strlen(prefix);
  if(!stricmp(sections[section_index].name,prefix))return 1;
  return !strnicmp(sections[section_index].name,prefix,n) && sections[section_index].name[n]=='\\';
}

static void remove_menu_node(int node_index)
{
  int i,j=0;char msg[160],prefix[128];
  if(node_index<0||node_index>=node_count)return;
  if(nodes[node_index].is_folder)sprintf(msg,"Remove folder '%s' and all of its contents?",nodes[node_index].title);
  else if(!strcmp(nodes[node_index].title,"-"))strcpy(msg,"Remove this separator?");
  else sprintf(msg,"Remove '%s'?",nodes[node_index].title);
  if(!show_confirm_dialog("Remove Item",msg))return;
  prefix[0]=0;if(nodes[node_index].is_folder && nodes[node_index].child_section_index>=0)strcpy(prefix,sections[nodes[node_index].child_section_index].name);
  for(i=0;i<node_count;i++){
    if(i==node_index)continue;
    if(prefix[0] && section_is_under(nodes[i].section_index,prefix))continue;
    if(j!=i)nodes[j]=nodes[i];
    if(managerWnd)manager_expanded[j]=manager_expanded[i];
    j++;
  }
  node_count=j;
  if(managerWnd)for(i=j;i<MAX_NODES;i++)manager_expanded[i]=0;
  if(!save_launch_menu())show_message_dialog(managerWnd?managerWnd:gWnd,"Launch!","Could not update the selected menu file.",MSG_ERROR);
}

static int manager_selected_node(void)
{
  int row;
  if(!managerTree)return -1;
  row=(int)SendMessage(managerTree,LB_GETCURSEL,0,0L);
  if(row==LB_ERR||row<0||row>=manager_row_count)return -1;
  return manager_rows[row].node_index;
}

static int manager_target_section(void)
{
  int node=manager_selected_node(),root;
  if(node>=0&&node<node_count){
    if(nodes[node].is_folder&&nodes[node].child_section_index>=0)return nodes[node].child_section_index;
    return nodes[node].section_index;
  }
  root=find_section("Launcher");return root;
}

static int adjacent_same_section(int node_index,int direction)
{
  int i,section;if(node_index<0||node_index>=node_count)return -1;section=nodes[node_index].section_index;
  if(direction<0){for(i=node_index-1;i>=0;i--)if(nodes[i].section_index==section)return i;}
  else {for(i=node_index+1;i<node_count;i++)if(nodes[i].section_index==section)return i;}
  return -1;
}

static int manager_has_later_sibling(int node_index,int section_index)
{
  int i;
  for(i=node_index+1;i<node_count;i++)if(nodes[i].section_index==section_index)return 1;
  return 0;
}

static void manager_add_section_rows(int section_index,int depth,unsigned long branch_mask)
{
  int i,row,has_next;
  for(i=0;i<node_count&&manager_row_count<MAX_NODES;i++)if(nodes[i].section_index==section_index){
    has_next=manager_has_later_sibling(i,section_index);
    row=manager_row_count++;
    manager_rows[row].node_index=i;manager_rows[row].depth=depth;
    manager_rows[row].is_windows=(!nodes[i].is_folder&&strcmp(nodes[i].title,"-")&&nodes[i].windows_program)?1:0;
    manager_rows[row].branch_mask=branch_mask;manager_rows[row].has_next=has_next;
    SendMessage(managerTree,LB_ADDSTRING,0,(LPARAM)(LPSTR)"");
    SendMessage(managerTree,LB_SETITEMDATA,row,(LPARAM)i);
    if(nodes[i].is_folder&&manager_expanded[i]&&nodes[i].child_section_index>=0)
      manager_add_section_rows(nodes[i].child_section_index,depth+1,branch_mask|(has_next?(1UL<<depth):0L));
  }
}

static void manager_reset_expansion(void)
{
  memset(manager_expanded,0,sizeof(manager_expanded));
}

static void manager_rebuild_tree(int select_node)
{
  int root,row=0;
  if(!managerTree)return;
  SendMessage(managerTree,WM_SETREDRAW,FALSE,0L);
  SendMessage(managerTree,LB_RESETCONTENT,0,0L);manager_row_count=0;
  root=find_section("Launcher");if(root>=0)manager_add_section_rows(root,0,0L);
  if(select_node>=0){while(row<manager_row_count&&manager_rows[row].node_index!=select_node)row++;}
  if(row>=manager_row_count)row=manager_row_count?0:-1;
  if(row>=0)SendMessage(managerTree,LB_SETCURSEL,row,0L);
  SendMessage(managerTree,WM_SETREDRAW,TRUE,0L);InvalidateRect(managerTree,NULL,TRUE);
}

static void manager_update_buttons(void)
{
  int node=manager_selected_node(),can_edit=0,can_remove=0,can_up=0,can_down=0;
  if(node>=0&&node<node_count){
    can_edit=strcmp(nodes[node].title,"-")!=0;can_remove=1;
    can_up=adjacent_same_section(node,-1)>=0;can_down=adjacent_same_section(node,1)>=0;
  }
  EnableWindow(GetDlgItem(managerWnd,MANAGER_EDIT),can_edit);
  EnableWindow(GetDlgItem(managerWnd,MANAGER_REMOVE),can_remove);
  EnableWindow(GetDlgItem(managerWnd,MANAGER_UP),can_up);
  EnableWindow(GetDlgItem(managerWnd,MANAGER_DOWN),can_down);
}

static void manager_toggle_folder(int node,int expand)
{
  if(node<0||node>=node_count||!nodes[node].is_folder)return;
  if(expand<0)manager_expanded[node]=manager_expanded[node]?0:1;
  else manager_expanded[node]=(unsigned char)(expand?1:0);
  manager_rebuild_tree(node);manager_update_buttons();
}

static void manager_move(int direction)
{
  int node,other;MENU_NODE tmp;
  node=manager_selected_node();if(node<0)return;other=adjacent_same_section(node,direction);if(other<0)return;
  {unsigned char exptmp=manager_expanded[node];
    tmp=nodes[node];nodes[node]=nodes[other];nodes[other]=tmp;
    manager_expanded[node]=manager_expanded[other];manager_expanded[other]=exptmp;
  }
  if(!save_launch_menu()){show_message_dialog(managerWnd,"Menu Manager","Could not update the selected menu file.",MSG_ERROR);return;}
  manager_rebuild_tree(other);manager_update_buttons();
}

static int manager_open_path(const char *path)
{
  char old[160];FILE *f;
  if(!path||!path[0]||!is_menu_filename(path))return 0;
  f=fopen(path,"rt");if(!f)return 0;fclose(f);
  strcpy(old,menu_path);strncpy(menu_path,path,sizeof(menu_path)-1);menu_path[sizeof(menu_path)-1]=0;
  if(!load_launch_menu()){strcpy(menu_path,old);load_launch_menu();return 0;}
  if(managerPath)SetWindowText(managerPath,menu_path);
  manager_reset_expansion();manager_rebuild_tree(-1);manager_update_buttons();return 1;
}

static void manager_apply_path_field(void)
{
  char path[160];
  if(!managerPath)return;GetWindowText(managerPath,path,sizeof(path));trim(path);
  if(!path[0]||!stricmp(path,menu_path)){SetWindowText(managerPath,menu_path);return;}
  if(!manager_open_path(path)){
    show_message_dialog(managerWnd,"Menu Manager","The specified .MNU file could not be opened.",MSG_ERROR);
    SetWindowText(managerPath,menu_path);
  }
}

static void manager_show_add_menu(void)
{
  HWND b;RECT r;HMENU m;
  b=GetDlgItem(managerWnd,MANAGER_ADD);if(!b)return;GetWindowRect(b,&r);
  m=CreatePopupMenu();if(!m)return;
  AppendMenu(m,MF_STRING,IDM_MANAGER_ADD_LAUNCHER,"Launcher");
  AppendMenu(m,MF_STRING,IDM_MANAGER_ADD_FOLDER,"Folder");
  AppendMenu(m,MF_STRING,IDM_MANAGER_ADD_SEPARATOR,"Separator");
  TrackPopupMenu(m,0,r.left,r.bottom,0,managerWnd,NULL);DestroyMenu(m);
}

static void manager_add_item_command(int command)
{
  int section=manager_target_section(),ok=0,old_count=node_count,i;
  if(command==IDM_MANAGER_ADD_LAUNCHER)ok=add_menu_item_to_section(0,section);
  else if(command==IDM_MANAGER_ADD_FOLDER)ok=add_menu_item_to_section(1,section);
  else if(command==IDM_MANAGER_ADD_SEPARATOR)ok=add_menu_separator_to_section(section);
  if(ok){for(i=old_count;i<node_count;i++)manager_expanded[i]=0;manager_rebuild_tree(-1);manager_update_buttons();}
}

static void manager_edit_selected(void)
{
  int node=manager_selected_node();if(node<0||!strcmp(nodes[node].title,"-"))return;
  edit_menu_node(node);manager_rebuild_tree(node);manager_update_buttons();
}

static void manager_remove_selected(void)
{
  int node=manager_selected_node();if(node<0)return;
  remove_menu_node(node);manager_rebuild_tree(-1);manager_update_buttons();
}

static void manager_draw_folder(HDC dc,int x,int y,COLORREF colour)
{
  HPEN pen,old;HBRUSH oldb;
  pen=CreatePen(PS_SOLID,1,colour);old=(HPEN)SelectObject(dc,pen);oldb=(HBRUSH)SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
  MoveTo(dc,x,y+4);LineTo(dc,x+5,y+4);LineTo(dc,x+7,y+6);LineTo(dc,x+14,y+6);
  LineTo(dc,x+14,y+14);LineTo(dc,x,y+14);LineTo(dc,x,y+4);
  SelectObject(dc,oldb);SelectObject(dc,old);DeleteObject(pen);
}

static void manager_draw_windows_marker(HDC dc,int right,int top,COLORREF colour)
{
  HPEN pen,old;HBRUSH oldb;int x=right-17,y=top+3;
  pen=CreatePen(PS_SOLID,1,colour);old=(HPEN)SelectObject(dc,pen);oldb=(HBRUSH)SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
  Rectangle(dc,x,y,x+13,y+11);MoveTo(dc,x+1,y+3);LineTo(dc,x+12,y+3);
  MoveTo(dc,x+3,y+1);LineTo(dc,x+4,y+1);MoveTo(dc,x+6,y+1);LineTo(dc,x+7,y+1);
  SelectObject(dc,oldb);SelectObject(dc,old);DeleteObject(pen);
}

static void manager_draw_row(LPDRAWITEMSTRUCT dis)
{
  int row=(int)dis->itemID,node,depth,x,d,cy;COLORREF bg,fg,linec;HBRUSH b;HPEN pen,oldp;TEXTMETRIC tm;
  if(row<0||row>=manager_row_count)return;node=manager_rows[row].node_index;depth=manager_rows[row].depth;if(node<0||node>=node_count)return;
  bg=(dis->itemState&ODS_SELECTED)?GetSysColor(COLOR_HIGHLIGHT):GetSysColor(COLOR_WINDOW);
  fg=(dis->itemState&ODS_SELECTED)?GetSysColor(COLOR_HIGHLIGHTTEXT):GetSysColor(COLOR_WINDOWTEXT);
  linec=(dis->itemState&ODS_SELECTED)?fg:GetSysColor(COLOR_BTNSHADOW);
  b=CreateSolidBrush(bg);FillRect(dis->hDC,&dis->rcItem,b);DeleteObject(b);SetBkMode(dis->hDC,TRANSPARENT);SetTextColor(dis->hDC,fg);
  cy=(dis->rcItem.top+dis->rcItem.bottom)/2;x=dis->rcItem.left+5;
  pen=CreatePen(PS_SOLID,1,linec);oldp=(HPEN)SelectObject(dis->hDC,pen);
  for(d=0;d<depth;d++){
    int tx=x+d*14+4;
    if(d<depth-1){
      if(manager_rows[row].branch_mask&(1UL<<d)){MoveTo(dis->hDC,tx,dis->rcItem.top);LineTo(dis->hDC,tx,dis->rcItem.bottom);}
    } else {
      MoveTo(dis->hDC,tx,dis->rcItem.top);
      LineTo(dis->hDC,tx,manager_rows[row].has_next?dis->rcItem.bottom:cy);
    }
  }
  if(depth>0){int tx=x+depth*14-10;MoveTo(dis->hDC,tx,cy);LineTo(dis->hDC,x+depth*14+3,cy);}
  SelectObject(dis->hDC,oldp);DeleteObject(pen);x+=depth*14;
  if(nodes[node].is_folder){
    HBRUSH oldb=(HBRUSH)SelectObject(dis->hDC,GetStockObject(HOLLOW_BRUSH));
    Rectangle(dis->hDC,x,cy-4,x+9,cy+5);SelectObject(dis->hDC,oldb);MoveTo(dis->hDC,x+2,cy);LineTo(dis->hDC,x+7,cy);
    if(!manager_expanded[node]){MoveTo(dis->hDC,x+4,cy-3);LineTo(dis->hDC,x+4,cy+4);}
    manager_draw_folder(dis->hDC,x+13,dis->rcItem.top-1,fg);x+=31;
  } else x+=13;
  if(!strcmp(nodes[node].title,"-")){
    pen=CreatePen(PS_SOLID,1,linec);oldp=(HPEN)SelectObject(dis->hDC,pen);MoveTo(dis->hDC,x,cy);LineTo(dis->hDC,dis->rcItem.right-6,cy);SelectObject(dis->hDC,oldp);DeleteObject(pen);
  } else {
    GetTextMetrics(dis->hDC,&tm);
    TextOut(dis->hDC,x,dis->rcItem.top+(dis->rcItem.bottom-dis->rcItem.top-tm.tmHeight)/2,nodes[node].title,strlen(nodes[node].title));
    if(manager_rows[row].is_windows)manager_draw_windows_marker(dis->hDC,dis->rcItem.right,dis->rcItem.top,fg);
  }
  if(dis->itemState&ODS_FOCUS)DrawFocusRect(dis->hDC,&dis->rcItem);
}

/* Windows 3.0/3.1 owner-draw list boxes are normally painted through
   WM_DRAWITEM sent to the dialog owner.  Some real Win16 combinations can
   retain the selectable rows while failing to send/repaint those callbacks.
   Keep the owner-draw path, but also paint the visible rows directly after
   the list box has handled WM_PAINT.  This is only a rendering fallback;
   selection, scrolling and keyboard/mouse behaviour remain the native
   LISTBOX implementation. */
static void manager_paint_tree_fallback(HWND h)
{
  HDC dc;RECT client,ri;DRAWITEMSTRUCT dis;int top,count,sel,height,row,y;
  HBRUSH back;
  if(!h)return;
  count=(int)SendMessage(h,LB_GETCOUNT,0,0L);if(count<=0)return;
  top=(int)SendMessage(h,LB_GETTOPINDEX,0,0L);if(top<0)top=0;
  sel=(int)SendMessage(h,LB_GETCURSEL,0,0L);
  height=17;
  GetClientRect(h,&client);dc=GetDC(h);if(!dc)return;
  back=CreateSolidBrush(GetSysColor(COLOR_WINDOW));FillRect(dc,&client,back);DeleteObject(back);
  memset(&dis,0,sizeof(dis));dis.CtlType=ODT_LISTBOX;dis.CtlID=MANAGER_TREE;dis.hwndItem=h;dis.hDC=dc;
  y=client.top;
  for(row=top;row<count && y<client.bottom;row++,y+=height){
    ri.left=client.left;ri.right=client.right;ri.top=y;ri.bottom=y+height;if(ri.bottom>client.bottom)ri.bottom=client.bottom;
    dis.itemID=(UINT)row;dis.itemAction=ODA_DRAWENTIRE;dis.itemState=0;
    if(row==sel)dis.itemState|=ODS_SELECTED;
    if(row==sel && GetFocus()==h)dis.itemState|=ODS_FOCUS;
    dis.rcItem=ri;dis.itemData=(DWORD)SendMessage(h,LB_GETITEMDATA,row,0L);
    manager_draw_row(&dis);
  }
  ReleaseDC(h,dc);
}

/* Draw colour-keyed bitmap artwork using only Windows 3.0 GDI operations.
   The banner assets use RGB(192,192,192) as their background key.  A 1bpp
   mask leaves the current Windows menu/dialog colour visible underneath. */
static void transparent_stretch_bitmap(HDC dc,HBITMAP bmp,int x,int y,int w,int h,COLORREF key)
{
  BITMAP bm;HDC src=NULL,mask=NULL,inv=NULL,work=NULL;HBITMAP maskbm=NULL,invbm=NULL,workbm=NULL;
  HBITMAP oldsrc=NULL,oldmask=NULL,oldinv=NULL,oldwork=NULL;COLORREF oldbk,oldtx;
  if(!dc||!bmp||w<=0||h<=0)return;if(!GetObject(bmp,sizeof(bm),(LPSTR)&bm))return;
  src=CreateCompatibleDC(dc);mask=CreateCompatibleDC(dc);inv=CreateCompatibleDC(dc);work=CreateCompatibleDC(dc);
  if(!src||!mask||!inv||!work)goto done;
  maskbm=CreateBitmap(bm.bmWidth,bm.bmHeight,1,1,NULL);
  invbm=CreateBitmap(bm.bmWidth,bm.bmHeight,1,1,NULL);
  workbm=CreateCompatibleBitmap(dc,bm.bmWidth,bm.bmHeight);
  if(!maskbm||!invbm||!workbm)goto done;
  oldsrc=(HBITMAP)SelectObject(src,bmp);oldmask=(HBITMAP)SelectObject(mask,maskbm);
  oldinv=(HBITMAP)SelectObject(inv,invbm);oldwork=(HBITMAP)SelectObject(work,workbm);
  BitBlt(work,0,0,bm.bmWidth,bm.bmHeight,src,0,0,SRCCOPY);
  oldbk=SetBkColor(src,key);BitBlt(mask,0,0,bm.bmWidth,bm.bmHeight,src,0,0,SRCCOPY);SetBkColor(src,oldbk);
  BitBlt(inv,0,0,bm.bmWidth,bm.bmHeight,mask,0,0,NOTSRCCOPY);
  oldtx=SetTextColor(work,RGB(0,0,0));oldbk=SetBkColor(work,RGB(255,255,255));
  BitBlt(work,0,0,bm.bmWidth,bm.bmHeight,inv,0,0,SRCAND);SetTextColor(work,oldtx);SetBkColor(work,oldbk);
  oldtx=SetTextColor(dc,RGB(0,0,0));oldbk=SetBkColor(dc,RGB(255,255,255));
  StretchBlt(dc,x,y,w,h,mask,0,0,bm.bmWidth,bm.bmHeight,SRCAND);
  SetTextColor(dc,oldtx);SetBkColor(dc,oldbk);
  StretchBlt(dc,x,y,w,h,work,0,0,bm.bmWidth,bm.bmHeight,SRCPAINT);
done:
  if(oldsrc)SelectObject(src,oldsrc);if(oldmask)SelectObject(mask,oldmask);if(oldinv)SelectObject(inv,oldinv);if(oldwork)SelectObject(work,oldwork);
  if(maskbm)DeleteObject(maskbm);if(invbm)DeleteObject(invbm);if(workbm)DeleteObject(workbm);
  if(src)DeleteDC(src);if(mask)DeleteDC(mask);if(inv)DeleteDC(inv);if(work)DeleteDC(work);
}

static void transparent_tile_bitmap(HDC dc,HBITMAP bmp,int x,int y,int w,COLORREF key)
{
  BITMAP bm;HDC src=NULL,mask=NULL,inv=NULL,work=NULL;HBITMAP maskbm=NULL,invbm=NULL,workbm=NULL;
  HBITMAP oldsrc=NULL,oldmask=NULL,oldinv=NULL,oldwork=NULL;COLORREF oldbk,oldtx;int tx,cw;
  if(!dc||!bmp||w<=0)return;if(!GetObject(bmp,sizeof(bm),(LPSTR)&bm)||bm.bmWidth<=0||bm.bmHeight<=0)return;
  src=CreateCompatibleDC(dc);mask=CreateCompatibleDC(dc);inv=CreateCompatibleDC(dc);work=CreateCompatibleDC(dc);
  if(!src||!mask||!inv||!work)goto done;
  maskbm=CreateBitmap(bm.bmWidth,bm.bmHeight,1,1,NULL);
  invbm=CreateBitmap(bm.bmWidth,bm.bmHeight,1,1,NULL);
  workbm=CreateCompatibleBitmap(dc,bm.bmWidth,bm.bmHeight);
  if(!maskbm||!invbm||!workbm)goto done;
  oldsrc=(HBITMAP)SelectObject(src,bmp);oldmask=(HBITMAP)SelectObject(mask,maskbm);
  oldinv=(HBITMAP)SelectObject(inv,invbm);oldwork=(HBITMAP)SelectObject(work,workbm);
  BitBlt(work,0,0,bm.bmWidth,bm.bmHeight,src,0,0,SRCCOPY);
  oldbk=SetBkColor(src,key);BitBlt(mask,0,0,bm.bmWidth,bm.bmHeight,src,0,0,SRCCOPY);SetBkColor(src,oldbk);
  BitBlt(inv,0,0,bm.bmWidth,bm.bmHeight,mask,0,0,NOTSRCCOPY);
  oldtx=SetTextColor(work,RGB(0,0,0));oldbk=SetBkColor(work,RGB(255,255,255));
  BitBlt(work,0,0,bm.bmWidth,bm.bmHeight,inv,0,0,SRCAND);SetTextColor(work,oldtx);SetBkColor(work,oldbk);
  oldtx=SetTextColor(dc,RGB(0,0,0));oldbk=SetBkColor(dc,RGB(255,255,255));
  for(tx=x;tx<x+w;tx+=bm.bmWidth){
    cw=bm.bmWidth;if(tx+cw>x+w)cw=x+w-tx;
    BitBlt(dc,tx,y,cw,bm.bmHeight,mask,0,0,SRCAND);
    BitBlt(dc,tx,y,cw,bm.bmHeight,work,0,0,SRCPAINT);
  }
  SetTextColor(dc,oldtx);SetBkColor(dc,oldbk);
done:
  if(oldsrc)SelectObject(src,oldsrc);if(oldmask)SelectObject(mask,oldmask);if(oldinv)SelectObject(inv,oldinv);if(oldwork)SelectObject(work,oldwork);
  if(maskbm)DeleteObject(maskbm);if(invbm)DeleteObject(invbm);if(workbm)DeleteObject(workbm);
  if(src)DeleteDC(src);if(mask)DeleteDC(mask);if(inv)DeleteDC(inv);if(work)DeleteDC(work);
}

static void opaque_stretch_bitmap(HDC dc,HBITMAP bmp,int x,int y,int w,int h)
{
  BITMAP bm;HDC src;HBITMAP old;
  if(!dc||!bmp||w<=0||h<=0)return;
  if(!GetObject(bmp,sizeof(bm),(LPSTR)&bm))return;
  src=CreateCompatibleDC(dc);if(!src)return;
  old=(HBITMAP)SelectObject(src,bmp);
  StretchBlt(dc,x,y,w,h,src,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
  if(old)SelectObject(src,old);DeleteDC(src);
}

static void opaque_tile_bitmap(HDC dc,HBITMAP bmp,int x,int y,int w)
{
  BITMAP bm;HDC src;HBITMAP old;int tx,cw;
  if(!dc||!bmp||w<=0)return;
  if(!GetObject(bmp,sizeof(bm),(LPSTR)&bm)||bm.bmWidth<=0||bm.bmHeight<=0)return;
  src=CreateCompatibleDC(dc);if(!src)return;
  old=(HBITMAP)SelectObject(src,bmp);
  for(tx=x;tx<x+w;tx+=bm.bmWidth){
    cw=bm.bmWidth;if(tx+cw>x+w)cw=x+w-tx;
    BitBlt(dc,tx,y,cw,bm.bmHeight,src,0,0,SRCCOPY);
  }
  if(old)SelectObject(src,old);DeleteDC(src);
}

static void manager_paint_banner(HWND h)
{
  HDC dc;RECT r;BITMAP logo,tile;HBRUSH back;int x,remaining;
  if(!h||!gManagerLogoBmp||!gManagerTileBmp)return;dc=GetDC(h);if(!dc)return;
  GetClientRect(h,&r);back=CreateSolidBrush(GetSysColor(COLOR_BTNFACE));FillRect(dc,&r,back);DeleteObject(back);
  GetObject(gManagerLogoBmp,sizeof(logo),&logo);GetObject(gManagerTileBmp,sizeof(tile),&tile);
  transparent_stretch_bitmap(dc,gManagerLogoBmp,0,0,logo.bmWidth,logo.bmHeight,RGB(192,192,192));
  x=logo.bmWidth;remaining=r.right-x;
  if(remaining>0)transparent_tile_bitmap(dc,gManagerTileBmp,x,0,remaining,RGB(192,192,192));
  ReleaseDC(h,dc);
}

static LRESULT FAR PASCAL ManagerBannerProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  if(msg==WM_PAINT){LRESULT r=CallWindowProc(gManagerBannerProc,h,msg,wp,lp);manager_paint_banner(h);return r;}
  return CallWindowProc(gManagerBannerProc,h,msg,wp,lp);
}

static LRESULT FAR PASCAL ManagerTreeProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  if(msg==WM_PAINT){
    LRESULT r=CallWindowProc(gManagerTreeProc,h,msg,wp,lp);manager_paint_tree_fallback(h);return r;
  }
  if(msg==WM_SETFOCUS||msg==WM_KILLFOCUS){
    LRESULT r=CallWindowProc(gManagerTreeProc,h,msg,wp,lp);InvalidateRect(h,NULL,FALSE);return r;
  }
  if(msg==WM_KEYDOWN){
    int node=manager_selected_node();
    if(GetKeyState(VK_CONTROL)<0&&wp==VK_UP){SendMessage(managerWnd,WM_MANAGER_ACTION,MANAGER_UP,0L);return 0;}
    if(GetKeyState(VK_CONTROL)<0&&wp==VK_DOWN){SendMessage(managerWnd,WM_MANAGER_ACTION,MANAGER_DOWN,0L);return 0;}
    if((wp==VK_UP||wp==VK_DOWN)&&GetKeyState(VK_CONTROL)>=0){
      int row=(int)SendMessage(h,LB_GETCURSEL,0,0L),count=(int)SendMessage(h,LB_GETCOUNT,0,0L),next,top,height,visible;RECT r;
      if(count>0){
        if(row==LB_ERR)row=0;next=row+(wp==VK_UP?-1:1);if(next<0)next=0;if(next>=count)next=count-1;
        SendMessage(h,LB_SETCURSEL,next,0L);
        top=(int)SendMessage(h,LB_GETTOPINDEX,0,0L);height=17;GetClientRect(h,&r);
        visible=height>0?(r.bottom-r.top)/height:1;if(visible<1)visible=1;
        if(next<top)SendMessage(h,LB_SETTOPINDEX,next,0L);else if(next>=top+visible)SendMessage(h,LB_SETTOPINDEX,next-visible+1,0L);
        manager_update_buttons();InvalidateRect(h,NULL,TRUE);
      }
      return 0;
    }
    if(wp==VK_LEFT&&node>=0&&nodes[node].is_folder){manager_toggle_folder(node,0);return 0;}
    if(wp==VK_RIGHT&&node>=0&&nodes[node].is_folder){manager_toggle_folder(node,1);return 0;}
    if(wp==VK_RETURN){if(node>=0&&nodes[node].is_folder)manager_toggle_folder(node,-1);else SendMessage(managerWnd,WM_MANAGER_ACTION,MANAGER_EDIT,0L);return 0;}
    if(wp==VK_DELETE){SendMessage(managerWnd,WM_MANAGER_ACTION,MANAGER_REMOVE,0L);return 0;}
  }
  return CallWindowProc(gManagerTreeProc,h,msg,wp,lp);
}

static BOOL FAR PASCAL ManagerDlgProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  switch(msg){
    case WM_INITDIALOG:
      managerWnd=h;center_dialog(h);managerPath=GetDlgItem(h,MANAGER_PATH);managerTree=GetDlgItem(h,MANAGER_TREE);managerBanner=GetDlgItem(h,MANAGER_BANNER);
      SendMessage(managerPath,EM_LIMITTEXT,158,0L);SetWindowText(managerPath,menu_path);
      gManagerTreeInstProc=MakeProcInstance((FARPROC)ManagerTreeProc,gInst);
      if(gManagerTreeInstProc)gManagerTreeProc=(FARPROC)SetWindowLong(managerTree,GWL_WNDPROC,(LONG)gManagerTreeInstProc);
      if(managerBanner){
        gManagerLogoBmp=LoadBitmap(gInst,MAKEINTRESOURCE(IDB_MANAGER_LOGO));gManagerTileBmp=LoadBitmap(gInst,MAKEINTRESOURCE(IDB_MANAGER_TILE));
        gManagerBannerInstProc=MakeProcInstance((FARPROC)ManagerBannerProc,gInst);
        if(gManagerBannerInstProc)gManagerBannerProc=(FARPROC)SetWindowLong(managerBanner,GWL_WNDPROC,(LONG)gManagerBannerInstProc);
      }
      manager_reset_expansion();manager_rebuild_tree(-1);manager_update_buttons();SetFocus(managerTree);return FALSE;
    case WM_MEASUREITEM:
      if(wp==MANAGER_TREE){LPMEASUREITEMSTRUCT mi=(LPMEASUREITEMSTRUCT)lp;mi->itemHeight=17;return TRUE;}break;
    case WM_DRAWITEM:
      if(wp==MANAGER_TREE){manager_draw_row((LPDRAWITEMSTRUCT)lp);return TRUE;}break;
    case WM_COMMAND:
      if(wp==MANAGER_PATH&&HIWORD(lp)==EN_KILLFOCUS){manager_apply_path_field();return TRUE;}
      if(wp==MANAGER_BROWSE){char file[160];strncpy(file,menu_path,sizeof(file)-1);file[sizeof(file)-1]=0;if(browse_for_menu(h,file,sizeof(file))&&!manager_open_path(file))show_message_dialog(h,"Menu Manager","The selected .MNU file could not be opened.",MSG_ERROR);return TRUE;}
      if(wp==MANAGER_TREE&&HIWORD(lp)==LBN_SELCHANGE){manager_update_buttons();return TRUE;}
      if(wp==MANAGER_TREE&&HIWORD(lp)==LBN_DBLCLK){int node=manager_selected_node();if(node>=0&&nodes[node].is_folder)manager_toggle_folder(node,-1);else manager_edit_selected();return TRUE;}
      if(wp==MANAGER_ADD){manager_show_add_menu();return TRUE;}
      if(wp==MANAGER_EDIT){manager_edit_selected();return TRUE;}
      if(wp==MANAGER_REMOVE){manager_remove_selected();return TRUE;}
      if(wp==MANAGER_UP){manager_move(-1);return TRUE;}
      if(wp==MANAGER_DOWN){manager_move(1);return TRUE;}
      if(wp==IDM_MANAGER_ADD_LAUNCHER||wp==IDM_MANAGER_ADD_FOLDER||wp==IDM_MANAGER_ADD_SEPARATOR){manager_add_item_command((int)wp);return TRUE;}
      if(wp==IDCANCEL||wp==MANAGER_CLOSE){EndDialog(h,IDCANCEL);return TRUE;}break;
    case WM_MANAGER_ACTION:
      if(wp==MANAGER_EDIT)manager_edit_selected();else if(wp==MANAGER_REMOVE)manager_remove_selected();else if(wp==MANAGER_UP)manager_move(-1);else if(wp==MANAGER_DOWN)manager_move(1);return TRUE;
    case WM_CLOSE:EndDialog(h,IDCANCEL);return TRUE;
    case WM_DESTROY:
      if(gManagerLogoBmp){DeleteObject(gManagerLogoBmp);gManagerLogoBmp=NULL;}
      if(gManagerTileBmp){DeleteObject(gManagerTileBmp);gManagerTileBmp=NULL;}
      managerWnd=NULL;managerTree=NULL;managerPath=NULL;managerBanner=NULL;return TRUE;
  }
  return FALSE;
}

static void show_menu_manager_dialog(void)
{
  FARPROC proc;int rc;HWND owner=gWnd?gWnd:NULL;
  if(managerWnd){SetActiveWindow(managerWnd);return;}
  if(!ensure_menu_file())return;if(!load_launch_menu()){show_message_dialog(owner,"Menu Manager","The selected menu file could not be opened.",MSG_ERROR);return;}
  strcpy(manager_original_path,menu_path);
  proc=MakeProcInstance((FARPROC)ManagerDlgProc,gInst);if(!proc)return;
  menu_tracking=1;rc=DialogBox(gInst,MAKEINTRESOURCE(IDD_MANAGER),owner,proc);menu_tracking=0;
  FreeProcInstance(proc);managerWnd=NULL;
  /* The child controls are now destroyed, so their subclass thunks can be
     released without racing a final WM_DESTROY callback. */
  if(gManagerTreeInstProc){FreeProcInstance(gManagerTreeInstProc);gManagerTreeInstProc=NULL;}
  if(gManagerBannerInstProc){FreeProcInstance(gManagerBannerInstProc);gManagerBannerInstProc=NULL;}
  gManagerTreeProc=NULL;gManagerBannerProc=NULL;
  /* Browsing in Menu Manager is an editing choice, not a permanent change to
     the resident launcher's active menu. */
  strncpy(menu_path,manager_original_path,sizeof(menu_path)-1);menu_path[sizeof(menu_path)-1]=0;load_launch_menu();
  if(gWnd)SetActiveWindow(gWnd);
  if(rc==-1)show_message_dialog(owner,"Menu Manager","Could not create the Menu Manager dialog.",MSG_ERROR);
}

static void launch_menu_manager_program(void)
{
  char command[360];UINT rc;
  /* Menu Manager is a separate Win16 task.  This avoids entering its modal
     dialog from the resident Launch! button task and also lets Program
     Manager invoke exactly the same executable directly. */
  sprintf(command,"%s\\!MNUMAN.EXE %s",base_dir,menu_path);
  rc=(UINT)WinExec(command,SW_SHOWNORMAL);
  if(rc<32)show_message_dialog(gWnd,"Menu Manager","Windows could not start !MNUMAN.EXE.",MSG_ERROR);
}

static void show_management_menu(void)
{
  HMENU menu,add;POINT pt;RECT wr;UINT flags;HWND previous_active,previous_focus;
  manage_mode=MANAGE_MENU;resize_button_for_mode();
  menu=CreatePopupMenu();add=CreatePopupMenu();
  if(!menu||!add){
    if(menu)DestroyMenu(menu);if(add)DestroyMenu(add);
    manage_mode=MANAGE_NONE;resize_button_for_mode();return;
  }
  AppendMenu(add,MF_STRING,IDM_MGMT_ADD_LAUNCHER,"Launcher");
  AppendMenu(add,MF_STRING,IDM_MGMT_ADD_FOLDER,"Folder");
  AppendMenu(add,MF_STRING,IDM_MGMT_ADD_SEPARATOR,"Separator");
  if(ensure_menu_header_bitmaps())AppendMenu(menu,MF_OWNERDRAW|MF_DISABLED,IDM_MENU_BANNER,NULL);
  AppendMenu(menu,MF_POPUP,(UINT)add,"Add");
  AppendMenu(menu,MF_STRING,IDM_MGMT_EDIT,"Edit");
  AppendMenu(menu,MF_STRING,IDM_MGMT_REMOVE,"Remove");
  AppendMenu(menu,MF_SEPARATOR,0,NULL);
  AppendMenu(menu,MF_STRING,IDM_MGMT_REORDER,"Menu Manager...");
  begin_popup_tracking(&previous_active,&previous_focus);
  GetWindowRect(gWnd,&wr);pt.x=wr.left;flags=0;
  if(button_edge){pt.y=wr.top;if(launch_win31_or_later())flags|=TPM_BOTTOMALIGN;}else pt.y=wr.bottom;
  queue_initial_menu_focus();
  TrackPopupMenu(menu,flags,pt.x,pt.y,0,gWnd,NULL);
  DestroyMenu(menu);
  end_popup_tracking(previous_active,previous_focus);
  manage_mode=MANAGE_NONE;resize_button_for_mode();
}

static void begin_manage_mode(int mode)
{
  if(menu_tracking)return;
  manage_mode=mode;resize_button_for_mode();
  show_launch_menu();
  manage_mode=MANAGE_NONE;resize_button_for_mode();
}

static const char *current_button_text(void)
{
  if(manage_mode==MANAGE_EDIT)return "Edit...";
  if(manage_mode==MANAGE_REMOVE)return "Remove...";
  if(manage_mode==MANAGE_MENU)return "Manage";
  return clock_mode?clock_text:suite_title;
}

static void button_size(int *bw,int *bh)
{
  int cw=GetSystemMetrics(SM_CXSIZE),ch=GetSystemMetrics(SM_CYSIZE);
  int tw=0;HDC dc;HFONT oldf=NULL;DWORD ext;const char *label=current_button_text();
  dc=GetDC(NULL);
  if(dc){
    if(gButtonFont)oldf=(HFONT)SelectObject(dc,gButtonFont);
    ext=GetTextExtent(dc,label,strlen(label));tw=(int)LOWORD(ext);
    if(oldf)SelectObject(dc,oldf);
    ReleaseDC(NULL,dc);
  }
  *bw=tw+14;if(*bw<cw+8)*bw=cw+8;
  *bh=ch>18?ch:18;
}

static void resize_button_for_mode(void)
{
  int bw,bh,sw,sh,by;
  if(!gWnd)return;
  if(gButton)SetWindowText(gButton,current_button_text());
  button_size(&bw,&bh);
  sw=GetSystemMetrics(SM_CXSCREEN);sh=GetSystemMetrics(SM_CYSCREEN);
  if(button_x<0)button_x=0;if(button_x>sw-bw)button_x=sw-bw;
  by=button_edge?sh-bh:0;
  SetWindowPos(gWnd,HWND_TOP,button_x,by,bw,bh,SWP_NOACTIVATE);
  if(gButton){MoveWindow(gButton,0,0,bw,bh,TRUE);InvalidateRect(gButton,NULL,TRUE);UpdateWindow(gButton);}
  else InvalidateRect(gWnd,NULL,FALSE);
}

static void raise_button(HWND h)
{
  /* Z-order only: never change the canonical (24,0) geometry here. */
  SetWindowPos(h,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}

static int execute_run_dialog(HWND h)
{
  char cmd[256];int show=IsDlgButtonChecked(h,RUN_MIN)?SW_SHOWMINIMIZED:SW_SHOWNORMAL;
  GetDlgItemText(h,RUN_EDIT,cmd,sizeof(cmd));
  if(!cmd[0])return 0;
  if(WinExec(cmd,show)<32){show_message_dialog(h,"Run","Windows could not run this command.",MSG_ERROR);return 0;}
  return 1;
}

static int browse_for_program(HWND owner,char *file,int maxfile)
{
  HINSTANCE lib;FARPROC proc;OPENFILENAME of;BOOL ok;
  lib=LoadLibrary("COMMDLG.DLL");
  if((UINT)lib<32){
    show_message_dialog(owner,"Launch!","Browse requires the Windows common-dialog library.",MSG_INFO);
    return 0;
  }
  proc=GetProcAddress(lib,"GetOpenFileName");
  if(!proc){FreeLibrary(lib);return 0;}
  memset(&of,0,sizeof(of));of.lStructSize=sizeof(of);of.hwndOwner=owner;
  of.lpstrFilter="Programs (*.EXE;*.COM;*.BAT)\0*.EXE;*.COM;*.BAT\0All Files (*.*)\0*.*\0\0";
  of.lpstrFile=file;of.nMaxFile=maxfile;of.Flags=OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
  ok=((BOOL (FAR PASCAL *)(LPOPENFILENAME))proc)(&of);
  FreeLibrary(lib);return ok?1:0;
}


static int browse_for_menu(HWND owner,char *file,int maxfile)
{
  HINSTANCE lib;FARPROC proc;OPENFILENAME of;BOOL ok;
  lib=LoadLibrary("COMMDLG.DLL");
  if((UINT)lib<32){show_message_dialog(owner,"Menu Manager","Browse requires the Windows common-dialog library.",MSG_INFO);return 0;}
  proc=GetProcAddress(lib,"GetOpenFileName");if(!proc){FreeLibrary(lib);return 0;}
  memset(&of,0,sizeof(of));of.lStructSize=sizeof(of);of.hwndOwner=owner;
  of.lpstrFilter="Launch! Menus (*.MNU)\0*.MNU\0All Files (*.*)\0*.*\0\0";
  of.lpstrFile=file;of.nMaxFile=maxfile;of.Flags=OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
  ok=((BOOL (FAR PASCAL *)(LPOPENFILENAME))proc)(&of);FreeLibrary(lib);return ok?1:0;
}

static BOOL FAR PASCAL RunDlgProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  switch(msg){
    case WM_INITDIALOG:
      runWnd=h;center_dialog(h);runEdit=GetDlgItem(h,RUN_EDIT);runMin=GetDlgItem(h,RUN_MIN);
      SetFocus(runEdit);return FALSE;
    case WM_COMMAND:
      if(wp==IDOK){if(execute_run_dialog(h))EndDialog(h,IDOK);return TRUE;}
      if(wp==IDCANCEL){EndDialog(h,IDCANCEL);return TRUE;}
      if(wp==RUN_BROWSE){char file[160]="";if(browse_for_program(h,file,sizeof(file)))SetWindowText(runEdit,file);return TRUE;}
      break;
    case WM_CLOSE:EndDialog(h,IDCANCEL);return TRUE;
    case WM_DESTROY:runWnd=NULL;return TRUE;
  }
  return FALSE;
}

static void show_run_dialog(void)
{
  FARPROC proc;int rc;
  if(runWnd){SetActiveWindow(runWnd);return;}
  proc=MakeProcInstance((FARPROC)RunDlgProc,gInst);if(!proc)return;
  menu_tracking=1;
  rc=DialogBox(gInst,MAKEINTRESOURCE(IDD_RUN),gWnd,proc);
  menu_tracking=0;
  FreeProcInstance(proc);runWnd=NULL;SetActiveWindow(gWnd);
  if(rc==-1)show_message_dialog(gWnd,"Launch!","Could not create the Run dialog.",MSG_ERROR);
}

static BOOL FAR PASCAL ConfirmDlgProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  (void)lp;
  switch(msg){
    case WM_INITDIALOG:
      center_dialog(h);SetWindowText(h,confirm_title);SetDlgItemText(h,CONFIRM_TEXT,confirm_text);
      SetFocus(GetDlgItem(h,IDNO));return FALSE;
    case WM_COMMAND:
      if(wp==IDYES){EndDialog(h,IDYES);return TRUE;}
      if(wp==IDNO||wp==IDCANCEL){EndDialog(h,IDNO);return TRUE;}
      break;
    case WM_CLOSE:EndDialog(h,IDNO);return TRUE;
  }
  return FALSE;
}

static BOOL FAR PASCAL MessageDlgProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  HWND erricon,okicon,qicon;
  (void)lp;
  switch(msg){
    case WM_INITDIALOG:
      center_dialog(h);SetWindowText(h,message_title);SetDlgItemText(h,MESSAGE_TEXT,message_text);
      erricon=GetDlgItem(h,MESSAGE_ERROR_ICON);okicon=GetDlgItem(h,MESSAGE_SUCCESS_ICON);qicon=GetDlgItem(h,MESSAGE_CONFIRM_ICON);
      if(erricon)ShowWindow(erricon,message_kind==MSG_ERROR?SW_SHOW:SW_HIDE);
      if(okicon)ShowWindow(okicon,(message_kind==MSG_INFO||message_kind==MSG_SUCCESS)?SW_SHOW:SW_HIDE);
      if(qicon)ShowWindow(qicon,message_kind==MSG_WARNING?SW_SHOW:SW_HIDE);
      SetFocus(GetDlgItem(h,IDOK));return FALSE;
    case WM_COMMAND:
      if(wp==IDOK||wp==IDCANCEL){EndDialog(h,IDOK);return TRUE;}
      break;
    case WM_CLOSE:EndDialog(h,IDOK);return TRUE;
  }
  return FALSE;
}

static void show_message_dialog(HWND owner,const char *title,const char *text,int kind)
{
  FARPROC proc;int rc;UINT mbflags=MB_OK;
  if(!owner)owner=gWnd;
  strncpy(message_title,title,sizeof(message_title)-1);message_title[sizeof(message_title)-1]=0;
  strncpy(message_text,text,sizeof(message_text)-1);message_text[sizeof(message_text)-1]=0;
  message_kind=kind;
  proc=MakeProcInstance((FARPROC)MessageDlgProc,gInst);
  if(!proc){
    if(kind==MSG_ERROR)mbflags|=MB_ICONSTOP;
    else if(kind==MSG_WARNING)mbflags|=MB_ICONEXCLAMATION;
    else mbflags|=MB_ICONINFORMATION;
    MessageBox(owner,text,title,mbflags);return;
  }
  rc=DialogBox(gInst,MAKEINTRESOURCE(IDD_MESSAGE),owner,proc);
  FreeProcInstance(proc);
  if(rc==-1){
    if(kind==MSG_ERROR)mbflags|=MB_ICONSTOP;
    else if(kind==MSG_WARNING)mbflags|=MB_ICONEXCLAMATION;
    else mbflags|=MB_ICONINFORMATION;
    MessageBox(owner,text,title,mbflags);
  }
}

static int show_confirm_dialog_resource(int dialog_id,const char *title,const char *text)
{
  FARPROC proc;int rc,old_tracking=menu_tracking;
  strncpy(confirm_title,title,sizeof(confirm_title)-1);confirm_title[sizeof(confirm_title)-1]=0;
  strncpy(confirm_text,text,sizeof(confirm_text)-1);confirm_text[sizeof(confirm_text)-1]=0;
  proc=MakeProcInstance((FARPROC)ConfirmDlgProc,gInst);if(!proc)return 0;
  confirm_dialog_id=dialog_id;
  { HWND owner=managerWnd?managerWnd:gWnd;
    menu_tracking=1;rc=DialogBox(gInst,MAKEINTRESOURCE(dialog_id),owner,proc);menu_tracking=old_tracking;
    FreeProcInstance(proc);SetActiveWindow(owner);
    if(rc==-1){MessageBox(owner,"Could not create the confirmation dialog.","Launch!",MB_OK|MB_ICONSTOP);return 0;}
  }
  return rc==IDYES;
}

static int show_confirm_dialog(const char *title,const char *text)
{
  return show_confirm_dialog_resource(IDD_CONFIRM,title,text);
}

static int show_exit_windows_dialog(void)
{
  return show_confirm_dialog_resource(IDD_EXITWIN,"Exit Windows","Exit Windows and return\r\nto the DOS prompt?");
}

/* Windows 3.0 fallback button.  The first Launch! W31 implementation used
   the popup window itself as the button and is known to run on Windows 3.0.
   Keep the later native child BUTTON on Windows 3.1+, but avoid the Win3.0
   child/subclass paint path entirely. */
static void paint_win30_button(HWND h,HDC dc,int pressed)
{
  RECT r;HBRUSH face;HPEN hi,sh,oldp;HFONT oldf=NULL;
  GetClientRect(h,&r);
  face=CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
  FillRect(dc,&r,face);DeleteObject(face);
  /* COLOR_BTNHIGHLIGHT is not exposed by the Windows 3.0 SDK.
     COLOR_WINDOW is the native Win3.0 raised-edge highlight colour. */
  hi=CreatePen(PS_SOLID,1,GetSysColor(COLOR_WINDOW));
  sh=CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));
  oldp=(HPEN)SelectObject(dc,pressed?sh:hi);
  MoveTo(dc,r.left,r.bottom-1);LineTo(dc,r.left,r.top);LineTo(dc,r.right-1,r.top);
  SelectObject(dc,pressed?hi:sh);
  MoveTo(dc,r.right-1,r.top);LineTo(dc,r.right-1,r.bottom-1);LineTo(dc,r.left,r.bottom-1);
  SelectObject(dc,oldp);DeleteObject(hi);DeleteObject(sh);
  SetBkMode(dc,TRANSPARENT);SetTextColor(dc,GetSysColor(COLOR_BTNTEXT));
  if(gButtonFont)oldf=(HFONT)SelectObject(dc,gButtonFont);
  if(pressed)OffsetRect(&r,1,1);
  DrawText(dc,current_button_text(),-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  if(oldf)SelectObject(dc,oldf);
}

static LRESULT FAR PASCAL ButtonProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  if(msg==WM_RBUTTONUP){SendMessage(gWnd,WM_RBUTTONUP,wp,lp);return 0;}
  if(msg==WM_LBUTTONDOWN && GetKeyState(VK_MENU)<0){SendMessage(gWnd,WM_BEGIN_BUTTON_DRAG,0,0L);return 0;}
  return CallWindowProc(gButtonProc,h,msg,wp,lp);
}

static LRESULT FAR PASCAL WndProc(HWND h,UINT msg,WPARAM wp,LPARAM lp)
{
  static int pressed=0;
  static int dragging=0;
  static int drag_grab_x=0;
  switch(msg){
    case WM_DDE_ACK:
      if(dde_initiating){dde_server=(HWND)wp;dde_initiating=0;return 0;}
      if(dde_waiting){ATOM a=(ATOM)HIWORD(lp);if(a)GlobalDeleteAtom(a);dde_request_ok=0;dde_waiting=0;return 0;}
      return 0;
    case WM_DDE_DATA:{
      HGLOBAL hData=(HGLOBAL)LOWORD(lp);ATOM a=(ATOM)HIWORD(lp);DDEDATA FAR *dd=NULL;int ack=0,release=0;
      if(hData)dd=(DDEDATA FAR*)GlobalLock(hData);
      if(dd && dd->cfFormat==CF_TEXT){
        DWORD len=(DWORD)lstrlen((LPSTR)dd->Value)+1L;LPSTR dst;HGLOBAL cp=GlobalAlloc(GMEM_MOVEABLE,len);
        ack=dd->fAckReq?1:0;release=dd->fRelease?1:0;
        if(cp && (dst=(LPSTR)GlobalLock(cp))!=NULL){lstrcpy(dst,(LPSTR)dd->Value);GlobalUnlock(cp);if(dde_reply)GlobalFree(dde_reply);dde_reply=cp;dde_request_ok=1;}
        else if(cp)GlobalFree(cp);
      }
      if(dd)GlobalUnlock(hData);
      if(ack && dde_server)PostMessage(dde_server,WM_DDE_ACK,(WPARAM)h,MAKELONG(0x8000,a));
      else if(a)GlobalDeleteAtom(a);
      if(release && hData)GlobalFree(hData);
      dde_waiting=0;return 0;}
    case WM_DDE_TERMINATE:
      if((HWND)wp==dde_server){HWND server=dde_server;dde_server=NULL;dde_waiting=0;dde_initiating=0;PostMessage(server,WM_DDE_TERMINATE,(WPARAM)h,0L);}return 0;
    case WM_CREATE:
      /* Keep parent creation deliberately empty for Windows 3.0.  The native
         BUTTON child, subclass and raise timer are installed from WinMain
         only after the popup host CreateWindow has returned successfully. */
      return 0;
    case WM_SHOW_LAUNCH_MENU:
      if(lp){clock_mode=1;clock_locale();clock_update();resize_button_for_mode();}
      /* A helper invocation only schedules the resident popup.  It exits
         immediately; the resident opens the menu later from its own task,
         after Windows has completed the Program Manager/task activation
         hand-off.  This avoids the native popup being dismissed with the
         helper task. */
      if(wp){
        char path[160];path[0]=0;GlobalGetAtomName((ATOM)wp,path,sizeof(path));GlobalDeleteAtom((ATOM)wp);
        if(path[0]){strncpy(menu_path,path,sizeof(menu_path)-1);menu_path[sizeof(menu_path)-1]=0;}
      }
      if(!menu_tracking && !runWnd){menu_open_pending=1;menu_open_delay=5;}
      return 0;
    case WM_SHOW_MENU_MANAGER:
      if(wp){
        char path[160];path[0]=0;GlobalGetAtomName((ATOM)wp,path,sizeof(path));GlobalDeleteAtom((ATOM)wp);
        if(path[0]){strncpy(menu_path,path,sizeof(menu_path)-1);menu_path[sizeof(menu_path)-1]=0;}
      }
      if(!menu_tracking&&!runWnd)launch_menu_manager_program();
      return 0;
    case WM_ACTIVATEAPP:
      /* Ordinary activation (including Alt+Tab) must never open the menu.
         Program Manager's Ctrl+Alt+\\ shortcut can activate an existing
         single-instance task while the actual shortcut keys are still down;
         accept only that exact key state as an invocation request. */
      if(wp && !menu_tracking && !runWnd &&
         GetKeyState(VK_CONTROL)<0 && GetKeyState(VK_MENU)<0 && GetKeyState(VK_OEM_5)<0){
        menu_open_pending=1;menu_open_delay=1;
      } else if(!wp && !menu_tracking){
        SetWindowPos(h,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        InvalidateRect(h,NULL,FALSE);
      }
      return 0;
    case WM_SYSCOMMAND:
      /* Windows also reports an application hot key explicitly as SC_HOTKEY.
         This path is distinct from normal focus/activation and therefore safe
         from Alt+Tab false positives. */
      if((wp&0xFFF0)==SC_HOTKEY){
        if(!menu_tracking && !runWnd){menu_open_pending=1;menu_open_delay=1;}
        return 0;
      }
      break;
    case WM_WININICHANGE:
      clock_locale();if(clock_update())resize_button_for_mode();return 0;
    case WM_TIMER:
      if(wp!=RAISE_TIMER)return 0;
      if(clock_update() && manage_mode==MANAGE_NONE){if(menu_tracking)InvalidateRect(h,NULL,FALSE);else resize_button_for_mode();}
      if(menu_tracking)return 0;
      if(menu_open_pending && !runWnd){
        if(GetKeyState(VK_CONTROL)<0 || GetKeyState(VK_MENU)<0 || GetKeyState(VK_OEM_5)<0)menu_open_delay=3;
        else if(menu_open_delay>0)menu_open_delay--;
        else {
          menu_open_pending=0;
          BringWindowToTop(h);SetActiveWindow(h);if(gButton)SetFocus(gButton);
          PostMessage(h,WM_OPEN_PENDING_MENU,0,0L);
          return 0;
        }
      }
      if(GetTopWindow(NULL)!=h){
        SetWindowPos(h,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        InvalidateRect(h,NULL,FALSE);
      }
      return 0;
    case WM_OPEN_PENDING_MENU:
      if(!menu_tracking && !runWnd)show_launch_menu();
      return 0;
    case WM_MOUSEACTIVATE:
      /* Clicking the desktop button must not itself activate the resident
         task; the button command opens the menu directly. */
      return MA_NOACTIVATE;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT ps;HDC pdc=BeginPaint(h,&ps);paint_win30_button(h,pdc,pressed);EndPaint(h,&ps);return 0;}
    case WM_LBUTTONDOWN:
      if(GetKeyState(VK_MENU)<0){POINT pt;RECT wr;GetCursorPos(&pt);GetWindowRect(h,&wr);dragging=1;pressed=0;drag_grab_x=pt.x-wr.left;SetCapture(h);return 0;}
      pressed=1;SetCapture(h);InvalidateRect(h,NULL,FALSE);return 0;
    case WM_SIZE:
      if(gButton)MoveWindow(gButton,0,0,LOWORD(lp),HIWORD(lp),TRUE);
      return 0;
    case WM_BEGIN_BUTTON_DRAG:{
      POINT pt;RECT wr;GetCursorPos(&pt);GetWindowRect(h,&wr);
      dragging=1;drag_grab_x=pt.x-wr.left;SetCapture(h);return 0;}
    case WM_MOUSEMOVE:
      if(dragging){
        POINT pt;RECT wr;int sw,sh,bw,bh,nx,ny;
        GetCursorPos(&pt);GetWindowRect(h,&wr);
        bw=wr.right-wr.left;bh=wr.bottom-wr.top;
        sw=GetSystemMetrics(SM_CXSCREEN);sh=GetSystemMetrics(SM_CYSCREEN);
        nx=pt.x-drag_grab_x;if(nx<0)nx=0;if(nx>sw-bw)nx=sw-bw;
        button_edge=pt.y>=sh/2?1:0;ny=button_edge?sh-bh:0;
        button_x=nx;
        SetWindowPos(h,HWND_TOP,nx,ny,0,0,SWP_NOSIZE|SWP_NOACTIVATE);
        return 0;
      }
      break;
    case WM_LBUTTONUP:
      if(dragging){dragging=0;ReleaseCapture();save_button_position();InvalidateRect(h,NULL,FALSE);return 0;}
      if(pressed){POINT pt;RECT r;pressed=0;ReleaseCapture();InvalidateRect(h,NULL,FALSE);pt.x=(int)(short)LOWORD(lp);pt.y=(int)(short)HIWORD(lp);GetClientRect(h,&r);if(PtInRect(&r,pt)&&!menu_tracking&&!runWnd)show_launch_menu();return 0;}
      break;
    case WM_RBUTTONUP:
      if(!menu_tracking && !runWnd)show_management_menu();
      return 0;
    case WM_MANAGE_ACTION:
      if(wp==IDM_MGMT_ADD_LAUNCHER){if(load_launch_menu())add_menu_item(0);return 0;}
      if(wp==IDM_MGMT_ADD_FOLDER){if(load_launch_menu())add_menu_item(1);return 0;}
      if(wp==IDM_MGMT_ADD_SEPARATOR){if(load_launch_menu())add_menu_separator();return 0;}
      if(wp==IDM_MGMT_EDIT){begin_manage_mode(MANAGE_EDIT);return 0;}
      if(wp==IDM_MGMT_REMOVE){begin_manage_mode(MANAGE_REMOVE);return 0;}
      if(wp==IDM_MGMT_REORDER){launch_menu_manager_program();return 0;}
      return 0;
    case WM_MANAGE_SELECTED:
      if((int)lp==MANAGE_EDIT)edit_menu_node((int)wp);
      else if((int)lp==MANAGE_REMOVE)remove_menu_node((int)wp);
      return 0;
    case WM_MEASUREITEM:
      if(measure_menu_header((MEASUREITEMSTRUCT FAR *)lp))return TRUE;
      break;
    case WM_DRAWITEM:
      if(draw_menu_header((DRAWITEMSTRUCT FAR *)lp))return TRUE;
      break;
    case WM_COMMAND:
      if(wp==BTN_ID){if(!menu_tracking&&!runWnd)show_launch_menu();return 0;}
      if(wp>=IDM_MGMT_ADD_LAUNCHER && wp<=IDM_MGMT_REORDER){PostMessage(h,WM_MANAGE_ACTION,wp,0L);return 0;}
      if(wp==IDM_RUN){show_run_dialog();return 0;}
      if(wp==IDM_EXPLORE){WinExec("WINFILE.EXE",SW_SHOWNORMAL);return 0;}
      if(wp==IDM_EXITWIN){if(show_exit_windows_dialog())ExitWindows(0,0);return 0;}
      if(wp>=IDM_EDIT_FIRST&&wp<IDM_EDIT_FIRST+MAX_NODES){edit_menu_node((int)wp-IDM_EDIT_FIRST);return 0;}
      if(wp>=IDM_REMOVE_FIRST&&wp<IDM_REMOVE_FIRST+MAX_NODES){remove_menu_node((int)wp-IDM_REMOVE_FIRST);return 0;}
      if(wp>=IDM_FIRST&&wp<IDM_FIRST+MAX_NODES){run_item((int)wp-IDM_FIRST);return 0;}
      break;
    case WM_DESTROY:KillTimer(h,RAISE_TIMER);gButton=NULL;PostQuitMessage(0);return 0;
  }
  return DefWindowProc(h,msg,wp,lp);
}

#ifdef MNUMAN_BUILD
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{
  HDC dc;DWORD winver;(void)prev;(void)show;gInst=inst;
  winver=GetVersion();gWin30=(LOBYTE(LOWORD(winver))==3 && HIBYTE(LOWORD(winver))==0);
  if(!get_base_dir())return 1;
  set_menu_path_from_command_line(cmd);start_menu_manager=0;
  gWindowBrush=CreateSolidBrush(GetSysColor(COLOR_WINDOW));
  load_win_preferences();
  dc=GetDC(NULL);
  gDialogFont=CreateFont(-MulDiv(8,GetDeviceCaps(dc,LOGPIXELSY),72),0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH|FF_SWISS,"MS Sans Serif");
  gButtonFont=CreateFont(-MulDiv(8,GetDeviceCaps(dc,LOGPIXELSY),72),0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH|FF_SWISS,"MS Sans Serif");
  ReleaseDC(NULL,dc);
  show_menu_manager_dialog();
  if(dde_reply)GlobalFree(dde_reply);
  if(gMenuLogoBmp)DeleteObject(gMenuLogoBmp);
  if(gMenuTileBmp)DeleteObject(gMenuTileBmp);
  if(gButtonFont)DeleteObject(gButtonFont);
  if(gDialogFont)DeleteObject(gDialogFont);
  if(gWindowBrush)DeleteObject(gWindowBrush);
  return 0;
}
#else
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{
  WNDCLASS wc;MSG msg;HDC dc;HWND prior;int menu_ok;UINT request;ATOM a;RECT br;UINT timer_id;DWORD winver;(void)show;gInst=inst;
  winver=GetVersion();gWin30=(LOBYTE(LOWORD(winver))==3 && HIBYTE(LOWORD(winver))==0);
  debug_open();debug_msg("01 WinMain entered");debug_val("02 hInstance",(unsigned long)inst);debug_val("03 hPrevInstance",(unsigned long)prev);debug_val("04 GetVersion",(unsigned long)winver);
  debug_msg("05 before get_base_dir");
  if(!get_base_dir()){debug_msg("06 get_base_dir FAILED");return 1;}
  debug_msg("06 get_base_dir OK");debug_msg(menu_path);
  set_menu_path_from_command_line(cmd);debug_msg("07 command line parsed");
  if(start_menu_manager){
    debug_msg("07a /MANAGE redirects to standalone !MNUMAN.EXE");
    launch_menu_manager_program();
    if(gDebugFile){fclose(gDebugFile);gDebugFile=NULL;}
    return 0;
  }
  /* Single resident instance.  A helper copy posts a request then exits;
     the resident deliberately waits several timer ticks before opening the
     popup so task termination/Program Manager activation is already over. */
  prior=FindWindow("LaunchW31Button",NULL);
  if(close_request){if(prior)PostMessage(prior,WM_CLOSE,0,0L);return 0;}debug_val("08 prior resident hwnd",(unsigned long)prior);
  if(prior){debug_msg("09 handing off to prior resident");
    request=WM_SHOW_LAUNCH_MENU;
    a=0;
    if(menu_path_explicit)a=GlobalAddAtom(menu_path);
    BringWindowToTop(prior);SetActiveWindow(prior);
    /* Menu Manager is modal, so a synchronous cross-task request is the
       reliable Win16 hand-off: the helper remains alive until the manager
       closes.  The normal launcher popup retains its deferred PostMessage
       path because native popup menus must not overlap helper termination. */
    if(!PostMessage(prior,request,(WPARAM)a,(LPARAM)clock_mode) && a){
      GlobalDeleteAtom(a);
    }
    return 0;
  }
  debug_msg("10 creating startup objects");
  gWindowBrush=CreateSolidBrush(GetSysColor(COLOR_WINDOW));debug_val("11 window brush",(unsigned long)gWindowBrush);load_win_preferences();debug_msg("12 preferences loaded");
  dc=GetDC(NULL);debug_val("13 screen DC",(unsigned long)dc);
  gDialogFont=CreateFont(-MulDiv(8,GetDeviceCaps(dc,LOGPIXELSY),72),0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH|FF_SWISS,"MS Sans Serif");
  gButtonFont=CreateFont(-MulDiv(8,GetDeviceCaps(dc,LOGPIXELSY),72),0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH|FF_SWISS,"MS Sans Serif");
  debug_val("14 dialog font",(unsigned long)gDialogFont);debug_val("15 button font",(unsigned long)gButtonFont);
  ReleaseDC(NULL,dc);debug_msg("16 fonts complete");
  if(!prev){
    debug_msg("17 creating host WndProc instance thunk");
    gHostInstProc=MakeProcInstance((FARPROC)WndProc,inst);
    if(!gHostInstProc){debug_msg("17a host MakeProcInstance FAILED");MessageBox(NULL,"Could not initialise Launch! W31 window procedure.","Launch!",MB_OK|MB_ICONSTOP);return 1;}
    debug_msg("17 registering host class");memset(&wc,0,sizeof(wc));wc.lpfnWndProc=(WNDPROC)gHostInstProc;wc.hInstance=inst;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=NULL;wc.lpszClassName="LaunchW31Button";
    if(!RegisterClass(&wc)){debug_msg("18 RegisterClass FAILED");MessageBox(NULL,"Could not register Launch! W31 window class.","Launch!",MB_OK|MB_ICONSTOP);return 1;}
    debug_msg("18 RegisterClass OK");
  } else debug_msg("17 class registration skipped because hPrevInstance != 0");
  /* Use the native Windows BUTTON.  The host WndProc and subclass callback
     are instance-thunked above so Windows 3.0 always enters them with this
     task's data segment established.  The popup host itself has no background
     brush, preserving the transparent rounded corners of the stock button. */
  debug_msg("19 before host CreateWindow");
  { int bw,bh,sw,sh,by; button_size(&bw,&bh);debug_val("19a button width",(unsigned long)bw);debug_val("19b button height",(unsigned long)bh);
    sw=GetSystemMetrics(SM_CXSCREEN);sh=GetSystemMetrics(SM_CYSCREEN);
    if(button_x<0)button_x=0;if(button_x>sw-bw)button_x=sw-bw;
    by=button_edge?sh-bh:0;
    gWnd=CreateWindow("LaunchW31Button","",WS_POPUP,button_x,by,bw,bh,NULL,NULL,inst,NULL);
  }
  debug_val("27 host hwnd",(unsigned long)gWnd);
  if(!gWnd){debug_msg("28 host CreateWindow FAILED");MessageBox(NULL,"Could not create Launch! W31 button.","Launch!",MB_OK|MB_ICONSTOP);return 1;}

  /* Use the same square, self-painted Launch! button on Windows 3.1/3.11 as
     the Windows 3.0 companion.  Only the button implementation is shared;
     the proven W31 menu/dialog architecture remains otherwise unchanged. */
  gButton=NULL;
  debug_msg("28a square custom host-button path selected");
  timer_id=SetTimer(gWnd,RAISE_TIMER,100,NULL);
  debug_val("28j SetTimer result",(unsigned long)timer_id);
  debug_msg("28k startup construction complete");
  /* Close the diagnostic stream before the window becomes visible.  DOS/Win3.0
     file I/O can yield to USER; once painting begins no C-runtime logging is
     allowed to re-enter the task. */
  if(gDebugFile){fclose(gDebugFile);gDebugFile=NULL;}
  ShowWindow(gWnd,SW_SHOWNOACTIVATE);
  SetWindowPos(gWnd,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
  /* The host popup has no pixels of its own: the stock BUTTON covers its
     complete client area.  Do not synchronously paint the host on Windows
     3.x; let USER dispatch the child control's normal WM_PAINT from the
     application message loop. */
  menu_ok=ensure_menu_file();
  while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
  if(dde_reply)GlobalFree(dde_reply);
  if(gMenuLogoBmp)DeleteObject(gMenuLogoBmp);
  if(gMenuTileBmp)DeleteObject(gMenuTileBmp);
  if(gButtonFont)DeleteObject(gButtonFont);
  if(gDialogFont)DeleteObject(gDialogFont);
  if(gWindowBrush)DeleteObject(gWindowBrush);
  debug_msg("99 WinMain returning");if(gDebugFile){fclose(gDebugFile);gDebugFile=NULL;}
  return (int)msg.wParam;
}

#endif /* MNUMAN_BUILD */
