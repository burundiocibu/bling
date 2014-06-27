#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

#include "server_msg.pb.h"
#include "pb_helpers.hpp"

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   using namespace std; // For strlen.
   try
   {
      if (argc != 3)
      {
         std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
         return 1;
      }

      boost::asio::io_service io_service;

      tcp::resolver resolver(io_service);
      tcp::resolver::query query(tcp::v4(), argv[1], argv[2]);
      tcp::resolver::iterator iterator = resolver.resolve(query);

      tcp::socket s(io_service);
      boost::asio::connect(s, iterator);

      while (true)
      {
         cout << "Enter message: ";
	 string msg;
	 getline(cin, msg);

         if (msg == "list")
         {
            bling::get_slave_list gsl;
            gsl.set_scan(true);
            gsl.set_tries(2);
            write_message(s, gsl);

            bling::slave_list sl;
            if (read_message(s, sl))
               cout << sl.DebugString() << endl;
         }
         else if (msg == "on")
         {
            bling::set_slave_tlc msg;
            msg.set_slave_id(3);
            for (int i=0; i<15; i++)
               msg.add_tlc(0xfff);
            write_message(s, msg);
         }
         else if (msg == "off")
         {
            bling::set_slave_tlc msg;
            msg.set_slave_id(3);
            for (int i=0; i<15; i++)
               msg.add_tlc(0x000);
            write_message(s, msg);
         }
         else if (msg == "quit" || msg == "q")
         {
            break;
         }
         else
         {
            write_string(s, msg);
            boost::asio::streambuf rx_buff;
            istream rx_stream(&rx_buff);
            size_t reply_length = boost::asio::read_until(s, rx_buff, '\n');
            string str;
            getline(rx_stream, str);
            cout << "Reply is: " << str << endl;
         }
      }
   }
   catch (std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << endl;
   }

   google::protobuf::ShutdownProtobufLibrary();
   return 0;
}
