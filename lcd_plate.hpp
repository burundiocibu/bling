#ifndef _LCD_PLATE_HPP
#define _LCD_PLATE_HPP

#include <string.h>
#include <stdint.h>
#include <inttypes.h>

namespace lcd_plate
{
   // Bits for the various buttons
   const uint8_t SELECT                  = 0x1;
   const uint8_t RIGHT                   = 0x2;
   const uint8_t DOWN                    = 0x4;
   const uint8_t UP                      = 0x8;
   const uint8_t LEFT                    = 0x10;

   int lcd_putchar(char c, FILE *stream);
   void setup(uint8_t addr);
   void puts(const char *s);
   void write(uint8_t data, const bool char_mode=true);
   void clear(void);
   void set_cursor(int row, int col);
   uint8_t read_buttons(void);
}
#endif
