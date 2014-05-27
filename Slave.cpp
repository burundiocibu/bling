#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

#include "Slave.hpp"
#include "ensemble.hpp"

extern RunTime runtime;

unsigned Slave::slave_count = 0;

namespace msg=messages;
using namespace std;

Slave::Slave(unsigned _id, const string& _drill_id, const string& _student_name)
   : id(_id), my_count(slave_count++), pwm(15),
     drill_id(_drill_id), student_name(_student_name),
     tx_cnt(0), tx_err(0), nack_cnt(0),
     no_resp(0), tx_dt(0), rx_dt(0),
     t_tx(0), t_rx(0), t_ping(0), slave_dt(0), mmc(0),
     major_version(0), minor_version(0),
     arc_cnt(0), plos_cnt(0),
     vcell(0), soc(0)
{
   if (id >= ensemble::num_slaves)
      cout << "Invalid slave " << id << endl;

   ack = id != 0;
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
};


bool Slave::operator==(const Slave &slave) const
{
   return this->id == slave.id;
}


bool Slave::operator<(const Slave& slave) const
{
   return this->id < slave.id;
}


void Slave::tx(unsigned repeat)
{
   using namespace nRF24L01;

   tx_cnt++;
   t_tx = runtime.usec();

   // Wait for the nrf to become ready.
   flush_tx();
   int clear_count=0;
   while (read_reg(STATUS) != 0xe && clear_count < 100)
   {
      write_reg(STATUS, 0xf0);
      delay_us(10);
   }
   if (clear_count == 100)
   {
      cout << "Slave " << id << " could not clear status of nrf." << endl;
      tx_err++;
      return;
   }
   else if (clear_count)
      cout << "Slave " << id << " took " << clear_count << " writes to reset status." << endl;

   int tx_read_cnt;
   for (int i=0; i<repeat; i++)
   {
      write_tx_payload(buff, sizeof(buff), (const char*)ensemble::slave_addr[id], ack);

      for(tx_read_cnt=0; tx_read_cnt<200; tx_read_cnt++)
      {
         status = read_reg(STATUS);
         if (status & STATUS_TX_DS)
            break;
         delay_us(5);
      }

      if (status & STATUS_MAX_RT)
      {
         nack_cnt++;
         write_reg(STATUS, STATUS_MAX_RT);
         // data doesn't get automatically removed...
         flush_tx();
      }
      else if (status & STATUS_TX_DS)
      {
         write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
         // No need to resend if we get an ack...
         if (ack)
            break;
      }
      else
      {
         cout << "Slave " << id << " tx error, tx_read_count=" << tx_read_cnt << endl;
         tx_err++;
      }

      if (i<repeat-1)
         delay_us(2500);
   }

   tx_dt = runtime.usec() - t_tx;

   if (ack)
   {
      uint8_t obs_tx = read_reg(OBSERVE_TX);
      arc_cnt += 0xf & obs_tx;
      plos_cnt += 0xf & (obs_tx>>4);
   }
}


void Slave::rx(void)
{
   using namespace nRF24L01;

   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
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

   if (i==100)
   {
      cout << "Slave " << id << " rx error, No response after " << i << " reads." << endl;
      no_resp++;
      return;
   }

   t_rx = runtime.usec();

   uint16_t slave_id;
   uint8_t freshness_count;

   msg::decode_status(buff, slave_id, t_ping, major_version, minor_version,
                      vcell, soc, mmc, freshness_count);

   soc = 0xff & (soc >> 8);
   rx_dt = t_rx - t_tx;
   slave_dt = t_ping - t_rx/1000;
}


void Slave::heartbeat(unsigned repeat)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_heartbeat(buff, runtime.msec());
   tx(repeat);
}


void Slave::ping(unsigned repeat)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_ping(buff);
   tx(repeat);
   rx();
}

void Slave::all_stop(unsigned repeat)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_all_stop(buff);
   tx(repeat);
}


void Slave::reboot(unsigned repeat)
{
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_reboot(buff);
   tx(repeat);
}


void Slave::slide_pwm_ch(unsigned ch, int dir)
{
   uint16_t& v = pwm[ch];
   if (dir>0)
   {
      if (v==0)
         v=1;
      else
         v = (v << 1) + 1;
      if (v >=4096)
         v=4095;
   }
   else
   {
      if (v>0)
         v >>= 1;
   }
};


void Slave::slide_pwm(int dir)
{
   for (int i=0; i<15; i++)
      slide_pwm_ch(i, dir);
}


void Slave::set_pwm(unsigned repeat)
{
   for (int ch=0; ch<15; ch++)
   {
      for (int i=0; i<sizeof(buff); i++) buff[i]=0;
      msg::encode_set_tlc_ch(buff, ch, pwm[ch]);
      tx(repeat);
   }
};



std::ostream& operator<<(std::ostream& s, const Slave& slave)
{
   s << left << setw(2) << slave.id
     << "  " << setw(4) << slave.tx_cnt
     << "  " << setw(8) << 1e-6*slave.t_tx
     << hex << setfill('0')
     << "  " << setw(3) << slave.pwm[0]
     << " " << setw(3) << slave.pwm[1]
     << " " << setw(3) << slave.pwm[2]
     << dec
     << "  " << setw(6) << 1e-3*slave.tx_dt
     << "  " << setw(3) << slave.tx_err
     << "  " << setw(8) << 1e-6*slave.t_rx
     << "  " << setw(8) << 1e-3*slave.rx_dt
     << "  " << setw(4) << slave.no_resp
     << "  " << slave.major_version << "." << slave.minor_version
     << "  " << setw(5) << 1e-3*slave.vcell
     << "  " << setw(3) << slave.soc
     << "  " << setw(5) << slave.mmc
     << "  " << setw(4) << slave.slave_dt
     << hex
     << "  staus:" << slave.status
     << ", nac_cnt:" << slave.nack_cnt
     << ", arc_cnt:" << slave.arc_cnt
     << ", plos_cnt:" << slave.plos_cnt
     << dec << endl;
   return s;
}
