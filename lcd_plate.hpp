#ifndef _LCD_PLATE_HPP
#define _LCD_PLATE_HPP

#include <string.h>
#include <util/delay.h> // F_CPU should come from the makefile...

#include "avr_i2c.hpp"

namespace lcd_plate
{
   int lcd_putchar(char c, FILE *stream);
   void setup(uint8_t addr);
   void puts(const char *s);
   void write(uint8_t data, const bool char_mode=true);
   void clear(void);
   void set_cursor(int row, int col);
}
#endif
