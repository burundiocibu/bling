#ifndef _AVR_RTC_HPP
#define _AVR_RTC_HPP

namespace avr_rtc
{
   // number of ms since start
   extern uint32_t t_ms;

   void step(int dt_ms);

   void setup(void);
}
#endif
