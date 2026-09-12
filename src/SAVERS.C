/* Integer-only EGA adaptations of five classic XScreenSaver effects, plus
   the original Paintball simulation.
   They deliberately share Launch!'s mode 10h renderer, timer, PRNG and
   input handling so no external saver data file is required. */

#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "SAVERS.H"

extern void ega_span(int y,int left,int right,unsigned char colour);
extern void ega_rectangle(int x,int y,int width,int height,unsigned char colour);
extern void ega_line(int x0,int y0,int x1,int y1,unsigned char colour);
extern int boing_isqrt(int value);
extern unsigned saver_random(unsigned limit);
extern unsigned long bios_ticks(void);
extern int saver_input(unsigned start_x,unsigned start_y);
extern void wait_vertical_retrace(void);

static unsigned long saver_distance2(int x1,int y1,int x2,int y2)
{
  long dx=(long)x1-x2,dy=(long)y1-y2;
  return (unsigned long)(dx*dx+dy*dy);
}

static void saver_disc(int cx,int cy,int radius,unsigned char colour)
{
  int dy,half;
  if(radius<1){ega_span(cy,cx,cx,colour);return;}
  for(dy=-radius;dy<=radius;dy++){
    half=radius*boing_isqrt(radius*radius-dy*dy)/radius;
    ega_span(cy+dy,cx-half,cx+half,colour);
  }
}

/* Erase only the portion of an old disc which is not covered by its new
   position.  Redrawing after this operation avoids the full-frame black
   flashes caused by erasing and repainting the entire object. */
static void saver_restore_circle_exposed(int old_x,int old_y,int old_radius,
                                         int new_x,int new_y,int new_radius)
{
  int y,dy,old_half,new_dy,new_half,old_left,old_right,new_left,new_right;
  for(y=old_y-old_radius;y<=old_y+old_radius;y++){
    dy=y-old_y;
    old_half=old_radius*boing_isqrt(old_radius*old_radius-dy*dy)/old_radius;
    old_left=old_x-old_half;old_right=old_x+old_half;
    new_dy=y-new_y;
    if(new_dy < -new_radius || new_dy > new_radius)
      ega_span(y,old_left,old_right,0);
    else {
      new_half=new_radius*boing_isqrt(new_radius*new_radius-new_dy*new_dy)/new_radius;
      new_left=new_x-new_half;new_right=new_x+new_half;
      if(old_left<new_left)ega_span(y,old_left,new_left-1,0);
      if(old_right>new_right)ega_span(y,new_right+1,old_right,0);
    }
  }
}

static void saver_box_outline(int cx,int cy,int half_width,int half_height,
                              unsigned char colour)
{
  ega_line(cx-half_width,cy-half_height,cx+half_width,cy-half_height,colour);
  ega_line(cx+half_width,cy-half_height,cx+half_width,cy+half_height,colour);
  ega_line(cx+half_width,cy+half_height,cx-half_width,cy+half_height,colour);
  ega_line(cx-half_width,cy+half_height,cx-half_width,cy-half_height,colour);
}

/* --------------------------- Mosaic (Abstractile) ------------------------ */

#define ABSTRACTILE_COLS 40
#define ABSTRACTILE_ROWS 25
#define ABSTRACTILE_CELLS (ABSTRACTILE_COLS*ABSTRACTILE_ROWS)
static unsigned char far abstractile_grid[ABSTRACTILE_CELLS];

static void abstractile_draw_tile(int column,int row,unsigned char tile)
{
  int x=column*16,y=row*14,line,edge;
  unsigned char first=(unsigned char)(9+(tile&3));
  unsigned char second=(unsigned char)(12+((tile>>2)&3));
  if(first==second)second=(unsigned char)(9+((second+1)&3));
  ega_rectangle(x,y,16,14,first);
  for(line=0;line<14;line++){
    edge=(line*15)/13;
    if(tile&1)edge=15-edge;
    if(tile&2)ega_span(y+line,x+edge,x+15,second);
    else ega_span(y+line,x,x+edge,second);
  }
  if(tile&1)ega_line(x,y,x+15,y+13,0);
  else ega_line(x+15,y,x,y+13,0);
}

