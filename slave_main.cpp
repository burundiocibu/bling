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
   char b1[11],b2[8];
   sprintf(b1, "%10ld", t);
   b2[0] = b1[4];
   b2[1] = b1[5]; 
   b2[2] = b1[6];
   b2[3] = '.';
   b2[4] = b1[7];
   b2[5] = b1[8];
   b2[6] = b1[9];
   b2[7] = 0;
   printf(b2);
}

struct Effect
{
   uint8_t id;
   uint32_t start_time;
   uint16_t duration;
   enum State
   {
      unstarted, started, complete
   } state;
   
   void execute(void);
};


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

   uint32_t t_hb=0;
   Effect effect;
   for (;;)
   {
      uint32_t ms = avr_rtc::t_ms;
      unsigned sec = ms/1000;
      ms -= sec*1000;

      if ((ms & 0x10) == 0)
      {
         // Stop throbbing if we loose the heartbeat
//         if ((avr_rtc::t_ms - t_hb) > 5000)
//            avr_tlc5940::set_channel(15, 0);            
         if (sec & 1)
            avr_tlc5940::set_channel(15, ms);
         else
            avr_tlc5940::set_channel(15, 1000-ms);
         avr_tlc5940::output_gsdata();
      }

      // Do we have data from the radio?
      // this test is less good: nRF24L01::read_reg(nRF24L01::STATUS) & nRF24L01::STATUS_RX_DR
      if (nRF24L01::rx_flag)
      {
         nRF24L01::read_rx_payload(buff, sizeof(buff));
         nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_RX_DR); // clear data received bit
         nRF24L01::rx_flag=0;

         switch (messages::get_id(buff))
         {
            case messages::heartbeat_id:
            {
               messages::decode_heartbeat(buff, t_hb);
               lcd_plate::set_cursor(0,0);
               int dt = t_hb - nRF24L01::t_rx;
               print_time(t_hb);
               printf(" %5d", dt);
               if (dt)
                  avr_rtc::step(dt);
               break;
            }

            case messages::all_stop_id:
            {
               lcd_plate::set_cursor(1,0);
               print_time(nRF24L01::t_rx);
               printf(":all stop");
               for (int ch=0; ch<14; ch++)
                  avr_tlc5940::set_channel(ch, 0);
               avr_tlc5940::output_gsdata();
               break;
            }

            case messages::start_effect_id:
            {
               effect.state = Effect::unstarted;
               messages::decode_start_effect(buff, effect.id, effect.start_time, effect.duration);
               lcd_plate::set_cursor(1,0);
               print_time(nRF24L01::t_rx);
               printf(":%02X %d %u",
                      effect.id,
                      static_cast<int>(effect.start_time - avr_rtc::t_ms),
                      effect.duration);
               break;
            }

            case messages::set_tlc_ch_id:
            {
               uint8_t ch;
               uint16_t value;
               messages::decode_set_tlc_ch(buff, ch, value);
               lcd_plate::set_cursor(1,0);
               print_time(nRF24L01::t_rx);
               printf(":%02X %03X  ", ch, value); 
               avr_tlc5940::set_channel(ch, value);
               avr_tlc5940::output_gsdata();
               break;
            }
         }
      }

      effect.execute();
      
      sleep_mode();
   }
}

void Effect::execute()
{
   int dt = avr_rtc::t_ms - start_time;
   if (dt>0 && dt<duration && state==unstarted)
   {
      state=started;
      avr_tlc5940::set_channel(0, 512);
      avr_tlc5940::set_channel(1, 512);
      avr_tlc5940::set_channel(2, 512);
      avr_tlc5940::output_gsdata();
   }
   else if (dt>duration && state==started)
   {
      state=complete;
      avr_tlc5940::set_channel(0, 0);
      avr_tlc5940::set_channel(1, 0);
      avr_tlc5940::set_channel(2, 0);
      avr_tlc5940::output_gsdata();
   }
}
