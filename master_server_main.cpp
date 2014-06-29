#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "server_msg.pb.h"
#include "pb_helpers.hpp"

#include "master_server.hpp"

using boost::asio::ip::tcp;
using namespace std;

int debug;

class session
{
public:
   session(boost::asio::io_service& io_service, Master_Server* _ms)
      : socket_(io_service),
        rx_buff(max_length),
        ms(_ms)
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
            case bling_pb::header::GET_SLAVE_LIST: tx_buff=ms->get_slave_list(s2); break;
            case bling_pb::header::SET_SLAVE_TLC:  tx_buff=ms->set_slave_tlc(s2);  break;
            default:
               cout << "Unknown message:" << endl;
               hexdump(s2);
               time_t now = time(0);
               string t(ctime(&now));
               tx_buff=t;
         }

         if (tx_buff.size())
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(tx_buff.c_str(), tx_buff.size()),
                                     boost::bind(&session::handle_write, this,
                                                 boost::asio::placeholders::error));

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


   tcp::socket socket_;
   string tx_buff;
   vector<char> rx_buff;

   Master_Server* ms;
};


class server
{
public:
   server(boost::asio::io_service& io_service, short port,
          Master_Server* _ms)
      : io_service_(io_service),
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        ms(_ms)
   {
      start_accept();
   }

private:
   void start_accept()
   {
      session* new_session = new session(io_service_, ms);
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
   Master_Server* ms;
};


void heartbeat(const boost::system::error_code& /*e*/,
               boost::asio::deadline_timer* t, Master_Server* ms)
{
   ms->heartbeat();
   t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
   t->async_wait(boost::bind(heartbeat, boost::asio::placeholders::error, t, ms));
}


void scan(const boost::system::error_code& /*e*/,
          boost::asio::deadline_timer* t, Master_Server* ms)
{
   ms->scan();
   t->expires_at(t->expires_at() + boost::posix_time::seconds(10));
   t->async_wait(boost::bind(scan, boost::asio::placeholders::error, t, ms));
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
      Master_Server ms;
      ms.debug = debug;

      boost::asio::io_service io_service;

      server s(io_service, atoi(argv[1]), &ms);

      boost::asio::deadline_timer t0(io_service, boost::posix_time::seconds(1));
      t0.async_wait(boost::bind(heartbeat, boost::asio::placeholders::error, &t0, &ms));

      boost::asio::deadline_timer t1(io_service, boost::posix_time::millisec(100));
      t1.async_wait(boost::bind(scan, boost::asio::placeholders::error, &t1, &ms));

      io_service.run();
   }
   catch (std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << endl;
   }
   
   google::protobuf::ShutdownProtobufLibrary();
   return 0;
}
