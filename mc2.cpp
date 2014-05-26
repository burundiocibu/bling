#include <cstdlib>
#include <cstdio>
#include <ncurses.h>
#include <unistd.h>
#include <cmath>
#include <vector>

#include <bcm2835.h>
#include "Lock.hpp"
#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"
#include "ensemble.hpp"

namespace msg=messages;
using namespace std;

RunTime runtime;

void hexdump(uint8_t* buff, size_t len);


class Slave
{
public:

   Slave(unsigned _id);
   vector<uint16_t> pwm;
   unsigned id;
   static unsigned slave_count;  // A count of the total number of Slave objects created
   unsigned my_count;

   unsigned tx_cnt;
   unsigned retry_cnt;
   unsigned tx_read_cnt;
   uint8_t obs_tx;    // OBSERVE_TX register after most recent operation
   unsigned tx_err;   // number of transmit errors
   unsigned nack_cnt; // number of transmissions with ACK req set that don't get responses.
   unsigned no_resp;  // number of ping responses we expected but didn't get
   unsigned tx_dt, rx_dt;  // time to complete tx/rx in usec
   uint32_t t_tx, t_rx; // time of most recent send and the time in the most recent received packet

   // These values are only filled in when there has been a response from a ping
   uint32_t t_ping; // slave time (msec)
   uint16_t soc, vcell, mmc;
   int8_t major_version, minor_version;

   void slide_pwm_ch(unsigned ch, int dir);
   void slide_pwm(int dir);
   void set_pwm();
   void tx(uint8_t *buff, size_t len, unsigned repeat);
   void rx();
   void ping();
   void heartbeat();
};

void display_header()
{
   mvprintw(0, 0, "       ____________________tx___________________   _______________rx______________");
   mvprintw(1, 0, "slave   #    time   ch0 ch1 ch2   dt    obs  err     time    ver  vcell   soc  mmc");
}

void display(const Slave& slave)
{
   mvprintw(2+slave.my_count, 0, "%3d  %4d %8.3f  %03x %03x %03x %6.3f  %2x  %3d",
            slave.id, slave.tx_cnt, 1e-6*slave.t_tx, slave.pwm[0], slave.pwm[1], slave.pwm[2],
            0.001*slave.tx_dt, slave.obs_tx, slave.tx_err);
   if (slave.t_ping != 0)
      mvprintw(2+slave.my_count, 50, "%8.3f  %2d.%1d  %1.3fmV %3d%%    %4d  ",
               0.001*slave.t_ping, slave.major_version, slave.minor_version,
               0.001*slave.vcell,  slave.soc, slave.mmc);
}

int debug;

int main(int argc, char **argv)
{
   Lock lock;

   opterr = 0;
   int c;
   unsigned slave_id=0;
   while ((c = getopt(argc, argv, "di:s:")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 's': slave_id = atoi(optarg); break;
         default:
            printf("Usage %s -i fn -s slave_id [-d]\n", argv[0]);
            exit(-1);
      }

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

   nRF24L01::channel = ensemble::default_channel;
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

   uint8_t buff[ensemble::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   Slave broadcast(0);
   Slave slave(slave_id);

   uint8_t h=0, s=255, v=0;
   unsigned hb_count=0;
   uint32_t last_hb=0;
   display_header();

   while (true)
   {
      if (runtime.msec() % 1000 <250)
         mvprintw(0, 0, "^");
      else
         mvprintw(0, 0, "_");

      if (runtime.usec() - broadcast.t_tx > 999000)
      {
         broadcast.heartbeat();
         display(broadcast);
      }

      char key = getch();
      if (key=='q')
         break;

      if (key == 0xff)
      {
         // sleep 10 ms
         bcm2835_delayMicroseconds(10000);
         continue;
      }

      switch(key)
      {
         case 'w':
            slave.slide_pwm(1);
            slave.set_pwm();
            break;

         case 'W':
            slave.slide_pwm(-1);
            slave.set_pwm();
            break;

         case 'p':
            slave.ping();
            break;

         case 'x':
            msg::encode_all_stop(buff);
            slave.tx(buff, sizeof(buff), 5);
            break;

         case 'z':
            msg::encode_reboot(buff);
            slave.tx(buff, sizeof(buff), 1);
            break;
      }
      display(slave);
   }

   nRF24L01::shutdown();
   endwin();
   return 0;
}


void Slave::rx(void)
{
   using namespace nRF24L01;
   uint8_t buff[ensemble::message_size];
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
         read_rx_payload((char*)buff, ensemble::message_size, pipe);
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }
      delay_us(200);
   }

   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();

   if (i==100)
      return;

   uint16_t slave_id;
   uint8_t freshness_count;

   msg::decode_status(buff, slave_id, t_ping, major_version, minor_version,
                      vcell, soc, mmc, freshness_count);

   soc = 0xff & (soc >> 8);
}


void hexdump(uint8_t* buff, size_t len)
{
   for (int i = 0; i <len; i++)
      printw("%.2X ", buff[i]);
}



//===================================================================================
unsigned Slave::slave_count = 0;


Slave::Slave(unsigned _id)
   : id(_id), my_count(slave_count++), pwm(15),
     tx_cnt(0), retry_cnt(0), tx_read_cnt(0),
     obs_tx(0), tx_err(0), nack_cnt(0),
     no_resp(0), tx_dt(0), rx_dt(0),
     t_tx(0), t_rx(0), t_ping(0),
     vcell(0), soc(0)
{};


void Slave::slide_pwm_ch(unsigned ch, int dir)
{
   uint16_t& v = pwm[ch];
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
};


void Slave::slide_pwm(int dir)
{
   for (int i=0; i<15; i++)
      slide_pwm_ch(i, dir);
}


void Slave::set_pwm()
{
   uint8_t buff[ensemble::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;

   for (int ch=0; ch<15; ch+=3)
   {
      msg::encode_set_tlc_ch(buff, ch, pwm[ch]);
      tx(buff, sizeof(buff), 1);
   }
};


void Slave::tx(uint8_t *buff, size_t len, unsigned repeat)
{
   using namespace nRF24L01;

   bool ack = id != 0;
   int i,j;

   tx_cnt++;
   t_tx = runtime.usec();

   for (i=0; i<repeat; i++)
   {
      if (i>0)
         retry_cnt++;
      write_tx_payload(buff, len, (const char*)ensemble::slave_addr[id], ack);

      uint8_t status;
      for(tx_read_cnt=0; tx_read_cnt<100; tx_read_cnt++)
      {
         status = read_reg(STATUS);
         if (status & STATUS_TX_DS)
            break;
         delay_us(5);
      }

      if (status & STATUS_MAX_RT)
      {
         nack_cnt++;
         write_reg(STATUS, STATUS_MAX_RT);
         // data doesn't get automatically removed...
         flush_tx();
      }
      else if (status & STATUS_TX_DS)
      {
         write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
         if (ack)
            break;
      }
      else
         tx_err++;

      if (i<repeat)
         delay_us(2500);
   }

   obs_tx = read_reg(OBSERVE_TX);
   tx_dt = runtime.usec() - t_tx;
}


void Slave::heartbeat()
{
   uint8_t buff[ensemble::message_size];
   msg::encode_heartbeat(buff, runtime.msec());
   tx(buff, sizeof(buff), 1);
}


void Slave::ping()
{
   uint8_t buff[ensemble::message_size];
   msg::encode_ping(buff);
   tx(buff, sizeof(buff), 1);
   rx();
}
