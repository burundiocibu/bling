#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ncurses.h>
#include <string>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

#include "Slave.hpp"
#include "ensemble.hpp"

RunTime runtime;

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
   mvprintw(1, 0, "  #      t_tx   tx_dt   tx_cnt   RTC   NACK    \%%          t_rx   rx_dt   no_resp");

   nRF24L01::channel = 2;
   memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
   memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
   memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[2], nRF24L01::addr_len);
   nRF24L01::setup();

   if (!nRF24L01::configure_base())
   {
      printf("Failed to find nRF24L01. Exiting.\n");
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

   uint32_t last_hb=0;
   Slave ship[ensemble::num_slaves];
   for (int i=0; i < ensemble::num_slaves; i++)
      ship[i].m_Slave_no = i;

   for (long i=0; ; i++)
   {
      char key = getch();
      if (key=='q')
         break;
      else if (key==' ')
      {
         while(true)
         {
            key = getch();
            if (key==' ')
               break;
         }
      }

      uint32_t t = runtime.msec();

      if (t - last_hb > 990)
      {
         messages::encode_heartbeat(ship[0].buff, t);
         ship[0].tx();
         last_hb = t;
      }

      for (int j=1; j <4; j++)
      {
         Slave& slave = ship[j];
         messages::encode_ping(slave.buff);
         if (slave.tx())
            slave.rx();
         mvprintw(1+slave.m_Slave_no, 0, slave.status().c_str());
      }
   }

   nRF24L01::shutdown();
   endwin();
   return 0;
}
