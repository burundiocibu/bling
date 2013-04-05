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

void print_time(uint32_t t)
{
   char b1[11],b2[7];
   sprintf(b1, "%10ld", t);
   b2[0] = b1[4];
   b2[1] = b1[5]; 
   b2[2] = b1[6];
   b2[3] = '.';
   b2[4] = b1[7];
   b2[5] = b1[8];
   b2[6] = 0;
   printf(b2);
}


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
   nRF24L01::write_reg(nRF24L01::RX_PW_P0, messages::message_size);
   uint8_t buff[messages::message_size];

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
         if (throb >= 1024 || throb <= 0)
            dir = -dir;
         throb += dir;
         avr_tlc5940::set_channel(15, throb);
         avr_tlc5940::output_gsdata();
      }

      // Do we have data from the radio?
      // this test is less good: nRF24L01::read_reg(nRF24L01::STATUS) & nRF24L01::STATUS_RX_DR
      if (nRF24L01::rx_flag)
      {
         // We got some data!!
         nRF24L01::read_rx_payload(buff, sizeof(buff));
         nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_RX_DR); // clear data received bit
         nRF24L01::rx_flag=0;

         switch (messages::get_id(buff))
         {
            case messages::heartbeat_id:
            {
               messages::Heartbeat msg(buff);
               lcd_plate::set_cursor(0,0);
               print_time(msg.t_ms);
               break;
            }
            case messages::set_tlc_ch_id:
            {
               messages::Set_tlc_ch msg(buff);
               lcd_plate::set_cursor(1,0);
               print_time(avr_rtc::t_ms);
               printf(":%02X %02X", msg.ch, msg.value);
               avr_tlc5940::set_channel(msg.ch, msg.value);
               avr_tlc5940::output_gsdata();
               break;
            }
         }

         avr_led::toggle();
      }
      sleep_mode();
   }
}
