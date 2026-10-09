/*
    __                           __    __
   / /   ____ ___  ______  _____/ /_  / /
  / /   / __ `/ / / / __ \/ ___/ __ \/ / 
 / /___/ /_/ / /_/ / / / /__/ / / /_/  
/_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
Launch! for DOS ---------------------
*/
/*
 * MAINTAINER NOTES - Launch! 3.79
 * File: UNITS.C
 * Role: !UNITS unit-conversion accessory
 * Build/ownership: Standalone accessory using the shared Launch! accessory runtime.
 * Maintainer contract: Conversions within a category are live; selecting a category on either side keeps both sides in the same dimension.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era small model. Keep tables compact and avoid large automatic buffers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ACCLIB.H"

#define UNIT_SEG_BASE 128
#define UNIT_LOWER_HALF 222
#define UNIT_KEY_LEFT 193
#define UNIT_KEY_RIGHT 139
#define UNIT_MAX_ENTRY 18

#define CAT_DISTANCE 0
#define CAT_AREA 1
#define CAT_VOLUME 2
#define CAT_MASS 3
#define CAT_TEMP 4
#define CAT_SPEED 5
#define CAT_TIME 6
#define CAT_DATA 7
#define CAT_DATARATE 8
#define CAT_FREQ 9
#define CAT_PRESSURE 10
#define CAT_ENERGY 11
#define CAT_POWER 12
#define CAT_ANGLE 13
#define CAT_FORCE 14
#define CAT_COUNT 15

typedef struct {
  char name[20];
  double scale;
  double offset;
} UNIT_DEF;

typedef struct {
  char name[16];
  int count;
} UNIT_CATEGORY;

static UNIT_DEF distance_units[]={
  {"Millimetres",0.001,0.0},{"Centimetres",0.01,0.0},{"Metres",1.0,0.0},
  {"Kilometres",1000.0,0.0},{"Inches",0.0254,0.0},{"Feet",0.3048,0.0},
  {"Yards",0.9144,0.0},{"Miles",1609.344,0.0},{"Nautical miles",1852.0,0.0}
};
static UNIT_DEF area_units[]={
  {"Millimetres2",0.000001,0.0},{"Centimetres2",0.0001,0.0},{"Metres2",1.0,0.0},
  {"Kilometres2",1000000.0,0.0},{"Hectares",10000.0,0.0},{"Acres",4046.8564224,0.0},
  {"Feet2",0.09290304,0.0},{"Inches2",0.00064516,0.0}
};
static UNIT_DEF volume_units[]={
  {"Millilitres",0.001,0.0},{"Litres",1.0,0.0},{"Metres3",1000.0,0.0},
  {"Teaspoons US",0.00492892159375,0.0},{"Tablespoons US",0.01478676478125,0.0},
  {"Fl ounces US",0.0295735295625,0.0},{"Cups US",0.2365882365,0.0},
  {"Pints US",0.473176473,0.0},{"Quarts US",0.946352946,0.0},
  {"Gallons US",3.785411784,0.0},{"Gallons Imp",4.54609,0.0}
};
static UNIT_DEF mass_units[]={
  {"Milligrams",0.001,0.0},{"Grams",1.0,0.0},{"Kilograms",1000.0,0.0},
  {"Ounces",28.349523125,0.0},{"Pounds",453.59237,0.0},{"Stone",6350.29318,0.0},
  {"Tonnes",1000000.0,0.0},{"Tons US",907184.74,0.0}
};
static UNIT_DEF temp_units[]={
  {"Celsius",1.0,0.0},{"Fahrenheit",0.555555555555556,-17.7777777777778},
  {"Kelvin",1.0,-273.15},{"Rankine",0.555555555555556,-273.15}
};
static UNIT_DEF speed_units[]={
  {"Metres/sec",1.0,0.0},{"Kilometres/hr",0.277777777777778,0.0},
  {"Miles/hr",0.44704,0.0},{"Knots",0.514444444444444,0.0},{"Feet/sec",0.3048,0.0}
};
static UNIT_DEF time_units[]={
  {"Milliseconds",0.001,0.0},{"Seconds",1.0,0.0},{"Minutes",60.0,0.0},
  {"Hours",3600.0,0.0},{"Days",86400.0,0.0},{"Weeks",604800.0,0.0}
};
static UNIT_DEF data_units[]={
  {"Bits",0.125,0.0},{"Bytes",1.0,0.0},{"KB",1024.0,0.0},{"MB",1048576.0,0.0},
  {"GB",1073741824.0,0.0},{"TB",1099511627776.0,0.0},{"Kbits",128.0,0.0},
  {"Mbits",131072.0,0.0},{"Gbits",134217728.0,0.0}
};
static UNIT_DEF datarate_units[]={
  {"Bits/sec",1.0,0.0},{"Kbits/sec",1000.0,0.0},{"Mbits/sec",1000000.0,0.0},
  {"Gbits/sec",1000000000.0,0.0},{"Bytes/sec",8.0,0.0},{"KB/sec",8000.0,0.0},
  {"MB/sec",8000000.0,0.0},{"GB/sec",8000000000.0,0.0}
};
static UNIT_DEF freq_units[]={
  {"Hertz",1.0,0.0},{"Kilohertz",1000.0,0.0},{"Megahertz",1000000.0,0.0},{"Gigahertz",1000000000.0,0.0}
};
static UNIT_DEF pressure_units[]={
  {"Pascals",1.0,0.0},{"Kilopascals",1000.0,0.0},{"Bar",100000.0,0.0},
  {"PSI",6894.757293168,0.0},{"Atmospheres",101325.0,0.0},{"mmHg",133.322387415,0.0},
  {"inHg",3386.389,0.0}
};
static UNIT_DEF energy_units[]={
  {"Joules",1.0,0.0},{"Kilojoules",1000.0,0.0},{"Calories",4.184,0.0},
  {"Kilocalories",4184.0,0.0},{"Watt hours",3600.0,0.0},{"Kilowatt hours",3600000.0,0.0},
  {"BTU",1055.05585262,0.0}
};
static UNIT_DEF power_units[]={
  {"Milliwatts",0.001,0.0},{"Watts",1.0,0.0},{"Kilowatts",1000.0,0.0},
  {"Megawatts",1000000.0,0.0},{"Horsepower",745.699871582,0.0}
};
static UNIT_DEF angle_units[]={
  {"Degrees",0.0174532925199433,0.0},{"Radians",1.0,0.0},
  {"Gradians",0.0157079632679490,0.0},{"Revolutions",6.283185307179586,0.0}
};
static UNIT_DEF force_units[]={
  {"Newtons",1.0,0.0},{"Kilonewtons",1000.0,0.0},{"Pound-force",4.4482216152605,0.0},
  {"Kilogram-force",9.80665,0.0}
};

static UNIT_CATEGORY categories[CAT_COUNT]={
  {"Distance",sizeof(distance_units)/sizeof(distance_units[0])},
  {"Area",sizeof(area_units)/sizeof(area_units[0])},
  {"Volume",sizeof(volume_units)/sizeof(volume_units[0])},
  {"Mass",sizeof(mass_units)/sizeof(mass_units[0])},
  {"Temperature",sizeof(temp_units)/sizeof(temp_units[0])},
  {"Speed",sizeof(speed_units)/sizeof(speed_units[0])},
  {"Time",sizeof(time_units)/sizeof(time_units[0])},
  {"Data",sizeof(data_units)/sizeof(data_units[0])},
  {"Data Rate",sizeof(datarate_units)/sizeof(datarate_units[0])},
  {"Frequency",sizeof(freq_units)/sizeof(freq_units[0])},
  {"Pressure",sizeof(pressure_units)/sizeof(pressure_units[0])},
  {"Energy",sizeof(energy_units)/sizeof(energy_units[0])},
  {"Power",sizeof(power_units)/sizeof(power_units[0])},
  {"Angle",sizeof(angle_units)/sizeof(angle_units[0])},
  {"Force",sizeof(force_units)/sizeof(force_units[0])}
};
/* Build popup pointer vectors at run time.  Keeping the actual names inline in
   DGROUP avoids fragile nested const-pointer initializers on 16-bit MSC. */
