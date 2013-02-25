#include <cstdlib>
#include <cstdio>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "HeartbeatMsg.hpp"

using namespace std;
using namespace nRF24L01;

int main(int argc, char **argv)
{
   RunTime rt;
   rpi_setup();
   configure_PRX();
   power_up_PRX();

   const int LED=RPI_GPIO_P1_07;
   bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);

   // Set up the expected message length for pipe 0
   write_reg(RX_PW_P0, sizeof(HeartbeatMsg));

   while(1)
   {
      rt.puts(); printf("  LOW\n");
      bcm2835_gpio_write(LED, LOW);
      delay_us(250000);
      bcm2835_gpio_write(LED, HIGH);

      //wait until a packet has been received
      for (int j=0; ((read_reg(STATUS) & STATUS_RX_DR) == 0x00) & j<50; j++)
         delay_us(100);
      if ((read_reg(STATUS) & STATUS_RX_DR) == 0x00)
      {
         rt.puts(); printf("  Nope\n");
         continue;
      }

      HeartbeatMsg heartbeat;
      read_rx_payload((char*)&heartbeat, sizeof(heartbeat)); //read the packet into data
      write_reg(STATUS, STATUS_RX_DR); // clear data received bit
      
      heartbeat.decode();
      rt.puts();
      printf("  heartbeat:%d\n", heartbeat.t_ms);
      rt.step(heartbeat.t_ms);
      rt.puts(); printf("  stepped\n");

      // wait till next second
      struct timeval tv;
      rt.tv(tv);
      rt.puts(); printf("  delay_us %ld\n", 1000000 - tv.tv_usec);
      delay_us(1000000 - tv.tv_usec);
   }

   rpi_shutdown();
   return 0;
}

