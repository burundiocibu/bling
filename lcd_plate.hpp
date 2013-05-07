#ifndef _LCD_PLATE_HPP
#define _LCD_PLATE_HPP

#include <string.h>
#include <stdint.h>
#include <inttypes.h>

namespace lcd_plate
{
   // Bits for the various buttons
   const uint8_t NONE                    = 0x0;
   const uint8_t SELECT                  = 0x1;
   const uint8_t RIGHT                   = 0x2;
   const uint8_t DOWN                    = 0x4;
   const uint8_t UP                      = 0x8;
   const uint8_t LEFT                    = 0x10;

   // LED colors
   const uint8_t OFF                     = 0x00;
   const uint8_t RED                     = 0x01;
   const uint8_t GREEN                   = 0x02;
   const uint8_t BLUE                    = 0x04;
   const uint8_t YELLOW                  = RED + GREEN;
   const uint8_t TEAL                    = GREEN + BLUE;
   const uint8_t VIOLET                  = RED + BLUE;
   const uint8_t WHITE                   = RED + GREEN + BLUE;
   const uint8_t ON                      = RED + GREEN + BLUE;

   int lcd_putchar(char c, FILE *stream);
   void setup(uint8_t addr);
   void shutdown(void);
   void puts(const char *s);
   void write(uint8_t data, const bool char_mode=true);
   void clear(void);
   void set_cursor(int row, int col);
   void set_backlight(int color);
   uint8_t read_buttons(void);
}
#endif
