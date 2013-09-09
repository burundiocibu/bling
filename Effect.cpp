#include "Effect.hpp"
#include "messages.hpp"
#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"

#include "avr/pgmspace.h"



const uint8_t e1_delays[] PROGMEM = {
   //           2     3     4     5     6     7     8     9
   0xff, 0xff, 0x00, 0xff, 0xff, 0x08, 0x02, 0x04, 0x06, 0xff
};

const uint8_t e3_delays[] PROGMEM = {
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 3, 255, 42, 255, 14, 255, 29, 255, 255, 28, 255, 255, 10, 255, 255, 56, 11, 255, 43, 255, 41, 255, 255, 255, 54, 255, 255, 36, 8, 255, 255, 61, 55, 20, 255, 255, 255, 255, 255, 255, 255, 22, 255, 255, 53, 255, 255, 58, 21, 255, 255, 255, 255, 48, 16, 27, 255, 17, 255, 47, 255, 255, 255, 57, 255, 2, 255, 15, 4, 23, 255, 255, 50, 255, 255, 255, 255, 44, 255, 255, 38, 19, 255, 255, 51, 255, 255, 0, 255, 31, 255, 255, 60, 255, 59, 255, 13, 255, 255, 255, 12, 26, 255, 32, 255, 255, 255, 255, 46, 255, 255, 52, 1, 30, 255, 49, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };


Effect::Effect(uint16_t slave)
{
   slave_id = slave;
   reset();

   for (size_t i=0; i<max_effect; i++)
      se_delay[i] = 0;

   se_delay[1] = 0xffff;
   if (slave_id < sizeof(e1_delays))
   {
      uint8_t d = pgm_read_byte( &(e1_delays[slave_id]));
      if  (d!=0xff)
         se_delay[1] = d * 30;
   }

   se_delay[3] = 0xffff;
   if (slave_id < sizeof(e3_delays))
   {
      uint8_t d = pgm_read_byte( &(e3_delays[slave_id]));
      if (d!=0xff)
         se_delay[3] = d * 30;
   }

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


// Flash red/green/blue for the duration of the effect; mainly for testing.
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


// Set 4, Count 56 start
void Effect::e3()
{
   e1();
}


void Effect::e4()
{
}


void Effect::e5()
{
}
