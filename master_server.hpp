#ifndef _MASTER_SERVER_HPP
#define _MASTER_SERVER_HPP

#include <string>
#include "Slave.hpp"

class Master_Server
{
public:
   Master_Server(int _debug, std::string& slave_list_fn, std::string& slave_main_fn);

   ~Master_Server();

   std::string send_all_stop(std::string& msg);

   std::string get_slave_list(std::string& msg);

   std::string set_slave_tlc(std::string& msg);
   
   std::string start_effect(std::string& msg);

   std::string ping_slave(std::string& msg);

   std::string reboot_slave(std::string& msg);

   std::string program_slave(std::string& msg);

   void heartbeat();

   Slave* find_slave(unsigned slave_id);

   SlaveList all;
   Slave broadcast;

   int debug;
   RunTime runtime;

   std::string nak, ack;

   std::string slave_main_fn;

private:
   void set(bling_pb::slave_list::slave_info* s, const Slave& slave);
};
#endif
