
#include <string.h>

#include "showLists.hpp"

namespace showlist
{
   const int maxNumberEffects = 8;
   ShowEffect showList[maxNumberEffects] = 
   {
      "Flash       ", 1000,
      "LED test    ", 30000,
      "ramp up     ", 3000,
      "pre hit     ", 10000,
      "sparkle     ", 30000,
      "fade sparkle", 4000,
      "dim red     ", 10000,
      "R->L fade   ", 10000
   };
}

