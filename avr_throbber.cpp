#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "avr_throbber.hpp"

namespace avr_throbber
{
   enum { UP, DOWN };

   ISR (TIMER3_OVF_vect)
   {
      static uint16_t pwm;
      static uint8_t direction;

      switch (direction)
      {
         case UP:
            if (++pwm == 0x3ff)
               direction = DOWN;
            break;

         case DOWN:
            if (--pwm == 0)
               direction = UP;
            break;
      }

      OCR3A = pwm;
   }


   void setup(void)
   {
      // timer3
      TCCR3A |= _BV(WGM30) | _BV(WGM31); // phase correct 10 bit PWM (mode3)
      TCCR3A |= _BV(COM3A1); // Clear OC3A on compare match, set OC3A at TOP
      TCCR3B |= _BV(CS31);   // prescaler div = 8, 2 MHz
      OCR3A = 0; // Initialize PWM value

      // Make PC6 (OC3A) an output
      DDRC |= _BV(PC6);

      // Enable timer 0 overflow interrupt. should occur every 512 us
      TIMSK3 = _BV(TOIE3);
      sei();
   }
}