static const char *category_popup_items[CAT_COUNT];
static const char *unit_popup_items[16];

static UNIT_DEF *unit_table(int cat)
{
  switch(cat){
    case CAT_DISTANCE:return distance_units; case CAT_AREA:return area_units;
    case CAT_VOLUME:return volume_units; case CAT_MASS:return mass_units;
    case CAT_TEMP:return temp_units; case CAT_SPEED:return speed_units;
    case CAT_TIME:return time_units; case CAT_DATA:return data_units;
    case CAT_DATARATE:return datarate_units; case CAT_FREQ:return freq_units;
    case CAT_PRESSURE:return pressure_units; case CAT_ENERGY:return energy_units;
    case CAT_POWER:return power_units; case CAT_ANGLE:return angle_units;
    case CAT_FORCE:return force_units;
  }
  return distance_units;
}
static int unit_count(int cat)
{
  if(cat<0||cat>=CAT_COUNT)return 0;
  return categories[cat].count;
}
static UNIT_DEF *unit_at(int cat,int unit)
{
  UNIT_DEF *table;int count=unit_count(cat);
  if(count<1)return distance_units;
  if(unit<0)unit=0;if(unit>=count)unit=count-1;
  table=unit_table(cat);return &table[unit];
}


static unsigned char old_digits[11][32];
static unsigned char old_lower[32];
static unsigned char old_key[2][32];
static const unsigned char keycap16[2][16]={
 {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F},
 {0xF0,0xFC,0xFC,0xFE,0xFE,0xFE,0xFE,0xFA,0xFE,0xFA,0xFA,0xFA,0xF2,0xE6,0x1C,0xF0}
};
static const unsigned char keycap14[2][14]={
 {0x0F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x0F},
 {0xF0,0xFC,0xFE,0xFE,0xFE,0xFA,0xFE,0xFA,0xFA,0xFA,0xF2,0xE6,0x1C,0xF0}
};
static const char unit_keys[11]={'7','8','9','4','5','6','1','2','3','0','.'};
static const int key_x[11]={0,4,8,0,4,8,0,4,8,0,8};
static const int key_y[11]={0,0,0,2,2,2,4,4,4,6,6};
static const int key_w[11]={3,3,3,3,3,3,3,3,3,7,3};

