#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>

#include <bcm2835.h>

#include "rt_utils.hpp"

#include "nrf24l01.hpp"


namespace nRF24L01
{
   //===============================================================================================
   // This part of the namespace is the hardware-dependant part of the interface
#define nRF24L01_IO_DEBUG1(str) RunTime::puts(); printf("  %s %.2x\n", str, data)
#define nRF24L01_IO_DEBUG2(str) RunTime::puts(); printf("  %s ",str); dump(data, len)
#define nRF24L01_IO_DEBUG3(str) RunTime::puts(); printf("  %s\n",str)

   char write_data(char data)
   {
      nRF24L01_IO_DEBUG1("tx");
      char rc = bcm2835_spi_transfer(data);
      nRF24L01_IO_DEBUG1("rx");
      return data;
   }

   void write_data(char* data, size_t len)
   {
      nRF24L01_IO_DEBUG2("tx");
      bcm2835_spi_transfern(data, len);
      nRF24L01_IO_DEBUG2("rx");
   }

   void delay_us(uint32_t us)
   { bcm2835_delayMicroseconds(us); };

   const int CE=RPI_GPIO_P1_15;

   void clear_ce()
   { bcm2835_gpio_write(CE, LOW); };

   void set_ce()
   { bcm2835_gpio_write(CE, HIGH); };

   void rpi_setup()
   {
      if (!bcm2835_init())
         exit(-1);

      // This sets up P1-15 as the CE for the n24L01
      bcm2835_gpio_fsel(CE, BCM2835_GPIO_FSEL_OUTP);
      clear_ce(); // Make sure the chip is quite until told otherwise

      bcm2835_spi_begin();
      bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
      bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
      bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);    // 3.9 MHz
      bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
      bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

      delay_us(20);
   }

   void rpi_shutdown()
   {
      bcm2835_spi_end();
      bcm2835_close();
   }



   //===============================================================================================
   //Code below should be low-level hardware independant

   void write_reg(char reg, char data)
   {
      char buff[2] = {(char)(W_REGISTER | reg), data};
      write_data(buff, 2);
   };

   char read_reg(char reg)
   {
      char buff[2] = {(char)(R_REGISTER | reg), 0};
      write_data(buff, 2);
      return buff[1];
   };

   void write_reg(char reg, char* data, size_t len)
   {
      char buff[len+1];
      buff[0]=W_REGISTER | reg;
      memcpy(buff+1, data, len);
      write_data(buff, len+1);
   };

   void read_reg(char reg, char* data, size_t len)
   {
      char buff[len+1];
      buff[0]=R_REGISTER | reg;
      memcpy(buff+1, data, len);
      write_data(buff, len+1);
   };


   void configure_base()
   {
      write_reg(CONFIG, CONFIG_EN_CRC | CONFIG_MASK_TX_DS | CONFIG_MASK_MAX_RT);
      write_reg(SETUP_RETR, SETUP_RETR_ARC_0); // auto retransmit off
      write_reg(SETUP_AW, SETUP_AW_3BYTES);  // 3 byte addresses
      write_reg(RF_SETUP, 0x07);  // 1Mbps data rate, 0dBm
      write_reg(RF_CH, 0x02); // use channel 2

      // Clear the various interrupt bits
      write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);

      //shouldn't have to do this, but it won't TX if you don't
      write_reg(EN_AA, 0x00); //disable auto-ack, RX mode
   }

   // Setup device as primary receiver
   void configure_PRX(void)
   {
      nRF24L01_IO_DEBUG3("config PRX");
      configure_base();

      char config = read_reg(CONFIG);
      config |= CONFIG_PRIM_RX;
      write_reg(CONFIG, config);

      char buff[addr_len];
      memcpy(buff, ptx_addr, addr_len);
      write_reg(TX_ADDR, buff, addr_len);

      memcpy(buff, ptx_addr, addr_len);
      write_reg(RX_ADDR_P0, buff, addr_len);
      // Enable just pipe 0
      write_reg(EN_RXADDR, 0x01);
   }

   void power_up_PRX()
   {
      write_data(FLUSH_RX);
      char config = read_reg(CONFIG);
      nRF24L01_IO_DEBUG3("powering up");
      config |= CONFIG_PWR_UP;
      write_reg(CONFIG, config);
      delay_us(1500);
      set_ce();
   }

   void read_rx_payload(void* data, unsigned int len)
   {
      char buff[len+1];
      buff[0]=R_RX_PAYLOAD;
      clear_ce();
      write_data(buff, len+1);
      set_ce();
      memcpy(data, buff+1, len);
   }


   // Setup device as the primary transmitter
   void configure_PTX(void)
   {
      nRF24L01_IO_DEBUG3("config PTX");
      configure_base();

      // Note the TX_ADDR must be equal the RX_ADDR_P0 in the PTX
      char buff[addr_len];
      memcpy(buff, ptx_addr, addr_len);
      write_reg(TX_ADDR, buff, addr_len);

      memcpy(buff, ptx_addr, addr_len);
      write_reg(RX_ADDR_P0, buff, addr_len);
   }

   void power_up_PTX()
   {
      write_data(FLUSH_TX);
      char config = read_reg(CONFIG);
      nRF24L01_IO_DEBUG3("powering up");
      config |= CONFIG_PWR_UP;
      write_reg(CONFIG, config);
   };

   void write_tx_payload(void* data, unsigned int len)
   {
      char buff[len+1];
      buff[0]=W_TX_PAYLOAD;
      memcpy(buff+1, data, len);
      write_data(buff, len+1);
   }

   void pulse_CE()
   {
      nRF24L01_IO_DEBUG3("CE high");
      set_ce();
      delay_us(10);
      nRF24L01_IO_DEBUG3("CE low");
      clear_ce();
   }

} // end of namespace nRF24L01

