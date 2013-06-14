#ifndef _SLAVE_HPP
#define _SLAVE_HPP

#include <string>
#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

class Slave
{
public:

   unsigned slave_no;
   unsigned tx_cnt;
   unsigned retry_cnt;
   unsigned nack_cnt;  // number of transmissions with ACK req set that don't get responses.
   unsigned no_resp; // number of ping responses we expected but didn't get
   unsigned tx_dt, rx_dt;  // time to complete tx/rx in usec
   uint32_t t_tx, t_rx; // time of most recent send and the time in the most recent received packet

   uint8_t id;
   int32_t batt_cap;

   // used for the tx & rx functions
   uint8_t buff[messages::message_size];

   bool tx(); // Returns true if it was successfull
   void rx();
   std::string status() const;
};

#endif
