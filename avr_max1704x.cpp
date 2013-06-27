#include <avr/io.h>

#include "avr_max1704x.hpp"
#include "i2c.hpp"

namespace avr_max1704x
{

   void setup(void)
   {
      i2c::setup();
   }

   
   uint16_t read_soc()
   {
      i2c::send_start();
      i2c::send_data(WRITE_ADDR);
      i2c::send_data(SOC);
      i2c::send_stop();
      
      uint8_t msb, lsb;
      i2c::send_start();
      i2c::send_data(READ_ADDR);
      i2c::receive_data(msb, true);
      i2c::receive_data(lsb, false);
      i2c::send_stop();
      return lsb | (msb << 8) ;
   }


   uint16_t read_vcell()
   {
      i2c::send_start();
      i2c::send_data(WRITE_ADDR);
      i2c::send_data(VCELL);
      i2c::send_stop();
      
      uint8_t msb, lsb;
      i2c::send_start();
      i2c::send_data(READ_ADDR);
      i2c::receive_data(msb, true);
      i2c::receive_data(lsb, false);
      i2c::send_stop();
      int vcell = 0xfff & ((lsb|(msb << 8)) >> 4);
      return (vcell << 1) + (vcell >> 1); // return 2.5 * vcell;
   }


   uint16_t read_version()
   {
      return 0;
   }
}
