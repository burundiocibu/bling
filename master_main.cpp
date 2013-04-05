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

   messages::Heartbeat heartbeat;
   uint8_t buff[messages::message_size];

   uint8_t red=0,green=0,blue=0;

   unsigned hb_count=0;
   for (int i=0; ; i++)
   {
      uint32_t t = runtime.msec();
      if (t % 1000 <250)
         bcm2835_gpio_write(LED, LOW);
      else 
         bcm2835_gpio_write(LED, HIGH);

      if (t - heartbeat.t_ms > 1000)
      {
         hb_count++;
         heartbeat.t_ms = runtime.msec();
         heartbeat.encode(buff);
         nRF24L01::write_tx_payload(buff, sizeof(buff));
         nRF24L01::pulse_CE();
         
         mvprintw(0, 0, "%5d %8.3f  ", hb_count, 0.001* heartbeat.t_ms);
         for(int j=0; ((nRF24L01::read_reg(nRF24L01::STATUS) & nRF24L01::STATUS_TX_DS)== 0x00) && j<100; j++)
            bcm2835_delayMicroseconds(10);;
         nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_TX_DS); //Clear the data sent notice
      }

      char key = getch();
      if (key=='q')
         break;

      if (key > 'A' && key < 'z')
      {
         mvprintw(2, 0, "%8.3f  %c", i, 0.0001*runtime.msec(), key);
         uint8_t ch=0, value=0;
         switch(key)
         {
            case 'R': red+=16;   ch=0; value=red;   break;
            case 'r': red-=16;   ch=0; value=red;   break;
            case 'G': green+=16; ch=1; value=green; break;
            case 'g': green-=16; ch=1; value=green; break;
            case 'B': blue+=16;  ch=2; value=blue;  break;
            case 'b': blue-=16;  ch=2; value=blue;  break;
         }
         messages::Set_tlc_ch set_tlc_ch;
         set_tlc_ch.ch = ch;
         set_tlc_ch.value = value;
         set_tlc_ch.encode(buff);
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
   return 0;
}

