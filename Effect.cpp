#include "Effect.hpp"
#include "messages.hpp"
#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"

#include "avr/pgmspace.h"



Effect::Effect(uint16_t slave)
{
   state = complete;
   slave_id = slave;
};


void Effect::init(uint8_t* buff)
{
   uint8_t new_id;
   uint32_t new_start_time;
   uint16_t new_duration;

   messages::decode_start_effect(buff, new_id, new_start_time, new_duration);

   // If we are already running this command, don't start over...
   if (new_id == id && new_start_time == start_time && new_duration == duration)
      return;

   id = new_id;
   start_time = new_start_time;
   duration = new_duration;
   dt = 0;
   prev_dt = 0;
   state = unstarted;
   
   // offset start of effect 1 by 300 ms / slave
   if (id==1)
      start_time += slave_id*300;
}


void Effect::execute()
{
   if (state==complete)
      return;

   dt = avr_rtc::t_ms - start_time;

   if (dt>int(duration) && state==started)
   {
      state=complete;
      for (unsigned ch=0; ch<14; ch++)
         avr_tlc5940::set_channel(ch, 0);
      return;
   }
   else if (dt>0 && dt<int(duration) && state==unstarted)
      state=started;

   // Don't really need to update the LEDs more often than 50 Hz
   if (prev_dt>0 && dt - prev_dt < 20)
      return;

   if (state==started)
      switch(id)
      {
         case 0: e0(); break;
         case 1: e1(); break;
         case 2: e2(); break;
         case 3: e3(); break;
      }
   prev_dt = dt;
}


void Effect::all_stop()
{
   state = complete;
   for (int ch=0; ch<14; ch++)
      avr_tlc5940::set_channel(ch, 0);
}


// Flash all full intensity and fade out over a the duration
void Effect::e0()
{
   const long vmax = 4095;
   long m = vmax/duration;
   int v = vmax - dt * vmax / duration;
   if (v<0)
      v=0;
   for (unsigned ch=0; ch<12; ch++)
      avr_tlc5940::set_channel(ch, v);
}


// A simple color fade that cycles every 3 seconds and fades out at the end
void Effect::e1()
{
   long cl = 3000; // length of cycle in ms
   long phi = 0; // offset into cycle
   long tte = duration - dt; // time to end

   long vmax = 4095; // intensity at peak
   // and this fades out intensity at the end...
   if (tte <1500)
   {
      vmax *= tte;
      vmax /= 1000;
   }
   long cl2 = cl >> 1; 

   long dtp = dt-phi;
   long cldt = dtp<=0 ? 0 : dtp % cl;

   int v;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=1; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);

   phi += 1000;
   dtp = dt-phi;
   cldt = dtp<=0 ? 0 : dtp % cl;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   for (unsigned ch=2; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);

   phi += 2000;
   dtp = dt-phi;
   cldt = dtp<=0 ? 0 : dtp % cl;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=0; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);
}


// Flash red/green/blue for the duration of the effect
// mainly for testing
void Effect::e2()
{
   long vmax = 512; // intensity at peak
   long cldt = dt & 0x3ff;
   for (unsigned ch=0; ch<12; ch++)
      avr_tlc5940::set_channel(ch, 1);
   if (cldt < 170)
      for (unsigned ch=0; ch<12; ch+=3)
         avr_tlc5940::set_channel(ch, vmax);
   else if (341 < cldt && cldt < 511)
      for (unsigned ch=1; ch<12; ch+=3)
         avr_tlc5940::set_channel(ch, vmax);
   else if (680 < cldt && cldt < 852)
      for (unsigned ch=2; ch<12; ch+=3)
         avr_tlc5940::set_channel(ch, vmax);
}


const uint16_t set4_count56[] PROGMEM = {
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 90, 65535, 1260, 65535, 420, 65535, 870, 65535, 65535, 840, 65535, 65535, 300, 65535, 65535, 1680, 330, 65535, 1290, 65535, 1230, 65535, 65535, 65535, 1620, 65535, 65535, 1080, 240, 65535, 65535, 1830, 1650, 600, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 660, 65535, 65535, 1590, 65535, 65535, 1740, 630, 65535, 65535, 65535, 65535, 1440, 480, 810, 65535, 510, 65535, 1410, 65535, 65535, 65535, 1710, 65535, 60, 65535, 450, 120, 690, 65535, 65535, 1500, 65535, 65535, 65535, 65535, 1320, 65535, 65535, 1140, 570, 65535, 65535, 1530, 65535, 65535, 0, 65535, 930, 65535, 65535, 1800, 65535, 1770, 65535, 390, 65535, 65535, 65535, 360, 780, 65535, 960, 65535, 65535, 65535, 65535, 1380, 65535, 65535, 1560, 30, 900, 65535, 1470, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535 };


// Set 4, Count 56 start
void Effect::e3()
{
   if (slave_id > sizeof(set4_count56))
      return;
   uint16_t offset = pgm_read_word( &(set4_count56[slave_id]));
   if (offset == 0xffff)
      return;
}

void Effect::e4()
{
}

void Effect::e5()
{
}
