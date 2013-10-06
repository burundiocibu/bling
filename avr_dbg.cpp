#include <inttypes.h>
#include <stdlib.h>
#include <util/delay.h> // F_CPU should come from the makefile...

#include "avr_dbg.hpp"
#include "avr_rtc.hpp"
#include "avr_tlc5940.hpp"

namespace avr_dbg
{
   // NUMBER_OF_TLC5940 comes from the makefile
   const unsigned chan = (NUMBER_OF_TLC5940-1)*16 + 15;

   void throbber(uint32_t t_hb)
   {
      uint32_t ms = avr_rtc::t_ms;
      unsigned sec = ms/1000;
      ms -= sec*1000;

      if ((ms & 0x10) == 0)
      {
         // Stop throbbing if we loose the heartbeat
         if (labs( avr_rtc::t_ms - t_hb) > 4000)
            avr_tlc5940::set_channel(chan, 1);
         else if (t_hb)
         {
            unsigned v = ms;
            if (sec & 1)
               v = 1000 - ms;
            ms <<=2;
            avr_tlc5940::set_channel(chan, v);
         }
      }
   }

   void blink(int n, unsigned v)
   {
      for (int i=0; i<n; i++)
      {
         avr_tlc5940::set_channel(chan, v);
         avr_tlc5940::output_gsdata();
         _delay_ms(111);
         avr_tlc5940::set_channel(chan, 0);
         avr_tlc5940::output_gsdata();
         _delay_ms(222);
      }
   }   

   void die(int n, unsigned v)
   {
      while(true)
      {
         blink(n, 512);
         _delay_ms(1000);
      }
   }
}
