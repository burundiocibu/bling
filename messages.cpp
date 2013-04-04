#include "messages.hpp"

#include <string.h>

template <class T>
uint8_t* decode_var(uint8_t *p, T &v )
{
   v = *reinterpret_cast<T*>(p);
   return p + sizeof(T);
}

namespace messages
{
   void Heartbeat::decode(uint8_t* p)
   {
      p++; // skip ID
      p = decode_var<uint32_t>(p, t_ms);
   }

   void Heartbeat::encode(uint8_t* p)
   {
      *p++ = 1;
      *reinterpret_cast<uint32_t*>(p) = t_ms;
   }
}
