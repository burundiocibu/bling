#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "avr_rtc.hpp"

namespace avr_rtc
{
   uint32_t t_ms=0;

   ISR(TIMER3_COMPA_vect)
   {
      t_ms++;
   }
   
   // Sets up timer 3 to generate an interrupt every ms
   void setup(void)
   {
      t_ms = 0;

      // Wanted to use CTC mode but could not get that to work
      TCCR3A = _BV(WGM30); // Set timer to CTC mode
      // 16 MHz / 64 = 250 kHz
      TCCR3B = _BV(CS31) | _BV(CS30) | _BV(WGM33); // prescaler = 64
      TIMSK3 |= _BV(OCIE3A); // Interrupt on output compare on A  
      OCR3A = 125;

      sei();
   }

   void set(uint32_t t_new)
   {
      cli();
      t_ms = t_new;
      sei();
   }


   void step(int dt_ms)
   {
      cli();
      t_ms += dt_ms;
      sei();
   }
}
