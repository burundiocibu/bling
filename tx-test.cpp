#include <cstdlib>
#include <cstdio>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

using namespace std;

int main(int argc, char **argv)
{
   RunTime rt;
   nRF24L01::setup();

   if (!nRF24L01::configure_base())
   {
      printf("Failed to find nRF24L01. Exiting.\n");
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::power_up_PTX();
   nRF24L01::flush_tx();
   nRF24L01::write_reg(nRF24L01::RX_PW_P0, messages::message_size);

   const int LED=RPI_GPIO_P1_07;
   bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);

   messages::Heartbeat heartbeat;
   uint8_t buff[messages::message_size];
   for (int i=0; ; i++)
   {
      bcm2835_gpio_write(LED, LOW);

      heartbeat.t_ms = rt.msec();
      heartbeat.encode(buff);
      nRF24L01::write_tx_payload(buff, sizeof(buff));
      nRF24L01::pulse_CE();

      printf(" #%d %ld\n", i, heartbeat.t_ms);
      for(int j=0; ((nRF24L01::read_reg(nRF24L01::STATUS) & nRF24L01::STATUS_TX_DS)== 0x00) && j<100; j++)
            bcm2835_delayMicroseconds(10);;
      nRF24L01::write_reg(nRF24L01::STATUS, nRF24L01::STATUS_TX_DS); //Clear the data sent notice

      bcm2835_delayMicroseconds(125000);
      bcm2835_gpio_write(LED, HIGH);

      // wait till next second
      struct timeval tv;
      rt.tv(tv);
      //rt.puts(); printf("  delay_us %ld\n", 1000000 - tv.tv_usec);
      bcm2835_delayMicroseconds(1001000 - tv.tv_usec);
   }

   nRF24L01::shutdown();
   return 0;
}

