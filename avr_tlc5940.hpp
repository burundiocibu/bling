#ifndef _AVR_TLC5940_HPP
#define _AVR_TLC5940_HPP

#include <stdint.h>
#include <string.h>
#include <inttypes.h>

namespace avr_tlc5940
{
   extern uint8_t gsdata[];
   void setup(void);
   void set_channel(int ch, int value);
   unsigned get_channel(int ch);
   void output_gsdata(void);
}

#endif
