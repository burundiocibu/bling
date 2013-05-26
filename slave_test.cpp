#include <stdlib.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"
#include "messages.hpp"
#include "nrf24l01.hpp"
#include "avr_dbg.hpp"

int main (void)
{
   avr_tlc5940::setup();
   avr_dbg::blink(1, 10);

   // turn off 12v supply
   //DDRB |= _BV(PB5);
   //PORTB &= ~_BV(PB5);


   avr_rtc::setup();
   _delay_ms(2000);
   int dt = avr_rtc::t_ms - 2000;
   const int err=40; //40 ms works on the sparkfun breakout board
   if (dt > err)
      avr_dbg::die(2, 30);
   else if (-dt > err)
      avr_dbg::die(3, 30);
      

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
      avr_dbg::die(4, 20);

   for (int i=0; i<15; i++)
      avr_tlc5940::set_channel(i, 1);
   avr_tlc5940::output_gsdata();
   while(true)
   {
      avr_dbg::throbber(avr_rtc::t_ms);
      sleep_mode();
   }
}
