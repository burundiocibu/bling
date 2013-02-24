#include <cstdlib>
#include <cstdio>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"


#include <arpa/inet.h>

struct HeartbeatMsg 
{
   HeartbeatMsg() 
      : id(0x01), t_ms(0)
   {}

   void send(const uint32_t t)
   {
      id=0x01;
      t_ms=htonl(t);
      nRF24L01::write_tx_payload(this, sizeof(*this));
      nRF24L01::pulse_CE();
   }

   uint8_t id;
   uint32_t t_ms;
} __attribute__((packed));


using namespace std;
using namespace nRF24L01;


int main(int argc, char **argv)
{
   RunTime rt;
   rpi_setup();
   configure_PTX();
   power_up_PTX();
   write_data(FLUSH_TX);

   HeartbeatMsg hb;
   for (int i=0; i<5; i++)
   {
      printf("\ni:%d\n", i);
      hb.send(rt.msec());
      for(int j=0; ((read_reg(STATUS) & STATUS_TX_DS)== 0x00) && j<100; j++)
         delay_us(10);
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice

      // wait till next second
      struct timeval tv;
      rt.tv(tv);
      delay_us(1000000 - tv.tv_usec);
   }

   rpi_shutdown();
   return 0;
}