void abstractile_saver_loop(unsigned start_x,unsigned start_y)
{
  int column,row,index,placed=0,tries,step;
  unsigned char tile;
  unsigned long last_tick=bios_ticks(),tick;
  _fmemset(abstractile_grid,0xFF,sizeof(abstractile_grid));
  ega_rectangle(0,0,640,350,0);
  column=(int)saver_random(ABSTRACTILE_COLS);
  row=(int)saver_random(ABSTRACTILE_ROWS);
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    wait_vertical_retrace();
    for(step=0;step<5;step++){
      for(tries=0;tries<80;tries++){
        index=row*ABSTRACTILE_COLS+column;
        if(abstractile_grid[index]==0xFF)break;
        switch(saver_random(4)){
          case 0:column=(column+1)%ABSTRACTILE_COLS;break;
          case 1:column=(column+ABSTRACTILE_COLS-1)%ABSTRACTILE_COLS;break;
          case 2:row=(row+1)%ABSTRACTILE_ROWS;break;
          default:row=(row+ABSTRACTILE_ROWS-1)%ABSTRACTILE_ROWS;break;
        }
      }
      if(tries==80){
        do{index=(int)saver_random(ABSTRACTILE_CELLS);}
        while(abstractile_grid[index]!=0xFF);
        row=index/ABSTRACTILE_COLS;column=index%ABSTRACTILE_COLS;
      }
      tile=(unsigned char)saver_random(16);abstractile_grid[index]=tile;
      abstractile_draw_tile(column,row,tile);placed++;
      if(placed>=ABSTRACTILE_CELLS){
        _fmemset(abstractile_grid,0xFF,sizeof(abstractile_grid));
        placed=0;ega_rectangle(0,0,640,350,0);
      }
      switch(saver_random(4)){
        case 0:column=(column+1)%ABSTRACTILE_COLS;break;
        case 1:column=(column+ABSTRACTILE_COLS-1)%ABSTRACTILE_COLS;break;
        case 2:row=(row+1)%ABSTRACTILE_ROWS;break;
        default:row=(row+ABSTRACTILE_ROWS-1)%ABSTRACTILE_ROWS;break;
      }
    }
  }
}

/* -------------------------------- Blaster -------------------------------- */

#define BLASTER_SHIPS 6
#define BLASTER_SHOTS 36
typedef struct {
  int x,y,dx,dy,cooldown,explosion;
  unsigned char colour,alive;
} BLASTER_SHIP;
typedef struct {
  int x,y,dx,dy,owner;
  unsigned char colour,active;
} BLASTER_SHOT;
static BLASTER_SHIP far blaster_ships[BLASTER_SHIPS];
static BLASTER_SHOT far blaster_shots[BLASTER_SHOTS];

static void blaster_draw_ship(const BLASTER_SHIP far *ship,unsigned char colour)
{
  int radius;
  if(ship->alive){
    ega_line(ship->x,ship->y-6,ship->x-6,ship->y+5,colour);
    ega_line(ship->x-6,ship->y+5,ship->x+6,ship->y+5,colour);
    ega_line(ship->x+6,ship->y+5,ship->x,ship->y-6,colour);
    ega_span(ship->y+1,ship->x-2,ship->x+2,colour);
  } else if(ship->explosion){
    radius=22-ship->explosion;
    ega_line(ship->x-radius,ship->y,ship->x+radius,ship->y,colour);
    ega_line(ship->x,ship->y-radius,ship->x,ship->y+radius,colour);
    ega_line(ship->x-radius,ship->y-radius,ship->x+radius,ship->y+radius,colour);
    ega_line(ship->x+radius,ship->y-radius,ship->x-radius,ship->y+radius,colour);
  }
}

