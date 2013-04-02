#ifndef _AVR_SPI_HPP
#define _AVR_SPI_HPP

namespace avr_spi
{
   void setup(void);
   void tx(const char *p_tx, char* p_rx, const short len);
}
#endif
