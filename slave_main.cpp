#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <util/delay.h> // F_CPU should come from the makefile...
#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#include "avr_tlc5940.hpp"
#include "avr_rtc.hpp"
#include "avr_dbg.hpp"
#include "nrf24l01.hpp"
#include "avr_max1704x.hpp"
#include "messages.hpp"
#include "slave_eeprom.h"


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

void do_all_stop(void);
void do_heartbeat(uint8_t* buff, uint32_t& t_hb);
void do_set_tlc_ch(uint8_t* buff);
void do_start_effect(uint8_t* buff, Effect& effect);
void do_set_rgb(uint8_t* buff);
void do_ping(uint8_t* buff, uint8_t pipe);

uint16_t slave_id;

int main (void)
{
   avr_tlc5940::setup();
   avr_rtc::setup();
   avr_max1704x::setup();

   slave_id = eeprom_read_word(EE_SLAVE_ID);
   nRF24L01::channel  = eeprom_read_byte((const uint8_t*)EE_CHANNEL);
   eeprom_read_block((void*)nRF24L01::master_addr,    (const void*)EE_MASTER_ADDR,    nRF24L01::addr_len);
   eeprom_read_block((void*)nRF24L01::broadcast_addr, (const void*)EE_BROADCAST_ADDR, nRF24L01::addr_len);
   eeprom_read_block((void*)nRF24L01::slave_addr,     (const void*)EE_SLAVE_ADDR,     nRF24L01::addr_len);

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
      avr_dbg::die(1, 1000);
   nRF24L01::configure_PRX();
   uint8_t buff[messages::message_size];

   // Turn on 12V supply
   DDRB |= _BV(PB1);
   PORTB |= _BV(PB1);

   // Things to wake us up:
   // nRF IRQ,              random
   // TLC5940 BLANK needed  1.024 kHz
   // rtc:      1 kHz

   uint32_t t_hb=0;
   Effect effect;
   typedef void APP(void);  // used to jump to the bootloader

   for (;;)
   {
      avr_dbg::throbber(t_hb);
      
      // Handle any data from the radio
      for(int cnt=0; ; cnt++)
      {
         if (cnt>10)
            avr_dbg::blink(3, 4095);
         uint8_t pipe;
         uint8_t status=nRF24L01::read_reg(nRF24L01::STATUS);
         if (status == 0x0e)
            break;
         nRF24L01::read_rx_payload(buff, sizeof(buff), pipe);
         nRF24L01::clear_IRQ();
         
         switch (messages::get_id(buff))
         {
            case messages::heartbeat_id:    do_heartbeat(buff, t_hb); break;
            case messages::all_stop_id:     do_all_stop(); break;
            case messages::start_effect_id: do_start_effect(buff, effect); break;
            case messages::set_tlc_ch_id:   do_set_tlc_ch(buff); break;
            case messages::set_rgb_id:      do_set_rgb(buff); break;
            case messages::ping_id:         do_ping(buff, pipe); break;
            case messages::bootloader_id:   ((APP*)BOOTADDR)(); break;
         }
      }

      effect.execute();
      avr_tlc5940::output_gsdata();
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
}


void do_all_stop(void)
{
   for (int ch=0; ch<14; ch++)
      avr_tlc5940::set_channel(ch, 0);
}


void do_heartbeat(uint8_t* buff, uint32_t& t_hb)
{
   messages::decode_heartbeat(buff, t_hb);
   long dt = t_hb - nRF24L01::t_rx;
   if (labs(dt)>10000)
      avr_rtc::set(t_hb);
   else if (labs(dt)>3)
      avr_rtc::step(dt);
}

void do_set_tlc_ch(uint8_t* buff)
{
   uint8_t ch;
   uint16_t value;
   messages::decode_set_tlc_ch(buff, ch, value);
   avr_tlc5940::set_channel(ch, value);
}


void do_start_effect(uint8_t* buff, Effect& effect)
{
   effect.state = Effect::unstarted;
   messages::decode_start_effect(buff, effect.id, effect.start_time, effect.duration);
}


void do_set_rgb(uint8_t* buff)
{}


void do_ping(uint8_t* buff, uint8_t pipe)
{
   using namespace nRF24L01;

   // Only respond to pings on our private address
   if (pipe==0)
      return;

   clear_CE();  // Turn off receiver
   char config = read_reg(CONFIG);
   write_reg(CONFIG, config & ~CONFIG_PWR_UP); // power down
   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // become PTX
   flush_tx();
   write_reg(CONFIG, config | CONFIG_PWR_UP); // power back up

   // delay_us(150);
   // above delay not required because the I2C reads will take much longer than that.
   uint8_t* p = (uint8_t*)(&iobuff[0]);
   *p++ = W_TX_PAYLOAD;
   *p++ = messages::status_id;
   p = messages::encode_var<uint32_t>(p, t_rx);
   uint16_t vcell = avr_max1704x::read_vcell();
   p = messages::encode_var<uint16_t>(p, vcell);
   uint16_t soc = avr_max1704x::read_soc();
   p = messages::encode_var<uint16_t>(p, soc);
   p = messages::encode_var<uint16_t>(p, slave_id);
   write_data(iobuff, messages::message_size+1);
   
   set_CE();
   delay_us(10);
   clear_CE();


   // wait for tx to complete
   uint8_t status;
   for(int j=0; j<100; j++)
   {
      status = read_reg(STATUS);
      if (status & STATUS_TX_DS)
         break;
      delay_us(5);
   }

   if (status & STATUS_MAX_RT)
   {
      write_reg(nRF24L01::STATUS, nRF24L01::STATUS_MAX_RT);
      flush_tx();
   }
   else if (status & STATUS_TX_DS)
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice

   // and switch back to be a PRX
   write_reg(CONFIG, config & ~CONFIG_PWR_UP); // power down
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config | CONFIG_PWR_UP); // power back up
   delay_us(150);
   set_CE();
}

