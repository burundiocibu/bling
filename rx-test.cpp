#include <cstdlib>
#include <cstdio>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

class OutState
{
public:
   OutState(uint8_t _pin)
   : pin(_pin)
   {
      bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
      bcm2835_gpio_write(pin, HIGH);
      state=HIGH;
   }

   inline void set_high()
   {
      if (state==LOW)
      {
         bcm2835_gpio_write(pin, HIGH);
         state=HIGH;
      }
   }

   inline void set_low()
   {
      if (state==HIGH)
      {
         bcm2835_gpio_write(pin, LOW);
         state=LOW;
      }
   }

private:
   uint8_t pin;
   uint8_t state;
};


using namespace std;
using namespace nRF24L01;

int main(int argc, char **argv)
{
   int debug=1;

   RunTime rt;
   setup();

   if (!configure_base())
   {
      printf("Failed to find nRF24l01. Exiting.\n");
      return -1;
   }

   configure_PRX();
   power_up_PRX();

   OutState led(RPI_GPIO_P1_07);

   // Set up the expected message length for pipe 0
   write_reg(RX_PW_P0, sizeof(messages::Heartbeat));

   struct timeval tv;
   while(1)
   {
      rt.tv(tv);
      if (tv.tv_usec <= 250000)
         led.set_low();
      else
         led.set_high();
      
      // see if we got something...
      if ((read_reg(STATUS) & STATUS_RX_DR) == 0x00)
      {
         bcm2835_delayMicroseconds(2000);
         continue;
      }

      uint32_t trx = rt.msec();
      messages::Heartbeat heartbeat;
      uint8_t buff[sizeof(heartbeat)];
      read_rx_payload((char*)buff, sizeof(heartbeat));
      write_reg(STATUS, STATUS_RX_DR); // clear data received bit
      heartbeat.decode(buff);

      int dt = heartbeat.t_ms - trx;
      printf("  heartbeat:%.3f, dt=%d ms\n", 1e-3*heartbeat.t_ms, dt);
      if (abs(dt)>10)
      {
         rt.step(heartbeat.t_ms);
         printf("  stepped %d ms\n", dt);
      }
   }

   shutdown();
   return 0;
}

