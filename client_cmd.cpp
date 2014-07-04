#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>


#include "server_msg.pb.h"
#include "pb_helpers.hpp"

int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   using boost::asio::ip::tcp;
   using namespace std;
   namespace po = boost::program_options;

   int verbose,debug;
   string hostname, protocol;
   po::options_description base_opts("General options");
   po::positional_options_description pd;
   pd.add("command", -1);
   vector<string> command;
   base_opts.add_options()
      ("help,h", "Produce help message")
      ("server,s", po::value<string>(&hostname)->default_value("threepi"), "hostname of master_server.")
      ("port,p", po::value<string>(&protocol)->default_value("9321"), "Protocol port to use to talk to master_server.")
      ("verbose,v", po::value<int>(&verbose)->default_value(0), "Produce more verbose status messages. ")
      ("debug,d", "Enable debug output.")
      ("command", po::value<vector<string> >(&command)->multitoken()->required(), "Commnad to provide to master_server");
      ;

   try
   {
      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).options(base_opts).positional(pd).run(), vm);
      po::notify(vm);
      if (vm.count("help"))
      {
         cout << "Performs a single command/response to master_server." << endl
              << base_opts << endl;
         exit(1);
      }

      debug = vm.count("debug");

      if (debug)
      {
         cout << "command: ";
         for (auto i=command.begin(); i != command.end(); i++)
            cout << *i << " ";
         cout << endl;
      }

      boost::asio::io_service io_service;

      tcp::resolver resolver(io_service);
      tcp::resolver::query query(tcp::v4(), hostname.c_str(), protocol.c_str());
      tcp::resolver::iterator iterator = resolver.resolve(query);

      tcp::socket s(io_service);
      boost::asio::connect(s, iterator);

      if (command[0] == "list")
      {
         bling_pb::get_slave_list gsl;
         if (command.size() > 1)
            
         gsl.set_scan(false);
         write_message(s, gsl);

         bling_pb::slave_list sl;
         if (read_message(s, sl))
            cout << sl.DebugString() << endl;
      }
      else if (command[0] == "on")
      {
         bling_pb::set_slave_tlc msg;
         msg.set_slave_id(3);
         for (int i=0; i<15; i++)
            msg.add_tlc(0xfff);
         write_message(s, msg);
      }
      else if (command[0] == "off")
      {
         bling_pb::set_slave_tlc msg;
         msg.set_slave_id(3);
         for (int i=0; i<15; i++)
            msg.add_tlc(0x000);
         write_message(s, msg);
      }
      else if (command[0] == "what")
      {
         write_string(s, command[0]);
         boost::asio::streambuf rx_buff;
         istream rx_stream(&rx_buff);
         size_t reply_length = boost::asio::read_until(s, rx_buff, '\n');
         string str;
         getline(rx_stream, str);
         cout << "Reply is: " << str << endl;
      }
      else
      {
         cout << "Unknown command" << endl;
      }
   }
   catch (po::error& e)
   {
      cout << e.what() << endl
           << base_opts << endl;
      exit(1);
   }
   catch (std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << endl;
   }

   google::protobuf::ShutdownProtobufLibrary();
   return 0;
}
