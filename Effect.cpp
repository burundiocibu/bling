#include "avr/pgmspace.h"
#include "stdlib.h"

#include "Effect.hpp"
#include "messages.hpp"
#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"

const uint8_t  all_l2r [] PROGMEM = {
255, 255, 255, 255, 255, 120, 20, 0, 140, 255, 46, 160, 60, 103, 144, 33, 99, 80, 55, 102, 255, 255, 156, 106, 255, 15, 153, 181, 91, 255, 63, 255, 255, 70, 21, 255, 58, 78, 162, 255, 184, 0, 18, 126, 85, 142, 255, 151, 73, 82, 199, 120, 255, 196, 24, 94, 97, 75, 123, 180, 124, 81, 255, 39, 138, 105, 51, 135, 84, 42, 255, 109, 148, 12, 255, 255, 255, 141, 174, 117, 154, 255, 255, 100, 193, 52, 112, 54, 175, 255, 72, 129, 115, 190, 30, 255, 255, 186, 159, 93, 111, 169, 3, 88, 6, 255, 147, 255, 132, 255, 150, 108, 49, 90, 133, 163, 79, 145, 45, 100, 40, 27, 183, 96, 255, 36, 177, 255, 255, 255, 255, 255, 171, 165, 168, 43, 255, 9, 255, 114, 136, 121, 87, 69, 178, 157, 48, 76, 61, 255, 255, 255, 255, 255, 127, 187, 130, 60, 172, 166, 64, 255, 66, 118, 255, 255, 255, 40, 255, 255, 255, 255, 255, 255, 255 };
const uint8_t  m1_set13 [] PROGMEM = {
255, 255, 255, 255, 255, 20, 8, 12, 16, 255, 22, 8, 14, 26, 30, 10, 8, 28, 18, 8, 255, 12, 22, 30, 255, 16, 28, 6, 22, 255, 6, 255, 255, 24, 20, 255, 18, 2, 22, 255, 8, 16, 12, 12, 20, 4, 255, 10, 22, 18, 16, 6, 255, 6, 10, 18, 14, 4, 10, 26, 28, 2, 255, 12, 30, 6, 14, 28, 0, 14, 255, 28, 6, 16, 255, 255, 255, 30, 26, 8, 14, 255, 255, 24, 4, 26, 26, 10, 255, 255, 2, 12, 30, 12, 20, 255, 24, 22, 26, 0, 8, 14, 18, 20, 20, 255, 30, 255, 28, 255, 28, 6, 20, 0, 12, 4, 22, 8, 18, 16, 24, 20, 24, 2, 10, 16, 22, 255, 255, 255, 255, 255, 24, 26, 24, 26, 255, 18, 255, 4, 16, 30, 0, 4, 10, 14, 18, 24, 20, 255, 255, 255, 255, 255, 28, 12, 28, 0, 14, 16, 24, 255, 4, 30, 255, 255, 255, 4, 255, 255, 255, 255, 255, 255, 255 };
const uint8_t  m3_set21 [] PROGMEM = {
255, 255, 255, 255, 255, 70, 28, 42, 56, 255, 60, 36, 60, 64, 42, 33, 90, 98, 76, 87, 60, 255, 30, 60, 255, 15, 33, 4, 92, 255, 63, 65, 255, 88, 21, 255, 84, 78, 24, 255, 32, 0, 18, 60, 104, 12, 255, 16, 255, 92, 28, 69, 90, 52, 24, 88, 100, 75, 63, 6, 52, 81, 55, 39, 48, 84, 51, 51, 84, 42, 8, 72, 16, 12, 255, 255, 255, 45, 12, 72, 36, 50, 255, 84, 40, 80, 68, 54, 24, 255, 72, 57, 48, 0, 30, 255, 66, 0, 27, 93, 78, 20, 3, 100, 6, 95, 39, 75, 54, 255, 36, 81, 64, 90, 56, 0, 96, 12, 45, 96, 68, 27, 3, 93, 255, 36, 9, 255, 255, 255, 100, 255, 15, 21, 18, 72, 80, 9, 70, 75, 8, 56, 87, 69, 24, 28, 48, 84, 76, 255, 85, 255, 255, 255, 48, 40, 44, 0, 57, 4, 80, 255, 66, 44, 255, 255, 105, 14, 255, 255, 255, 255, 255, 255, 255 };
const uint8_t  m3_set30_AB [] PROGMEM = {
255, 255, 255, 255, 255, 1, 0, 0, 1, 255, 1, 0, 1, 0, 1, 0, 255, 1, 0, 0, 255, 1, 0, 1, 255, 1, 0, 0, 0, 255, 0, 255, 255, 1, 1, 255, 1, 1, 0, 255, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 255, 1, 0, 1, 0, 255, 0, 0, 1, 1, 255, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 255, 255, 255, 1, 1, 1, 0, 255, 255, 1, 0, 1, 0, 1, 1, 255, 0, 1, 1, 0, 0, 255, 0, 0, 0, 0, 1, 1, 0, 0, 0, 255, 1, 255, 1, 255, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 255, 1, 0, 255, 255, 255, 255, 255, 1, 1, 1, 0, 255, 0, 255, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 255, 255, 255, 255, 255, 1, 0, 0, 0, 1, 0, 1, 255, 0, 0, 255, 255, 255, 0, 255, 255, 255, 255, 255, 255, 255 };
const uint8_t  section [] PROGMEM = {
255, 255, 255, 255, 255, 2, 2, 2, 2, 255, 2, 2, 1, 2, 1, 1, 1, 2, 2, 1, 0, 2, 1, 2, 255, 1, 1, 2, 2, 255, 1, 1, 255, 2, 1, 255, 2, 1, 1, 255, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 2, 1, 2, 2, 1, 1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 255, 255, 255, 1, 1, 1, 2, 0, 255, 2, 2, 2, 2, 1, 2, 255, 1, 1, 2, 2, 1, 255, 1, 1, 1, 1, 1, 2, 1, 2, 1, 0, 1, 1, 1, 255, 1, 1, 2, 1, 2, 2, 2, 2, 1, 2, 2, 1, 1, 1, 2, 1, 1, 255, 255, 255, 0, 255, 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1, 2, 2, 1, 2, 2, 255, 1, 255, 255, 255, 2, 2, 2, 2, 1, 2, 2, 255, 1, 2, 255, 255, 0, 2, 255, 255, 255, 255, 255, 255, 255 };


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
      case 0: e0(); break;
      case 1: e1(); break;
      case 2: e2(); break;
      case 3: e3(); break;
      case 4: e4(); break;
      case 5: e5(); break;
      case 6: e6(); break;
      case 7: e7(); break;
      case 8: e8(); break;
      case 9: e9(); break;
      case 10: e10(); break;
      case 11: e11(); break;
      case 12: e12(); break;
      case 13: e13(); break;
      case 14: e14(); break;
      case 15: e15(); break;
      case 16: e16(); break;
      case 17: e17(); break;
      case 18: e18(); break;
      case 19: e19(); break;
      case 20: e20(); break;
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

