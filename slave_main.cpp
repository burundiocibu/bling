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
#include "nrf_boot.h"
#include "ensemble.hpp"

struct Effect
{
   Effect() :
      state(complete) {};
   uint8_t id;
   uint32_t start_time;
   uint16_t duration;
   int dt;
   int prev_dt;
   int cldt;

   enum State
   {
      complete, unstarted, started
   } state;
   
   void execute(void);

   void e0();
   void e1();
};

void do_all_stop(Effect& effect);
void do_heartbeat(uint8_t* buff, uint32_t& t_hb);
void do_set_tlc_ch(uint8_t* buff);
void do_start_effect(uint8_t* buff, Effect& effect);
void do_set_rgb(uint8_t* buff);
void do_ping(uint8_t* buff, uint8_t pipe);

uint16_t slave_id;
uint16_t bad_id_count;
uint8_t last_bad_id;
uint16_t good_id_count;

int main (void)
{
   avr_tlc5940::setup();
   avr_rtc::setup();
   avr_max1704x::setup();

   last_bad_id = 0xff;
   bad_id_count = 0;
   good_id_count = 0;

   slave_id = eeprom_read_word(EE_SLAVE_ID);
   nRF24L01::channel  = eeprom_read_byte((const uint8_t*)EE_CHANNEL);
   eeprom_read_block((void*)nRF24L01::master_addr,    (const void*)EE_MASTER_ADDR,    nRF24L01::addr_len);
   eeprom_read_block((void*)nRF24L01::broadcast_addr, (const void*)EE_BROADCAST_ADDR, nRF24L01::addr_len);
   eeprom_read_block((void*)nRF24L01::slave_addr,     (const void*)EE_SLAVE_ADDR,     nRF24L01::addr_len);

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
      avr_dbg::die(1, 1000);
   nRF24L01::configure_PRX();
   uint8_t buff[ensemble::message_size];

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


         const uint8_t boot_id = boot_magic_word >> 8;

         good_id_count++;
         switch (messages::get_id(buff))
         {
            case messages::heartbeat_id:    do_heartbeat(buff, t_hb); break;
            case messages::all_stop_id:     do_all_stop(effect); break;
            case messages::start_effect_id: do_start_effect(buff, effect); break;
            case messages::set_tlc_ch_id:   do_set_tlc_ch(buff); break;
            case messages::set_rgb_id:      do_set_rgb(buff); break;
            case messages::ping_id:         do_ping(buff, pipe); break;
            case boot_id:                   ((APP*)BOOTADDR)(); 
            default:
               bad_id_count++;
               good_id_count--;
               last_bad_id=messages::get_id(buff);
         }
      }
      
      effect.execute();
      avr_tlc5940::output_gsdata();
      sleep_mode();
   }
}


void Effect::e0()
{
   int v = 4096 - dt*4;
   if (v<0)
      v=0;
   for (unsigned ch=0; ch<12; ch++)
      avr_tlc5940::set_channel(ch, v);
}


// Ramp up to vmax and down to zero every cl ms
void Effect::e1()
{
   long cl = 3000; // length of cycle in ms
   long phi = 0; // offset into cycle
   long vmax = 4095; // intensity at peak
   long cl2 = cl >> 1; 

   long dtp = dt-phi;
   long cldt = dtp<=0 ? 0 : dtp % cl;

   int v;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=1; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);

   phi = 1000;
   dtp = dt-phi;
   cldt = dtp<=0 ? 0 : dtp % cl;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   for (unsigned ch=2; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);

   phi = 2000;
   dtp = dt-phi;
   cldt = dtp<=0 ? 0 : dtp % cl;
   if (cldt < cl2)
      v = 2 * (vmax * cldt) / cl;
   else
      v = -2 * (vmax * cldt) / cl + 2*vmax;

   // red starts at ch 1, green starts at ch 0
   for (unsigned ch=0; ch<12; ch+=3)
      avr_tlc5940::set_channel(ch, v);
}


void Effect::execute()
{
   if (state==complete)
      return;

   dt = avr_rtc::t_ms - start_time;

   if (dt>int(duration) && state==started)
   {
      state=complete;
      for (unsigned ch=0; ch<14; ch++)
         avr_tlc5940::set_channel(ch, 0);
      return;
   }
   else if (dt>0 && dt<int(duration) && state==unstarted)
      state=started;

   // Don't really need to update the LEDs more often than 50 Hz
   if (prev_dt>0 && dt - prev_dt < 20)
      return;

   if (state==started)
      switch(id)
      {
         case 0: e0(); break;
         case 1: e1(); break;
      }
   prev_dt = dt;
}


void do_all_stop(Effect& effect)
{
   effect.state = Effect::complete;
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
   messages::decode_start_effect(buff, effect.id, effect.start_time, effect.duration);
   effect.cldt = 0;
   effect.dt = 0;
   effect.prev_dt = 0;
   effect.state = Effect::unstarted;
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
   p = messages::encode_var<uint16_t>(p, bad_id_count);
   p = messages::encode_var<uint8_t>(p, last_bad_id);
   write_data(iobuff, ensemble::message_size+1);
   
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
