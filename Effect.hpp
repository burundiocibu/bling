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
   uint8_t noise[32];

   void init(uint8_t* buff);
   void start(uint8_t id, uint32_t duration);
   void execute(void);
   void all_stop(uint8_t* buff);
   void reset();
   uint8_t get_delay(const uint8_t lut[], size_t len);

   enum State
   {
      stopped, pending, running
   } state;
   

   // slave/effect specific delay, indexed by effect number
   static const unsigned max_effect = 21;

   void flash_id(); // Blinks slave id 
   void pulse(); // Fast on white 50%, fades for duration

   void sel_on(); // Fast on white 50%, stays on for duration
   void all_on(); // Fast on white 50%, stays on for duration
   void all_white_sparkle(); // flash randomly (each slave differently)
};

#endif