static void blaster_respawn(int i)
{
  BLASTER_SHIP far *ship=&blaster_ships[i];
  ship->x=20+(int)saver_random(600);ship->y=20+(int)saver_random(310);
  ship->dx=(int)saver_random(5)-2;ship->dy=(int)saver_random(5)-2;
  if(!ship->dx && !ship->dy)ship->dx=1;
  ship->cooldown=8+(int)saver_random(30);ship->explosion=0;
  ship->colour=(unsigned char)(9+saver_random(7));ship->alive=1;
}

static void blaster_fire(int owner)
{
  int i,target,dx,dy,scale;BLASTER_SHOT far *shot;
  BLASTER_SHIP far *ship=&blaster_ships[owner];
  for(i=0;i<BLASTER_SHOTS;i++)if(!blaster_shots[i].active)break;
  if(i==BLASTER_SHOTS)return;
  do{target=(int)saver_random(BLASTER_SHIPS);}while(target==owner);
  dx=blaster_ships[target].x-ship->x;dy=blaster_ships[target].y-ship->y;
  scale=abs(dx)>abs(dy)?abs(dx):abs(dy);if(!scale)scale=1;
  shot=&blaster_shots[i];shot->x=ship->x;shot->y=ship->y;
  shot->dx=dx*6/scale;shot->dy=dy*6/scale;
  if(!shot->dx&&!shot->dy)shot->dx=1;
  shot->owner=owner;shot->colour=ship->colour;shot->active=1;
}

void blaster_saver_loop(unsigned start_x,unsigned start_y)
{
  int i,j;BLASTER_SHIP far *ship;BLASTER_SHOT far *shot;
  unsigned long last_tick=bios_ticks(),tick;
  ega_rectangle(0,0,640,350,0);_fmemset(blaster_shots,0,sizeof(blaster_shots));
  for(i=0;i<BLASTER_SHIPS;i++)blaster_respawn(i);
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    wait_vertical_retrace();
    for(i=0;i<BLASTER_SHIPS;i++)blaster_draw_ship(&blaster_ships[i],0);
    for(i=0;i<BLASTER_SHOTS;i++)if(blaster_shots[i].active){
      shot=&blaster_shots[i];ega_line(shot->x,shot->y,shot->x-shot->dx,shot->y-shot->dy,0);
    }
    for(i=0;i<BLASTER_SHIPS;i++){
      ship=&blaster_ships[i];
      if(!ship->alive){if(ship->explosion)ship->explosion--;else blaster_respawn(i);continue;}
      if(saver_random(24)==0){ship->dx=(int)saver_random(5)-2;ship->dy=(int)saver_random(5)-2;}
      ship->x+=ship->dx;ship->y+=ship->dy;
      if(ship->x<10){ship->x=10;ship->dx=abs(ship->dx);}
      else if(ship->x>629){ship->x=629;ship->dx=-abs(ship->dx);}
      if(ship->y<10){ship->y=10;ship->dy=abs(ship->dy);}
      else if(ship->y>339){ship->y=339;ship->dy=-abs(ship->dy);}
      if(ship->cooldown)ship->cooldown--;else {blaster_fire(i);ship->cooldown=12+(int)saver_random(35);}
    }
    for(i=0;i<BLASTER_SHOTS;i++)if(blaster_shots[i].active){
      shot=&blaster_shots[i];shot->x+=shot->dx;shot->y+=shot->dy;
      if(shot->x<0||shot->x>639||shot->y<0||shot->y>349){shot->active=0;continue;}
      for(j=0;j<BLASTER_SHIPS;j++)if(j!=shot->owner&&blaster_ships[j].alive&&
          saver_distance2(shot->x,shot->y,blaster_ships[j].x,blaster_ships[j].y)<64UL){
        blaster_ships[j].alive=0;blaster_ships[j].explosion=20;shot->active=0;break;
      }
    }
    for(i=0;i<BLASTER_SHIPS;i++)blaster_draw_ship(&blaster_ships[i],blaster_ships[i].colour);
    for(i=0;i<BLASTER_SHOTS;i++)if(blaster_shots[i].active){
      shot=&blaster_shots[i];ega_line(shot->x,shot->y,shot->x-shot->dx,shot->y-shot->dy,shot->colour);
    }
  }
}

