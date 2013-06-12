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
void nrf_rx();
void slider(uint8_t ch, uint16_t &v, int dir);
void hexdump(uint8_t* buff, size_t len);

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
   nRF24L01::flush_tx();

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
            case 'G': slider(0, red,  -1);  break;
            case 'g': slider(0, red,   1);  break;
            case 'R': slider(1, green, -1); break;
            case 'r': slider(1, green,  1); break;
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
            case 'p':
               messages::encode_ping(buff);
               nrf_tx(buff, sizeof(buff), slave);
               nrf_rx();
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

   uint8_t obs_tx = read_reg(OBSERVE_TX);
   mvprintw(1, 27, "%3d  %3d       %02x    %3d", j, tx_err, obs_tx, ack_err);

   mvprintw(4+buff[0], 0, "%8.3f  ", 0.001* runtime.msec());
   hexdump(buff, len);
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

   uint32_t t_rx;
   uint8_t id;
   uint8_t* p = buff;
   p = messages::decode_var<uint8_t>(p, id);
   p = messages::decode_var<uint32_t>(p, t_rx);
   mvprintw(14, 0, "%8.3f ", 0.001*t_rx);
   printw("%2d %3d   ", id, i);
   hexdump(buff, messages::message_size);
}


void hexdump(uint8_t* buff, size_t len)
{
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
