#include <cstdlib>
#include <cstdio>
#include <ncurses.h>
#include <unistd.h>
#include <cmath>
#include <vector>
#include <sys/mman.h> // for settuing up no page swapping

#include <bcm2835.h>
#include "Lock.hpp"
#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "Slave.hpp"

using namespace std;

RunTime runtime;

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
   printw("  [nack_cnt:%d, arc_cnt:%d, plos_cnt:%d]  ",
          slave.nack_cnt, slave.arc_cnt, slave.plos_cnt);
   mvprintw(24, 0, ">");
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

   // lock this process into memory
   if (false)
   {
      struct sched_param sp;
      memset(&sp, 0, sizeof(sp));
      sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
      sched_setscheduler(0, SCHED_FIFO, &sp);
      mlockall(MCL_CURRENT | MCL_FUTURE);
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
      cout << "Failed to find nRF24L01. Exiting." << endl;
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
      if (runtime.msec() % 1000 < 250)  mvprintw(0, 0, "^");
      else                             mvprintw(0, 0, "_");
      mvprintw(24, 0, ">");

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