/* ----------------------- Paintball (Splotchy) ---------------------------- */

#define SPLOTCH_DROPS 20
#define SPLOTCH_LOBES 14
#define SPLOTCH_DRIPS 5
typedef struct { int x,y,radius; } SPLOTCH_DROP;
typedef struct { int x,y,length,target,width,speed; } SPLOTCH_DRIP;
static SPLOTCH_DROP far splotch_drops[SPLOTCH_DROPS];
static SPLOTCH_DROP far splotch_lobes[SPLOTCH_LOBES];
static SPLOTCH_DRIP far splotch_drips[SPLOTCH_DRIPS];
static const signed char splotch_dx[16]={8,7,6,3,0,-3,-6,-7,-8,-7,-6,-3,0,3,6,7};
static const signed char splotch_dy[16]={0,3,6,7,8,7,6,3,0,-3,-6,-7,-8,-7,-6,-3};

static void splotchy_make(int *cx,int *cy,int *radius,unsigned char *colour)
{
  int i,direction,distance;SPLOTCH_DROP far *drop;SPLOTCH_DRIP far *drip;
  *cx=75+(int)saver_random(490);*cy=55+(int)saver_random(150);
  *radius=20+(int)saver_random(17);*colour=(unsigned char)(9+saver_random(7));
  for(i=0;i<SPLOTCH_LOBES;i++){
    direction=(int)saver_random(16);
    distance=*radius/3+(int)saver_random(*radius*2/3+1);
    drop=&splotch_lobes[i];drop->x=*cx+splotch_dx[direction]*distance/8;
    drop->y=*cy+splotch_dy[direction]*distance/8;
    drop->radius=*radius/4+(int)saver_random(*radius/3+1);
  }
  for(i=0;i<SPLOTCH_DROPS;i++){
    direction=(int)saver_random(16);distance=*radius/2+(int)saver_random(*radius+28);
    drop=&splotch_drops[i];drop->x=*cx+splotch_dx[direction]*distance/8;
    drop->y=*cy+splotch_dy[direction]*distance/8;
    drop->radius=1+(int)saver_random(5);
  }
  for(i=0;i<SPLOTCH_DRIPS;i++){
    drip=&splotch_drips[i];drip->x=*cx-*radius/2+(int)saver_random(*radius);
    drip->y=*cy+*radius-(int)saver_random(7);drip->length=0;
    drip->target=28+(int)saver_random(155);drip->width=2+(int)saver_random(4);
    drip->speed=1+(int)saver_random(3);
  }
}

static void splotchy_draw_splat(int cx,int cy,int radius,unsigned char colour)
{
  int i;SPLOTCH_DROP far *drop;
  saver_disc(cx,cy,radius/2,colour);
  for(i=0;i<SPLOTCH_LOBES;i++){
    drop=&splotch_lobes[i];saver_disc(drop->x,drop->y,drop->radius,colour);
  }
  for(i=0;i<SPLOTCH_DROPS;i++){
    drop=&splotch_drops[i];saver_disc(drop->x,drop->y,drop->radius,colour);
  }
}

void splotchy_saver_loop(unsigned start_x,unsigned start_y)
{
  int i,cx,cy,radius,frame=0,amount;unsigned char colour;
  SPLOTCH_DRIP far *drip;unsigned long last_tick=bios_ticks(),tick;
  ega_rectangle(0,0,640,350,0);splotchy_make(&cx,&cy,&radius,&colour);
  splotchy_draw_splat(cx,cy,radius,colour);
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;frame++;
    if(frame>=22&&frame<150){
      wait_vertical_retrace();
      for(i=0;i<SPLOTCH_DRIPS;i++){
        drip=&splotch_drips[i];
        if(drip->length<drip->target){
          amount=drip->speed;if(drip->length+amount>drip->target)amount=drip->target-drip->length;
          ega_rectangle(drip->x,drip->y+drip->length,drip->width,amount,colour);
          drip->length+=amount;
          saver_disc(drip->x+drip->width/2,drip->y+drip->length,drip->width,colour);
        }
      }
    }
    if(frame>=190){
      splotchy_make(&cx,&cy,&radius,&colour);
      splotchy_draw_splat(cx,cy,radius,colour);frame=0;
    }
  }
}

