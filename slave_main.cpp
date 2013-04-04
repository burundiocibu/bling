#include <string.h>
#include <stdio.h>

#include <util/delay.h> // F_CPU should come from the makefile...
#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"
#include "avr_led.hpp"
#include "avr_mike.hpp"
#include "lcd_plate.hpp"

#include "nrf24l01.hpp"
#include "messages.hpp"


int main (void)
{
   avr_led::setup();
   avr_rtc::setup();
   lcd_plate::setup(0x40);
   printf("Bling!");
   
   nRF24L01::setup();
   nRF24L01::configure_base();
   nRF24L01::configure_PRX();
   nRF24L01::power_up_PRX();
   nRF24L01::write_reg(nRF24L01::RX_PW_P0, sizeof(messages::Heartbeat));

   avr_tlc5940::setup();
   avr_mike::setup();

   // Things to wake us up:
   // nRF IRQ,              random
   // TLC5940 BLANK needed  1.024 kHz
   // rtc:      1 kHz

   int dir=4;
   int throb = dir;
   for (;;)
   {
      
      if ((avr_rtc::t_ms & 0xf) == 0)
      {
         lcd_plate::set_cursor(0,11);
         printf("%3d", avr_mike::avg);
         if (avr_mike::avg>100)
            avr_tlc5940::set_channel(1, 1024);
         else
            avr_tlc5940::set_channel(1, 20);
            
         if (throb >= 1024 || throb <= 0)
            dir = -dir;
         throb += dir;
         avr_tlc5940::set_channel(15, throb);
         avr_tlc5940::output_gsdata();
      }

      if ((avr_rtc::t_ms & 0x7f) == 0)
      {
         lcd_plate::set_cursor(0,0);
         printf("%8ld", avr_rtc::t_ms);

      }
   
      // Do we have data from the radio?
      // this test is less good: nRF24L01::read_reg(nRF24L01::STATUS) & nRF24L01::STATUS_RX_DR
      if (nRF24L01::rx_flag)
      {
         // We got some data!!
         uint8_t buff[sizeof(messages::Heartbeat)];
         nRF24L01::read_rx_payload(buff, sizeof(buff));
         nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_RX_DR); // clear data received bit
         nRF24L01::rx_flag=0;

         switch (messages::get_id(buff))
         {
            case messages::heartbeat_id:
            {
               messages::Heartbeat heartbeat(buff);
               lcd_plate::set_cursor(1,0);
               printf("%8ld", heartbeat.t_ms);
            }
         }

         avr_led::toggle();
      }
      sleep_mode();
   }
}
