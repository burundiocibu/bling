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

int main(int argc, char **argv)
{
   Lock lock;

   int debug=0;

   string list_fn;
   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "di:l:s:v:p")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 'l': list_fn = string(optarg); break;
         default:
            cout << "Usage " << argv[0] << " -l [slave_fn] [-d]" << endl
                 << " -l  specifies board list file" << endl
                 << " -d  Enable debug ouput" << endl;
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

   SlaveList todo;
   if (list_fn.size())
   {
      todo = read_slaves(list_fn);
      if (debug>2)
         cout << list_fn << ":" << endl << todo;
   }
   else
      for (int id=1; id < ensemble::num_slaves; id++)
         todo.push_back(Slave(id));

   SlaveList found = scan(todo);

   cout << "Boards found:" << endl
        << found << endl;

   if (debug>2)
      cout << "No response:" << endl
           << todo << endl;

   nRF24L01::shutdown();
   return 0;
}
