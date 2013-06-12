#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ncurses.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

using namespace std;

RunTime runtime;

void hexdump(uint8_t* buff, size_t len);

class Slave
{
public:
   Slave() 
      : slave_no(0), ack_err(0), tx_err(0), j_rx(0), j_tx(0), 
        obs_tx(0), t_tx(0), t_rx(0)
   {};

   unsigned slave_no;
   unsigned ack_err;
   unsigned tx_err;
   unsigned j_tx, j_rx;
   unsigned obs_tx;
   uint32_t t_tx, t_rx;

   void tx(uint8_t *buff, size_t len);
   void rx();
};



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
   mvprintw(1,0,"  #      t_tx   j_tx  tx_err  tx_obs ack_err     t_rx  j_rx   ");

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
   uint32_t last_hb=0;
   Slave ship[nRF24L01::num_chan];
   for (int i=0; i < nRF24L01::num_chan; i++)
      ship[i].slave_no = i;

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
         messages::encode_heartbeat(buff, t);
         ship[0].tx(buff, sizeof(buff));
         last_hb = t;
      }

      mvprintw(0, 0, "tx_cnt: %5d", i);
      for (int i=1; i<4; i++)
      {
         Slave& slave = ship[i];
         messages::encode_ping(buff);
         slave.tx(buff, sizeof(buff));
         slave.rx();
         mvprintw(2+slave.slave_no, 0, "%3d  %9.3f  %3d    %3d     %02x   %5d  ",
                  slave.slave_no, 1e-3*slave.t_tx, slave.j_rx, slave.tx_err, slave.obs_tx, slave.ack_err);
         if (slave.j_rx != 100)
            mvprintw(2+slave.slave_no, 43, "%9.3f  %3d  ", 0.001*slave.t_rx, slave.j_rx);
      }
      // sleep 10 ms
      bcm2835_delayMicroseconds(10000);
   }

   nRF24L01::shutdown();
   endwin();
   return 0;
}


void Slave::tx(uint8_t *buff, size_t len)
{
   using namespace nRF24L01;
   if (slave_no >= num_chan)
      exit(-1);

   t_tx = runtime.msec();
   write_tx_payload(buff, len, slave_no);

   uint8_t status;
   for(j_tx=0; j_tx<100; j_tx++)
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

   obs_tx = read_reg(OBSERVE_TX);
}



void Slave::rx()
{
   using namespace nRF24L01;
   uint8_t buff[messages::message_size];
   memset(buff, 0, sizeof(buff));

   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   delay_us(1000);
   set_CE();

   for (j_rx=0; j_rx<100; j_rx++)
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

   uint8_t id;
   uint8_t* p = buff;
   p = messages::decode_var<uint8_t>(p, id);
   p = messages::decode_var<uint32_t>(p, t_rx);
}


void hexdump(uint8_t* buff, size_t len)
{
   for (int i = 0; i <len; i++)
      printw("%.2X ", buff[i]);
}
