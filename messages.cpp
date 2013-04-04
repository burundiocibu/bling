#include "messages.hpp"

#include <string.h>


namespace messages
{
   template <class T>
   uint8_t* decode_var(uint8_t *p, T &v )
   {
      v = *reinterpret_cast<T*>(p);
      return p + sizeof(T);
   }
   
   template <class T>
   uint8_t* encode_var(uint8_t *p, T &v )
   {
      *reinterpret_cast<T*>(p) = v;
      return p + sizeof(T);
   }


   void Heartbeat::decode(uint8_t* p)
   {
      p++; // skip ID
      p = decode_var<uint32_t>(p, t_ms);
   }

   void Heartbeat::encode(uint8_t* p)
   {
      *p++ = heartbeat_id;
      *reinterpret_cast<uint32_t*>(p) = t_ms;
   }

   
   void Set_tlc_ch::decode(uint8_t* p)
   {
      p++; // skip ID
      p = decode_var<uint8_t>(p, ch);
      p = decode_var<uint8_t>(p, value);
   };


   void Set_tlc_ch::encode(uint8_t* p)
   {
      *p++ = set_tlc_ch_id;
      p = encode_var<uint8_t>(p, ch);
      p = encode_var<uint8_t>(p, value);
   }

}
