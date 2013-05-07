#include <stdio.h>

#include "lcd_plate.hpp"

#include "i2c.hpp"

namespace lcd_plate
{
   const uint8_t MCP23017_IOCON_BANK0    = 0x0A;  // IOCON when Bank 0 active
   const uint8_t MCP23017_IOCON_BANK1    = 0x15;  // IOCON when Bank 1 active

   // These are register addresses when in Bank 1 only:
   const uint8_t MCP23017_GPIOA          = 0x09;
   const uint8_t MCP23017_IODIRB         = 0x10;
   const uint8_t MCP23017_GPIOB          = 0x19;


   // LCD Commands
   const uint8_t LCD_CLEARDISPLAY        = 0x01;
   const uint8_t LCD_RETURNHOME          = 0x02;
   const uint8_t LCD_ENTRYMODESET        = 0x04;
   const uint8_t LCD_DISPLAYCONTROL      = 0x08;
   const uint8_t LCD_CURSORSHIFT         = 0x10;
   const uint8_t LCD_FUNCTIONSET         = 0x20;
   const uint8_t LCD_SETCGRAMADDR        = 0x40;
   const uint8_t LCD_SETDDRAMADDR        = 0x80;

   // Flags for display on/off control
   const uint8_t LCD_DISPLAYON           = 0x04;
   const uint8_t LCD_DISPLAYOFF          = 0x00;
   const uint8_t LCD_CURSORON            = 0x02;
   const uint8_t LCD_CURSOROFF           = 0x00;
   const uint8_t LCD_BLINKON             = 0x01;
   const uint8_t LCD_BLINKOFF            = 0x00;

   // Flags for display entry mode
   const uint8_t LCD_ENTRYRIGHT          = 0x00;
   const uint8_t LCD_ENTRYLEFT           = 0x02;
   const uint8_t LCD_ENTRYSHIFTINCREMENT = 0x01;
   const uint8_t LCD_ENTRYSHIFTDECREMENT = 0x00;

   // Flags for display/cursor shift
   const uint8_t LCD_DISPLAYMOVE = 0x08;
   const uint8_t LCD_CURSORMOVE  = 0x00;
   const uint8_t LCD_MOVERIGHT   = 0x04;
   const uint8_t LCD_MOVELEFT    = 0x00;

   uint8_t addr;
   uint8_t porta=0, portb=0; // keep around what should be on those ports

#ifdef AVR
   // this is unique to the avr-libc
   static FILE mystdout;
#endif


   void setup(uint8_t _addr)
   {
      i2c::setup();
      addr = _addr;
      // Set MCP23017 IOCON register to Bank 0 with sequential operation.
      // If chip is already set for Bank 0, this will just write to OLATB,
      // which won't seriously bother anything on the plate right now
      // (blue backlight LED will come on, but that's done in the next
      // step anyway).
      i2c::write(addr, MCP23017_IOCON_BANK1, 0);
         
      // Brute force reload ALL registers to known state.  This also
      // sets up all the input pins, pull-ups, etc. for the Pi Plate.
      const uint8_t buff[] =
         {0b00111111,   // IODIRA - R+G LEDs=outputs, buttons=inputs
          0b00000000,   // IODIRB - Blue LED=output, LCD D7=input, D6..4=output
          0b00000000,   // IPOLA
          0b00000000,   // IPOLB
          0b00000000,   // GPINTENA - Disable interrupt-on-change
          0b00000000,   // GPINTENB
          0b00000000,   // DEFVALA
          0b00000000,   // DEFVALB
          0b00000000,   // INTCONA
          0b00000000,   // INTCONB
          0b00000000,   // IOCON
          0b00000000,   // IOCON
          0b00111111,   // GPPUA - Enable pull-ups on buttons
          0b00000000,   // GPPUB
          0b00000000,   // INTFA
          0b00000000,   // INTFB
          0b00000000,   // INTCAPA
          0b00000000,   // INTCAPB
          0b00000000,   // GPIOA
          0b00000000,   // GPIOB
          0b00000000,   // OLATA - 0 on all outputs; side effect of
          0b00000000 }; // OLATB - turning on R+G+B backlight LEDs.
      i2c::write(addr, 0, buff, sizeof(buff));

      // Switch to Bank 1 and disable sequential operation.
      // From this point forward, the register addresses do NOT match
      // the list immediately above.  Instead, use the constants defined
      // at the start of the class.  Also, the address register will no
      // longer increment automatically after this -- multi-byte
      // operations must be broken down into single-byte calls.
      i2c::write(addr, MCP23017_IOCON_BANK0, 0b10100000);

      // turn all the backlight LEDs off
      porta = 0b11000000;
      portb = 0b00000001;
      i2c::write(addr, MCP23017_GPIOA, porta);
      i2c::write(addr, MCP23017_GPIOB, portb);

      write(0x33, false); // Init
      write(0x32, false); // Init
      write(0x28, false); // 2 line 5x8 matrix
      write(LCD_CLEARDISPLAY, false);
      write(LCD_CURSORSHIFT    | LCD_CURSORMOVE | LCD_MOVERIGHT, false);
      write(LCD_ENTRYMODESET   | LCD_ENTRYLEFT |  LCD_ENTRYSHIFTDECREMENT, false);
      write(LCD_DISPLAYCONTROL | LCD_DISPLAYON |  LCD_CURSOROFF | LCD_BLINKOFF, false);
      write(LCD_RETURNHOME, false);

#ifdef AVR
      fdev_setup_stream(&mystdout, lcd_putchar, NULL, _FDEV_SETUP_WRITE);
      stdout=&mystdout;
#endif
   }