/* ---------------------------- Space Junk (Rocks) ------------------------- */

#define ROCK_COUNT 28
typedef struct { int x,y,z,shape;unsigned char colour; } ROCK;
static ROCK far rocks[ROCK_COUNT];
static int far rock_old_x[ROCK_COUNT];
static int far rock_old_y[ROCK_COUNT];
static int far rock_old_radius[ROCK_COUNT];
static int far rock_old_shape[ROCK_COUNT];
static int far rock_new_x[ROCK_COUNT];
static int far rock_new_y[ROCK_COUNT];
static int far rock_new_radius[ROCK_COUNT];

static void rock_reset(ROCK far *rock,int deep)
{
  rock->x=(int)saver_random(601)-300;rock->y=(int)saver_random(401)-200;
  rock->z=deep?80+(int)saver_random(80):16+(int)saver_random(140);
  rock->shape=(int)saver_random(4);rock->colour=(unsigned char)(8+saver_random(8));
}

static int rock_x(const ROCK far *rock)
{
  return 320+(int)((long)rock->x*80/rock->z);
}

static int rock_y(const ROCK far *rock)
{
  return 175+(int)((long)rock->y*80/rock->z);
}

static int rock_radius(const ROCK far *rock)
{
  int radius=600/rock->z;
  if(radius<3)radius=3;
  if(radius>48)radius=48;
  return radius;
}

static void rock_outline(int cx,int cy,int radius,int shape,
                         unsigned char colour)
{
  ega_line(cx-radius,cy,cx-radius/2,cy-radius,colour);
  ega_line(cx-radius/2,cy-radius,cx+radius/2,cy-radius+shape,colour);
  ega_line(cx+radius/2,cy-radius+shape,cx+radius,cy,colour);
  ega_line(cx+radius,cy,cx+radius/3,cy+radius,colour);
  ega_line(cx+radius/3,cy+radius,cx-radius/2,cy+radius-shape,colour);
  ega_line(cx-radius/2,cy+radius-shape,cx-radius,cy,colour);
}

static void rock_draw(const ROCK far *rock)
{
  int cx,cy,radius;unsigned char colour=rock->colour;
  cx=rock_x(rock);cy=rock_y(rock);radius=rock_radius(rock);saver_disc(cx,cy,radius,8);
  rock_outline(cx,cy,radius,rock->shape,colour);
}

void rocks_saver_loop(unsigned start_x,unsigned start_y)
{
  int i;unsigned long last_tick=bios_ticks(),tick;
  ega_rectangle(0,0,640,350,0);for(i=0;i<ROCK_COUNT;i++){rock_reset(&rocks[i],0);rock_draw(&rocks[i]);}
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    for(i=0;i<ROCK_COUNT;i++){
      rock_old_x[i]=rock_x(&rocks[i]);rock_old_y[i]=rock_y(&rocks[i]);
      rock_old_radius[i]=rock_radius(&rocks[i]);
      rock_old_shape[i]=rocks[i].shape;
    }
    for(i=0;i<ROCK_COUNT;i++){rocks[i].z-=3;if(rocks[i].z<9)rock_reset(&rocks[i],1);}
    wait_vertical_retrace();
    for(i=0;i<ROCK_COUNT;i++){
      rock_new_x[i]=rock_x(&rocks[i]);rock_new_y[i]=rock_y(&rocks[i]);
      rock_new_radius[i]=rock_radius(&rocks[i]);
      saver_restore_circle_exposed(rock_old_x[i],rock_old_y[i],rock_old_radius[i],
                                   rock_new_x[i],rock_new_y[i],rock_new_radius[i]);
      rock_outline(rock_old_x[i],rock_old_y[i],rock_old_radius[i],
                   rock_old_shape[i],0);
    }
    for(i=0;i<ROCK_COUNT;i++)rock_draw(&rocks[i]);
  }
}

