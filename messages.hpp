#ifndef _messages_HPP
#define _messages_HPP
#include <stdint.h>
#include <string.h>

namespace messages
{
   enum message_ids
   {
      all_stop_id = 0,
      heartbeat_id = 1,
      set_tlc_ch_id = 2,
      get_tlc_ch_id = 3,
      start_effect_id = 4,
      set_rgb_id = 5,
      ack_id = 6
   };
   const size_t message_size=12;
   const size_t ack_size=12;

   inline uint8_t get_id(uint8_t* p)
   {return *p;}

   void encode_all_stop(uint8_t* p);
   void decode_all_stop(uint8_t* p);

   void encode_heartbeat(uint8_t* p, uint32_t  t_ms);
   void decode_heartbeat(uint8_t* p, uint32_t &t_ms);

   void encode_set_tlc_ch(uint8_t* p, uint8_t  ch, uint16_t  value);
   void decode_set_tlc_ch(uint8_t* p, uint8_t &ch, uint16_t &value);

   void encode_start_effect(uint8_t* p, uint8_t  effect_id, uint32_t  start_time, uint16_t  duration);
   void decode_start_effect(uint8_t* p, uint8_t &effect_id, uint32_t &start_time, uint16_t &duration);

   void encode_ack(uint8_t* p);
   void decode_ack(uint8_t* p);

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

}

#endif
