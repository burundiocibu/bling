#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "server_msg.pb.h"
#include "pb_helpers.hpp"

using boost::asio::ip::tcp;
using namespace std;

class session
{
public:
   session(boost::asio::io_service& io_service)
      : socket_(io_service),
        rx_buff(max_length),
        debug(1)
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

   int debug;

private:
   void handle_read(const boost::system::error_code& error,
                    size_t bytes_transferred)
   {
      if (!error)
      {
         if (debug)
         {
            cout << "session::handle_read: " << bytes_transferred << " bytes received."<< endl;
            hexdump(&rx_buff[0], bytes_transferred);
         }

         string rx_hdr(rx_buff.begin(), rx_buff.begin()+header_length);
         bling::header hdr;
         if (hdr.ParseFromString(rx_hdr))
         {
            if (debug)
               cout << "header: " << hdr.ShortDebugString() << endl;
         }
         else
            cout << "error decoding header";

         string rx_msg(rx_buff.begin()+header_length, rx_buff.begin()+header_length+hdr.len());

         bling::get_slave_list gsl;
         bling::set_slave_tlc sst;
         size_t bytes_to_send=0;
         switch (hdr.msg_id())
         {
            case bling::header::GET_SLAVE_LIST:
               if (gsl.ParseFromString(rx_msg))
               {
                  if (debug)
                     cout << "body: " << gsl.ShortDebugString() << endl;
            
                  bling::slave_list sl;
                  for (int i=0; i<5; i++)
                  {
                     bling::slave_list::slave_info* slave=sl.add_slave();
                     slave->set_slave_id(i+3);
                  }
                  cout << "slave_list::" << sl.ShortDebugString() << endl;

                  string msg_str;
                  sl.SerializeToString(&msg_str);
                  cout << "msg_str:" << endl;
                  hexdump(msg_str);

                  bling::header hdr;
                  hdr.set_msg_id(bling::header::SLAVE_LIST);
                  hdr.set_len(msg_str.size());
                  string hdr_str;
                  hdr.SerializeToString(&hdr_str);
                  cout << "hdr_str:" << endl;
                  hexdump(hdr_str);

                  strncpy(data_, hdr_str.c_str(), hdr_str.size());
                  strncpy(data_+hdr_str.size(), msg_str.c_str(), msg_str.size());
                  bytes_to_send=hdr_str.size() + msg_str.size();
		  hexdump(data_, bytes_to_send);
               };
               break;

            case bling::header::SET_SLAVE_TLC:
               if (sst.ParseFromString(rx_msg))
               {
                  cout << "received set_slave_tlc:" << endl
                       << sst.DebugString();
               }
               break;

            default:
               cout << "Unknown message" << endl;
               time_t now = time(0);
               string t(ctime(&now));
               strncpy(data_, t.c_str(), t.size());
               bytes_to_send=t.size();
         }

         if (bytes_to_send)
            boost::asio::async_write(socket_,
                                  boost::asio::buffer(data_, bytes_to_send),
                                     boost::bind(&session::handle_write, this,
                                                 boost::asio::placeholders::error));

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
         if (debug)
            cout << "session::handle_write"<< endl;
      }
      else
      {
         delete this;
      }
   }

   tcp::socket socket_;
   enum { max_length = 1024 };
   char data_[max_length];
   vector<char> rx_buff;
};


class server
{
public:
   server(boost::asio::io_service& io_service, short port)
      : io_service_(io_service),
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
   {
      start_accept();
   }

private:
   void start_accept()
   {
      session* new_session = new session(io_service_);
      acceptor_.async_accept(new_session->socket(),
                             boost::bind(&server::handle_accept, this, new_session,
                                         boost::asio::placeholders::error));
   }

   void handle_accept(session* new_session,
                      const boost::system::error_code& error)
   {
      if (!error)
      {
         new_session->start();
      }
      else
      {
         delete new_session;
      }

      start_accept();
   }

   boost::asio::io_service& io_service_;
   tcp::acceptor acceptor_;
};


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

      boost::asio::io_service io_service;

      server s(io_service, atoi(argv[1]));

      io_service.run();
   }
   catch (std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << "\n";
   }
   
   google::protobuf::ShutdownProtobufLibrary();
   return 0;
}
