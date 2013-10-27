/*
  Flasher.hpp copyright Jon Little (2013)

  Interface to the Flasher class. This encapsulates the reprogramming of a slave
  over the radio.
  -----------------------------------------------------------------------------
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _FLASHER_HPP
#define _FLASHER_HPP

#include <fstream>
#include <string>

class Flasher
{
public:
   Flasher(int debug_level);

   // returns true for a successfull programming
   bool prog_slave(uint16_t slave_no, uint8_t* image_buff, size_t image_size, std::string version=std::string());
   bool ping_slave(uint16_t slave_no);
   std::string timestamp(void);

   std::string rx_version;
   double rx_vcell;
   int rx_soc;

private:
   bool nrf_tx(uint8_t* buff, size_t len, const unsigned max_retry, unsigned &loss_count);
   bool nrf_rx(void);
   void write_reg(uint8_t reg, const uint8_t* data, const size_t len);
   void write_reg(uint8_t reg, uint8_t data);
   uint8_t read_reg(uint8_t reg);
   void read_reg(uint8_t reg, uint8_t* data, const size_t len);
   void clear_CE(void);
   void set_CE(void);
   void nrf_setup(int slave);
   void nrf_set_slave(int slave);

   std::ofstream log;
   int debug;

};

#endif
