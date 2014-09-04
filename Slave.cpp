#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

#include "Slave.hpp"
#include "ensemble.hpp"

extern RunTime runtime;

namespace msg=messages;
using namespace std;


string Slave::stream_header("id     #     t_tx    tx_dt   err    t_rx      rx_dt    NR    ver   Vcell      SOC    MMC    dt   nac   arc ");

int Slave::debug = 0;


Slave::Slave(unsigned _id, const string& _drill_id, const string& _student_name)
   : id(_id), my_line(0), pwm(15),
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

// Across this whole routine (i.e. dt_tx at the end), I see
// a delay of about 360 us for an tx that does not request
// ACK. Returns false if there was an ack requested and none was received.
int Slave::tx(unsigned repeat)
{
   using namespace nRF24L01;
   int rc=0;
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
      rc=3;
      return rc;
   }
   else if (clear_count)
      cout << "Slave " << id << " tx: took " << clear_count << " writes to reset status." << endl;

   int tx_read_cnt;
   for (int i=0; i<repeat; i++)
   {
      write_tx_payload(buff, sizeof(buff), (const char*)ensemble::slave_addr[id], ack);

      // It looks like for 1 retry, 400 us (> ~80 counts)
      // 2 retries 600 us (>~120 counts)
      // 3 retries 855 us (> ~170 counts)
      for(tx_read_cnt=0; tx_read_cnt<300; tx_read_cnt++)
      {
         status = read_reg(STATUS);
         if (status & STATUS_TX_DS)
            break;
         delay_us(5);
      }

      if (debug>1 && tx_read_cnt>85 && tx_read_cnt <300)
         cout << "Slave " << id << " tx: tx_read_cnt=" << tx_read_cnt << endl;

      if (status & STATUS_MAX_RT)
      {
         nack_cnt++;
         rc=2;
         write_reg(STATUS, STATUS_MAX_RT);
         // data doesn't get automatically removed...
         flush_tx();
      }
      else if (status & STATUS_TX_DS)
      {
         write_reg(STATUS, STATUS_TX_DS); // Clear the data sent notice
         // No need to resend if we get an ack...
         if (ack)
            break;
      }
      else
      {
         rc=1;
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
   return rc;
}


int Slave::rx(void)
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

   if (debug>1 && i>15)
      cout << "Slave " << id << " rx: rx_read_cnt=" << i << endl;

   if (i==100)
   {
      if (debug>1)
         cout << "Slave " << id << " rx: error " << endl;
      no_resp++;
      return 1;
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

   return 0;
}


void Slave::start_effect(uint8_t effect_id, uint32_t start_time, uint32_t duration, unsigned repeat)
{
   msg::encode_start_effect(buff, effect_id, start_time, duration);
   tx(repeat);
}


void Slave::set_tlc_ch(uint8_t* p, uint8_t  ch, uint16_t  value, unsigned repeat)
{
   if (ch >= 15)
      return;

   pwm[ch] = value;
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   msg::encode_set_tlc_ch(buff, ch, pwm[ch]);
   tx(repeat);
}


void Slave::set_tlc(uint8_t* p, uint16_t value[], unsigned repeat)
{
   for (int ch=0; ch<15; ch++)
      pwm[ch] = value[ch];
   set_pwm(repeat);
}


void Slave::heartbeat(unsigned repeat)
{
   msg::encode_heartbeat(buff, runtime.msec());
   tx(repeat);
}


int Slave::ping(unsigned repeat)
{
   msg::encode_ping(buff);
   int rc = tx(repeat);
   if (rc)
      return rc;
   return rx();
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
   // This only uses 4 bits to set each channel
   msg::encode_set_tlc(buff, &pwm[0]);
   tx(repeat);
   // This approach sends all 12 bits.
   if (false)
      for (int ch=0; ch<15; ch++)
      {
         for (int i=0; i<sizeof(buff); i++) buff[i]=0;
         msg::encode_set_tlc_ch(buff, ch, pwm[ch]);
         tx(repeat);
      }
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
       << "   "  << setw(6) << setprecision(2) << slave.soc
       << "  "   << setw(5) << slave.mmc
       << "  "   << setw(4) << slave.slave_dt;
   else
     s << "  "   << setw(8) << "-"
       << "  "   << setw(8) << "-"
       << "  "   << setw(4) << "-"
       << "    " << setw(3) << "-"
       << "  "   << setw(6) << "-"
       << "   "  << setw(6) << "-"
       << "  "   << setw(5) << "-"
       << "  "   << setw(4) << "-";
   s << "  " << setw(4) << slave.nack_cnt
     << "  " << setw(4) << slave.arc_cnt;
   return s;
}


void sort_by_dril_id(SlaveList& slave_list)
{
   map<string, Slave> sm;

   list<Slave>::const_iterator j;
   for (j=slave_list.begin(); j != slave_list.end(); j++)
      sm[j->drill_id] = *j;

   slave_list.clear();
   map<string, Slave>::const_iterator i;
   for (i=sm.begin(); i != sm.end(); i++)
      slave_list.push_back(i->second);
}


ostream& operator<<(ostream& s, const SlaveList& slave_list)
{
   if (slave_list.size())
      for (auto i=slave_list.begin(); i != slave_list.end(); i++)
         s << left << setw(3) << i->id << "  " << setw(3) << i->version
           << right << fixed
           << "  " << setw(6) << setprecision(3) << i->vcell << "V"
           << "  " << setw(5) << setprecision(2) << i->soc << "%"
           << "  " << i->student_name
           << endl;
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


SlaveList scan(SlaveList& slave_list, int tries)
{
   SlaveList found;
   for (int pass=0; slave_list.size() > 0 && pass < tries; pass++)
   {
      if (Slave::debug) 
         cout << "Start scan at " << runtime.sec() << " s." << endl;
      for (auto i = slave_list.begin(); i != slave_list.end(); i++)
      {
         if (i->ping() == 0)
            found.push_back(*i);
         if (Slave::debug)
         {
            if (i->t_rx) cout << "." << flush;
            else         cout << "x" << flush;
         }
      }
      if (Slave::debug)
         cout << endl
              << "Scan done at " << runtime.sec() << " s." << endl;;
   }

   return found;
}


SlaveList::iterator scan_some(SlaveList& slave_list,  SlaveList::iterator slave, int num_to_scan)
{
   if (slave_list.size() == 0)
      return slave;

   for (int n=0; n < num_to_scan; n++)
   {
      slave->ping();
      slave++;
      if (slave==slave_list.end())
         slave=slave_list.begin();
   }
   return slave;
}
