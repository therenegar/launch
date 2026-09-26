/*
    __                           __    __
   / /   ____ ___  ______  _____/ /_  / /
  / /   / __ `/ / / / __ \/ ___/ __ \/ / 
 / /___/ /_/ / /_/ / / / /__/ / / /_/  
/_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
Launch! for DOS ---------------------
*/
/*
 * MAINTAINER NOTES - Launch! 3.73
 * File: SAVERS.H
 * Role: Screensaver interface
 * Build/ownership: Consumed by Core.
 * Maintainer contract: Keep declarations synchronized with SAVERS.C and Config saver IDs/names.
 * Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
 * DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
 */
#ifndef LAUNCH_SAVERS_H
#define LAUNCH_SAVERS_H

void abstractile_saver_loop(unsigned start_x,unsigned start_y);
void blaster_saver_loop(unsigned start_x,unsigned start_y);
void rocks_saver_loop(unsigned start_x,unsigned start_y);
void scooter_saver_loop(unsigned start_x,unsigned start_y);
void squiral_saver_loop(unsigned start_x,unsigned start_y);
void splotchy_saver_loop(unsigned start_x,unsigned start_y);

#endif
