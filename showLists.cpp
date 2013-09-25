
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
      "ww intro   ", 2000,
      "brass intro", 2000,
      "sparkle    ", 30000,
      "red fade   ", 4000
   };
}

