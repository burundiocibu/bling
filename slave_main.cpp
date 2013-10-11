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
#include "Effect.hpp"


void do_heartbeat(uint8_t* buff, uint32_t& t_hb);
void do_set_tlc_ch(uint8_t* buff);
void do_set_rgb(uint8_t* buff);
void do_ping(uint8_t* buff, uint8_t pipe);
void do_sleep();

uint16_t slave_id;

// 0.1 was original app load to slaves
const int8_t major_version = 0;
const int8_t minor_version = 4;

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
   uint8_t buff[ensemble::message_size];

   // Turn on 12V supply
   DDRB |= _BV(PB1);
   PORTB |= _BV(PB1);
   // Things to wake us up:
   // nRF IRQ,              random
   // TLC5940 BLANK needed  1.024 kHz
   // rtc:      1 kHz

   uint32_t t_hb=0;
   Effect effect(slave_id);
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

         switch (messages::get_id(buff))
         {
            case messages::heartbeat_id:    do_heartbeat(buff, t_hb); break;
            case messages::all_stop_id:     effect.all_stop(buff); break;
            case messages::start_effect_id: effect.init(buff); break;
            case messages::set_tlc_ch_id:   do_set_tlc_ch(buff); break;
            case messages::set_rgb_id:      do_set_rgb(buff); break;
            case messages::ping_id:         do_ping(buff, pipe); break;
            case messages::reboot_id:       ((APP*)0)(); break;
            case boot_id:                   ((APP*)BOOTADDR)(); break;
            case messages::sleep_id:        do_sleep(); break
         }
      }
      
      effect.execute();
      avr_tlc5940::output_gsdata();
      sleep_mode();
   }
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


void do_set_rgb(uint8_t* buff)
{}


void do_ping(uint8_t* buff, uint8_t pipe)
{
   using namespace nRF24L01;
   messages::decode_ping(buff);

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

   // Gather some data to send back...
   uint16_t vcell = avr_max1704x::read_vcell();
   uint16_t soc = avr_max1704x::read_soc();

   // delay_us(150);
   // above delay not required because the I2C reads will take much longer than that.
   iobuff[0] = W_TX_PAYLOAD;
   messages::encode_status((uint8_t*)&iobuff[1], slave_id, t_rx, major_version, minor_version,
                           vcell, soc);
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

void do_sleep()
{
   //avr_tlc5940::setup();
   // uses TIMER0 for GS clock, TIMER1 to drive blank (and an IRQ)
   // just disable TIMER0 using PRR
   PRR |= _BV(PRTIM2);

   // Set the CPU clock prescaler to be 1024. 8MHz / 1024 = 7.8125 kHz
   CLKPR = CLKPCE;
   CLKPR = CLKPS3;

   //avr_rtc::setup();
   // uses TIMER2 to generate a 1kHz interrupt
   // uses a prescaler of 
   // lets make this wake us up periodically from sleep
   // prescaler = 1024, f=7.8125 kHz/1024 = 7.6294 Hz
   // with OCR=125, 7.6294 / 125 = 0.061035 Hz = 16.38 seconds between IRQ
   TCCR2B = _BV(CS22|CS21|CS20) ;

   // Turn off 12V supply
   PORTB &= ~_BV(PB1);

   // Got to "power-save" mode
   SMCR = SM1 | SM0;   
   
   //nRF24L01::setup();
   // generates an INT0 on rx of data when powered on
   // leave this as is..
   using namespace nRF24L01;
   char config = read_reg(CONFIG);

   while(true)
   {
      clear_CE();  // Turn off receiver
      write_reg(CONFIG, config & ~CONFIG_PWR_UP); // power down

      power_save_mode();

      write_reg(CONFIG, config & CONFIG_PWR_UP); // power up
      set_CE();
      
      power_save_mode();
      
      uint8_t status = nRF24L01::read_reg(nRF24L01::STATUS);
      if (status != 0x0e)
      {
         uint8_t pipe;
         nRF24L01::read_rx_payload(buff, sizeof(buff), pipe);
         nRF24L01::clear_IRQ();

      }
   }


   // need to turn back on all hardware here...

   // Turn back on the tlc5940 GS clock
   PRR &= ~_BV(PRTIM2);

   // Restore the prescaler for the tlc5940
   TCCR0B = _BV(CS00); // prescaler div = 1

   // Restore the prescaler for the rtc
   TCCR2B = _BV(CS22) ; // prescaler = 64, f=8e6/64=125kHz

   // Turn back on 12V supply
   PORTB |= _BV(PB1);

   return;
   // or maybe ((APP*)0)();
}
