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
}

#endif
