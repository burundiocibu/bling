#include <string.h>
#include <stdio.h>

#include <util/delay.h> // F_CPU should come from the makefile...
#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"
#include "avr_mike.hpp"
#include "lcd_plate.hpp"


int main(void)
{
   avr_rtc::setup();
   lcd_plate::setup(0x40);
   avr_tlc5940::setup();
   avr_mike::setup();

   for (;;)
   {
      uint32_t ms = avr_rtc::t_ms;
      unsigned sec = ms/1000;
      ms -= sec*1000;

      if ((ms & 0x10) == 0)
      {
         // Stop throbbing if we loose the heartbeat
         if (sec & 1)
            avr_tlc5940::set_channel(15, ms+20);
         else
            avr_tlc5940::set_channel(15, 1020-ms);
         avr_tlc5940::output_gsdata();
      }
      if ((ms % 500) == 0)
      {
         lcd_plate::set_cursor(0,0);
         printf("%d  %d  %u  %d", avr_mike::min, avr_mike::max, avr_mike::v, avr_mike::avg);
      }

      sleep_mode();
   }
}