/* -------------------------------- Scooter -------------------------------- */

#define SCOOTER_DOORS 15
#define SCOOTER_STARS 48
typedef struct { int x,y,z;unsigned char colour; } SCOOTER_STAR;
static int far scooter_door_z[SCOOTER_DOORS];
static SCOOTER_STAR far scooter_stars[SCOOTER_STARS];
static int far scooter_old_door_z[SCOOTER_DOORS];
static SCOOTER_STAR far scooter_old_stars[SCOOTER_STARS];

static int scooter_wave(int phase)
{
  phase&=255;if(phase<64)return phase*2;
  if(phase<192)return 256-phase*2;return phase*2-512;
}

static void scooter_draw_state(unsigned phase,unsigned char erase,
                               const int far *door_z,
                               const SCOOTER_STAR far *stars)
{
  int i,z,cx,cy,half_width,half_height,x,y,size;unsigned char colour;
  for(i=0;i<SCOOTER_DOORS;i++){
    z=door_z[i];cx=320+scooter_wave(phase+z)/2;cy=175+scooter_wave(phase*2+z)/5;
    half_width=4200/z;half_height=2750/z;colour=erase?0:(unsigned char)(9+(i%7));
    saver_box_outline(cx,cy,half_width,half_height,colour);
  }
  for(i=0;i<SCOOTER_STARS;i++){
    z=stars[i].z;x=320+(int)((long)stars[i].x*70/z);
    y=175+(int)((long)stars[i].y*70/z);size=180/z;if(size<1)size=1;
    colour=erase?0:stars[i].colour;ega_rectangle(x,y,size,size,colour);
  }
}

void scooter_saver_loop(unsigned start_x,unsigned start_y)
{
  int i;unsigned phase=0,old_phase;unsigned long last_tick=bios_ticks(),tick;
  ega_rectangle(0,0,640,350,0);
  for(i=0;i<SCOOTER_DOORS;i++)scooter_door_z[i]=24+i*12;
  for(i=0;i<SCOOTER_STARS;i++){
    scooter_stars[i].x=(int)saver_random(801)-400;scooter_stars[i].y=(int)saver_random(501)-250;
    scooter_stars[i].z=18+(int)saver_random(190);scooter_stars[i].colour=(unsigned char)(8+saver_random(8));
  }
  scooter_draw_state(phase,0,scooter_door_z,scooter_stars);
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;
    old_phase=phase;
    for(i=0;i<SCOOTER_DOORS;i++)scooter_old_door_z[i]=scooter_door_z[i];
    for(i=0;i<SCOOTER_STARS;i++)scooter_old_stars[i]=scooter_stars[i];
    phase=(phase+2)&255;
    for(i=0;i<SCOOTER_DOORS;i++){scooter_door_z[i]-=3;if(scooter_door_z[i]<20)scooter_door_z[i]+=180;}
    for(i=0;i<SCOOTER_STARS;i++){
      scooter_stars[i].z-=4;if(scooter_stars[i].z<10){
        scooter_stars[i].x=(int)saver_random(801)-400;scooter_stars[i].y=(int)saver_random(501)-250;
        scooter_stars[i].z=200;
      }
    }
    wait_vertical_retrace();
    /* Put the next complete tunnel on screen before removing its predecessor.
       A final repaint repairs intersections touched by the old outlines. */
    scooter_draw_state(phase,0,scooter_door_z,scooter_stars);
    scooter_draw_state(old_phase,1,scooter_old_door_z,scooter_old_stars);
    scooter_draw_state(phase,0,scooter_door_z,scooter_stars);
  }
}

/* ---------------------------- Spiro (Squiral) ---------------------------- */

#define SQUIRAL_COLS 80
#define SQUIRAL_ROWS 43
#define SQUIRAL_CELLS (SQUIRAL_COLS*SQUIRAL_ROWS)
#define SQUIRAL_WORMS 14
typedef struct { int x,y,direction,hand;unsigned char colour; } SQUIRAL_WORM;
static SQUIRAL_WORM far squiral_worms[SQUIRAL_WORMS];
static unsigned char far *squiral_cells;
static const signed char squiral_dx[4]={1,0,-1,0};
static const signed char squiral_dy[4]={0,1,0,-1};

