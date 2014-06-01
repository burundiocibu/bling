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
   cout << broadcast.stream_header << endl;

   while (true)
   {
      // Send out heartbeat ever second
      if (runtime.usec() - broadcast.t_tx > 999000)
      {
         broadcast.heartbeat();
         if (debug)
            cout << broadcast << endl;
      }

      // Ping slave every 5 seconds
      if (runtime.usec() - slave.t_tx > 4999000)
      {
         slave.ping();
         cout << slave << endl;
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

      cout << endl;

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
      cout << slave << endl;
   }

   nRF24L01::shutdown();
   return 0;
}
