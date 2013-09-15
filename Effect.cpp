#include "Effect.hpp"
#include "messages.hpp"
#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"

#include "avr/pgmspace.h"


const uint8_t  e2_delay [] PROGMEM = {
255, 255, 30, 255, 255, 20, 10, 0, 10, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 20, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 40, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 50, 255, 255, 255, 40, 255, 255, 255, 255, 30, 255, 255, 255, 50, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

const uint8_t  e3_delay [] PROGMEM = {
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 3, 255, 42, 255, 14, 255, 29, 255, 255, 28, 255, 255, 10, 255, 255, 57, 11, 255, 43, 255, 41, 255, 255, 255, 55, 255, 255, 36, 8, 255, 255, 62, 56, 20, 255, 255, 255, 255, 255, 255, 255, 22, 255, 255, 54, 255, 255, 59, 21, 255, 255, 255, 255, 49, 16, 27, 45, 17, 255, 48, 255, 255, 255, 58, 255, 2, 255, 15, 4, 23, 255, 255, 51, 255, 255, 255, 255, 44, 255, 255, 38, 19, 255, 255, 52, 255, 255, 0, 9, 31, 255, 255, 61, 255, 60, 255, 13, 255, 255, 255, 12, 26, 255, 32, 255, 255, 255, 255, 47, 255, 255, 53, 1, 30, 255, 50, 255, 18, 255, 255, 255, 255, 5, 7, 6, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

const uint8_t  e4_delay [] PROGMEM = {
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 13, 255, 32, 255, 45, 255, 255, 255, 255, 255, 255, 255, 255, 9, 255, 255, 6, 255, 255, 255, 255, 11, 44, 255, 255, 47, 255, 255, 255, 5, 255, 255, 255, 38, 19, 255, 255, 42, 39, 0, 255, 255, 1, 255, 35, 34, 255, 255, 255, 25, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 17, 255, 255, 255, 255, 255, 255, 255, 15, 255, 255, 255, 2, 49, 255, 255, 8, 36, 255, 255, 28, 20, 255, 255, 255, 255, 255, 255, 255, 10, 255, 37, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 12, 40, 18, 255, 33, 53, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

const uint8_t  e5_delay [] PROGMEM = {
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 149, 255, 149, 255, 149, 255, 149, 255, 255, 149, 255, 255, 149, 255, 255, 149, 149, 255, 149, 255, 149, 255, 255, 255, 149, 255, 255, 149, 149, 255, 255, 149, 149, 149, 255, 255, 255, 255, 255, 255, 255, 149, 255, 255, 149, 255, 255, 149, 149, 255, 255, 255, 255, 149, 149, 149, 149, 149, 255, 149, 255, 255, 255, 149, 255, 149, 255, 149, 149, 149, 255, 255, 149, 255, 255, 255, 255, 149, 255, 255, 149, 149, 255, 255, 149, 255, 255, 149, 149, 149, 255, 255, 149, 255, 149, 255, 149, 255, 255, 255, 149, 149, 255, 149, 255, 255, 255, 255, 149, 255, 255, 149, 149, 149, 255, 149, 255, 149, 255, 255, 255, 255, 149, 149, 149, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };


uint16_t Effect::get_delay(const uint8_t lut[], size_t len)
{
   if (slave_id < len)
   {
      uint8_t d = pgm_read_byte( &(lut[slave_id]));
      if  (d!=0xff)
         return d * 4;
   }
   return 0xffff;
}


Effect::Effect(uint16_t slave)
{
   slave_id = slave;
   reset();

   for (size_t i=0; i<max_effect; i++)
      se_delay[i] = 0;

   se_delay[2] = get_delay(e2_delay, sizeof(e2_delay));
   se_delay[3] = get_delay(e3_delay, sizeof(e3_delay));
   se_delay[4] = get_delay(e4_delay, sizeof(e4_delay));
   se_delay[5] = get_delay(e5_delay, sizeof(e5_delay));
};


void Effect::init(uint8_t* buff)
{
   uint8_t new_id;
   uint32_t new_start_time;
   uint16_t new_duration;
   messages::decode_start_effect(buff, new_id, new_start_time, new_duration);

   if (new_id >= max_effect)
   {
      reset();
      return;
   }

   // If we are already running this command, don't start over...
   if (new_id !=0 && new_id == id && new_start_time == start_time && new_duration == duration)
      return;

   // If our effect/slave specific delay is 0xffff, don't execute this effect
   if (se_delay[new_id] == 0xffff)
      return;

   reset();

   id = new_id;
   start_time = new_start_time + se_delay[id];
   duration = new_duration;
   state = pending;
}


void Effect::execute()
{
   if (state==stopped)
      return;

   dt = avr_rtc::t_ms - start_time;

   if (dt < 0)
      return;

   if (dt>int(duration) && state!=stopped)
   {
      reset();
      return;
   }
   
   if (state==pending)
      state=running;
//   else if (prev_dt>0 && dt - prev_dt < 20)
      // Don't really need to update the LEDs more often than 50 Hz
//      return;

   switch(id)
   {
      case 0: e0(); break;
      case 1: e1(); break;
      case 2: e2(); break;
      case 3: e3(); break;
      case 4: e4(); break;
   }
   prev_dt = dt;
}


void Effect::all_stop(uint8_t* buff)
{
   uint8_t new_id;
   uint32_t new_start_time;
   uint16_t new_duration;
   messages::decode_start_effect(buff, new_id, new_start_time, new_duration);
   reset();
}


void Effect::reset()
{
   state = stopped;
   id = messages::all_stop_id;
   start_time = 0;
   duration = 0;

   for (int ch=0; ch<14; ch++)
      avr_tlc5940::set_channel(ch, 0);
}


// Flash all full intensity and fade out over a the duration
void Effect::e0()
{
   const long vmax = 4095;
   int v = vmax - dt * vmax / duration;
   if (v<0)
      v=0;
   for (unsigned ch=0; ch<12; ch++)
      avr_tlc5940::set_channel(ch, v);
}


// Flash red/green/blue for the duration of the effect; mainly for testing.
void Effect::e1()
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


/*
         _________
        /         \
       /           \
______/             \___________________________
      |  |        |  |                     |
     t0  t1       t2 t3                   cl

 */
void Effect::e2()
{
   const long cl = 2000; // length of cycle in ms
   const long dwell = 500; // time at full intensity
   const long rise_time = 100; // time to ramp light
   const long vmax = 4095; // intensity at peak
   const long t1 = rise_time;
   const long t2 = rise_time + dwell;
   const long t3 = 2*rise_time + dwell;

   long cldt = dt<=0 ? 0 : dt % cl;
   
   int v;
   if (cldt < t1)
      v = (vmax * cldt) / rise_time;
   else if (cldt < t2)
      v = vmax;
   else if (cldt < t3)
      v = vmax - (vmax * cldt) / rise_time;
   else
      v = 0;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=1; ch<12; ch+=3)  // red
      avr_tlc5940::set_channel(ch, v);
   for (unsigned ch=2; ch<12; ch+=3)  // green
      avr_tlc5940::set_channel(ch, 0);
   for (unsigned ch=0; ch<12; ch+=3)  // blue
      avr_tlc5940::set_channel(ch, 0);
}


// A simple color fade that cycles every 3 seconds and fades out at the end
void Effect::e3()
{
   long cl = 2000; // length of cycle in ms
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


// Set 4, Count 56 start
void Effect::e4()
{
   e3();
}


void Effect::e5()
{
   e2();
}
