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

   void init(uint8_t* buff);

   enum State
   {
      complete, unstarted, started
   } state;
   
   void execute(void);
   void all_stop(void);

   void e0(); // Fast on white, fades out over 4 seconds
   void e1(); // a simple color cycle...
   void e2(); // The test pattern 
   void e3(); // Set4, Count 56 effect
   void e4(); // Set4, count 66 effect
   void e5(); 
};

#endif
