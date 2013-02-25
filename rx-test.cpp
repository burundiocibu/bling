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
   configure_PRX();
   power_up_PRX();

   HeartbeatMsg hb;
   for (int i=0; i<5; i++)
   {
      printf("\ni:%d\n", i);

      while(1)
      {
         //wait until a packet has been received
         for (int j=0; ((read_reg(STATUS) & STATUS_RX_DR) == 0x00) & j<200; j++)
            delay_us(5000);
         if ((read_reg(STATUS) & STATUS_RX_DR) == 0x00)
            continue;

         char buff[sizeof(hb)];
         read_rx_payload(buff, sizeof(hb)); //read the packet into data
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         dump(&hb, sizeof(hb));

         // delay_us(130); //wait for the other 24L01 to come from standby to RX
         // nrf24l01_set_as_tx(); //change the device to a TX to send back from the other 24L01
         // nrf24l01_write_tx_payload(&data, 1, true); //transmit received char over RF
      }
   }

   rpi_shutdown();
   return 0;
}

