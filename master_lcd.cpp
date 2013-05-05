#include <cstdlib>
#include <cstdio>
#include <ncurses.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "lcd_plate.hpp"

#include "messages.hpp"

using namespace std;

RunTime runtime;

unsigned slave=0;
void nrf_tx(uint8_t *buff, size_t len, unsigned slave);
void nrf_rx();
void slider(uint8_t ch, uint16_t &v, int dir);
void hexdump(uint8_t* buff, size_t len);


int main(int argc, char **argv)
{
   lcd_plate::setup(0x20);
   lcd_plate::clear();
   lcd_plate::set_cursor(0,0);
   lcd_plate::puts("master_lcd");

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
   {
      printf("Failed to find nRF24L01. Exiting.\n");
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

   uint8_t buff[messages::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   uint16_t red=0,green=0,blue=0;
   unsigned hb_count=0;
   uint32_t last_hb=0;

   uint8_t button=0;
   for (int i=0; ; i++)
   {
      uint32_t t = runtime.msec();

      if (t - last_hb > 990)
      {
         messages::encode_heartbeat(buff, t);
         nrf_tx(buff, sizeof(buff), slave);
         hb_count++;
         last_hb = t;
      }

      uint8_t b = ~lcd_plate::read_buttons();
      if (b!=button)
      {
         button = b;
         printf("%8.3f %02x\n", 1e-3*runtime.msec(), b);
         
         if ( button & lcd_plate::SELECT)
         {
               messages::encode_start_effect(buff, 0, t, 1000);
               nrf_tx(buff, sizeof(buff), slave);
         }
         else if (button & lcd_plate::UP)
         {
            slider(0, red,   1);
            slider(1, green, 1);
            slider(2, blue,  1);
         }
         else if (button & lcd_plate::DOWN)
         {
            slider(0, red,   -1);
            slider(1, green, -1);
            slider(2, blue,  -1);
         }
      }

      bcm2835_delayMicroseconds(5000);
   }

   nRF24L01::shutdown();
   return 0;
}



void nrf_tx(uint8_t *buff, size_t len, unsigned slave)
{
   using namespace nRF24L01;

   static unsigned ack_err=0;
   static unsigned tx_err=0;

   write_tx_payload(buff, len, slave);

   uint8_t status;
   int j;
   for(j=0; j<100; j++)
   {
      status = read_reg(STATUS);
      if (status & STATUS_TX_DS)
         break;
      delay_us(5);
   }

   if (status & STATUS_MAX_RT)
   {
      ack_err++;
      write_reg(STATUS, STATUS_MAX_RT);
      // data doesn't automatically removed...
      flush_tx();
   }
   else if (status & STATUS_TX_DS)
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
   else
      tx_err++;
}



void nrf_rx(void)
{
   using namespace nRF24L01;
   uint8_t buff[messages::message_size];
   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   delay_us(1000);
   set_CE();

   int i;
   for (i=0; i<100; i++)
   {
      if (read_reg(STATUS) & STATUS_RX_DR)
      {
         read_rx_payload((char*)buff, messages::message_size, pipe);
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }
      delay_us(200);
   }

   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();

}


void slider(uint8_t ch, uint16_t &v, int dir)
{
   if (dir>0)
   {
      if (v==0)
         v=1;
      else
         v = (v << 1) + 1;
      if (v >=4096)
         v=4095;
   }
   else
   {
      if (v>0)
         v >>= 1;
   }

   uint8_t buff[messages::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   messages::encode_set_tlc_ch(buff, ch, v);
   nrf_tx(buff, sizeof(buff), slave);
}
