#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <bcm2835.h>
#include "Lock.hpp"
#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "Slave.hpp"

#include "server_msg.pb.h"
#include "pb_helpers.hpp"

using boost::asio::ip::tcp;
using namespace std;

RunTime runtime;

int debug;

class session
{
public:
   session(boost::asio::io_service& io_service, SlaveList* _all, SlaveList* _found)
      : socket_(io_service),
        rx_buff(max_length),
        all(_all), found(_found)
   {
      if (debug)
         cout << "session::session()" << endl;
   }

   ~session()
   {
      if (debug)
         cout << "session::~session()" << endl;
   }

   tcp::socket& socket()
   {
      return socket_;
   }

   void start()
   {
      socket_.async_read_some(boost::asio::buffer(rx_buff),
                              boost::bind(&session::handle_read, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
   }

private:
   void handle_read(const boost::system::error_code& error,
                    size_t bytes_transferred)
   {
      if (!error)
      {
         if (debug>2)
         {
            cout << "session::handle_read: " << bytes_transferred << " bytes received."<< endl;
            hexdump(&rx_buff[0], bytes_transferred);
         }

         string s1(rx_buff.begin(), rx_buff.begin()+header_length);
         bling_pb::header hdr;
         if (hdr.ParseFromString(s1))
         {
            if (debug>2)
               cout << "header: " << hdr.ShortDebugString() << endl;
         }
         else
            cout << "error decoding header";

         string s2(rx_buff.begin()+header_length, rx_buff.begin()+header_length+hdr.len());

         switch (hdr.msg_id())
         {
            case bling_pb::header::GET_SLAVE_LIST: get_slave_list(s2); break;
            case bling_pb::header::SET_SLAVE_TLC:  set_slave_tlc(s2);  break;
            default:
               cout << "Unknown message:" << endl;
               hexdump(s2);
               time_t now = time(0);
               string t(ctime(&now));
               schedule_send(t);
         }

         // Always schedule the next read
         socket_.async_read_some(boost::asio::buffer(rx_buff),
                                 boost::bind(&session::handle_read, this,
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
      }
      else
      {
         delete this;
      }
   }


   void schedule_send(const string& msg)
   {
      if (msg.size()==0)
         return;
      if (debug>2)
      {
         cout << "schedule_send: "<< msg.size() << endl;
         hexdump(msg);
      }
      // copy the message to a persistant buffer.
      tx_buff = msg;
      boost::asio::async_write(socket_,
                               boost::asio::buffer(tx_buff.c_str(), tx_buff.size()),
                               boost::bind(&session::handle_write, this,
                                           boost::asio::placeholders::error));
   }


   void handle_write(const boost::system::error_code& error)
   {
      if (!error)
      {
         if (debug>2)
            cout << "session::handle_write"<< endl;
      }
      else
         delete this;
   }


   void send_all_stop(std::string& msg)
   {}

   void get_slave_list(std::string& msg)
   {
      bling_pb::get_slave_list gsl;
      if (gsl.ParseFromString(msg))
      {
         if (debug>2)
            cout << "body: " << gsl.ShortDebugString() << endl;
            
         bling_pb::slave_list sl;
         for (auto i=found->begin(); i!=found->end(); i++)
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
         schedule_send(s1+s2);
      }
   }

   void set_slave_tlc(std::string& msg)
   {
      bling_pb::set_slave_tlc sst;
   }
   
   void start_effect(std::string& msg)
   {}

   void ping_slave(std::string& msg)
   {}

   void reboot_slave(std::string& msg)
   {}

   tcp::socket socket_;
   string tx_buff;
   vector<char> rx_buff;

   SlaveList* all;
   SlaveList* found;
};


class server
{
public:
   server(boost::asio::io_service& io_service, short port,
          SlaveList* _all, SlaveList* _found)
      : io_service_(io_service),
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        all(_all), found(_found)
   {
      start_accept();
   }

private:
   void start_accept()
   {
      session* new_session = new session(io_service_, all, found);
      acceptor_.async_accept(new_session->socket(),
                             boost::bind(&server::handle_accept, this, new_session,
                                         boost::asio::placeholders::error));
   }

   void handle_accept(session* new_session,
                      const boost::system::error_code& error)
   {
      if (!error)
         new_session->start();
      else
         delete new_session;

      start_accept();
   }

   boost::asio::io_service& io_service_;
   tcp::acceptor acceptor_;
   SlaveList* all;
   SlaveList* found;
};


void heartbeat(const boost::system::error_code& /*e*/,
               boost::asio::deadline_timer* t, Slave* broadcast)
{
   broadcast->heartbeat();
   if (debug)
      cout << *broadcast << endl;
   t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
   t->async_wait(boost::bind(heartbeat, boost::asio::placeholders::error, t, broadcast));
}


void scan(const boost::system::error_code& /*e*/,
          boost::asio::deadline_timer* t, SlaveList* all, SlaveList* found)
{
   static unsigned scan_count=0;
   static unsigned call_count=0;
   call_count++;

   if (debug)
      cout << "scan" << endl;
   if (all->size() && scan_count==0)
   {
      scan_count++;
      SlaveList more = scan(*all);
      if (more.size())
      {
         cout << "Added " << more.size() << " slaves, "
              << all->size() << " not found." << endl;
         found->splice(found->end(), more);
      }
   }
   for (auto i=found->begin(); i!=found->end(); i++)
   {
      i->ping();
      cout << *i << endl;
   }
   t->expires_at(t->expires_at() + boost::posix_time::seconds(10));
   t->async_wait(boost::bind(scan, boost::asio::placeholders::error, t, all, found));
}


int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;
   try
   {
      if (argc != 2)
      {
         std::cerr << "Usage: async_tcp_echo_server <port>\n";
         return 1;
      }

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
         return -1;
      }
      nRF24L01::configure_PTX();
      nRF24L01::flush_tx();

      // Reset all the slaves, and give them a chance to come back up
      Slave broadcast(0);
      broadcast.reboot();
      bcm2835_delayMicroseconds(50000);

      cout << broadcast.stream_header << endl;

      SlaveList all, found;
      for (int id=1; id < ensemble::num_slaves; id++)
         all.push_back(Slave(id));

      boost::asio::io_service io_service;

      server s(io_service, atoi(argv[1]), &all, &found);

      boost::asio::deadline_timer t0(io_service, boost::posix_time::seconds(1));
      t0.async_wait(boost::bind(heartbeat, boost::asio::placeholders::error, &t0, &broadcast));

      boost::asio::deadline_timer t1(io_service, boost::posix_time::millisec(100));
      t1.async_wait(boost::bind(scan, boost::asio::placeholders::error, &t1, &all, &found));

      io_service.run();
   }
   catch (std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << endl;
   }
   
   google::protobuf::ShutdownProtobufLibrary();
   nRF24L01::shutdown();
   return 0;
}
