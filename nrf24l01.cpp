#include <stdlib.h>
#include <string.h>

#ifdef AVR
#include <util/delay.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "avr_spi.hpp"
#else
#include <cstdio>
#include <ctime>
#include <bcm2835.h>
#endif

#include "nrf24l01.hpp"
#include "avr_rtc.hpp"
#include "messages.hpp"

namespace nRF24L01
{
   const size_t addr_len=4;
   
   // The first address is the 'all-hands' address
   const char slave_addr[][messages::message_size]=
   {
      {0xE1, 0xE3, 0xE5, 0xE6},
      {0x61, 0x63, 0x65, 0x66},
      {0x51, 0x53, 0x55, 0x56},
      {0x31, 0x33, 0x35, 0x36}
   };
   char iobuff[messages::message_size];

   const uint8_t channel = 2;



   void write_data(char* data, size_t len)
   {
#ifdef AVR
      avr_spi::tx(data, data, len);
#else
      bcm2835_spi_transfern(data, len);
#endif
   }


   void delay_us(uint32_t us)
   {
#ifdef AVR
      for (uint16_t i=0; i<us; i++)
         _delay_us(1);
#else
      bcm2835_delayMicroseconds(us);
#endif
   }


   void clear_CE(void)
   {
#ifdef AVR
      PORTB &= ~_BV(PB6);
#else
      bcm2835_gpio_write(RPI_GPIO_P1_15, LOW);
#endif
   }


   void set_CE(void)
   {
#ifdef AVR
      PORTB |= _BV(PB6);
#else
      bcm2835_gpio_write(RPI_GPIO_P1_15, HIGH);
#endif
   }


#ifdef AVR
   uint32_t t_rx;

   ISR(PCINT0_vect)
   {
      t_rx = avr_rtc::t_ms;
   }
#endif


   bool setup(void)
   {
#ifdef AVR
      avr_spi::setup();

      // use B.6 as the CE to the nrf24l01
      DDRB |= _BV(PB6);

      // use PCINT4 (PB4) to signal the MCU we has data.
      cli();
      PCICR |= _BV(PCIE0);   // enable pin change interrupts
      PCMSK0 |= _BV(PCINT4); // enable PCINT4
      PCIFR &= ~_BV(PCIF0);  // clear any pending PCINT interrupt
      sei();
      
      return true;
#else
      if (!bcm2835_init())
         return false;

      // This sets up P1-15 as the CE for the n24L01
      bcm2835_gpio_fsel(RPI_GPIO_P1_15, BCM2835_GPIO_FSEL_OUTP);
      clear_CE(); // Make sure the chip is quiet until told otherwise

      bcm2835_spi_begin();
      bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
      bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
      bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);    // 3.9 MHz
      bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
      bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

