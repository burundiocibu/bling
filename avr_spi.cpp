#include <avr/io.h>

#include "avr_spi.hpp"

namespace avr_spi
{
   // B2: SS (active low chip select)
   // B3: MOSI
   // B4: MISO
   // B5: SCK

   void setup(void)
   {
      // MOSI, SCK, and SS are outputs
      DDRB |= _BV(PB2) | _BV(PB3) | _BV(PB5);
      // Turn on SPI as master @ Fosc/16 
      SPCR = _BV(SPE) | _BV(MSTR);
      PORTB |= _BV(PB2); // Make sure SS is high
   }
   
   
   // Performs a complete SPI transfer
   void tx(const char* p_tx, char* p_rx, const short len)
   {
      PORTB &= ~_BV(PB2); //Drive SS low
      int i;
      for(i=0; i<len; i++)
      {
         SPDR = *p_tx++;
         while (!(SPSR & _BV(SPIF)));
         *p_rx++ = SPDR;
      }
      PORTB |= _BV(PB2); // Drive SS back high
   }
}
