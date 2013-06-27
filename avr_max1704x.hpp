#ifndef _AVR_MAX1704x_HPP
#define _AVR_MAX1704x_HPP

namespace avr_max1704x
{
   // Register/address definitions
   const uint8_t WRITE_ADDR = 0x6C;
   const uint8_t READ_ADDR = 0x6D;
   const uint8_t VERSION = 0x08;
   const uint8_t SOC = 0x04;
   const uint8_t VCELL = 0x02;

   void setup(void);
   uint16_t read_soc();
   uint16_t read_vcell();
   uint16_t read_version();
}
#endif
