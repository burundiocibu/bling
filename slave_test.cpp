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
#include "i2c.hpp"
#include "avr_max1704x.hpp"
#include "lcd_plate.hpp"

void hex_dump(const void* buff, size_t len)
{
   const uint8_t *p = static_cast<const uint8_t*>(buff);
   for (unsigned i=0; i<len; i++)
      printf("%02x", *p++);
}

int main (void)
{
   // 12V supply control
   DDRB |= _BV(PB1);
   PORTB &= ~_BV(PB1);
   //PORTB |= _BV(PB1);

   avr_tlc5940::setup();
   avr_dbg::blink(1, 1000);

   avr_rtc::setup();
   _delay_ms(2000);
   int dt = avr_rtc::t_ms - 2000;
   const int err=50; //40 ms works on the sparkfun breakout board
   if (dt > err)
      avr_dbg::die(2, 100);
   else if (-dt > err)
      avr_dbg::die(3, 100);

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
      avr_dbg::die(4, 100);

   for (int i=0; i<15; i++)
      avr_tlc5940::set_channel(i, 4095);
   avr_tlc5940::output_gsdata();

   _delay_ms(1000);
   avr_dbg::blink(2, 1000);

   lcd_plate::setup(0x40);
   lcd_plate::set_cursor(0, 0);
   printf("slave_test");

   lcd_plate::set_cursor(1,0);
   printf("%04d mv", avr_max1704x::read_vcell());
   lcd_plate::set_cursor(1,9);
   printf("%03d ", avr_max1704x::read_soc());


   while(true)
   {
      avr_dbg::throbber(avr_rtc::t_ms);
      avr_tlc5940::output_gsdata();
      sleep_mode();
   }

   int dir=1;
   for (uint32_t v=0; ; v+=dir)
   {
      if (v == 0)
         dir=1;
      if (v == 1024 * 16)
         dir=-1;
      for (int i=0; i<=15; i++)
         avr_tlc5940::set_channel(i, v>>5);
      avr_tlc5940::output_gsdata();
   }

}
