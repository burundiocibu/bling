#ifndef _HEARTBEATMSG_HPP
#define _HEARTBEATMSG_HPP
#include <stdint.h>


struct HeartbeatMsg
{
   void decode(const uint8_t* data);
   void encode(uint8_t* data);

   uint8_t id;
   uint32_t t_ms;
} __attribute__((packed));
#endif
