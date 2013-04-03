#ifndef _messages_HPP
#define _messages_HPP
#include <stdint.h>

namespace messages
{

   struct Heartbeat
   {
      void decode(const uint8_t* data);
      void encode(uint8_t* data);
      
      uint8_t id;
      uint32_t t_ms;
   } __attribute__((packed));
}

#endif
