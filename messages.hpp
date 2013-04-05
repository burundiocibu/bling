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
      stop_effect_id = 5
   };
   const size_t message_size=12;

   inline uint8_t get_id(uint8_t* p)
   {return *p;}

   void encode_heartbeat(uint8_t* p, uint32_t  t_ms);
   void decode_heartbeat(uint8_t* p, uint32_t &t_ms);

   void encode_set_tlc_ch(uint8_t* p, uint8_t  ch, uint8_t  value);
   void decode_set_tlc_ch(uint8_t* p, uint8_t &ch, uint8_t &value);

   void encode_start_effect(uint8_t* p, uint8_t  effect_id, uint32_t  start_time, uint16_t  duration);
   void decode_start_effect(uint8_t* p, uint8_t &effect_id, uint32_t &start_time, uint16_t &duration);

   // ========================================
   struct Heartbeat
   {
      uint32_t t_ms;

      Heartbeat() {};
      Heartbeat(uint8_t* p) {decode(p);};

      void decode(uint8_t* p);
      void encode(uint8_t* p);
   };


   // ========================================
   struct Set_tlc_ch
   {
      uint8_t ch;
      uint8_t value;

      Set_tlc_ch() {};
      Set_tlc_ch(uint8_t* p) {decode(p);};

      void decode(uint8_t* p);
      void encode(uint8_t* p);
   };


   // ========================================
   struct Start_effect
   {
      uint8_t effect_id;   // tbd
      uint32_t start_time; // ms
      uint16_t duration;   // ms

      Start_effect() {};
      Start_effect(uint8_t* p) {decode(p);};

      void decode(uint8_t* p);
      void encode(uint8_t* p);
   };


   // ========================================
   struct All_stop
   {
      All_stop() {};
      All_stop(uint8_t* p) {decode(p);};

      void decode(uint8_t* p);
      void encode(uint8_t* p);
   };

}

#endif
