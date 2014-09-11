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

Master_Server::Master_Server(int _debug, string& slave_list_fn, string& slave_main_fn)
   : debug(_debug), slave_main_fn(slave_main_fn)
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

   Slave broadcast(0);

   if (debug)
      cout << broadcast.stream_header << endl;
   
   if (slave_list_fn.size())
      all = read_slaves(slave_list_fn);
   else
      for (int id=1; id < ensemble::max_slave; id++)
         all.push_back(Slave(id));

   Slave::debug = debug;

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
      for (auto i=all.begin(); i!=all.end(); i++)
         if (i->id == slave_id)
            return &(*i);

   if (debug>1)
      cout << "Slave " << slave_id << " not found in list." << endl;

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


void Master_Server::set(bling_pb::slave_list::slave_info* s, const Slave& slave)
{
   double age = slave.t_rx ? (runtime.usec() - slave.t_rx) * 1e-6 : 999999;

   s->set_slave_id(slave.id);
   s->set_student_name(slave.student_name);
   s->set_drill_id(slave.drill_id);
   s->set_vcell(slave.vcell);
   s->set_version(slave.version);
   s->set_age(age);
   s->set_soc(slave.soc);
   s->set_mmc(slave.mmc);
   s->add_tlc(slave.tlc[0]);
   s->add_tlc(slave.tlc[1]);
   s->add_tlc(slave.tlc[2]);
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
      SlaveList wow = ::scan(all);
      cout << "Found " << wow.size() << " slaves out of "
           << all.size() << " defined." << endl;
   }
   bling_pb::slave_list sl;
   for (auto i=all.begin(); i!=all.end(); i++)
   {
      if (gsl.has_slave_id() && i->id != gsl.slave_id())
         continue;

      double age = (runtime.usec() - i->t_rx) * 1e-6;
      if (gsl.active() && (i->t_rx == 0 || age > 30))
         continue;

      set(sl.add_slave(), *i);
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

   bling_pb::slave_list sl;
   if (slave->t_rx != 0)
      set(sl.add_slave(), *slave);

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
   static long hb_time = -hb_dt;
   if (runtime.msec() - hb_time >= hb_dt)
   {
      hb_time += hb_dt;

      broadcast.heartbeat();
      if (debug & 0x4)
         cout << broadcast << endl;
   }

   static SlaveList::iterator scanner = all.begin();

   const long dump_dt = 60*1000;
   static long dump_time = 0;
   scanner = scan_some(all, scanner, 3);
   if (debug)
   {
      if (runtime.msec() - dump_time >= dump_dt)
      {
         cout << "rt=" << runtime.sec() << " s ---------------------" << endl;
         dump_time += dump_dt;
         for (auto i=all.begin(); i!=all.end(); i++)
            if (i->t_rx == 0)
               continue;
            else
               cout << *i << endl;
      }
   }
}


string Master_Server::program_slave(string& msg)
{
   bling_pb::program_slave ps;
   if (!ps.ParseFromString(msg))
      return nak;

   Slave* slave = find_slave(ps.slave_id());
   if (slave == NULL)
      return nak;

   slave->program(slave_main_fn);
   return ack;
}
