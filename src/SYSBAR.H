/* SysBar display and actions share exact hit rectangles. */
static char sysbar_command[32];
static int sysbar_x[6],sysbar_y[6],sysbar_w[6];
static unsigned sysbar_country;
static int sysbar_rows(void){return appearance.show_sysbar?(screen_cols==40?2:1):0;}
static unsigned sysbar_read_country(void)
{
  union REGS r;struct SREGS sr;static unsigned char info[34];
  memset(&r,0,sizeof(r));segread(&sr);r.x.ax=0x3800;
  sr.ds=FP_SEG(info);r.x.dx=FP_OFF(info);int86x(0x21,&r,&r,&sr);
  return r.x.cflag?0:r.x.bx;
}
static const char *sysbar_country_name(unsigned c)
{
  switch(c){
    case 1:return "United States";case 2:return "Canada";
    case 3:return "Latin America";case 7:return "Russia";
    case 20:return "Egypt";case 27:return "South Africa";
    case 30:return "Greece";case 31:return "Netherlands";
    case 32:return "Belgium";case 33:return "France";
    case 34:return "Spain";case 36:return "Hungary";
    case 39:return "Italy";case 40:return "Romania";
    case 41:return "Switzerland";case 42:return "Czechoslovakia";
    case 43:return "Austria";case 44:return "United Kingdom";
    case 45:return "Denmark";case 46:return "Sweden";
    case 47:return "Norway";case 48:return "Poland";
    case 49:return "Germany";case 52:return "Mexico";
    case 54:return "Argentina";case 55:return "Brazil";
    case 56:return "Chile";case 57:return "Colombia";
    case 58:return "Venezuela";case 60:return "Malaysia";
    case 61:return "Australia";case 62:return "Indonesia";
    case 63:return "Philippines";case 64:return "New Zealand";
    case 65:return "Singapore";case 66:return "Thailand";
    case 81:return "Japan";case 82:return "South Korea";
    case 84:return "Vietnam";case 86:return "China";
    case 90:return "Turkey";case 91:return "India";
    case 351:return "Portugal";case 352:return "Luxembourg";
    case 353:return "Ireland";case 354:return "Iceland";
    case 358:return "Finland";case 886:return "Taiwan";
    case 972:return "Israel";default:return "Unknown country";
  }
}
static void sysbar_region(int id,int x,int y,int width)
{sysbar_x[id]=x;sysbar_y[id]=y;sysbar_w[id]=width;}
static void draw_sysbar(void)
{
  char s[160],disk[20],vname[4],country[16],*p;int x,y,w,i,net,locks;
  int base=ATTR(appearance.controls_bg,appearance.controls_fg);
  int dim=ATTR(appearance.controls_bg,(appearance.controls_fg&7)|8);
  int active=ATTR(appearance.controls_bg,appearance.launchers);union REGS r;
  unsigned mem=sysbar_largest_kb(),env=sysbar_env_free_kb();
  sysbar_drive_free(disk);net=sysbar_packet_driver();
  locks=*(unsigned char far *)MAKE_FP(0x40,0x17);sysbar_country=sysbar_read_country();
  memset(&r,0,sizeof(r));r.x.ax=0x1A00;int86(0x10,&r,&r);
  strcpy(vname,r.h.al==0x1A?"VGA":"EGA");
  sprintf(country,"%u-%s",sysbar_country,sysbar_country==61?"AUS":sysbar_country==1?"USA":sysbar_country==44?"GBR":"INT");
  for(i=0;i<6;i++)sysbar_w[i]=0;
  sprintf(s,"  MEM %uK \263 ENV %uK \263 %s",mem,env,disk);
  if(screen_cols!=40){strcat(s," \263 NL CL SL \263 MOUSE ");strcat(s,vname);strcat(s," NET \263 ");strcat(s,country);strcat(s,"  ");}
  w=strlen(s);x=screen_cols>w?screen_cols-w:0;y=0;textout(x,y,s,base,screen_cols-x);
  p=strstr(s," \263 ");sysbar_region(0,x+2,0,p?(int)(p-s)-2:8);
  p=strstr(s,disk);if(p)sysbar_region(1,x+(int)(p-s),0,strlen(disk));
  if(screen_cols==40){sprintf(s,"NL CL SL \263 MOUSE %s NET \263 %s  ",vname,country);w=strlen(s);x=screen_cols>w?screen_cols-w:0;y=1;textout(x,y,s,base,screen_cols-x);}
  p=strstr(s,"CL");if(p){sysbar_region(2,x+(int)(p-s),y,2);textout(sysbar_x[2],y,"CL",locks&0x40?active:dim,2);}
  p=strstr(s,"NL");if(p){sysbar_region(3,x+(int)(p-s),y,2);textout(sysbar_x[3],y,"NL",locks&0x20?active:dim,2);}
  p=strstr(s,"SL");if(p){sysbar_region(4,x+(int)(p-s),y,2);textout(sysbar_x[4],y,"SL",locks&0x10?active:dim,2);}
  p=strstr(s,country);if(p)sysbar_region(5,x+(int)(p-s),y,strlen(country));
  p=strstr(s,"MOUSE");if(p)textout(x+(int)(p-s),y,"MOUSE",mouse_present?active:dim,5);
  p=strstr(s,vname);if(p)textout(x+(int)(p-s),y,vname,active,3);
  p=strstr(s,"NET");if(p)textout(x+(int)(p-s),y,"NET",net?active:dim,3);
}
static int sysbar_country_dialog(void)
{
  char code[6];int x=(screen_cols-38)/2,y=(screen_rows-9)/2;
  int n,focus=0,k=0,mx=0,my=0,hit;unsigned mb=0,value;
  sprintf(code,"%u",sysbar_country);n=strlen(code);
  for(;;){
    subdialog_box(x,y,38,9,"Country");
    textout(x+3,y+2,"Enter country code:",C_INPUT_LABEL,24);
    textout(x+3,y+3,code,C_INPUT_FIELD,5);
    if(focus==0)cell(x+3+n,y+3,'_',C_INPUT_FIELD);
    value=(unsigned)atoi(code);
    textout(x+3,y+4,*code?sysbar_country_name(value):"",C_TITLE,32);
    toolbar_divider(x,y+5,38);
    draw_button(x+3,y+6,"  OK  ",6,focus==1);
    draw_button(x+25,y+6,"  Cancel  ",10,focus==2);
    wait_input(&k,&mx,&my,&mb);hit=-1;
    if(my==y+3&&mx>=x+3&&mx<x+9)hit=0;
    if(my==y+6&&mx>=x+3&&mx<x+9)hit=1;
    if(my==y+6&&mx>=x+25&&mx<x+35)hit=2;
    if(mb&MOUSE_MOVED)continue;
    if(mb&1){if(hit<0)continue;focus=hit;k=hit?13:0;}
    if(k==27||(k==13&&focus==2))return 0;
    if(k==9){focus=(focus+1)%3;continue;}
    if(k==0x0F00){focus=(focus+2)%3;continue;}
    if(k==13&&*code&&value>0&&value<=999){sprintf(sysbar_command,"COUNTRY %u\r",value);return 1;}
    if(focus==0){if(k==8&&n)code[--n]=0;else if(k>='0'&&k<='9'&&n<3){code[n++]=(char)k;code[n]=0;}}
  }
}
/* Returns 2 for a queued command, 1 for a consumed click, 0 otherwise. */
static int sysbar_click(int mx,int my)
{
  int i;union REGS r;static const unsigned char mask[3]={0x40,0x20,0x10};
  if(!appearance.show_sysbar)return 0;
  for(i=0;i<6;i++)if(my==sysbar_y[i]&&mx>=sysbar_x[i]&&mx<sysbar_x[i]+sysbar_w[i])break;
  if(i==6)return my<sysbar_rows()?1:0;
  if(i==0){strcpy(sysbar_command,"MEM /C\r");return 2;}
  if(i==1){strcpy(sysbar_command,"FREE C:\r");return 2;}
  if(i==5)return sysbar_country_dialog()?2:1;
  /* Let the BIOS reconcile its LED state on the non-consuming status call.
     Do not touch controller ports or consume a pending key. */
  _disable();*(unsigned char far *)MAKE_FP(0x40,0x17)^=mask[i-2];_enable();
  memset(&r,0,sizeof(r));r.h.ah=1;int86(0x16,&r,&r);return 1;
}
