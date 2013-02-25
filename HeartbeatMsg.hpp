#ifndef _HEARTBEATMSG_HPP
#define _HEARTBEATMSG_HPP
#include <arpa/inet.h>
#include <cstdint>

class HeartbeatMsg 
{
public:
   inline HeartbeatMsg() 
      : id(0x01), t_ms(0)
   {}

   inline void decode() 
   {
      t_ms = ntohl(t_ms);
   }

   inline void encode(const uint32_t t)
   {
      id=0x01;
      t_ms=htonl(t);
   }

   uint8_t id;
   uint32_t t_ms;
} __attribute__((packed));
#endif
