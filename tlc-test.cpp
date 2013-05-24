#include <string.h>
#include <stdio.h>

#include <util/delay.h> // F_CPU should come from the makefile...
#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "avr_tlc5940.hpp"
#include "avr_led.hpp"

int main (void)
{
   avr_led::setup();
   avr_tlc5940::setup();

   DDRB |= _BV(PB5);
   PORTB &= ~_BV(PB5);
   PORTB |= _BV(PB5);

   for (int ch=0; ch<24; ch++)
      avr_tlc5940::set_channel(ch, 1);

   avr_tlc5940::output_gsdata();

   avr_led::on();

   int dir=1;
   int throb = 1;
   for (;;)
   {
      // Yep, upper two bits seem to be worthless
      if (throb == 12 || throb == 0)
         dir = -dir;
      throb += dir;
      int v = (1<<throb) - 1;
      avr_tlc5940::set_channel(0, v);
      avr_tlc5940::set_channel(15, v);

      avr_tlc5940::output_gsdata();
      _delay_ms(1000);
      sleep_mode();
   }

}