/*
 Flash all full intensity and fade out over a the duration
rgb:   
      |\
______| \____
*/
void Effect::e0()
{
   const long vmax = 4095;
   int v = vmax - dt * vmax / duration;
   if (v<0)
      v=0;
   set_rgb(v, v, v);
}

// Flash red/green/blue for the duration of the effect; mainly for testing.
void Effect::e1()
{
   long vmax = 512; // intensity at peak
   long cldt = dt & 0x3ff;
   for (unsigned ch=0; ch<12; ch++)
      avr_tlc5940::set_channel(ch, 1);
   if (cldt < 170)
      set_green(vmax);
   else if (341 < cldt && cldt < 511)
      set_red(vmax);
   else if (680 < cldt && cldt < 852)
      set_blue(vmax);
}


/*
        _________
all    /         |
______/          |_____________
T:    0 1        2
 */
void Effect::e2()
{
   const long dwell = 500; // t2-t1
   const long rise = duration - dwell; // t1-t0
   const long vmax = 4095; // intensity at peak

   int v;
   if (dt < rise)
      v = (vmax * dt) / rise;
   else
      v = vmax;

   set_rgb(v, v, v);
}


/*
        _________
red    /         |
______/          |_____________
T:    0 1        2
 */
void Effect::e3()
{
   const long rise_time = 5000;
   const long vmax = 4095; // intensity at peak

   int v;
   if (dt < rise_time)
      v = (vmax * dt) / rise_time;
   else
      v = vmax;

   set_red(v);
}



/*
red/blue sparkle
      ____________________________
Red  /
____/
           ______          ______
Blue      /      |        /
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

   set_red(r);

   unsigned b=0;
   long cldt = dt<=0 ? 0 : dt % cl;

   if (t0 < cldt && cldt < t0+dwell )
   {
      if (cldt < rise_time)
         b = (vmax * cldt) / rise_time;
      else
         b = vmax;
   }
   set_blue(b);
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

   set_red(r);

   unsigned b=0;
   long cldt = dt<=0 ? 0 : dt % cl;

   if (t0 < cldt && cldt < t0+dwell )
   {
      if (cldt < rise_time)
         b = (vmax * cldt) / rise_time;
      else
         b = vmax;
   }
   set_blue(b);
}


/* just brings red up to 50% */
void Effect::e6()
{
   const long rise_time = 2000;
   const long vmax = 2047; // intensity at peak

   int v;
   if (dt > rise_time)
      v = vmax;
   else
      v = (vmax * dt) / rise_time;

   set_red(v);
}

