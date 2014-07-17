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
   mvprintw(slave.my_line, 0, ss.str().c_str());
   if (slave.id)
      printw("   pwm:%03x %03x %03x", slave.tlc[0], slave.tlc[1], slave.tlc[2]);
   mvprintw(24, 0, ">");
}


int debug;

int main(int argc, char **argv)
{
   Lock lock;

   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "d")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         default:
            printf("Usage %s [-d]\n", argv[0]);
            exit(-1);
      }

   // lock this process into memory
#ifdef linux
      struct sched_param sp;
      memset(&sp, 0, sizeof(sp));
      sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
      sched_setscheduler(0, SCHED_FIFO, &sp);
      mlockall(MCL_CURRENT | MCL_FUTURE);
#endif

   nRF24L01::channel = ensemble::default_channel;
   memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
   memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
   memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[2], nRF24L01::addr_len);

   if (!nRF24L01::setup() || !nRF24L01::configure_base())
   {
      cout << "Failed to find nRF24L01. Exiting." << endl;
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

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

   mvprintw(0, 0, Slave::stream_header.c_str());

   // Reset all the slaves, and give them a chance to come back up
   Slave broadcast(0);
   broadcast.my_line=1;
   broadcast.reboot();
   bcm2835_delayMicroseconds(100000);

   SlaveList all, found;
   for (int id=1; id < ensemble::num_slaves; id++)
      all.push_back(Slave(id));

   long t_hb=-1000, t_ping=-1000;
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
         t_hb=t;
         broadcast.heartbeat();
         display(broadcast);
      }

      // Ping slave every 5 seconds
      if (t - t_ping >= 5)
      {
         t_ping = t;
         if (all.size())
         {
            SlaveList more = scan(all);
            if (more.size())
            {
               found.splice(found.end(), more);
               int line = 2;
               for (auto i = found.begin(); i != found.end(); i++)
                  i->my_line = line++;
            }
         }
         for (auto i=found.begin(); i!=found.end(); i++)
         {
            i->ping();
            display(*i);
         }
      }

      char key = getch();
      if (key=='q')
         break;

      if (key == '\377')
      {
         // sleep 10 ms
         bcm2835_delayMicroseconds(10000);
         continue;
      }

      for (auto slave=found.begin(); slave!=found.end(); slave++)
      {
         switch(key)
         {
            case 'w':
               slave->slide_pwm(1);
               slave->set_pwm();
               slave->ping();
               break;

            case 'W':
               slave->slide_pwm(-1);
               slave->set_pwm();
               slave->ping();
               break;

            case 'p':
               slave->ping();
               break;

            case 'x':
               slave->all_stop();
               break;

            case 'z':
               slave->reboot();
               break;
         }
         display(*slave);
      }
   }

   nRF24L01::shutdown();
   endwin();
   return 0;
}
