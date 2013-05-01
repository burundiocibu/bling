#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
   Effect() :
      state(complete) {};
   uint8_t id;
   uint32_t start_time;
   uint16_t duration;
   enum State
   {
      complete, unstarted, started
   } state;
   
   void execute(void);
};


int main (void)
{
   avr_led::setup();
   avr_rtc::setup();
   lcd_plate::setup(0x40);

   nRF24L01::setup();
   nRF24L01::configure_base();
   nRF24L01::configure_PRX(SLAVE_NUMBER);
   nRF24L01::power_up_PRX();
   uint8_t buff[messages::message_size];

   avr_tlc5940::setup();
   avr_tlc5940::set_channel(15, 0);
   avr_tlc5940::output_gsdata();

   avr_mike::setup();

   // Things to wake us up:
   // nRF IRQ,              random
   // TLC5940 BLANK needed  1.024 kHz
   // rtc:      1 kHz

   uint32_t t_hb=0;
   Effect effect;
   int late_count=0;
   for (;;)
   {
      uint32_t ms = avr_rtc::t_ms;
      unsigned sec = ms/1000;
      ms -= sec*1000;

      if ((ms & 0x10) == 0)
      {
         // Stop throbbing if we loose the heartbeat
         if (labs( avr_rtc::t_ms - t_hb) > 4000)
         {
            avr_tlc5940::set_channel(15, 1);
            lcd_plate::set_cursor(0,0);
            printf("????");
         }
         else if (t_hb)
         {
            if (sec & 1)
               avr_tlc5940::set_channel(15, ms);
            else
               avr_tlc5940::set_channel(15, 1000-ms);
         }
         avr_tlc5940::output_gsdata();
      }

      // Handle any data from the radio
      while(true)
      {
         uint8_t pipe;
         uint8_t status=nRF24L01::read_reg(nRF24L01::STATUS);
         if (status == 0x0e)
            break;
         nRF24L01::read_rx_payload(buff, sizeof(buff), pipe);
         nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_RX_DR); // clear data received bit

         lcd_plate::set_cursor(0, 8);
         printf("%02x %d", status, pipe);
         switch (messages::get_id(buff))
         {
            case messages::heartbeat_id:
            {
               messages::decode_heartbeat(buff, t_hb);
               long dt = t_hb - nRF24L01::t_rx;
               if (labs(dt) < 2000)
               {
                  lcd_plate::set_cursor(0,0);
                  printf("%4ld %d", dt, late_count);
               }
               else
                  late_count++;
               if (labs(dt)>10000)
                  avr_rtc::set(t_hb);
               else if (labs(dt)>3)
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
   if (state==complete)
      return;

   int dt = avr_rtc::t_ms - start_time;
   int v = 1024 - dt*2;
   if (v<0)
      v=0;

   if (dt>0 && dt<int(duration) && state==unstarted)
   {
      state=started;
   }
   else if (dt>int(duration) && state==started)
   {
      state=complete;
      v=0;
   }

   for (unsigned ch=0; ch<9; ch++)
      avr_tlc5940::set_channel(ch, v);
   avr_tlc5940::output_gsdata();
}
