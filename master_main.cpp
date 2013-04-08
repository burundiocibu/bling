#include <cstdlib>
#include <cstdio>
#include <ncurses.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

using namespace std;

RunTime runtime;


void master_tx(uint8_t *buff, size_t len, unsigned slave_num);
void slider(uint8_t ch, uint16_t &v, int dir);

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

   nRF24L01::setup();

   int slave_num=0;

   if (!nRF24L01::configure_base())
   {
      printf("Failed to find nRF24L01. Exiting.\n");
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::power_up_PTX();
   nRF24L01::flush_tx();

   const int LED=RPI_GPIO_P1_07;
   bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);

   uint8_t buff[messages::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   uint16_t red=0,green=0,blue=0;
   unsigned hb_count=0;
   uint32_t last_hb=0;

   for (int i=0; ; i++)
   {
      uint32_t t = runtime.msec();
      if (t % 1000 <250)
         bcm2835_gpio_write(LED, LOW);
      else 
         bcm2835_gpio_write(LED, HIGH);

      if (t - last_hb > 1000)
      {
         messages::encode_heartbeat(buff, t);
         master_tx(buff, sizeof(buff), slave_num);
         hb_count++;
         last_hb = t;
         mvprintw(0, 0, "i:%5d", hb_count);
      }

      char key = getch();
      if (key=='q')
         break;

      if (key != 0xff)
      {
         switch(key)
         {
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
               master_tx(buff, sizeof(buff), slave_num);
               break;
            case 'S':
            case 's':
               messages::encode_all_stop(buff);
               master_tx(buff, sizeof(buff), slave_num);
               red=0;green=0;blue=0;
               break;
            case '0': slave_num=0; break;
            case '1': slave_num=1; break;
            case '2': slave_num=2; break;
         }
         mvprintw(1, 0, "RGB: %3x %3x %3x", red, green, blue);
      }
      // sleep 10 ms
      bcm2835_delayMicroseconds(10000);
   }

   nRF24L01::shutdown();
   endwin();
   bcm2835_gpio_write(LED, HIGH);
   return 0;
}


void master_tx(uint8_t *buff, size_t len, unsigned slave_num)
{
   static unsigned ack_err=0;
   static unsigned tx_err=0;

   nRF24L01::write_tx_payload(buff, len, slave_num);
   nRF24L01::pulse_CE();

   uint8_t status;
   for(int j=0; j<100; j++)
   {
      status = nRF24L01::read_reg(nRF24L01::STATUS);
      if (status & nRF24L01::STATUS_TX_DS || status & nRF24L01::STATUS_MAX_RT)
         break;
      bcm2835_delayMicroseconds(10);
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
   mvprintw(0,10, "slave:%-3d tx_err:%-3d  obs_tx:%02x  ack_err:%-3d", slave_num, tx_err, obs_tx, ack_err);
   mvprintw(2+buff[0], 0, "Tx:%8.3f  ", 0.001* runtime.msec());
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
   master_tx(buff, sizeof(buff), 0);
}

