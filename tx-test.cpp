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
   configure_PTX();
   power_up_PTX();
   write_data(FLUSH_TX);

   const int LED=RPI_GPIO_P1_07;
   bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);

   HeartbeatMsg heartbeat;
   for (int i=0; ; i++)
   {
      rt.puts(); printf("  LOW\n");
      bcm2835_gpio_write(LED, LOW);
      delay_us(250000);
      bcm2835_gpio_write(LED, HIGH);

      heartbeat.encode(rt.msec());
      write_tx_payload(&heartbeat, sizeof(heartbeat));
      pulse_CE();
      heartbeat.decode();

      rt.puts(); printf(" #%d %ld\n", i, heartbeat.t_ms);
      for(int j=0; ((read_reg(STATUS) & STATUS_TX_DS)== 0x00) && j<100; j++)
         delay_us(10);
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice

      // wait till next second
      struct timeval tv;
      rt.tv(tv);
      rt.puts(); printf("  delay_us %ld\n", 1000000 - tv.tv_usec);
      delay_us(1000000 - tv.tv_usec);
   }

   rpi_shutdown();
   return 0;
}

