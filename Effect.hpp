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

   void e0();
   void e1();
};

#endif
