#include "avr/pgmspace.h"
#include "stdlib.h"

#include "Effect.hpp"
#include "messages.hpp"
#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"

const uint8_t  all_l2r [] PROGMEM = {
255, 255, 36, 255, 255, 74, 12, 0, 86, 255, 113, 105, 20, 67, 48, 255, 33, 50, 35, 34, 255, 255, 52, 69, 255, 5, 51, 119, 255, 255, 21, 255, 255, 45, 7, 255, 37, 26, 54, 255, 121, 0, 6, 42, 55, 93, 255, 99, 47, 53, 131, 40, 255, 129, 8, 61, 63, 25, 41, 60, 81, 27, 255, 13, 46, 35, 17, 45, 28, 14, 255, 71, 97, 4, 22, 255, 255, 47, 58, 39, 101, 255, 11, 62, 127, 33, 73, 18, 115, 59, 24, 43, 75, 125, 10, 255, 255, 62, 53, 31, 37, 111, 1, 57, 2, 255, 49, 255, 44, 255, 50, 36, 31, 30, 87, 107, 51, 95, 15, 65, 25, 9, 61, 32, 255, 12, 59, 255, 255, 255, 255, 255, 57, 55, 56, 27, 255, 3, 255, 38, 89, 79, 29, 23, 117, 103, 16, 49, 39, 255, 255, 255, 255, 255, 83, 123, 85, 109, 29, 255, 41, 255, 255, 77, 255, 255, 255, 24, 255, 255, 255, 255, 255, 255, 255 };

uint16_t Effect::get_delay(const uint8_t lut[], size_t len)
{
   if (slave_id < len)
   {
      uint8_t d = pgm_read_byte( &(lut[slave_id]));
      if  (d!=0xff)
         return d * 40;
   }
   return 0xffff;
}


Effect::Effect(uint16_t slave)
{
   slave_id = slave;
   srand(slave);
   my_rand = rand();
   reset();
};


void Effect::init(uint8_t* buff)
{
   uint8_t new_id;
   uint32_t new_start_time;
   uint32_t new_duration;
   messages::decode_start_effect(buff, new_id, new_start_time, new_duration);

   if (new_id >= max_effect)
   {
      reset();
      return;
   }

   // If we are already running this command, don't start over...
   if (new_id !=0 && new_id == id && new_start_time == start_time && new_duration == duration)
      return;

   se_delay = 0;
   switch(new_id)
   {
      case 7: se_delay = get_delay(all_l2r, sizeof(all_l2r));    break;
   }

   // If our effect/slave specific delay is 0xffff, don't execute this effect
   if (se_delay == 0xffff)
      return;

   id = new_id;
   start_time = new_start_time + se_delay;
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

   if (dt>duration && state!=stopped)
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
      case 5: e5(); break;
      case 6: e6(); break;
      case 7: e7(); break;
   }
   prev_dt = dt;
}


void Effect::all_stop(uint8_t* buff)
{
   uint8_t new_id;
   uint32_t new_start_time;
   uint32_t new_duration;
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
        /         |
       /          |
______/           |_____________
      |  |        |
     t0  t1       dur

 */
void Effect::e2()
{
   const long dwell = 500; // time at full intensity
   const long t1 = duration - dwell; // time to ramp light
   const long vmax = 4095; // intensity at peak

   int v;
   if (dt < t1)
      v = (vmax * dt) / t1;
   else
      v = vmax;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=1; ch<12; ch+=3)  // red
      avr_tlc5940::set_channel(ch, v);
   for (unsigned ch=2; ch<12; ch+=3)  // green
      avr_tlc5940::set_channel(ch, v);
   for (unsigned ch=0; ch<12; ch+=3)  // blue
      avr_tlc5940::set_channel(ch, v);
}


// Same as e2 but red
void Effect::e3()
{
   const long rise_time = 5000;
   const long vmax = 4095; // intensity at peak

   int v;
   if (dt < rise_time)
      v = (vmax * dt) / rise_time;
   else
      v = vmax;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=1; ch<12; ch+=3)  // red
      avr_tlc5940::set_channel(ch, v);
}



/*
red sparkle
Red:  ____________________________
     /
____/
Blue:      ______          ______
          /      |        /
_________/       |_______/
    | |  | |     |
    t t  t t     t
    0 1  2 3     4
 */
void Effect::e4()
{
   // my rand = [0..7fff];
   const long t0 = (my_rand >> 8) * 2; // length of cycle in ms 0 ... 127*5 
   const long cl = (my_rand & 0xff) * 4 + 512; // 
   const long dwell = 400; // time at full intensity
   const long rise_time = 100; // time to ramp light
   const long vmax = 4095; // intensity at peak

   unsigned r=0;

   if (dt < rise_time)
      r = (vmax * dt) / rise_time;
   else
      r = vmax;

   for (unsigned ch=1; ch<12; ch+=3)  // red
      avr_tlc5940::set_channel(ch, r);

   unsigned b=0;
   long cldt = dt<=0 ? 0 : dt % cl;

   if (t0 < cldt && cldt < t0+dwell )
   {
      if (cldt < rise_time)
         b = (vmax * cldt) / rise_time;
      else
         b = vmax;
   }
   for (unsigned ch=0; ch<12; ch+=3)  // blue
      avr_tlc5940::set_channel(ch, b);
}


// Same as 5 but fades out 
void Effect::e5()
{
   // my rand = [0..7fff];
   const long t0 = (my_rand >> 8) * 2; // length of cycle in ms 0 ... 127*5 
   const long cl = (my_rand & 0xff) * 4 + 512; // 
   const long dwell = 400; // time at full intensity
   const long rise_time = 100; // time to ramp light
   long vmax = 4095; // intensity at peak
   const long fade_time=1500;

   vmax = dt < fade_time ? vmax * ( fade_time - dt) / fade_time : 0;

   unsigned r=0;

   r = vmax;

   for (unsigned ch=1; ch<12; ch+=3)  // red
      avr_tlc5940::set_channel(ch, r);

   unsigned b=0;
   long cldt = dt<=0 ? 0 : dt % cl;

   if (t0 < cldt && cldt < t0+dwell )
   {
      if (cldt < rise_time)
         b = (vmax * cldt) / rise_time;
      else
         b = vmax;
   }
   for (unsigned ch=0; ch<12; ch+=3)  // blue
      avr_tlc5940::set_channel(ch, b);
}

void Effect::e6()
{
   const long rise_time = 2000;
   const long vmax = 512; // intensity at peak

   int v;
   if (dt > rise_time)
      v = vmax;
   else
      v = (vmax * dt) / rise_time;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=1; ch<12; ch+=3)  // red
      avr_tlc5940::set_channel(ch, v);
}


void Effect::e7()
{
   int v = 0;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=1; ch<12; ch+=3)  // red
      avr_tlc5940::set_channel(ch, v);
}
