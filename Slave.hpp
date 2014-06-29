#ifndef _SLAVE_HPP
#define _SLAVE_HPP

#include <string>
#include <iostream>
#include <vector>
#include <list>

#include "rt_utils.hpp"
#include "messages.hpp"
#include "ensemble.hpp"

class Slave
{
public:
   Slave(unsigned _id=0, const std::string& _drill_id=std::string(), const std::string& _student_name=std::string());
   bool operator==(const Slave &) const;
   bool operator<(const Slave &) const;

   void slide_pwm_ch(unsigned ch, int dir);
   void slide_pwm(int dir);

   // Returns zero for no known errors
   int tx(unsigned repeat);
   int rx();

   // The following are higher level comands to the slaves
   // repeat indicates how many times the command will be sent
   int ping(unsigned repeat=1);
   void heartbeat(unsigned repeat=1);
   void all_stop(unsigned repeat=1);
   void reboot(unsigned repeat=1);
   void set_pwm(unsigned repeat=1);

   void start_effect(uint8_t effect_id, uint32_t start_time, uint32_t duration, unsigned repeat=1);
   void set_tlc_ch(uint8_t* p, uint8_t  ch, uint16_t  value, unsigned repeat=1);
   void set_tlc(uint8_t* p, uint16_t value[], unsigned repeat=1);

   static std::string stream_header;

   unsigned id;
   std::string drill_id;
   std::string student_name;

   std::vector<uint16_t> pwm;
   uint16_t tlc[3];  // Used to store what the slave reports as being the channel values...
   unsigned my_line; // Used to indicate where on screen this slave should be placed...
   bool ack; // whether to request acks on the messages to this slave

   unsigned long tx_cnt;
   uint8_t status;    // STATUS register after most recent operation
   unsigned tx_err;   // number of failed transmissions
   unsigned nack_cnt; // number of transmissions with ACK req set that don't get responses.
   unsigned no_resp;  // number of ping responses we expected but didn't get
   unsigned arc_cnt;  // running counter of ARC_CNT from OBSERVE_TX
   unsigned plos_cnt; // ditto but for PLOS_CNT
   unsigned tx_dt, rx_dt;  // time to complete tx/rx in usec
   uint64_t t_tx, t_rx; // time of most recent send and the time in the most recent received packet

   // These values are only filled in when there has been a response from a ping
   uint32_t t_ping; // slave time (msec)
   int slave_dt;    // estimate of slave clock offset in ms
   std::string version;
   uint16_t mmc;
   double soc, vcell;

   uint8_t buff[ensemble::message_size];
};

typedef std::list<Slave> SlaveList;

SlaveList read_slaves(const std::string filename);
SlaveList scan(SlaveList& slave_list, int tries=1);

std::ostream& operator<<(std::ostream& s, const Slave& slave);
std::ostream& operator<<(std::ostream& s, const SlaveList& slave_list);

#endif
