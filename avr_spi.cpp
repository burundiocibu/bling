#include <avr/io.h>

#include "avr_spi.hpp"

namespace avr_spi
{
   void setup(void)
   {
      // MOSI, SCK, and SS are outputs
      DDRB |= _BV(PB2) | _BV(PB1) | _BV(PB0);
      // Turn on SPI as master @ Fosc/4 
      SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0);
      PORTB |= _BV(PB0); // Make sure SS (CSN) is high
   }
   
   
   // Performs a complete SPI transfer
   void tx(const char* p_tx, char* p_rx, const short len)
   {
      PORTB &= ~_BV(PB0); //Drive SS low
      int i;
      for(i=0; i<len; i++)
      {
         SPDR = *p_tx++;
         while (!(SPSR & _BV(SPIF)));
         *p_rx++ = SPDR;
      }
      PORTB |= _BV(PB0); // Drive SS (CSN) back high
   }
}
