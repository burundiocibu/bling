#include <cstdlib>
#include <cstdio>
#include <ncurses.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

using namespace std;

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

   RunTime runtime;
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
   uint8_t red=0,green=0,blue=0;
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
         nRF24L01::write_tx_payload(buff, sizeof(buff));
         nRF24L01::pulse_CE();
         hb_count++;
         last_hb = t;
         
         mvprintw(0, 0, "%5d %8.3f  ", hb_count, 0.001* t);
         for(int j=0; ((nRF24L01::read_reg(nRF24L01::STATUS) & nRF24L01::STATUS_TX_DS)== 0x00) && j<100; j++)
            bcm2835_delayMicroseconds(10);;
         nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_TX_DS); //Clear the data sent notice
      }

      char key = getch();
      if (key=='q')
         break;

      if (key != 0xff)
      {
         mvprintw(2, 0, "%8.3f  %c", i, 0.001*runtime.msec(), key);
         switch(key)
         {
            case 'R': red+=16;   messages::encode_set_tlc_ch(buff, 0, red);   break;
            case 'r': red-=16;   messages::encode_set_tlc_ch(buff, 0, red);   break;
            case 'G': green+=16; messages::encode_set_tlc_ch(buff, 1, green); break;
            case 'g': green-=16; messages::encode_set_tlc_ch(buff, 1, green); break;
            case 'B': blue+=16;  messages::encode_set_tlc_ch(buff, 2, blue);  break;
            case 'b': blue-=16;  messages::encode_set_tlc_ch(buff, 2, blue);  break;
            case ' ': messages::encode_start_effect(buff, 0, t, 1000); break;
         }
         printw(" %3d %3d %3d", red, green, blue);
         nRF24L01::write_tx_payload(buff, sizeof(buff));
         nRF24L01::pulse_CE();
         for(int j=0; ((nRF24L01::read_reg(nRF24L01::STATUS) & nRF24L01::STATUS_TX_DS)== 0x00) && j<100; j++)
            bcm2835_delayMicroseconds(10);;
         nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_TX_DS); //Clear the data sent notice
      }
      // sleep 10 ms
      bcm2835_delayMicroseconds(10000);
   }

   nRF24L01::shutdown();
   endwin();
   bcm2835_gpio_write(LED, HIGH);
   return 0;
}

