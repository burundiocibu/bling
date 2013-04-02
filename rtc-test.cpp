#include <string.h>
#include <stdio.h>

#include <util/delay.h> // F_CPU should come from the makefile...
#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "avr_rtc.hpp"
#include "avr_led.hpp"
#include "lcd_plate.hpp"

int main (void)
{
   avr_led::setup();
   avr_rtc::setup();
   lcd_plate::setup(0x40);
   printf("t_ms:");

   for (;;)
   {
      if (avr_rtc::t_ms%250 == 0)
      {
         lcd_plate::set_cursor(0,6);
         printf("%6ld", avr_rtc::t_ms);

      }
      sleep_mode();
   }
}