/* just turns all off */
void Effect::e7()
{
   const uint8_t d = get_delay(all_l2r, sizeof(all_l2r));
   if (d==0xff)
      return;
   const uint16_t delay = d*25;
   if (dt <= delay)
      return;

   set_rgb(0, 0, 0);
}


/*
        _________          ________
all    /         \        /        \
______/           \______/          \______
T:    0 1       2 3      4...
 */
void Effect::e8()
{
   const uint8_t d = get_delay(m1_set13, sizeof(m1_set13));
   if (d==0xff)
      return;
   const uint16_t delay = d*25;

   const long t1 = 150;
   const long t2 = 675;
   const long t3 = 825;
   const long t4 = 2550; // cycle length
   const long vmax = 2047; // intensity at peak

   unsigned v=0;
   long cldt = dt<=delay ? 0 : (dt-delay) % t4;

   if (cldt < t1)
      v = (vmax * cldt) / t1;
   else if (cldt < t2)
      v = vmax;
   else if (cldt < t3)
      v = (vmax * (t3-cldt)) / (t3-t2);
   else
      v = 0;

   set_rgb(v, v, v);
}

void flash_rgb(uint16_t r, uint16_t g, uint16_t b)
{
}


/*
movement 3, A1..9
         ____
rgb     |    \
________|     \____
T       0   1  2
*/
void Effect::e9()
{
   int v = vmax - dt * vmax / duration;
   if (v<0)
      v=0;
   set_rgb(0, v, 0);
}


/*
  movement 3, A10
*/
void Effect::e10()
{
   const uint8_t s = get_delay(section, sizeof(section));
   if (s==1 || s==2 || s==3)
   {
      int v = vmax - dt * vmax / 1411;
      if (v<0)
         v=0;
      set_rgb(0, v, 0);
   }
   else
   {
      set_rgb(0, vmax, 0);
   }
      
}

/*
  movement 3, D1
       _________vmax
      |         \
      |          \________vmin
      |
______|
T     0        1 2
green
*/
uint16_t flash(long t1, long t2, long dt, uint16_t vmax, uint16_t vmin)
{
   if (dt < t1)
      return vmax;
   else if (dt < t2)
      return (vmax * (t2-dt) + vmin*(dt-t1)) / (t2-t1);
   return vmin;
}


void Effect::e11()
{
   const uint8_t d = get_delay(m3_set21, sizeof(m3_set21));
   if (d==0xff)
      return;
   const uint16_t delay = d*25;
   if (dt < delay)
      return;
   dt -= delay;

   if (id==11)
      set_rgb(0,flash(650, 1500, dt, vmax, vmax>>3),0);
   else if (id==12)
      set_rgb(0,0,flash(650, 1500, dt, vmax, vmax>>3));
   else if (id==13)
      set_rgb(0,flash(650, 3000, dt, vmax, 0),0);
}

void Effect::e12()
{
   e11();
}

void Effect::e13()
{
   e11();
}

void Effect::e14()
{
   const uint8_t d = get_delay(m3_set30_AB, sizeof(m3_set30_AB));
   if (d==0)
   {
      set_rgb(vmax, 0, vmax);
   }
   else
   {
      set_rgb(0, vmax, 0);
   }
}

void Effect::e15()
{
   const uint8_t d = get_delay(m3_set30_AB, sizeof(m3_set30_AB));
   if (d==0)
   {
      set_rgb(0, 0, vmax);
   }
   else
   {
      set_rgb(vmax, 0, 0);
   }
}

void Effect::e16()
{
   const uint8_t d = get_delay(m3_set30_AB, sizeof(m3_set30_AB));
   if (d==0)
   {
      set_rgb(0, vmax, 0);
   }
   else
   {
      set_rgb(vmax, 0, vmax);
   }
}

void Effect::e17()
{
   const uint8_t d = get_delay(m3_set30_AB, sizeof(m3_set30_AB));
   if (d==0)
   {
      set_rgb(vmax, 0, 0);
   }
   else
   {
      set_rgb(0, 0, vmax);
   }
}
void Effect::e18()
{
   const uint8_t d = get_delay(m3_set30_AB, sizeof(m3_set30_AB));
   if (d==0)
   {
      set_rgb(0, 0, vmax);
   }
   else
   {
      set_rgb(0, 0, vmax);
   }
}

void Effect::e19()
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

   set_red(r);

   unsigned b=0;
   long cldt = dt<=0 ? 0 : dt % cl;

   if (t0 < cldt && cldt < t0+dwell )
   {
      if (cldt < rise_time)
         b = (vmax * cldt) / rise_time;
      else
         b = vmax;
   }
   set_blue(b);
}

void Effect::e20()
{
   set_rgb(0,vmax,0);
}
