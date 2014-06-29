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

Master_Server::Master_Server()
{
   debug = 1;
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
   
   cout << broadcast.stream_header << endl;
   
   for (int id=1; id < ensemble::num_slaves; id++)
      all.push_back(Slave(id));
}

Master_Server::~Master_Server()
{
   nRF24L01::shutdown();
}

string Master_Server::send_all_stop(string& msg)
{
   return string();
}

string Master_Server::get_slave_list(string& msg)
{
   bling_pb::get_slave_list gsl;
   if (gsl.ParseFromString(msg))
   {
      if (debug>2)
         cout << "body: " << gsl.ShortDebugString() << endl;
            
      bling_pb::slave_list sl;
      for (auto i=found.begin(); i!=found.end(); i++)
      {
         bling_pb::slave_list::slave_info* slave=sl.add_slave();
         slave->set_slave_id(i->id);
         slave->set_vcell(i->vcell);
      }
      if (debug>1)
         cout << "slave_list::" << sl.ShortDebugString() << endl;

      string s1,s2;
      sl.SerializeToString(&s2);
      bling_pb::header header;
      header.set_msg_id(bling_pb::header::SLAVE_LIST);
      header.set_len(s2.size());
      header.SerializeToString(&s1);
      return s1+s2;
   }
   return string();
}

string Master_Server::set_slave_tlc(string& msg)
{
   return string();
}

string Master_Server::start_effect(string& msg)
{
   return string();
}

string Master_Server::ping_slave(string& msg)
{
   return string();
}

string Master_Server::reboot_slave(string& msg)
{
   return string();
}

void Master_Server::heartbeat()
{
   broadcast.heartbeat();
   if (debug)
      cout << broadcast << endl;
}

void Master_Server::scan()
{
   static unsigned scan_count=0;
   static unsigned call_count=0;
   call_count++;

   if (debug)
      cout << "scan" << endl;
   if (all.size() && scan_count==0)
   {
      scan_count++;
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
