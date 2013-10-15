#ifndef _MESSAGES_HPP
#define _MESSAGES_HPP

#include <stdint.h>
#include <string.h>

namespace messages
{
   enum message_ids
   {
      all_stop_id = 0,
      heartbeat_id = 1,
      set_tlc_ch_id = 2,
      ping_id = 3,
      start_effect_id = 4,
      set_rgb_id = 5,
      status_id = 6,
      reboot_id = 7,
      sleep_id = 8,
      wake_id = 9
   };

   extern uint16_t missed_message_count;
   extern uint8_t freshness_count;

   inline uint8_t get_id(uint8_t* p)
   {return *p;}

   void encode_all_stop(uint8_t* p);
   void decode_all_stop(uint8_t* p);

   void encode_heartbeat(uint8_t* p, uint32_t  t_ms);
   void decode_heartbeat(uint8_t* p, uint32_t &t_ms);

   void encode_set_tlc_ch(uint8_t* p, uint8_t  ch, uint16_t  value);
   void decode_set_tlc_ch(uint8_t* p, uint8_t &ch, uint16_t &value);

   void encode_start_effect(uint8_t* p, uint8_t  effect_id, uint32_t  start_time, uint32_t  duration);
   void decode_start_effect(uint8_t* p, uint8_t &effect_id, uint32_t &start_time, uint32_t &duration);

   void decode_status(uint8_t* p, uint16_t &slave_id, uint32_t &t_rx, int8_t &major_version, int8_t &minor_version,
                      uint16_t &vcell, uint16_t &soc, uint16_t &missed_message_count, uint8_t &freshness_count);
   void encode_status(uint8_t* p, uint16_t slave_id, uint32_t t_rx, int8_t major_version, int8_t minor_version,
                      uint16_t vcell, uint16_t soc);

   void encode_ping(uint8_t* p);
   void decode_ping(uint8_t* p);

   void encode_reboot(uint8_t* p);
   void decode_reboot(uint8_t* p);

   void encode_sleep(uint8_t* p);
   void decode_sleep(uint8_t* p);

   void encode_wake(uint8_t* p);
   void decode_wake(uint8_t* p);

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
