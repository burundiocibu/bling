#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "avr_rtc.hpp"

namespace avr_rtc
{
   uint32_t t_ms=0;

   ISR(TIMER2_COMPA_vect)
   {
      t_ms++;
   }
   
   // Sets up timer 2 to generate an interrupt every ms
   void setup(void)
   {
      t_ms = 0;

      TCCR2A = _BV(WGM21); // CTC mode, resets on OCR2A match
      TCCR2B = _BV(CS22) ; // prescaler = 64, f=8e6/64=125kHz
      TIMSK2 |= _BV(OCIE2A); // Interrupt on output compare on A  
      OCR2A = 125;

      sei();
   }

   void step(long dt_ms)
   {
      cli();
      t_ms += dt_ms;
      sei();
   }

   void set(uint32_t t)
   {
      cli();
      t_ms = t;
      sei();
   }
}
