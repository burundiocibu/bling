#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "avr_mike.hpp"

namespace avr_mike
{
   int sum=0;
   int avg=0;
   int min=256;
   int max=0;
   uint8_t v;

   void setup(void)
   {
      // Ref voltage is AVcc
      // Input channel is single-ended 0 (PF0)
      ADMUX |= _BV(REFS0);

      // put 8 MSB of ADC result into ADCH
      ADMUX |= _BV(ADLAR);

      // disable the digital input buffer on PF0
      DIDR0 |= _BV(ADC0D);
      DDRF |= _BV(PF0);

      // use a prescaler of 128 for the clock
      // 16e6/128 = 125 kHz conversion clock
      // 125 kHz/13 = 9 kHz conversion rate
      ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

      // defaults to free run mode so don't need to write ADCSRB

      // enable the ADC, start it up, and enable the IRQ
      ADCSRA |= _BV(ADEN) | _BV(ADSC) | _BV(ADIE);
   }

   ISR(ADC_vect)
   {
      v = ADCW;
      if (v>max)
         max=v;
      if (v<min)
         min=v;
      
      int diff = v - avg;
      avg += diff / 10;
   }


}
