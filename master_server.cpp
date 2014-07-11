#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>

#include <bcm2835.h>

#include "Lock.hpp"
#include "rt_utils.hpp"
#include "nrf24l01.hpp"

#include "server_msg.pb.h"

#include "master_server.hpp"

using namespace std;

Master_Server::Master_Server(int _debug)
   : debug(_debug)
{
   Lock lock;

   nRF24L01::channel = ensemble::default_channel;
   memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
   memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
   memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[2], nRF24L01::addr_len);

   nRF24L01::setup();

   if (!nRF24L01::configure_base())
   {
      cout << "Failed to find nRF24L01. Exiting." << endl;
      exit(-1);
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

   // Reset all the slaves, and give them a chance to come back up
   Slave broadcast(0);
   broadcast.reboot();
   bcm2835_delayMicroseconds(50000);

   if (debug)
      cout << broadcast.stream_header << endl;
   
   for (int id=1; id < ensemble::num_slaves; id++)
      all.push_back(Slave(id));

   bling_pb::nak nak_msg;
   nak_msg.SerializeToString(&nak);
   bling_pb::ack ack_msg;
   ack_msg.SerializeToString(&ack);
}

Master_Server::~Master_Server()
{
   nRF24L01::shutdown();
}


Slave* Master_Server::find_slave(unsigned slave_id)
{
   if (slave_id == 0)
      return &broadcast;
   else
      for (auto i=found.begin(); i!=found.end(); i++)
         if (i->id == slave_id)
            return &(*i);
   return NULL;
}


string Master_Server::send_all_stop(string& msg)
{
   bling_pb::send_all_stop sas;
   if (!sas.ParseFromString(msg))
      return nak;

   Slave* slave = find_slave(sas.slave_id());
   if (slave == NULL)
      return nak;

   slave->all_stop(sas.repeat());

   return ack;
}

string Master_Server::get_slave_list(string& msg)
{
   bling_pb::get_slave_list gsl;
   if (!gsl.ParseFromString(msg))
      return nak;
   if (debug>2)
      cout << "body: " << gsl.ShortDebugString() << endl;
   if (gsl.scan())
   {
      if (all.size())
      {
         SlaveList more = ::scan(all);
         if (more.size())
         {
            cout << "Added " << more.size() << " slaves, "
                 << all.size() << " not found." << endl;
            found.splice(found.end(), more);
         }
      }
      for (auto i=found.begin(); i!=found.end(); i++)
      {
         i->ping();
         cout << *i << endl;
      }
   }
   bling_pb::slave_list sl;
   for (auto i=found.begin(); i!=found.end(); i++)
   {
      if (gsl.has_slave_id() && i->id != gsl.slave_id())
         continue;
      bling_pb::slave_list::slave_info* slave=sl.add_slave();
      slave->set_slave_id(i->id);
      slave->set_vcell(i->vcell);
      slave->set_version(i->version);
      slave->set_t_rx(i->t_rx);
      slave->set_soc(i->soc);
      slave->set_mmc(i->mmc);
      slave->add_tlc(i->tlc[0]);
      slave->add_tlc(i->tlc[1]);
      slave->add_tlc(i->tlc[2]);
   }

   if (debug)
      cout << "slave_list:" << sl.ShortDebugString() << endl;

   string s1,s2;
   sl.SerializeToString(&s2);
   bling_pb::header header;
   header.set_msg_id(bling_pb::header::SLAVE_LIST);
   header.set_len(s2.size());
   header.SerializeToString(&s1);
   return s1+s2;
}

string Master_Server::set_slave_tlc(string& msg)
{
   bling_pb::set_slave_tlc sst;
   if (!sst.ParseFromString(msg))
      return nak;
   if (sst.tlc_size() == 0)
      return nak;

   if (debug)
      cout << "set_slave_tlc:" << sst.ShortDebugString() << endl;

   Slave* slave = find_slave(sst.slave_id());
   if (slave == NULL)
      return nak;


   for (int i=0; i<sst.tlc_size(); i++)
      slave->pwm[i] = sst.tlc(i);

   slave->set_pwm(sst.repeat());
   return ack;
}

string Master_Server::start_effect(string& msg)
{
   bling_pb::start_effect se;
   if (!se.ParseFromString(msg))
      return nak;

   if (debug)
      cout << "start_effect:" << se.ShortDebugString() << endl;

   Slave* slave = find_slave(se.slave_id());
   if (slave == NULL)
      return nak;

   if (debug)
      cout << "start_effect: found slave, id=" << slave->id << endl;
   
   slave->start_effect(se.effect_id(), runtime.msec() + se.start_time(), se.duration(), se.repeat());
   return ack;
}

string Master_Server::ping_slave(string& msg)
{
   bling_pb::ping_slave ps;
   if (!ps.ParseFromString(msg))
      return nak;

   Slave* slave = find_slave(ps.slave_id());
   if (slave == NULL)
      return nak;

   slave->ping(ps.repeat());
   return ack;
}

string Master_Server::reboot_slave(string& msg)
{
   bling_pb::reboot_slave rs;
   if (!rs.ParseFromString(msg))
      return nak;

   Slave* slave = find_slave(rs.slave_id());
   if (slave == NULL)
      return nak;

   slave->reboot(rs.repeat());
   return ack;
}

void Master_Server::heartbeat()
{
   // All times in ms
   const long hb_dt = 1000;
   const long ping_dt = 5000;
   static long hb_time = -hb_dt;
   static long ping_time = -ping_dt;

   long t = runtime.msec();
   if (t - hb_time >= hb_dt)
   {
      hb_time += hb_dt;

      broadcast.heartbeat();
      if (debug & 0x4)
         cout << broadcast << endl;
   }

   static SlaveList::iterator next_found = found.begin();
   if (next_found == found.end())
      next_found = found.begin();
   if (next_found != found.end())
   {
      if (t - next_found->t_rx/1000 > ping_dt)
      {
         next_found->ping();
         if (debug & 0x2)
            cout << *next_found << endl;
         long age = t - next_found->t_rx/1000;
         if (age > 4 * ping_dt && debug)
            cout << "Lost contact with " << next_found->id << " age " << age << endl;  
         next_found++;
      }
   }

   static long pass = 0;
   static long last_scan=0;
   if (pass > 2 && t - last_scan < 100)
      return;

   static SlaveList::iterator next = all.begin();
   if (next == all.end())
      return;
   last_scan = t;
   if (next->ping() == 0)
   {
      if (debug & 0x2)
         cout << *next << endl;
      found.push_back(*next);
      next = found.erase(next);
   }
   else
   {
      next++;
      if (next == all.end())
      {
         next = all.begin();
         pass++;
      }
   }
}
