#ifndef _MASTER_SERVER_HPP
#define _MASTER_SERVER_HPP

#include <string>
#include "Slave.hpp"

class Master_Server
{
public:
   Master_Server();

   ~Master_Server();

   std::string send_all_stop(std::string& msg);

   std::string get_slave_list(std::string& msg);

   std::string set_slave_tlc(std::string& msg);
   
   std::string start_effect(std::string& msg);

   std::string ping_slave(std::string& msg);

   std::string reboot_slave(std::string& msg);

   void heartbeat();

   void scan();

   SlaveList all;
   SlaveList found;
   SlaveList::iterator last_scanned;
   Slave broadcast;

   int debug;
   RunTime runtime;
};
#endif
