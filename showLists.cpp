
#include <string.h>

#include "showLists.hpp"

namespace showlist
{
   const int maxNumberEffects = 7;
   ShowEffect showList[maxNumberEffects] = 
   {
      "Flash      ", 1000,
      "LED test   ", 30000,
      "ramp up    ", 3000,
      "ww intro   ", 10000,
      "brass intro", 10000,
      "sparkle    ", 18000,
      "sparklefade", 4000
   };
}

