#ifndef _AVR_LED_HPP
#define _AVR_LED_HPP

namespace avr_led
{
   inline void off(void)    {PORTC |=  _BV(PC7);}
   inline void on(void)     {PORTC &= ~_BV(PC7);}
   inline void toggle(void) {PORTC ^=  _BV(PC7);}
   inline void setup(void)  {DDRC  |=  _BV(PC7); off();}
}
#endif