   void shutdown()
   {
      i2c::shutdown();
   }


   // The LCD data pins (D4-D7) connect to MCP pins 12-9 (PORTB4-1), in
   // that order.  Because this sequence is 'reversed,' a direct shift
   // won't work.  This table remaps 4-bit data values to MCP PORTB
   // outputs, incorporating both the reverse and shift.
   uint8_t flip[] = { 0b00000000, 0b00010000, 0b00001000, 0b00011000,
                      0b00000100, 0b00010100, 0b00001100, 0b00011100,
                      0b00000010, 0b00010010, 0b00001010, 0b00011010,
                      0b00000110, 0b00010110, 0b00001110, 0b00011110 };


   // Low-level 4-bit interface for LCD output.  This doesn't actually
   // write data, just returns a byte array of the PORTB state over time.
   void out4(const uint8_t bitmask, const uint8_t value, uint8_t *rv)
   {
      uint8_t hi = bitmask | flip[value >> 4];
      uint8_t lo = bitmask | flip[value & 0x0F];
      *rv++ = hi | 0b00100000;
      *rv++ = hi;
      *rv++ = lo | 0b00100000;
      *rv++ = lo;
   }


   void write(uint8_t data, const bool char_mode)
   {
      uint8_t bitmask = portb & 0b00000001; // maskout LCD backlight bit
      if (char_mode) bitmask |= 0b10000000;
      uint8_t buff[4];
      out4(bitmask, data, buff);
      i2c::write(addr, MCP23017_GPIOB, buff, sizeof(buff));
      if (data==LCD_CLEARDISPLAY || data==LCD_RETURNHOME)
         i2c::delay_us(2000);
      else
         i2c::delay_us(37);
   }


   void puts(const char *s)
   {
      for (; *s !=0 ; s++)
         write(*s);
   }


   void clear(void)
   {
      write(LCD_CLEARDISPLAY, false);
   }


   int lcd_putchar(char c, FILE *stream)
   {
      if (c == '\n')
         lcd_plate::write('\r');
      lcd_plate::write(c);
      return 0;
   }

   void set_cursor(int row, int col)
   {
      const uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
      if (row>1)
         row=1;
      if (row<0)
         row=0;
      write(LCD_SETDDRAMADDR | (col + row_offsets[row]), false);
   }


   uint8_t read_buttons(void)
   {
      uint8_t rv;
      i2c::read(addr, MCP23017_GPIOA, &rv, 1);
      return rv;
   }

   void set_backlight(int color)
   {
      if (color & RED)
         porta &= ~0x40;
      else
         porta |= 0x40;

      if (color & GREEN)
         porta &= ~0x80;
      else
         porta |= 0x80;

      if (color & BLUE)
         portb &= ~0x01;
      else
         portb |= 0x01;

      i2c::write(addr, MCP23017_GPIOA, porta);
      i2c::write(addr, MCP23017_GPIOB, portb);
   }


}
