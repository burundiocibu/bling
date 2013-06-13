#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ncurses.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

#include "Slave.hpp"


extern RunTime runtime;


void Slave::tx(uint8_t *buff, size_t len)
{
   using namespace nRF24L01;
   if (slave_no >= num_chan)
      exit(-1);

   t_tx = runtime.msec();
   tx_cnt++;
   write_tx_payload(buff, len, slave_no);

   uint64_t t0=runtime.usec();
   for (int i=0; i < 200; i++)
   {
      uint8_t status = read_reg(STATUS);
      if (status & STATUS_MAX_RT)
      {
         nack_cnt++;
         write_reg(STATUS, STATUS_MAX_RT);
         // data doesn't automatically removed...
         flush_tx();
         break;
      }
      else if (status & STATUS_TX_DS)
      {
         tx_dt = runtime.usec() - t0;
         write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
         break;
      }
      delay_us(10);
   }

   uint8_t obs_tx = read_reg(OBSERVE_TX);
   retry_cnt += obs_tx & 0x0f;
}


void Slave::rx()
{
   using namespace nRF24L01;
   uint8_t buff[messages::message_size];
   memset(buff, 0, sizeof(buff));

   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   delay_us(1200);
   set_CE();

   uint64_t t0=runtime.usec();
   for (int i=0; i<200; i++)
   {
      uint8_t status = read_reg(STATUS);
      if (status & STATUS_RX_DR)
      {
         read_rx_payload((char*)buff, messages::message_size, pipe);
         rx_dt = runtime.usec() - t0;
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         uint8_t* p = buff;
         p = messages::decode_var<uint8_t>(p, id);
         p = messages::decode_var<uint32_t>(p, t_rx);
         p = messages::decode_var<int32_t>(p, batt_cap);
         break;
      }

      if (runtime.usec() - t0 > 2000)
      {
         no_resp++;
         rx_dt = runtime.usec() - t0;
         break;
      }
      delay_us(200);
   }

   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();
}


std::string Slave::status() const
{
   if (tx_cnt == 0)
      return std::string();
   float nack_pct = 100.0 * nack_cnt/tx_cnt;
   char buff[160];
   sprintf(buff, "%3d  %9.3f  %5d  %5d   %5d  %5d(%7.3f)  %9.3f  %5d  %6d  ",
           slave_no, 0.001*t_tx, tx_dt, tx_cnt, retry_cnt, nack_cnt, nack_pct, 0.001*t_rx, rx_dt, no_resp);
   if (id==6)
      sprintf(buff+80, "%5d", batt_cap);
   return std::string(buff);
}
