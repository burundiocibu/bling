#include <arpa/inet.h>
#include <cstdint>

struct HeartbeatMsg 
{
   HeartbeatMsg() 
      : id(0x01), t_ms(0)
   {}

   HeartbeatMsg(uint8_t* buff) 
      : id(0), t_ms(0)
   {
      decode(uint8_t* buff);
   }

   void decode(uint8_t* buff) 
      : id(0), t_ms(0)
   {
      id=buff[0];
      t_ms = *((uint32_t*)(buff+1));
      t_ms = ntohl(t_ms);
   }

   void send(const uint32_t t)
   {
      id=0x01;
      t_ms=htonl(t);
      nRF24L01::write_tx_payload(this, sizeof(*this));
      nRF24L01::pulse_CE();
   }

   uint8_t id;
   uint32_t t_ms;
} __attribute__((packed));
