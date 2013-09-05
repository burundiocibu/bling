#include "messages.hpp"

#include <string.h>


namespace messages
{
   // Really only used in the slave
   uint16_t missed_message_count = 0;

   // Used to keep track of fc to send in the master code and the
   // last received fc in the slave code
   uint8_t freshness_count = 0;

   uint8_t* check_fc(uint8_t* p)
   {
      uint8_t dfc = *p - freshness_count;
      if (dfc != 1 )
         missed_message_count += dfc-1;
      freshness_count = *p;
      return p+1;
   }


   // ========================================
   void encode_all_stop(uint8_t* p)
   {
      *p++ = all_stop_id;
      *p++ = ++freshness_count;
   }

   void decode_all_stop(uint8_t* p)
   {
      p++; // skip ID
      p = check_fc(p);
   }


   // ========================================
   void encode_heartbeat(uint8_t* p, uint32_t t_ms)
   {
      *p++ = heartbeat_id;
      *p++ = ++freshness_count;
      p = encode_var<uint32_t>(p, t_ms);
   }

   void decode_heartbeat(uint8_t* p, uint32_t &t_ms)
   {
      p++; // skip ID
      p = check_fc(p);
      p = decode_var<uint32_t>(p, t_ms);
   }


   // ========================================
   void encode_set_tlc_ch(uint8_t* p, uint8_t ch, uint16_t value)
   {
      *p++ = set_tlc_ch_id;
      *p++ = ++freshness_count;
      p = encode_var<uint8_t>(p, ch);
      p = encode_var<uint16_t>(p, value);
   }
      
   void decode_set_tlc_ch(uint8_t* p, uint8_t &ch, uint16_t &value)
   {
      p++; // skip ID
      p = check_fc(p);
      p = decode_var<uint8_t>(p, ch);
      p = decode_var<uint16_t>(p, value);
   }

   // ========================================
   void encode_start_effect(uint8_t* p, uint8_t effect_id, uint32_t start_time, uint16_t duration)
   {
      *p++ = start_effect_id;
      *p++ = ++freshness_count;
      p = encode_var<uint8_t>(p, effect_id);
      p = encode_var<uint32_t>(p, start_time);
      p = encode_var<uint16_t>(p, duration);
   }

   void decode_start_effect(uint8_t* p, uint8_t &effect_id, uint32_t &start_time, uint16_t &duration)
   {
      p++; // skip ID
      p = check_fc(p);
      p = decode_var<uint8_t>(p, effect_id);
      p = decode_var<uint32_t>(p, start_time);
      p = decode_var<uint16_t>(p, duration);
   }

   // ========================================
   // Note that this message doesn't get a freshness count since it generally goes in the 'oppisite'
   // direction.
   void encode_status(uint8_t* p, uint16_t slave_id, uint32_t t_rx, int8_t major_version, int8_t minor_version,
                      uint16_t vcell, uint16_t soc)
   {
      *p++ = status_id;
      p = messages::encode_var<uint16_t>(p, slave_id);
      *p++ = major_version;
      *p++ = minor_version;
      p = messages::encode_var<uint32_t>(p, t_rx);
      p = messages::encode_var<uint16_t>(p, vcell);
      p = messages::encode_var<uint16_t>(p, soc);
      p = messages::encode_var<uint16_t>(p, missed_message_count);
      *p++ = freshness_count;
   }

   void decode_status(uint8_t* p, uint16_t &slave_id, uint32_t &t_rx, int8_t &major_version, int8_t &minor_version,
                      uint16_t &vcell, uint16_t &soc, uint16_t &missed_message_count, uint8_t &freshness_count)
   {
      p++; // throw away the message id
      p = messages::decode_var<uint16_t>(p, slave_id);
      major_version = *p++;
      minor_version = *p++;
      p = messages::decode_var<uint32_t>(p, t_rx);
      p = messages::decode_var<uint16_t>(p, vcell);
      p = messages::decode_var<uint16_t>(p, soc);
      p = messages::decode_var<uint16_t>(p, missed_message_count);
      freshness_count = *p++;
   }

   // ========================================
   void encode_ping(uint8_t* p)
   {
      *p++ = ping_id;
      *p++ = ++freshness_count;
   }

   void decode_ping(uint8_t* p)
   {
      p++; // skip ID
      p = check_fc(p);
   }
}
