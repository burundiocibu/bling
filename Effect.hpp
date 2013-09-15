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
   uint16_t duration;
   int dt;
   int prev_dt;

   // slave/effect specific delay, indexed by effect number
   static const unsigned max_effect = 10;
   uint16_t se_delay[max_effect];

   void init(uint8_t* buff);
   void execute(void);
   void all_stop(uint8_t* buff);
   void reset();
   uint16_t get_delay(const uint8_t lut[], size_t len);

   enum State
   {
      stopped, pending, running
   } state;
   

   void e0(); // Fast on white, fades out over 4 seconds
   void e1(); // a simple color cycle...
   void e2(); // The test pattern 
   void e3(); // Set4, Count 56 effect
   void e4(); // Set4, count 66 effect
   void e5(); 
};

#endif
