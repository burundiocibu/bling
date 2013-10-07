#ifndef _EFFECT_HPP
#define _EFFECT_HPP

#include <stdint.h>
#include <string.h>

struct Effect
{
   Effect(uint16_t slave);

   uint16_t slave_id;
   uint8_t id;
   uint32_t start_time;
   uint32_t duration;
   long dt;
   long prev_dt;
   int my_rand;

   // slave/effect specific delay, indexed by effect number
   static const unsigned max_effect = 10;
   uint16_t se_delay;

   void init(uint8_t* buff);
   void execute(void);
   void all_stop(uint8_t* buff);
   void reset();
   uint16_t get_delay(const uint8_t lut[], size_t len);

   enum State
   {
      stopped, pending, running
   } state;
   

   void e0(); // Fast on white, fades out over duration
   void e1(); // The LED test pattern
   void e2(); // Fade up to full white over duration
   void e3(); // Fade all up to full red over duration 
   void e4(); // Fade all up to 20% RED
   void e5(); // Sparkle red/purple brass & woodwinds for duration
   void e6(); // Sparkle red/purple brass & ww fade out 
   void e7(); // Fade 20% red to 0% from R to L in two groups
   void e8(); // white wave back to front set13
};

#endif
