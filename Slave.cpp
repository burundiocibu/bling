#include <iostream>
#include <fstream>
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
bool Slave::header_output = false;

namespace msg=messages;
using namespace std;

Slave::Slave(unsigned _id, const string& _drill_id, const string& _student_name)
   : id(_id), my_count(slave_count++), pwm(15),
     drill_id(_drill_id), student_name(_student_name),
     tx_cnt(0), tx_err(0), nack_cnt(0),
     no_resp(0), tx_dt(0), rx_dt(0),
     t_tx(0), t_rx(0), t_ping(0), slave_dt(0), mmc(0),
     version("unk"),
     arc_cnt(0), plos_cnt(0),
     vcell(0), soc(0)
{
   if (id >= ensemble::num_slaves)
      cout << "Invalid slave " << id << endl;

   for (int i=0; i<3; i++) tlc[i]=0;

   stream_header =   "id     #     t_tx    tx_dt   err    t_rx      rx_dt    NR    ver   Vcell     SOC    MMC    dt   nac   arc ";

   ack = id != 0;
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
      cout << "Slave " << id << " tx: could not clear status of nrf." << endl;
      tx_err++;
      return;
   }
   else if (clear_count)
      cout << "Slave " << id << " tx: took " << clear_count << " writes to reset status." << endl;

   int tx_read_cnt;
   for (int i=0; i<repeat; i++)
   {
      write_tx_payload(buff, sizeof(buff), (const char*)ensemble::slave_addr[id], ack);

      // It looks like for 1 retry, 400 us
      // 2 retries 600 us
      // 3 retries 855 us
      for(tx_read_cnt=0; tx_read_cnt<1000; tx_read_cnt++)
      {
         status = read_reg(STATUS);
         if (status & STATUS_TX_DS)
            break;
         delay_us(5);
      }

      if (tx_read_cnt>35)
         cout << "Slave " << id << " tx: tx_read_cnt=" << tx_read_cnt << endl;

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
         cout << "Slave " << id << " tx: error" << endl;
         tx_err++;
      }

      if (i<repeat-1)
         delay_us(2500);
   }

   if (ack)
   {
      uint8_t obs_tx = read_reg(OBSERVE_TX);
      arc_cnt += 0xf & obs_tx;
      plos_cnt += 0xf & (obs_tx>>4);
   }

   flush_tx();

   tx_dt = runtime.usec() - t_tx;
}


void Slave::rx(void)
{
   using namespace nRF24L01;

   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   // have been using 1500 us here with marginal results
   // have to be careful that the master start listening prior to
   // the slave transmitting the response.
   delay_us(100);
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

   delay_us(100);  // Not sure this is useful...

   if (i>15)
      cout << "Slave " << id << " rx: rx_read_cnt=" << i << endl;

   if (i==100)
   {
      cout << "Slave " << id << " rx: error " << endl;
      no_resp++;
      return;
   }

   t_rx = runtime.usec();

   uint16_t slave_id, _soc, _vcell;
   uint8_t freshness_count;
   int8_t major, minor;

   msg::decode_status(buff, slave_id, t_ping, major, minor,
                      _vcell, _soc, mmc, freshness_count, tlc);
   version = std::to_string(major) + "." + std::to_string(minor);
   soc = 0xff & (_soc >> 8);
   soc += (0xff & _soc) / 256.0;
   vcell = 1e-3 * _vcell;
   rx_dt = t_rx - t_tx;
   slave_dt = t_ping - t_rx/1000;

}


void Slave::heartbeat(unsigned repeat)
{
   msg::encode_heartbeat(buff, runtime.msec());
   tx(repeat);
}


void Slave::ping(unsigned repeat)
{
   msg::encode_ping(buff);
   tx(repeat);
   rx();
}

void Slave::all_stop(unsigned repeat)
{
   msg::encode_all_stop(buff);
   tx(repeat);
}


void Slave::reboot(unsigned repeat)
{
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
   msg::encode_set_tlc(buff, &pwm[0]);
   tx(repeat);
/*
   for (int ch=0; ch<15; ch++)
   {
      for (int i=0; i<sizeof(buff); i++) buff[i]=0;
      msg::encode_set_tlc_ch(buff, ch, pwm[ch]);
      tx(repeat);
   }
*/
};


std::ostream& operator<<(std::ostream& s, const Slave& slave)
{
   s << left << setw(3) << slave.id
     << right
     << " " << setw(4) << slave.tx_cnt
     << right
     << fixed << setprecision(3)
     << "  " << setw(8) << 1e-6*slave.t_tx
     << "  " << setw(6) << 1e-3*slave.tx_dt
     << "  " << setw(3) << slave.tx_err;
   if (slave.t_rx)
     s << "  "   << setw(8) << 1e-6*slave.t_rx
       << "  "   << setw(8) << 1e-3*slave.rx_dt
       << "  "   << setw(4) << slave.no_resp
       << "    " << setw(3) << slave.version
       << "  "   << setw(6) << setprecision(3) << slave.vcell
       << "   "  << setw(5) << setprecision(2) << slave.soc
       << "  "   << setw(5) << slave.mmc
       << "  "   << setw(4) << slave.slave_dt;
   else
     s << "  "   << setw(8) << "-"
       << "  "   << setw(8) << "-"
       << "  "   << setw(4) << "-"
       << "    " << setw(3) << "-"
       << "  "   << setw(6) << "-"
       << "   "  << setw(5) << "-"
       << "  "   << setw(5) << "-"
       << "  "   << setw(4) << "-";
   s << "  " << setw(4) << slave.nack_cnt
     << "  " << setw(4) << slave.arc_cnt;
   return s;
}


string trim(const string& str, const string& whitespace = " \t")
{
   size_t i = str.find_first_not_of(whitespace);
   if (i == string::npos)
      return "";
   size_t j = str.find_last_not_of(whitespace);
   return str.substr(i, j-i+1);
}


SlaveList read_slaves(const std::string filename)
{
   SlaveList slaves;
   ifstream s(filename);
   string line;
   while (getline(s, line))
   {
      if (line[0] == '#')
         continue;
      size_t i=line.find(',');
      size_t j=line.find(',', i+1);
      string name(trim(line.substr(0,i)));
      int slave_id=stoi(line.substr(i+1,j-i-1));
      string drill_id(trim(line.substr(j+1)));

      slaves.push_back(Slave(slave_id, drill_id, name));
   }
   return slaves;
}
