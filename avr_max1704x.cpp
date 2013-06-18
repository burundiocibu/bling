#include <avr/io.h>

#include "avr_max1704x.hpp"

namespace avr_max1704x
{

   void setup(void)
   {
      i2c::setup();
   }

   
   int read_soc()
   {
      i2c::send_start();
      i2c::send_data(WRITE_ADDR);
      i2c::send_data(VCELL);
      i2c::send_stop();
      
      uint8_t xm, xl;
      i2c::send_start();
      i2c::send_data(READ_ADDR);
      i2c::receive_data(xm);
      i2c::receive_data(xl);
      i2c::send_stop();
      return xm;
   }


   int read_vcell()
   {
      i2c::send_start();
      i2c::send_data(WRITE_ADDR);
      i2c::send_data(VCELL);
      i2c::send_stop();
      
      uint8_t xm, xl;
      i2c::send_start();
      i2c::send_data(READ_ADDR);
      i2c::receive_data(xm);
      i2c::receive_data(xl);
      i2c::send_stop();
      return 2.50 * ((xl|(xm << 8)) >> 4);
   }


   int read_version()
   {
   }
}
