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
   while ((c = getopt(argc, argv, "di:s:")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         default:
            printf("Usage %s [-d]\n", argv[0]);
            exit(-1);
      }

   // lock this process into memory
   if (true)
   {
      struct sched_param sp;
      memset(&sp, 0, sizeof(sp));
      sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
      sched_setscheduler(0, SCHED_FIFO, &sp);
      mlockall(MCL_CURRENT | MCL_FUTURE);
   }

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
   SlaveList ship, found;

   // Reset all the slaves, and give them a chance to come back up
   broadcast.reboot();
   bcm2835_delayMicroseconds(100000);

   SlaveList todo, done, all;

   for (int id=1; id < ensemble::num_slaves; id++)
      todo.push_back(Slave(id));

   SlaveList::iterator i;
   for (int pass=1; todo.size() > 0 && pass < 10; pass++)
   {
      cout << "Pass " << pass << " " << 1e-6*runtime.usec() << " " << endl;
      for (i = todo.begin(); i != todo.end(); )
      {
         if (i->ping() == 0)
         {
            done.push_back(*i);
            cout << "." << flush;
            i = todo.erase(i);
         }
         else
         {
            cout << "x" << flush;
            i++;
         }
      }

      cout << endl;
      if (todo.size())
         sleep(1);
   }

   cout << "Done" << endl
        << endl
        << "Responded:" << endl
        << done << endl;

   if (debug>2)
      cout << "No response:" << endl
           << todo << endl;

   nRF24L01::shutdown();
   return 0;
}
