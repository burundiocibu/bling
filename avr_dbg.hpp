#ifndef _AVR_DBG_HPP
#define _AVR_DBG_HPP

namespace avr_dbg
{
   void throbber(uint32_t t_hb);
   void blink(int n, unsigned v=128);
   void die(int n, unsigned v=128);
}
#endif
