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

//===============================================================================================
// This part of the namespace is the hardware-dependant part of the interface


namespace nRF24L01
{
   const size_t addr_len=4;
   
   // The first address is the 'all-hands' address
   const char slave_addr[][4]={
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

   bool nrf_rx_flag=false;

#ifdef AVR
   uint8_t rx_flag=0;
   uint32_t t_rx;

   ISR(PCINT0_vect)
   {
      t_rx = avr_rtc::t_ms;
      rx_flag++;
   }
#endif


   bool setup(void)
   {
#ifdef AVR
      avr_spi::setup();

      // use B.6 as the CE to the nrf24l01
      DDRB |= _BV(PB6);

      rx_flag=0;
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



//===============================================================================================
//Code below should be low-level hardware independent
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
      write_reg(SETUP_RETR, SETUP_RETR_ARC_3 | SETUP_RETR_ARD_250); // auto retransmit 3 x 250us
      write_reg(SETUP_AW, SETUP_AW_4BYTES);  // 4 byte addresses
      write_reg(RF_SETUP, 0x07);  // 1Mbps data rate, 0dBm
      write_reg(RF_CH, channel); // use channel 2

      write_reg(nRF24L01::RX_PW_P0, messages::message_size);

      // Clear the various interrupt bits
      write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);

      //shouldn't have to do this, but it won't TX if you don't
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

      // Enable just pipes 0 & 1
      write_reg(EN_RXADDR, 3);
   }


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
   {}


   void power_up_PTX(void)
   {
      iobuff[0]=FLUSH_TX;
      write_data(iobuff, 1);
      char config = read_reg(CONFIG);
      config |= CONFIG_PWR_UP;
      write_reg(CONFIG, config);
   }


   void write_tx_payload(void* data, const size_t len, unsigned slave)
   {
      // Note the TX_ADDR must be equal the RX_ADDR_P0 in the PTX
      char buff[addr_len];
      memcpy(buff, slave_addr[slave], addr_len);
      write_reg(TX_ADDR, buff, addr_len);

      memcpy(buff, slave_addr[slave], addr_len);
      write_reg(RX_ADDR_P0, buff, addr_len);

      if (slave==0)
         iobuff[0]=W_TX_PAYLOAD_NO_ACK;
      else
         iobuff[0]=W_TX_PAYLOAD;

      memcpy(iobuff+1, data, len);
      write_data(iobuff, len+1);
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

}
