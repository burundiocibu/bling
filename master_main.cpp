#include <cstdlib>
#include <cstdio>
#include <ncurses.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

using namespace std;

RunTime runtime;

unsigned slave=0;
void nrf_tx(uint8_t *buff, size_t len, unsigned slave);
void slider(uint8_t ch, uint16_t &v, int dir);

/*
HB#    R   G    B     STATUS   j
123   fff fff  fff      2E     10

Tx       Data
123.345  01 3B EA 02 00 00 00 00 00 00 00 00
123.345  02 3B EA 02 00 00 00 00 00 00 00 00
*/

int main(int argc, char **argv)
{
   WINDOW *win;
   int prev_curs;

   win = initscr();
   cbreak();
   nodelay(win, true);
   noecho();
   nonl();
   intrflush(win, true);
   keypad(win, true);
   prev_curs = ::curs_set(0);   // we want an invisible cursor.
   mvprintw(0,0, "HB#   slave  R   G   B      j    tx_err  tx_obs ack_err");

   nRF24L01::setup();

   if (!nRF24L01::configure_base())
   {
      printf("Failed to find nRF24L01. Exiting.\n");
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::power_up_PTX();
   nRF24L01::flush_tx();
   nRF24L01::write_reg(nRF24L01::RX_PW_P0, messages::message_size);

   const int LED=RPI_GPIO_P1_07;
   bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);

   uint8_t buff[messages::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   uint16_t red=0,green=0,blue=0;
   unsigned hb_count=0;
   uint32_t last_hb=0;
   mvprintw(1, 6, "%3d   %03x %03x %03x", slave, red, green, blue);

   for (int i=0; ; i++)
   {
      uint32_t t = runtime.msec();
      if (t % 1000 <250)
         bcm2835_gpio_write(LED, LOW);
      else 
         bcm2835_gpio_write(LED, HIGH);

      if (t - last_hb > 990)
      {
         messages::encode_heartbeat(buff, t);
         nrf_tx(buff, sizeof(buff), slave);
         hb_count++;
         last_hb = t;
         mvprintw(1, 0, "%d", hb_count);
      }

      char key = getch();
      if (key=='q')
         break;

      if (key != 0xff)
      {
         switch(key)
         {
            case '0': slave=0; break;
            case '1': slave=1; break;
            case '2': slave=2; break;
            case 'R': slider(0, red,  -1);  break;
            case 'r': slider(0, red,   1);  break;
            case 'G': slider(1, green, -1); break;
            case 'g': slider(1, green,  1); break;
            case 'B': slider(2, blue,  -1); break;
            case 'b': slider(2, blue,   1); break;
            case 'w': slider(0, red,  1); slider(1, green,  1); slider(2, blue,  1); break;
            case 'W': slider(0, red, -1); slider(1, green, -1); slider(2, blue, -1); break;
            case ' ':
               messages::encode_start_effect(buff, 0, t, 1000);
               nrf_tx(buff, sizeof(buff), slave);
               break;
            case 's':
            case 'S':
               messages::encode_all_stop(buff);
               nrf_tx(buff, sizeof(buff), slave);
               red=0;green=0;blue=0;
               break;
         }
         mvprintw(1, 6, "%3d   %03x %03x %03x", slave, red, green, blue);
      }
      // sleep 10 ms
      bcm2835_delayMicroseconds(10000);
   }

   nRF24L01::shutdown();
   endwin();
   bcm2835_gpio_write(LED, HIGH);
   return 0;
}



void nrf_tx(uint8_t *buff, size_t len, unsigned slave)
{
   static unsigned ack_err=0;
   static unsigned tx_err=0;

   nRF24L01::write_tx_payload(buff, len, slave);
   nRF24L01::pulse_CE();

   uint8_t status;
   int j;
   for(j=0; j<100; j++)
   {
      status = nRF24L01::read_reg(nRF24L01::STATUS);
      if (status & nRF24L01::STATUS_TX_DS)
         break;
      bcm2835_delayMicroseconds(5);
   }

   if (status & nRF24L01::STATUS_MAX_RT)
   {
      ack_err++;
      nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_MAX_RT);
      // data doesn't automatically removed...
      nRF24L01::flush_tx();
   }
   else if (status & nRF24L01::STATUS_TX_DS)
      nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_TX_DS); //Clear the data sent notice
   else
      tx_err++;

   uint8_t obs_tx = nRF24L01::read_reg(nRF24L01::OBSERVE_TX);
   mvprintw(1, 27, "%3d  %3d       %02x    %3d", j, tx_err, obs_tx, ack_err);

   mvprintw(4+buff[0], 0, "%8.3f  ", 0.001* runtime.msec());
   for (int i = 0; i <len; i++)
      printw("%.2X ", buff[i]);
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
