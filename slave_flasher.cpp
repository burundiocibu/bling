#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <util/delay.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"
#include "avr_led.hpp"


int main (void)
{
   avr_led::setup();
   avr_rtc::setup();
   avr_tlc5940::setup();

   for (;;)
   {
      uint32_t ms = avr_rtc::t_ms;
      uint32_t sec = ms/1000;
      ms -= sec*1000;

      if ((ms & 0x10) == 0)
      {
         if (sec & 1)
            avr_tlc5940::set_channel(15, ms);
         else
            avr_tlc5940::set_channel(15, 1000-ms);

         unsigned val;
         if (ms<512)
            val = 1023 - 2*ms;
         else
            val = 0;
         switch(sec % 4)
         {
            case 0: for (unsigned ch=0; ch<14; ch++)  avr_tlc5940::set_channel(ch, val); break;
            case 1: for (unsigned ch=0; ch<14; ch+=3) avr_tlc5940::set_channel(ch, val); break;
            case 2: for (unsigned ch=1; ch<14; ch+=3) avr_tlc5940::set_channel(ch, val); break;
            case 3: for (unsigned ch=2; ch<14; ch+=3) avr_tlc5940::set_channel(ch, val); break;
         }

         avr_tlc5940::output_gsdata();
      }
      
      sleep_mode();
   }
}