      delay_us(20);
      return true;
#endif
   }

   void shutdown(void)
   {
#ifdef AVR
#else
      bcm2835_spi_end();
      bcm2835_close();
#endif
   }


   void write_reg(char reg, char* data, const size_t len)
   {
      iobuff[0]=W_REGISTER | reg;
      memcpy(iobuff+1, data, len);
      write_data(iobuff, len+1);
   }


   void write_reg(char reg, char data)
   {
      iobuff[0]=W_REGISTER | reg;
      iobuff[1]=data;
      write_data(iobuff, 2);
   }


   char read_reg(char reg)
   {
      iobuff[0]=R_REGISTER | reg;
      iobuff[1] = 0;
      write_data(iobuff, 2);
      return iobuff[1];
   }


   void read_reg(char reg, char* data, const size_t len)
   {
      iobuff[0]=R_REGISTER | reg;
      iobuff[1]=0;
      memcpy(iobuff+1, data, len);
      write_data(iobuff, len+1);
   }


   bool configure_base(void)
   {
      if (read_reg(CONFIG) == 0xff || read_reg(STATUS) == 0xff)
         return false;
      
      write_reg(CONFIG, CONFIG_EN_CRC | CONFIG_MASK_TX_DS | CONFIG_MASK_MAX_RT);
      write_reg(SETUP_RETR, SETUP_RETR_ARC_3); // auto retransmit 3 x 250us
      write_reg(SETUP_AW, SETUP_AW_4BYTES);  // 4 byte addresses
      write_reg(RF_SETUP, 0x07);  // 1Mbps data rate, 0dBm
      write_reg(RF_CH, channel); // use channel 2

      // Clear the various interrupt bits
      write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);

      write_reg(EN_AA, 0x00); //disable auto-ack, RX mode
      return true;
   }


   // Setup device as primary receiver
   void configure_PRX(unsigned slave)
   {
      char config = read_reg(CONFIG);
      config |= CONFIG_PRIM_RX;
      write_reg(CONFIG, config);

      char buff[addr_len];
      memcpy(buff, slave_addr[0], addr_len);
      write_reg(TX_ADDR, buff, addr_len);

      memcpy(buff, slave_addr[0], addr_len);
      write_reg(RX_ADDR_P0, buff, addr_len);

      memcpy(buff, slave_addr[slave], addr_len);
      write_reg(RX_ADDR_P1, buff, addr_len);

      write_reg(nRF24L01::RX_PW_P0, messages::message_size);
      write_reg(nRF24L01::RX_PW_P1, messages::message_size);

      write_reg(EN_RXADDR, EN_RXADDR_ERX_P0 | EN_RXADDR_ERX_P1);
      write_reg(EN_AA, EN_AA_ENAA_P1);  // auto ack on pipe 1 only

      // Used to enable ACK payloads
      write_reg(DYNPD, DYNPD_DPL_P1); // might also need to enable it for P0
      write_reg(FEATURE, FEATURE_EN_DPL | FEATURE_EN_ACK_PAY);
  }


   // Note that while powered up, we can't write to most registers.
   void power_up_PRX(void)
   {
      iobuff[0]=FLUSH_RX;
      write_data(iobuff, 1);
      char config = read_reg(CONFIG);
      config |= CONFIG_PWR_UP;
      write_reg(CONFIG, config);
      delay_us(1500);
      set_CE();
   }


   void read_rx_payload(void* data, const size_t len, uint8_t &pipe)
   {
      pipe = (read_reg(STATUS) & 0x0e) >> 1;
      iobuff[0]=R_RX_PAYLOAD;
      clear_CE();
      write_data(iobuff, len+1);
      set_CE();
      memcpy(data, iobuff+1, len);
   }


   // Setup device as the primary transmitter
   void configure_PTX(void)
   {
      write_reg(nRF24L01::RX_PW_P0, messages::message_size);
      write_reg(EN_RXADDR, EN_RXADDR_ERX_P0);
      write_reg(EN_AA, EN_AA_ENAA_P0);     // auto ack on pipe 1 only
      write_reg(FEATURE, FEATURE_EN_DYN_ACK);
      iobuff[0]=FLUSH_TX;
      write_data(iobuff, 1);
   }


   // Note that while powered up, we can't write to most registers.
   void power_up_PTX(void)
   {
      char config = read_reg(CONFIG);
      write_reg(CONFIG, config | CONFIG_PWR_UP);
   }


   void write_tx_payload(void* data, const size_t len, unsigned slave)
   {
      char config = read_reg(CONFIG);
      write_reg(CONFIG, config  & ~CONFIG_PWR_UP); // power down 
      if (slave==0)
         write_reg(EN_AA, 0);
      else
         write_reg(EN_AA, EN_AA_ENAA_P0);
      write_reg(CONFIG, config | CONFIG_PWR_UP); // power back up

      iobuff[0] = slave==0 ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD;;
      memcpy(iobuff+1, data, len);
      write_data(iobuff, len+1);

      // Note the TX_ADDR must be equal the RX_ADDR_P0 in the PTX
      char buff[addr_len];
      memcpy(buff, slave_addr[slave], addr_len);
      write_reg(TX_ADDR, buff, addr_len);

      memcpy(buff, slave_addr[slave], addr_len);
      write_reg(RX_ADDR_P0, buff, addr_len);
   }


   void pulse_CE(void)
   {
      set_CE();
      delay_us(10);
      clear_CE();
   }


   void flush_tx(void)
   {
      char buff=FLUSH_TX;
      write_data(&buff, 1);
   }


   void write_ack_payload(void* data, const unsigned int len, uint8_t pipe)
   {
      iobuff[0] = W_ACK_PAYLOAD | pipe;
      memcpy(iobuff+1, data, len);
      write_data(iobuff, len+1);
   }

   void read_ack_payload(void* data, const unsigned int len, uint8_t &pipe)
   {
   }

}
