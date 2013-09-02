#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"
#include "ensemble.hpp"

using namespace std;

RunTime runtime;

unsigned slave=0;
void nrf_tx(uint8_t *buff, size_t len, unsigned slave);
void nrf_rx();
void hexdump(uint8_t* buff, size_t len);

int debug;

int main(int argc, char **argv)
{
   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "di:s:")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 's': slave = atoi(optarg); break;
         default:
            printf("Usage %s -i fn -s slave_no [-d]\n", argv[0]);
            exit(-1);
      }


   nRF24L01::channel = ensemble::default_channel;
   memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
   memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
   memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[2], nRF24L01::addr_len);

   nRF24L01::setup();

   if (!nRF24L01::configure_base())
   {
      printf("Failed to find nRF24L01. Exiting.\n");
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

   const int LED=RPI_GPIO_P1_07;
   bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);

   uint8_t buff[ensemble::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   uint16_t red=0,green=0,blue=0;
   unsigned hb_count=0;
   uint32_t last_hb=0;

   for (int i=0; ; i++)
   {
      uint32_t t = runtime.msec();
      if (t % 1000 <250)
         bcm2835_gpio_write(LED, LOW);
      else 
         bcm2835_gpio_write(LED, HIGH);

      if (t - last_hb > 990)
      {
         messages::encode_heartbeat(buff, t);
         nrf_tx(buff, sizeof(buff), slave);
         hb_count++;
         last_hb = t;
         bcm2835_delayMicroseconds(100);
         messages::encode_start_effect(buff, 2, t, 20000);
         nrf_tx(buff, sizeof(buff), slave);
      }
      bcm2835_delayMicroseconds(10000);
   }

   nRF24L01::shutdown();
   bcm2835_gpio_write(LED, HIGH);
   return 0;
}



void nrf_tx(uint8_t *buff, size_t len, unsigned slave)
{
   using namespace nRF24L01;

   static unsigned ack_err=0;
   static unsigned tx_err=0;
   bool ack = slave != 0;

   write_tx_payload(buff, len, (const char*)ensemble::slave_addr[slave], ack);

   uint8_t status;
   int j;
   for(j=0; j<100; j++)
   {
      status = read_reg(STATUS);
      if (status & STATUS_TX_DS)
         break;
      delay_us(5);
   }

   if (status & STATUS_MAX_RT)
   {
      ack_err++;
      write_reg(STATUS, STATUS_MAX_RT);
      // data doesn't automatically removed...
      flush_tx();
   }
   else if (status & STATUS_TX_DS)
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
   else
      tx_err++;

   uint8_t obs_tx = read_reg(OBSERVE_TX);
   //printf("%3d  %3d       %02x    %3d", j, tx_err, obs_tx, ack_err);
   //printf("%8.3f  ", 0.001* runtime.msec());
   //hexdump(buff, len);
}



void nrf_rx(void)
{
   using namespace nRF24L01;
   uint8_t buff[ensemble::message_size];
   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   delay_us(1000);
   set_CE();

   int i;
   for (i=0; i<100; i++)
   {
      if (read_reg(STATUS) & STATUS_RX_DR)
      {
         read_rx_payload((char*)buff, ensemble::message_size, pipe);
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }
      delay_us(200);
   }

   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();

   uint32_t t_rx;
   uint16_t soc,vcell;
   uint8_t id;
   uint8_t* p = buff;
   p = messages::decode_var<uint8_t>(p, id);
   p = messages::decode_var<uint32_t>(p, t_rx);
   p = messages::decode_var<uint16_t>(p, vcell);
   p = messages::decode_var<uint16_t>(p, soc);
   printf("%8.3f ", 0.001*t_rx);
   soc = 0xff & (soc >> 8);
   printf("%2d %3d %1.3f mv %d%%  ", id, i, 1e-3*vcell, soc);
   hexdump(buff, ensemble::message_size);
}


void hexdump(uint8_t* buff, size_t len)
{
   for (int i = 0; i <len; i++)
      printf("%.2X ", buff[i]);
   printf("\n");
}
