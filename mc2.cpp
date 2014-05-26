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
   unsigned tx_read_cnt;
   uint8_t status;    // STATUS register after most recent operation
   uint8_t obs_tx;    // OBSERVE_TX register after most recent operation
   unsigned tx_err;   // number of failed transmissions
   unsigned nack_cnt; // number of transmissions with ACK req set that don't get responses.
   unsigned no_resp;  // number of ping responses we expected but didn't get
   unsigned tx_dt, rx_dt;  // time to complete tx/rx in usec
   int slave_dt;     // estimate of slave clock offset in ms
   uint32_t t_tx, t_rx; // time of most recent send and the time in the most recent received packet

   // These values are only filled in when there has been a response from a ping
   uint32_t t_ping; // slave time (msec)
   uint16_t soc, vcell, mmc;
   int8_t major_version, minor_version;

   uint8_t buff[ensemble::message_size];

   void slide_pwm_ch(unsigned ch, int dir);
   void slide_pwm(int dir);

   void tx(unsigned repeat);
   void rx();
   // The following are higher level comands to the slaves
   // repeat indicates how many times the command will be sent
   void ping(unsigned repeat=1);
   void heartbeat(unsigned repeat=1);
   void all_stop(unsigned repeat=1);
   void reboot(unsigned repeat=1);
   void set_pwm(unsigned repeat=1);
};


void display_header()
{
   mvprintw(0, 0, "       ____________________tx_______________");
   mvprintw(1, 0, "slave   #  time(s)  ch0 ch1 ch2  dt(ms)  err");
   mvprintw(0, 46, "  __________rx___________   ver  Vcell  SOC    MMC   clk  ");
   mvprintw(1, 46, "  time(s)    dt(ms)   NR          (v)   (%)          (ms)  ");
}

void display(const Slave& slave)
{
   mvprintw(2+slave.my_count, 0,
            "%3d  %4d %8.3f  %03x %03x %03x %6.3f  %3d",
            slave.id, slave.tx_cnt, 1e-6*slave.t_tx,
            slave.pwm[0], slave.pwm[1], slave.pwm[2],
            1e-3*slave.tx_dt, slave.tx_err);
   mvprintw(2+slave.my_count, 46,
            "%8.3f  %8.3f  %4d   %2d.%1d  %1.3f  %3d  %5d  %4d",
            1e-6*slave.t_rx, 1e-3*slave.rx_dt, slave.no_resp,
            slave.major_version, slave.minor_version,
            1e-3*slave.vcell,  slave.soc, slave.mmc, slave.slave_dt);
   printw("  [status:%02x, obs:%02x, tx_read_cnt:%d, nack_cnt:%d]",
          slave.status, slave.obs_tx, slave.tx_read_cnt, slave.nack_cnt);
}


void hexdump(const Slave& slave, size_t len)
{
   mvprintw(2+slave.my_count, 120, "> ");
   for (int i = 0; i <len; i++)
      printw("%.2X ", slave.buff[i]);
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


   Slave broadcast(0);
   Slave slave(slave_id);

   display_header();

   while (true)
   {
      // A simple throbber
      if (runtime.msec() % 1000 <250)  mvprintw(0, 0, "^");
      else                             mvprintw(0, 0, "_");

      // Send out heartbeat ever second
      if (runtime.usec() - broadcast.t_tx > 999000)
      {
         broadcast.heartbeat();
         display(broadcast);
      }

      // Ping slave every 5 seconds
      if (runtime.usec() - slave.t_rx > 4999000)
      {
         slave.ping();
         display(slave);
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
            slave.all_stop();
            break;

         case 'z':
            slave.reboot();
            break;
      }
      display(slave);
   }

   nRF24L01::shutdown();
   endwin();
   return 0;
}


//===================================================================================
unsigned Slave::slave_count = 0;


Slave::Slave(unsigned _id)
   : id(_id), my_count(slave_count++), pwm(15),
     tx_cnt(0), tx_read_cnt(0),
     obs_tx(0), tx_err(0), nack_cnt(0),
     no_resp(0), tx_dt(0), rx_dt(0),
     t_tx(0), t_rx(0), t_ping(0), slave_dt(0), mmc(0),
     vcell(0), soc(0)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
};


void Slave::tx(unsigned repeat)
{
   using namespace nRF24L01;

   bool ack = id != 0;

   tx_cnt++;
   t_tx = runtime.usec();

   for (int i=0; i<repeat; i++)
   {
      write_tx_payload(buff, sizeof(buff), (const char*)ensemble::slave_addr[id], ack);

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
         // No need to resend if we get an ack...
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

void Slave::rx(void)
{
   using namespace nRF24L01;

   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
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
   {
      no_resp++;
      return;
   }

   t_rx = runtime.usec();

   uint16_t slave_id;
   uint8_t freshness_count;

   msg::decode_status(buff, slave_id, t_ping, major_version, minor_version,
                      vcell, soc, mmc, freshness_count);

   soc = 0xff & (soc >> 8);
   rx_dt = t_rx - t_tx;
   slave_dt = t_ping - t_rx/1000;
}


void Slave::heartbeat(unsigned repeat)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_heartbeat(buff, runtime.msec());
   tx(repeat);
}


void Slave::ping(unsigned repeat)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_ping(buff);
   tx(repeat);
   rx();
}

void Slave::all_stop(unsigned repeat)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_all_stop(buff);
   tx(repeat);
}


void Slave::reboot(unsigned repeat)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_reboot(buff);
   tx(repeat);
}


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


void Slave::set_pwm(unsigned repeat)
{
   for (int ch=0; ch<15; ch++)
   {
      for (int i=0; i<sizeof(buff); i++) buff[i]=0;
      msg::encode_set_tlc_ch(buff, ch, pwm[ch]);
      tx(repeat);
   }
};
