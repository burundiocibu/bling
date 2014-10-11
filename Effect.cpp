#include "avr/pgmspace.h"
#include "stdlib.h"

#include "Effect.hpp"
#include "messages.hpp"
#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"

#include "Effect_sets.hpp"

const long vmax = 4095;


uint8_t Effect::get_delay(const uint8_t lut[], size_t len)
{
   if (slave_id < len)
   {
      uint8_t d = pgm_read_byte( &(lut[slave_id]));
      return d;
   }
   return 0xff;
}


Effect::Effect(uint16_t slave)
{
   slave_id = slave;
   srand(slave);
   my_rand = rand();
   for (size_t i=0; i<sizeof(noise); i++)
      noise[i] = rand();
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

   id = new_id;
   start_time = new_start_time;
   duration = new_duration;
   state = pending;
}

void Effect::start(uint8_t _id, uint32_t _duration)
{
   if (_id >= max_effect)
   {
      reset();
      return;
   }

   id = _id;
   start_time = avr_rtc::t_ms;
   duration = _duration;
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

   switch(id)
   {
      case 0: reset(); break;
      case 1: flash_id(); break;
      case 2: pulse(); break;
      case 3: sel_on(); break;
      case 4: opener_on(); break;
      case 5: opener_white_sparkle(); break;
      case 6: balad_on(); break;
      default: break;
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

void set_red(uint16_t v)
{
   for (unsigned ch=1; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);
}

void set_green(uint16_t v)
{
   for (unsigned ch=2; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);
}

void set_blue(uint16_t v)
{
   for (unsigned ch=0; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);
}

void set_rgb(uint16_t r, uint16_t g, uint16_t b)
{
   set_red(r);
   set_green(g);
   set_blue(b);
}

void set_all(uint16_t v)
{
   for (unsigned ch=0; ch<14; ch++)
      avr_tlc5940::set_channel(ch, v);
}

void Effect::reset()
{
   state = stopped;
   id = messages::all_stop_id;
   start_time = 0;
   duration = 0;

   set_all(0);
}

/*
A generic flash
          _________vmax
         /|       |\
        / |       | \________vmin
       /  |       | |
______/   |       | | 
T     0   1       2 3
*/
//             rise     dwell    fall              dwell
uint16_t flash(long t1, long t2, long t3, long dt, uint16_t vmax, uint16_t vmin)
{
   if (dt < t1)
      return vmax * dt / t1;
   else if (dt < t2)
      return vmax;
   else if (dt < t3)
      return (vmax * (t3-dt) + vmin*(dt-t2)) / (t3-t2);
   return vmin;
}


/* Flash the unit ID as an 8 bit code
   id 19 = 00010100
   0: __-_____
   1: __------
   Each bit is 500 ms long, whole code takes 4 seconds
*/
void Effect::flash_id()
{
   int n = dt / 500;
   int m = dt % 500;
   int mask = 1 << n;
   int b = slave_id & mask;  // really want this to be marcher ID
   int v = vmax / 64;
   if (m<100)
      set_all(0);
   else if (m<120)
      set_all(v);
   else if (b)
      set_all(v);
   else
      set_all(0);
}


/*
All slaves flash all channels on full intensity and fade out over a the duration
rgb:   
      |\
______| \____
*/
void Effect::pulse()
{
   const int vm = vmax / 2;
   int v = vm - dt * vm / duration;
   if (v<0)
      v=0;
   set_all(v);
}




/*
Selected slaves quickly turn on all channels 50% and stay on for duration
rgb:    _________
       /         |
______/          |_____________
T:    0 1        2
 */
void Effect::sel_on()
{
   const uint8_t s=get_delay(section, sizeof(section));
   if (!(s & 0x1))
      return;

   int v = flash(100, duration - 100, 0, dt, vmax/2, 0);
   set_all(v);
}


/*
Slaves quickly turn on all channels 50% and stay on for duration
 */
void Effect::opener_on()
{
   const uint8_t s=get_delay(section, sizeof(section));
   if (!(s & 0x2))
      return;

   int v = flash(100, duration - 100, 0, dt, vmax/2, 0);
   set_all(v);
}


/*
Slaves sparkle randomly for duration
        ___________           
        |         |             
  ______|         |
T 0     1         2
Cycle length = T2
*/
void Effect::opener_white_sparkle()
{
   const uint8_t s=get_delay(section, sizeof(section));
   if (!(s & 0x2))
      return;

   const long v = vmax/2;
   size_t n = 0x1f & (dt>>8);
   const long t1 = 128 + 16*(noise[n]>>4); // 128 to 256 ms
   const long t2 = t1 + 64 + 4*(0xf & noise[n]); // on for 64..192 ms
   long cldt = dt<=0 ? 0 : dt % t2;

   if (cldt < t1)
      set_all(0);
   else if (cldt < t2)
      set_all(v);
   else
      set_all(0);
   return;

}

/*
Slaves quickly turn on all channels 50% and stay on for duration
*/
void Effect::balad_on()
{
   const uint8_t s=get_delay(section, sizeof(section));
   if (!(s & 0x4))
      return;

   int v = flash(100, duration - 100, 0, dt, vmax/2, 0);
   set_all(v);
}
