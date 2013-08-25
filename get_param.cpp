#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stddef.h>

#include <unistd.h>

#include "ensemble.hpp"
#include "nrf_boot.h"


void hex_dump(const void* buff, size_t len)
{
   printf("\"");
   const uint8_t *p = static_cast<const uint8_t*>(buff);
   for (unsigned i=0; i<len-1; i++)
      printf("0x%02x ", *p++);
   printf("0x%02x\";", *p++);
}


using namespace std;

int main(int argc, char **argv)
{
   if (argc != 2)
   {
      fprintf(stderr, "Usage: %s slave_no\n  prints config for slave\n", argv[0]);
      exit(-1);
   }

   unsigned slave_no=atoi(argv[1]);

   if (slave_no >= ensemble::num_slaves)
   {
      fprintf(stderr, "Slave %d is not valid\n", slave_no);
      exit(-1);
   }

   printf("slave_no=%d;", slave_no);
   printf("channel_no=%d;", ensemble::default_channel);
   printf("master_addr=");hex_dump(ensemble::master_addr, ensemble::addr_len);
   printf("broadcast_addr="); hex_dump(ensemble::slave_addr[0], ensemble::addr_len);
   printf("slave_addr="); hex_dump(ensemble::slave_addr[slave_no], ensemble::addr_len);
}
