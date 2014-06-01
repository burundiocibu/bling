#include <cstdlib>
#include <cstdio>
#include <ncurses.h>
#include <unistd.h>
#include <cmath>
#include <vector>
#include <sstream>
#include <sys/mman.h> // for settuing up no page swapping

#include <bcm2835.h>
#include "Lock.hpp"
#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "Slave.hpp"

using namespace std;

RunTime runtime;


void display(const Slave& slave)
{
   ostringstream ss;
   ss << slave;
   mvprintw(1+slave.my_count, 0, ss.str().c_str());
   printw("   pwm:%03x %03x %03x", slave.tlc[0], slave.tlc[1], slave.tlc[2]);
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

   broadcast.header_output=true;
   mvprintw(0, 0, broadcast.stream_header.c_str());

   // Reset all the slaves, and give them a chance to come back up
   broadcast.reboot();
   bcm2835_delayMicroseconds(100000);

   unsigned long t_hb=0, t_ping=0;
   while (true)
   {
      // A simple throbber
      if (runtime.msec() % 1000 < 250) mvprintw(0, 7, "#");
      else                             mvprintw(0, 7, "_");
      mvprintw(24, 0, ">");

      // Send out heartbeat ever second
      unsigned long t=runtime.sec();
      if (t != t_hb)
      {
         broadcast.heartbeat();
         display(broadcast);
         t_hb=t;
      }

      // Ping slave every 5 seconds
      if (t - t_ping >= 5)
      {
         slave.ping();
         display(slave);
         t_ping = t;
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
