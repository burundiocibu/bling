#include <util/delay.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"
#include "messages.hpp"
#include "nrf24l01.hpp"

void blink(int n, unsigned v=128)
{
   for (int i=0; i<n; i++)
   {
      avr_tlc5940::set_channel(15, v);
      avr_tlc5940::output_gsdata();
      _delay_ms(111);
      avr_tlc5940::set_channel(15, 0);
      avr_tlc5940::output_gsdata();
      _delay_ms(222);
   }
}   

void die()
{
   avr_tlc5940::set_channel(15, 1);
   avr_tlc5940::output_gsdata();
   while(true)
      sleep_mode();
}

int main (void)
{
   avr_tlc5940::setup();

   // turn off 12v supply
   //DDRB |= _BV(PB5);
   //PORTB &= ~_BV(PB5);

   blink(1, 20);

   avr_rtc::setup();
   while(avr_rtc::t_ms < 2000)
      sleep_mode();

   blink(2, 20);
   _delay_ms(1000);

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
      die();
   blink(3, 20);
   _delay_ms(1000);

   nRF24L01::configure_PRX(SLAVE_NUMBER);

   uint8_t status=nRF24L01::read_reg(nRF24L01::STATUS);
   if (status != 0x0e)
      blink(~status);

   int dir = 1;
   for (unsigned i=32;;i+=dir)
   {
      avr_tlc5940::set_channel(15, i>>5);
      avr_tlc5940::output_gsdata();
      sleep_mode();
      if (i==32) dir = 1;
      if (i==2048) dir = -1;
   }
}
