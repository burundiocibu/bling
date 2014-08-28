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

   void e0(); // Fast on white, fades out over duration
   void e1(); // The LED test pattern
   void e2();
   void e3();
   void e4();
   void e5();
   void e6();
   void e7();
   void e8();
   void e9();
   void e10();
   void e11();
   void e12();
   void e13();
   void e14();
   void e15();
   void e16();
   void e17();
   void e18();
   void e19();
   void e20();
};

#endif
