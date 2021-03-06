#include <string.h>
#include "messages.hpp"
#include "ensemble.hpp"


namespace messages
{
   // Really only used in the slave
   uint16_t missed_message_count = 0;

   // Used to keep track of fc to send in the master code and the
   // last received fc in the slave code
   uint8_t freshness_count = 0;

   uint16_t rx_count = 0;

   uint8_t* check_fc(uint8_t* p)
   {
      uint8_t dfc = *p - freshness_count;
      if (dfc != 1 && rx_count > 0)
         missed_message_count += dfc-1;
      freshness_count = *p;
      rx_count += 1;
      return p+1;
   }


   // ========================================
   void encode_all_stop(uint8_t* p)
   {
      memset(p, 0, ensemble::message_size);
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
      memset(p, 0, ensemble::message_size);
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
      memset(p, 0, ensemble::message_size);
      *p++ = set_tlc_ch_id;
      *p++ = ++freshness_count;
      p = encode_var<uint8_t>(p, ch);
      p = encode_var<uint16_t>(p, value);
   }

   void decode_set_tlc_ch(uint8_t* p, uint8_t& ch, uint16_t& value)
   {
      p++; // skip ID
      p = check_fc(p);
      p = decode_var<uint8_t>(p, ch);
      p = decode_var<uint16_t>(p, value);
   }


   // ========================================
   void encode_start_effect(uint8_t* p, uint8_t effect_id, uint32_t start_time, uint32_t duration)
   {
      memset(p, 0, ensemble::message_size);
      *p++ = start_effect_id;
      *p++ = ++freshness_count;
      p = encode_var<uint8_t>(p, effect_id);
      p = encode_var<uint32_t>(p, start_time);
      p = encode_var<uint32_t>(p, duration);
   }

   void decode_start_effect(uint8_t* p, uint8_t &effect_id, uint32_t &start_time, uint32_t &duration)
   {
      p++; // skip ID
      p = check_fc(p);
      p = decode_var<uint8_t>(p, effect_id);
      p = decode_var<uint32_t>(p, start_time);
      p = decode_var<uint32_t>(p, duration);
   }


   // ========================================
   // Note that this message doesn't get a freshness count since it generally goes in the 'oppisite'
   // direction. Also need to manually make sure that this messages isn't longer than the
   // max message length.  Through the tlc2 value is 21 bytes
   void encode_status(uint8_t* p, uint16_t slave_id, uint32_t t_rx, int8_t major_version, int8_t minor_version,
                      uint16_t vcell, uint16_t soc, uint16_t tlc[])
   {
      memset(p, 0, ensemble::message_size);
      *p++ = status_id;
      p = messages::encode_var<uint16_t>(p, slave_id);
      *p++ = major_version;
      *p++ = minor_version;
      p = messages::encode_var<uint32_t>(p, t_rx);
      p = messages::encode_var<uint16_t>(p, vcell);
      p = messages::encode_var<uint16_t>(p, soc);
      p = messages::encode_var<uint16_t>(p, missed_message_count);
      *p++ = freshness_count;
      p = messages::encode_var<uint16_t>(p, tlc[0]);
      p = messages::encode_var<uint16_t>(p, tlc[1]);
      p = messages::encode_var<uint16_t>(p, tlc[2]);
   }

   void decode_status(uint8_t* p, uint16_t &slave_id, uint32_t &t_rx, int8_t &major_version, int8_t &minor_version,
                      uint16_t &vcell, uint16_t &soc, uint16_t &mmc, uint8_t &fc, uint16_t tlc[])
   {
      p++; // throw away the message id
      p = messages::decode_var<uint16_t>(p, slave_id);
      major_version = *p++;
      minor_version = *p++;
      p = messages::decode_var<uint32_t>(p, t_rx);
      p = messages::decode_var<uint16_t>(p, vcell);
      p = messages::decode_var<uint16_t>(p, soc);
      p = messages::decode_var<uint16_t>(p, mmc);
      fc = *p++;
      p = messages::decode_var<uint16_t>(p, tlc[0]);
      p = messages::decode_var<uint16_t>(p, tlc[1]);
      p = messages::decode_var<uint16_t>(p, tlc[2]);
   }


   // ========================================
   void encode_ping(uint8_t* p)
   {
      memset(p, 0, ensemble::message_size);
      *p++ = ping_id;
      *p++ = ++freshness_count;
   }

   void decode_ping(uint8_t* p)
   {
      p++; // skip ID
      p = check_fc(p);
   }


   // ========================================
   void encode_reboot(uint8_t* p)
   {
      memset(p, 0, ensemble::message_size);
      *p++ = reboot_id;
      *p++ = ++freshness_count;
   }

   void decode_reboot(uint8_t* p)
   {
      p++; // skip ID
      p = check_fc(p);
   }


   // ========================================
   void encode_sleep(uint8_t* p)
   {
      memset(p, 0, ensemble::message_size);
      *p++ = sleep_id;
      *p++ = ++freshness_count;
   }

   void decode_sleep(uint8_t* p)
   {
      p++; // skip ID
      p = check_fc(p);
   }


   // ========================================
   void encode_wake(uint8_t* p)
   {
      memset(p, 0, ensemble::message_size);
      *p++ = wake_id;
      *p++ = ++freshness_count;
   }

   void decode_wake(uint8_t* p)
   {
      p++; // skip ID
      p = check_fc(p);
   }


   // ========================================
   void encode_set_tlc(uint8_t* p, uint16_t value[])
   {
      memset(p, 0, ensemble::message_size);
      *p++ = set_tlc_id;
      *p++ = ++freshness_count;
      for (int i=0; i<15; i++)
      {
         uint8_t b=0;
         uint16_t v=value[i];
         for (b=0; v; b++)
            v>>=1;
         p = encode_var<uint8_t>(p, b);
      }
   }

   void decode_set_tlc(uint8_t* p, uint16_t value[])
   {
      p++; // skip ID
      p = check_fc(p);
      for (int i=0; i<15; i++)
      {
         uint8_t b;
         uint16_t v=0;
         p = decode_var<uint8_t>(p, b);
         for (int j=0; j<b; j++)
            v = v<<1 | 0x1; ;
         value[i] = v;
      }
   }
}