static void unit_glyphs_install(void)
{
  int i,h=acc_font_height();unsigned char g[32];
  for(i=0;i<11;i++){acc_glyph_read(UNIT_SEG_BASE+i,old_digits[i]);acc_glyph_library(64+i,UNIT_SEG_BASE+i);}
  acc_glyph_read(UNIT_LOWER_HALF,old_lower);memset(g,0,sizeof(g));for(i=h/2;i<h;i++)g[i]=0xFF;acc_glyph_write(UNIT_LOWER_HALF,g);
  acc_glyph_read(UNIT_KEY_LEFT,old_key[0]);acc_glyph_read(UNIT_KEY_RIGHT,old_key[1]);
  memset(g,0,sizeof(g));if(h==14)memcpy(g,keycap14[0],14);else memcpy(g,keycap16[0],16);acc_glyph_write(UNIT_KEY_LEFT,g);
  memset(g,0,sizeof(g));if(h==14)memcpy(g,keycap14[1],14);else memcpy(g,keycap16[1],16);acc_glyph_write(UNIT_KEY_RIGHT,g);
}
static void unit_glyphs_restore(void)
{
  int i;for(i=0;i<11;i++)acc_glyph_write(UNIT_SEG_BASE+i,old_digits[i]);
  acc_glyph_write(UNIT_LOWER_HALF,old_lower);acc_glyph_write(UNIT_KEY_LEFT,old_key[0]);acc_glyph_write(UNIT_KEY_RIGHT,old_key[1]);
}
static unsigned char unit_digit(unsigned char c)
{
  if(c>='0'&&c<='9')return (unsigned char)(UNIT_SEG_BASE+(c-'0'));
  if(c=='.')return (unsigned char)(UNIT_SEG_BASE+10);
  return c;
}
static int numeric_digits(const char *s){int n=0;for(;*s;s++)if(*s>='0'&&*s<='9')n++;return n;}
static void format_number(char *out,int outsz,double value)
{
  int p;char temp[48];
  if(outsz<2)return;
  if(value>1.0e99||value<-1.0e99){strncpy(out,"OVERFLOW",outsz-1);out[outsz-1]=0;return;}
  for(p=12;p>=1;p--){
    sprintf(temp,"%.*g",p,value);
    if(numeric_digits(temp)<=12){strncpy(out,temp,outsz-1);out[outsz-1]=0;return;}
  }
  strncpy(out,"OVERFLOW",outsz-1);out[outsz-1]=0;
}
static double entry_value(const char *entry){if(!entry[0]||!strcmp(entry,"-")||!strcmp(entry,"."))return 0.0;return atof(entry);}
static double convert_value(double value,int in_cat,int in_unit,int out_cat,int out_unit)
{
  double base;UNIT_DEF *a,*b;
  if(in_cat<0||in_cat>=CAT_COUNT||out_cat<0||out_cat>=CAT_COUNT||in_cat!=out_cat)return 0.0;
  if(in_unit<0||in_unit>=unit_count(in_cat)||out_unit<0||out_unit>=unit_count(out_cat))return 0.0;
  a=unit_at(in_cat,in_unit);b=unit_at(out_cat,out_unit);
  if(b->scale==0.0)return 0.0;
  base=value*a->scale+a->offset;
  return (base-b->offset)/b->scale;
}
static void display_box(int x,int y,int w,const char *text,int output)
{
  int i,start,inside_bg=output?acc_appearance.controls_bg:0;
  int digit_fg=output?acc_appearance.controls_fg:10;
  int edge=ACC_ATTR(acc_appearance.background,inside_bg),inside=ACC_ATTR(inside_bg,digit_fg);
  for(i=0;i<w;i++){acc_put(x+i,y,UNIT_LOWER_HALF,edge);acc_put(x+i,y+1,' ',inside);acc_put(x+i,y+2,223,edge);}
  start=x+w-2-(int)strlen(text);if(start<x+1)start=x+1;
  for(i=0;text[i]&&start+i<x+w-1;i++)acc_put(start+i,y+1,unit_digit((unsigned char)text[i]),inside);
}
static void cycle_control(int x,int y,int w,const char *value,int selected)
{
  int attr=selected?ACC_SELECT:ACC_CONTROL,n=(int)strlen(value);
  if(n>w-3)n=w-3;
  acc_fill(x,y,w,1,' ',attr);acc_text(x+1,y,value,attr,n);acc_put(x+w-2,y,31,attr);
}
static void keycap(int x,int y,int width,char label,int selected)
{
  int a=selected?ACC_SELECT:ACC_CONTROL,capfg=selected?acc_appearance.selected_bg:acc_appearance.controls_bg;
  int ca=ACC_ATTR(acc_appearance.background,capfg),start;
  acc_put(x,y,UNIT_KEY_LEFT,ca);if(width>2)acc_fill(x+1,y,width-2,1,' ',a);acc_put(x+width-1,y,UNIT_KEY_RIGHT,ca);
  start=x+width/2;acc_put(start,y,unit_digit((unsigned char)label),a);
}
static void draw_keypad(int x,int y,int focus)
{
  int i;for(i=0;i<11;i++)keycap(x+key_x[i],y+key_y[i],key_w[i],unit_keys[i],focus==4+i);
}
static int keypad_hit(int x,int y,int mx,int my)
{
  int i;for(i=0;i<11;i++)if(my==y+key_y[i]&&mx>=x+key_x[i]&&mx<x+key_x[i]+key_w[i])return i;return -1;
}
static int keypad_index(int ch){int i;for(i=0;i<11;i++)if(unit_keys[i]==ch)return i;return -1;}
static void flash_key(int x,int y,int index,int restore)
{
  unsigned long t;if(index<0)return;draw_keypad(x,y,4+index);t=acc_ticks();while(acc_ticks()==t);draw_keypad(x,y,restore);
}
static void append_digit(char *entry,int ch)
{
  int n=(int)strlen(entry);
  if(ch=='.'&&strchr(entry,'.'))return;
  if(n>=UNIT_MAX_ENTRY-1)return;
  if(!strcmp(entry,"0")&&ch!='.'){entry[0]=(char)ch;entry[1]=0;return;}
  entry[n]=(char)ch;entry[n+1]=0;
}
static void backspace_entry(char *entry)
{
  int n=(int)strlen(entry);if(n>0)entry[n-1]=0;if(!entry[0]||!strcmp(entry,"-"))strcpy(entry,"0");
}
static void toggle_sign(char *entry)
{
  int n=(int)strlen(entry);if(entry[0]=='-')memmove(entry,entry+1,n);else if(n<UNIT_MAX_ENTRY-1){memmove(entry+1,entry,n+1);entry[0]='-';}
}
static int choose_category(int x,int y,int current)
{
  int i;for(i=0;i<CAT_COUNT;i++)category_popup_items[i]=categories[i].name;
  if(current<0||current>=CAT_COUNT)current=0;
  return acc_select_popup(x,y,category_popup_items,CAT_COUNT,current,18);
}
static int choose_unit(int x,int y,int cat,int current)
{
  int i,count=unit_count(cat);UNIT_DEF *table=unit_table(cat);
  if(count>16)count=16;if(count<1)return -1;
  if(current<0||current>=count)current=0;
  for(i=0;i<count;i++)unit_popup_items[i]=table[i].name;
  return acc_select_popup(x,y,unit_popup_items,count,current,18);
}
static void sync_category(int cat,int *in_cat,int *in_unit,int *out_cat,int *out_unit)
{
  *in_cat=*out_cat=cat;*in_unit=*out_unit=0;
}
static void swap_sides(char *entry,int *in_cat,int *in_unit,int *out_cat,int *out_unit)
{
  int t;double value=entry_value(entry),converted=convert_value(value,*in_cat,*in_unit,*out_cat,*out_unit);char b[40];
  t=*in_cat;*in_cat=*out_cat;*out_cat=t;t=*in_unit;*in_unit=*out_unit;*out_unit=t;
  format_number(b,sizeof(b),converted);strncpy(entry,b,UNIT_MAX_ENTRY-1);entry[UNIT_MAX_ENTRY-1]=0;
}
static void draw_units(int bx,int by,int focus,const char *entry,int in_cat,int in_unit,int out_cat,int out_unit)
{
  char output[32];double out=convert_value(entry_value(entry),in_cat,in_unit,out_cat,out_unit);int i;
  acc_box(bx,by,59,18,"Units");
  format_number(output,sizeof(output),out);
  display_box(bx+3,by+2,24,entry,0);display_box(bx+32,by+2,24,output,1);
  for(i=2;i<=5;i++)acc_put(bx+29,by+i,179,ACC_BORDER);
  acc_put(bx+29,by+13,179,ACC_BORDER);
  acc_text(bx+3,by+6,"From",ACC_HEADING,4);acc_text(bx+40,by+6,"To",ACC_HEADING,2);
  cycle_control(bx+3,by+8,16,categories[in_cat].name,focus==0);
  cycle_control(bx+40,by+8,16,categories[out_cat].name,focus==2);
  cycle_control(bx+3,by+10,16,unit_at(in_cat,in_unit)->name,focus==1);
  cycle_control(bx+40,by+10,16,unit_at(out_cat,out_unit)->name,focus==3);
  draw_keypad(bx+24,by+6,focus);
  acc_button(bx+3,by+15," Clear ",focus==15);acc_button(bx+12,by+15," Swap ",focus==16);acc_button(bx+50,by+15," Exit ",focus==17);
}

