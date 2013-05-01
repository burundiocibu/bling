#ifndef _AVR_MIKE_HPP
#define _AVR_MIKE_HPP

namespace avr_mike
{
   // This is just a volume
   extern int avg, sum, min, max;
   extern uint8_t v;
   
   void setup(void);
}
#endif
