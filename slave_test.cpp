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
      _delay_ms(100);
      avr_tlc5940::set_channel(15, 0);
      avr_tlc5940::output_gsdata();
      _delay_ms(200);
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

   avr_tlc5940::set_channel(15, 1024);
   avr_tlc5940::output_gsdata();
   while(true)
      sleep_mode();

   // turn off 12v supply
   //DDRB |= _BV(PB5);
   //PORTB &= ~_BV(PB5);

   blink(1, 100);

   avr_rtc::setup();
   while(avr_rtc::t_ms < 2000)
      sleep_mode();

   blink(2, 20);
   _delay_ms(2000);

   {
      using namespace nRF24L01;
      setup();

      if (read_reg(CONFIG) != 0x08)
         die();
      blink(3, 20);

      if (read_reg(EN_AA) != 0x3f)
         die();
      blink(3, 20);

      if (read_reg(EN_RXADDR) != 0x03) 
         die();
      blink(3, 20);

      if (read_reg(STATUS) != 0x0e)
         die();
   }

   uint8_t buff[messages::message_size];
   while(true)
   {
      uint8_t pipe;
      uint8_t status=nRF24L01::read_reg(nRF24L01::STATUS);
      if (status == 0x0e)
         break;
      nRF24L01::read_rx_payload(buff, sizeof(buff), pipe);
      nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_RX_DR); // clear data received bit
      blink(4, 10);
   }

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
