#include "messages.hpp"

#include <string.h>

#ifdef AVR
// seems like avr don't have these
#define ntohs(x) (((x>>8) & 0xFF) | ((x & 0xFF)<<8))
#define htons(x) (((x>>8) & 0xFF) | ((x & 0xFF)<<8))
#define ntohl(x) ( ((x>>24) & 0xFF) | ((x>>8) & 0xFF00) | \
                   ((x & 0xFF00)<<8) | ((x & 0xFF)<<24)   \
                   )
#define htonl(x) ( ((x>>24) & 0xFF) | ((x>>8) & 0xFF00) |      \
                   ((x & 0xFF00)<<8) | ((x & 0xFF)<<24)        \
                   )
#else
#include <arpa/inet.h>
#endif

namespace messages
{
   void Heartbeat::decode(const uint8_t* buff)
   {
      id=buff[0];
      t_ms = ntohl(*((uint32_t*)(buff+1)));
   }

   void Heartbeat::encode(uint8_t* buff)
   {
      buff[0] = 0x01;
      *((uint32_t*)(buff+1)) = ntohl(t_ms);
   }
}