int main(int argc,char **argv)
{
  int bx,by,focus=0,key=0,mx=0,my=0,hit,choice,ki;
  int in_cat=CAT_DISTANCE,out_cat=CAT_DISTANCE,in_unit=1,out_unit=1;
  char entry[UNIT_MAX_ENTRY]="0";unsigned mb=0;
  if(acc_help(argc,argv,"!UNITS","Convert measurements, technical units and data sizes."))return 0;
  if(!acc_begin(argv[0],"Units",0))return 1;
  unit_glyphs_install();bx=(acc_cols-59)/2;by=(acc_rows-18)/2;
  while(key!=27){
    if(in_cat<0||in_cat>=CAT_COUNT)in_cat=CAT_DISTANCE;if(out_cat!=in_cat)out_cat=in_cat;
    if(in_unit<0||in_unit>=unit_count(in_cat))in_unit=0;if(out_unit<0||out_unit>=unit_count(out_cat))out_unit=0;
    draw_units(bx,by,focus,entry,in_cat,in_unit,out_cat,out_unit);
    acc_wait(&key,&mx,&my,&mb);
    if((mb&1)&&my==by+8&&mx>=bx+3&&mx<bx+19){focus=0;key=13;}
    else if((mb&1)&&my==by+10&&mx>=bx+3&&mx<bx+19){focus=1;key=13;}
    else if((mb&1)&&my==by+8&&mx>=bx+40&&mx<bx+56){focus=2;key=13;}
    else if((mb&1)&&my==by+10&&mx>=bx+40&&mx<bx+56){focus=3;key=13;}
    hit=(mb&1)?keypad_hit(bx+24,by+6,mx,my):-1;
    if(hit>=0){focus=4+hit;flash_key(bx+24,by+6,hit,focus);append_digit(entry,unit_keys[hit]);key=0;continue;}
    if((mb&1)&&my==by+15){
      if(mx>=bx+3&&mx<bx+10){focus=15;strcpy(entry,"0");key=0;continue;}
      if(mx>=bx+12&&mx<bx+18){focus=16;swap_sides(entry,&in_cat,&in_unit,&out_cat,&out_unit);key=0;continue;}
      if(mx>=bx+50&&mx<bx+56){focus=17;key=27;continue;}
    }
    if(key==9||key==271){focus=(key==271)?(focus+17)%18:(focus+1)%18;key=0;continue;}
    if((key==13||key==' ')&&focus==0){choice=choose_category(bx+3,by+9,in_cat);if(choice>=0)sync_category(choice,&in_cat,&in_unit,&out_cat,&out_unit);key=0;continue;}
    if((key==13||key==' ')&&focus==1){choice=choose_unit(bx+3,by+11,in_cat,in_unit);if(choice>=0){in_unit=choice;out_cat=in_cat;out_unit=in_unit;}key=0;continue;}
    if((key==13||key==' ')&&focus==2){choice=choose_category(bx+40,by+9,out_cat);if(choice>=0)sync_category(choice,&in_cat,&in_unit,&out_cat,&out_unit);key=0;continue;}
    if((key==13||key==' ')&&focus==3){choice=choose_unit(bx+40,by+11,out_cat,out_unit);if(choice>=0)out_unit=choice;key=0;continue;}
    if((key==13||key==' ')&&focus>=4&&focus<=14){append_digit(entry,unit_keys[focus-4]);key=0;continue;}
    if((key==13||key==' ')&&focus==15){strcpy(entry,"0");key=0;continue;}
    if((key==13||key==' ')&&focus==16){swap_sides(entry,&in_cat,&in_unit,&out_cat,&out_unit);key=0;continue;}
    if((key==13||key==' ')&&focus==17){key=27;continue;}
    if(key==8){backspace_entry(entry);key=0;continue;}
    if(key=='-'){toggle_sign(entry);key=0;continue;}
    if(key=='c'||key=='C'){strcpy(entry,"0");key=0;continue;}
    if((key>='0'&&key<='9')||key=='.'){
      ki=keypad_index(key);flash_key(bx+24,by+6,ki,focus);append_digit(entry,key);key=0;continue;
    }
    if(key!=27)key=0;
  }
  acc_caret_hide();unit_glyphs_restore();acc_end_screen();acc_end();return 0;
}
