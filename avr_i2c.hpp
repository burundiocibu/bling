#ifndef _AVR_I2C_HPP
#define _AVR_I2C_HPP

#include <stdint.h>
#include <string.h>
#include <inttypes.h>

namespace avr_i2c
{
   void setup(void);

   int write(const uint8_t slave_addr, const uint8_t cmd, const uint8_t data);
   int write(const uint8_t slave_addr, const uint8_t cmd, const uint8_t* data, const size_t len);

   int read(const uint8_t slave_addr, const uint8_t reg_addr, uint8_t& data);
   int read(const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* data, const size_t len);
}

#endif
