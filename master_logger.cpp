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

int debug;

int main(int argc, char **argv)
{
   Lock lock;

   opterr = 0;
   int c;
   unsigned slave_id=0;
   while ((c = getopt(argc, argv, "ds:")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 's': slave_id = atoi(optarg); break;
         default:
            printf("Usage %s -s slave_id [-d]\n", argv[0]);
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

   // Reset all the slaves, and give them a chance to come back up
   Slave broadcast(0);
   broadcast.reboot();

   cout << broadcast.stream_header << endl;

   SlaveList all, found;
   for (int id=1; id < ensemble::num_slaves; id++)
      all.push_back(Slave(id));
   SlaveList::iterator scanner=all.begin();

   long t_hb=-1000, t_dump=0;
   int line=2;
   while (true)
   {
      long t=runtime.sec();

      // Send out heartbeat ever second
      if (t != t_hb)
      {
         t_hb=t;
         broadcast.heartbeat();
         if (debug>1)
            cout << broadcast << endl;
      }

      scanner = scan_some(all, scanner, 10);
      for (auto i=all.begin(); i!=all.end(); i++)
         if (i->t_rx && i->my_line ==0)
               i->my_line = line++;

      if (t - t_dump > 5)
      {
         t_dump = t;
         for (auto i=all.begin(); i!=all.end(); i++)
            if (i->t_rx)
               cout << *i << endl;
      }

      char key = getch();
      if (key=='q')
         break;

      if (key == 0xff)
      {
         // sleep 10 ms
         bcm2835_delayMicroseconds(10000);
         continue;
      } else
         cout << "ch:" << key << endl;

   }

   nRF24L01::shutdown();
   return 0;
}
