#ifndef _SLAVE_EEPROM_HPP
#define _SLAVE_EEPROM_HPP

#include <avr/eeprom.h> 

#include "nrf24l01.hpp"

namespace eeprom
{
   // This data is only written by avrdude
   // see avrdude_write_eeprom.sh
   uint16_t  EEMEM slave_id; // the unit's serial number
   uint8_t   EEMEM channel;  // default radio channel
   uint8_t   EEMEM master_addr[nRF24L01::addr_len];
   uint8_t   EEMEM broadcast_addr[nRF24L01::addr_len];
   uint8_t   EEMEM slave_addr[nRF24L01::addr_len]; // unit specifc address
}
#endif
