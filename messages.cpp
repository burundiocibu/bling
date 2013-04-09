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


   // ========================================
   void encode_all_stop(uint8_t* p)
   {
      *p++ = all_stop_id;
   }

   void decode_all_stop(uint8_t* p)
   {}


   // ========================================
   void encode_heartbeat(uint8_t* p, uint32_t t_ms)
   {
      *p++ = heartbeat_id;
      p = encode_var<uint32_t>(p, t_ms);
   }

   void decode_heartbeat(uint8_t* p, uint32_t &t_ms)
   {
      p++; // skip ID
      p = decode_var<uint32_t>(p, t_ms);
   }


   // ========================================
   void encode_set_tlc_ch(uint8_t* p, uint8_t ch, uint16_t value)
   {
      *p++ = set_tlc_ch_id;
      p = encode_var<uint8_t>(p, ch);
      p = encode_var<uint16_t>(p, value);
   }
      
   void decode_set_tlc_ch(uint8_t* p, uint8_t &ch, uint16_t &value)
   {
      p++; // skip ID
      p = decode_var<uint8_t>(p, ch);
      p = decode_var<uint16_t>(p, value);
   }

   // ========================================
   void encode_start_effect(uint8_t* p, uint8_t effect_id, uint32_t start_time, uint16_t duration)
   {
      *p++ = start_effect_id;
      p = encode_var<uint8_t>(p, effect_id);
      p = encode_var<uint32_t>(p, start_time);
      p = encode_var<uint16_t>(p, duration);
   }

   void decode_start_effect(uint8_t* p, uint8_t &effect_id, uint32_t &start_time, uint16_t &duration)
   {
      p++; // skip ID
      p = decode_var<uint8_t>(p, effect_id);
      p = decode_var<uint32_t>(p, start_time);
      p = decode_var<uint16_t>(p, duration);
   }

}