static int squiral_clear_direction(const SQUIRAL_WORM far *worm,int direction)
{
  int x1=worm->x+squiral_dx[direction],y1=worm->y+squiral_dy[direction];
  int x2=x1+squiral_dx[direction],y2=y1+squiral_dy[direction];
  if(x1<0)x1+=SQUIRAL_COLS;if(x1>=SQUIRAL_COLS)x1-=SQUIRAL_COLS;
  if(x2<0)x2+=SQUIRAL_COLS;if(x2>=SQUIRAL_COLS)x2-=SQUIRAL_COLS;
  if(y1<0)y1+=SQUIRAL_ROWS;if(y1>=SQUIRAL_ROWS)y1-=SQUIRAL_ROWS;
  if(y2<0)y2+=SQUIRAL_ROWS;if(y2>=SQUIRAL_ROWS)y2-=SQUIRAL_ROWS;
  return !squiral_cells[y1*SQUIRAL_COLS+x1]&&!squiral_cells[y2*SQUIRAL_COLS+x2];
}

static void squiral_reset(void)
{
  int i;SQUIRAL_WORM far *worm;
  for(i=0;i<SQUIRAL_CELLS;i++)squiral_cells[i]=0;
  ega_rectangle(0,0,640,350,0);
  for(i=0;i<SQUIRAL_WORMS;i++){
    worm=&squiral_worms[i];worm->x=(int)saver_random(SQUIRAL_COLS);
    worm->y=(int)saver_random(SQUIRAL_ROWS);worm->direction=(int)saver_random(4);
    worm->hand=i&1;worm->colour=(unsigned char)(9+(i%7));
  }
}

void squiral_saver_loop(unsigned start_x,unsigned start_y)
{
  int i,turn,direction,attempt,index,moved,stalled=0;SQUIRAL_WORM far *worm;
  unsigned frames=0;
  unsigned long last_tick=bios_ticks(),tick;
  squiral_cells=(unsigned char far *)_fmalloc(SQUIRAL_CELLS);if(!squiral_cells)return;
  squiral_reset();
  while(!saver_input(start_x,start_y)){
    tick=bios_ticks();if(tick==last_tick)continue;last_tick=tick;wait_vertical_retrace();
    moved=0;for(i=0;i<SQUIRAL_WORMS;i++){
      worm=&squiral_worms[i];turn=worm->hand?1:3;direction=(worm->direction+turn)&3;
      for(attempt=0;attempt<4;attempt++){
        if(squiral_clear_direction(worm,direction))break;
        if(attempt==0)direction=worm->direction;
        else if(attempt==1)direction=(worm->direction+(worm->hand?3:1))&3;
        else direction=(worm->direction+2)&3;
      }
      if(attempt==4){
        worm->x=(int)saver_random(SQUIRAL_COLS);worm->y=(int)saver_random(SQUIRAL_ROWS);
        worm->direction=(int)saver_random(4);continue;
      }
      worm->direction=direction;worm->x+=squiral_dx[direction];worm->y+=squiral_dy[direction];
      if(worm->x<0)worm->x+=SQUIRAL_COLS;if(worm->x>=SQUIRAL_COLS)worm->x-=SQUIRAL_COLS;
      if(worm->y<0)worm->y+=SQUIRAL_ROWS;if(worm->y>=SQUIRAL_ROWS)worm->y-=SQUIRAL_ROWS;
      index=worm->y*SQUIRAL_COLS+worm->x;squiral_cells[index]=1;
      ega_rectangle(worm->x*8+1,worm->y*8+1,6,6,worm->colour);moved++;
    }
    frames++;if(moved)stalled=0;else stalled++;
    if(stalled>=90||frames>=1092){squiral_reset();stalled=0;frames=0;}
  }
  _ffree(squiral_cells);squiral_cells=0;
}
