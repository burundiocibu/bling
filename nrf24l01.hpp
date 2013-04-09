#ifndef _NRF24L01_HPP
#define _NRF24L01_HPP

// An interface to a nrf24l01 chip

#include <stddef.h>
#include <stdint.h>

#include "nrf24l01_defines.hpp"

namespace nRF24L01
{

   //===============================================================================================
   // This part of the namespace is the hardware-defendant part of the interface
   void write_data(char* data, const size_t len);
   void delay_us(uint32_t us);
   void clear_CE(void);
   void set_CE(void);
   bool setup(void);
   void shutdown(void);

   //===============================================================================================
   //Code below should be low-level hardware independent

   void write_reg(char reg, char data);
   void write_reg(char reg, char* data, const size_t len);
   char read_reg(char reg);
   void read_reg(char reg, char* data, const size_t len);

   // returns true on success
   bool configure_base(void);

   void configure_PRX(void);
   void power_up_PRX(void);

   void configure_PTX(void);
   void power_up_PTX(void);

   void write_tx_payload(void* data, const unsigned int len);
   void read_rx_payload(void* data, const unsigned int len, uint8_t &pipe);
   void flush_tx(void);
   void pulse_CE(void);

#ifdef AVR
   extern uint8_t rx_flag;
   extern uint32_t t_rx;
#endif
}
#endif
