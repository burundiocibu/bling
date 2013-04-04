#ifndef _messages_HPP
#define _messages_HPP
#include <stdint.h>

namespace messages
{
   enum message_ids
   {
      all_stop_id = 0,
      heartbeat_id = 1,
      set_tlc_ch_id = 2,
      get_tlc_ch_id = 3,
      start_effect_id = 4,
      stop_effect_id = 0
   };

   inline uint8_t get_id(uint8_t* p)
   {return *p;}

   

   struct Heartbeat
   {
      Heartbeat(uint8_t* p)
      {decode(p);}

      void decode(uint8_t* p);
      void encode(uint8_t* p);
      
      uint8_t id;
      uint32_t t_ms;
   } __attribute__((packed));
}

#endif
