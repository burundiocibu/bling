#ifndef _I2C_HPP
#define _I2C_HPP

#include <stdint.h>
#include <string.h>
#include <inttypes.h>

namespace i2c
{
   void setup(void);
   void shutdown(void);
   void delay_us(uint32_t ms);

   int write(const uint8_t slave_addr, const uint8_t cmd, const uint8_t* data, const size_t len);
   int write(const uint8_t slave_addr, const uint8_t cmd, const uint8_t data);

   int read(const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* data, const size_t len);
   int read(const uint8_t slave_addr, const uint8_t reg_addr, uint8_t& data);

   uint8_t wait_for_tx(void);
   uint8_t send_start(void);
   void send_stop(void);
   uint8_t send_data(const uint8_t data);
   uint8_t receive_data(uint8_t& data, bool ack=false);
}

#endif
