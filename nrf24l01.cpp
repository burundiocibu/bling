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

#define BCMLIB 1
#ifdef BCMLIB
#include <bcm2835.h>
#else
#include <string>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#endif
#endif

#include "nrf24l01.hpp"
#include "avr_rtc.hpp"
#include "messages.hpp"
#include "ensemble.hpp"
#include "rt_utils.hpp"


extern RunTime runtime;

namespace nRF24L01
{
   uint8_t channel;
   char master_addr[addr_len];
   char broadcast_addr[addr_len];
   char slave_addr[addr_len];
   char iobuff[ensemble::message_size+1];

#ifndef BCMLIB
   int spi_fd=-1;
   int ce_fd=-1;
   uint8_t bits = 8;
   uint32_t speed = 3900000;
   uint16_t delay = 0;
#endif

   void write_data(char* data, size_t len)
   {
      long long t0=runtime.usec();
#ifdef AVR
      avr_spi::tx(data, data, len);
#else
#ifdef BCMLIB
      bcm2835_spi_transfern(data, len);
#else
      static uint8_t rx[1024];
      struct spi_ioc_transfer tr;
      tr.tx_buf = (unsigned long)data;
      tr.rx_buf = (unsigned long)rx;
      tr.len = len;
      tr.delay_usecs = delay;
      tr.speed_hz = speed;
      tr.bits_per_word = bits;
      
      int rc = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
      if (rc < 1)
         printf("can't send spi message");
      
      memcpy(data, rx, len);
#endif
#endif
      long long t1=runtime.usec();
      printf("WR dt: %lld \n", t1-t0);
   }


   void delay_us(uint32_t us)
   {
#ifdef AVR
      for (uint16_t i=0; i<us; i++)
         _delay_us(1);
#else
#ifdef BCMLIB
      bcm2835_delayMicroseconds(us);
#else
      usleep(us);
      // nanosleep(us*1000);
#endif
#endif
   }


   void clear_CE(void)
   {
#ifdef AVR
      PORTD &= ~_BV(PD3);
#else
#ifdef BCMLIB
      bcm2835_gpio_write(RPI_GPIO_P1_15, LOW);
#else
      if (write(ce_fd, "0", 1) != 1)
         printf("Failed to clear CE\n");
#endif
#endif
   }


   void set_CE(void)
   {
#ifdef AVR
      PORTD |= _BV(PD3);
#else
#ifdef BCMLIB
      bcm2835_gpio_write(RPI_GPIO_P1_15, HIGH);
#else
   if (write(ce_fd, "1", 1) != 1)
      printf("Failed to set CE\n");
#endif
#endif
   }


#ifdef AVR
   uint32_t t_rx;

   ISR(INT0_vect)
   {
      t_rx = avr_rtc::t_ms;
   }
   
   void clear_IRQ(void)
   {
      write_reg(STATUS, STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT); // clear all the IRQ bits!
   }

#endif


   bool setup(void)
   {
      //if (num_chan != sizeof(slave_addr)/addr_len)
      //   return false;

#ifdef AVR
      avr_spi::setup();

      // use D3 as the CE to the nrf24l01
      DDRD |= _BV(PD3);
      clear_CE();

      // use INT0 (PD2) to signal the MCU we has data.
      cli();
      EICRA = _BV(ISC01); // falling edge interrupts
      EIMSK |= _BV(INT0); // enable INT0
      EIFR &= ~_BV(INTF0);  // clear any pending INT0 interrupt
      sei();
      
      return true;
#else
      
#ifdef BCMLIB
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

#else
      const char *spi_device = "/dev/spidev0.0";
      
      spi_fd = open(spi_device, O_RDWR);
      if (spi_fd < 0)
      {
         printf("can't open spi_device");
         return false;
      }

      uint8_t mode=SPI_MODE_0;
      int ret;
      ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
      if (ret == -1)
      {
         close(spi_fd);
         printf("can't set spi mode\n");
         return false;
      }

      ret = ioctl(spi_fd, SPI_IOC_RD_MODE, &mode);
      if (ret == -1)
      {
         close(spi_fd);
         printf("can't get spi mode\n");
         return false;
      }

      // bits per word
      ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
      if (ret == -1)
      {
         close(spi_fd);
         printf("can't set bits per word\n");
         return false;
      }

      ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
      if (ret == -1)
      {
         close(spi_fd);
         printf("can't get bits per word\n");
         return false;
      }

      //max speed hz
      ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
      if (ret == -1)
      {
         close(spi_fd);
         printf("can't set max speed hz\n");
         return false;
      }

      ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
      if (ret == -1)
      {
         close(spi_fd);
         printf("can't get max speed hz\n");
         return false;
      }

      printf("spi mode: %d\n", mode);
      printf("bits per word: %d\n", bits);
      printf("speed: %d KHz\n", speed/1000);


      // Now to set up the CE line
      std::string pin="15"; // AKA header1 pin 10, AKA RxD
      std::string dir_path="/sys/class/gpio/gpio"+pin+"/direction";
      std::string dir="out";
      std::string pin_path="/sys/class/gpio/gpio"+pin+"/value";
      int fd;

      fd = open("/sys/class/gpio/export", O_WRONLY);
      if (fd == -1)
      {
         printf("Failed to open gpio for writing!\n");
         return false;
      }
      write(fd, pin.c_str(), pin.length());
      close(fd);
        
      fd = open(dir_path.c_str(), O_WRONLY);
      if (fd == -1)
      {
         printf("Failed to open gpio direction for writing!\n");
         return false;
      }
      if (write(fd, dir.c_str(), dir.length()) != dir.length())
      {
         printf("Failed to set direction!\n");
         return false;
      }
      close(fd);

      ce_fd = open(pin_path.c_str(), O_WRONLY);
      if (ce_fd == -1)
      {
         printf("Failed to open pin for I/O!\n");
         return false;
      }
   
#endif

      return true;
#endif
   }

   void shutdown(void)
   {
#ifdef AVR
#else
#ifdef BCMLIB
      bcm2835_spi_end();
#else
#endif
      printf("Need to write shutdown code for gpio & spi\n");
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
      if (read_reg(CONFIG) == (char)0xff || read_reg(STATUS) == (char)0xff)
         return false;
      
      const uint8_t cfg=CONFIG_EN_CRC | CONFIG_CRCO | CONFIG_MASK_TX_DS | CONFIG_MASK_MAX_RT;
      write_reg(CONFIG, cfg);
      if (read_reg(CONFIG) != cfg)
         return false;
      
      write_reg(SETUP_RETR, SETUP_RETR_ARC_4); // auto retransmit 3 x 250us

      write_reg(SETUP_AW, SETUP_AW_4BYTES);  // 4 byte addresses
      write_reg(RF_SETUP, 0b00001110);  // 2Mbps data rate, 0dBm
      write_reg(RF_CH, channel); // use channel 2

      write_reg(nRF24L01::RX_PW_P0, ensemble::message_size);
      write_reg(nRF24L01::RX_PW_P1, ensemble::message_size);
      write_reg(nRF24L01::RX_PW_P2, ensemble::message_size);

      // Clear the various interrupt bits
      write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);

      write_reg(EN_AA, 0x00); //disable auto-ack, RX mode

      char buff=FLUSH_RX;
      write_data(&buff, 1);
      buff=FLUSH_TX;
      write_data(&buff, 1);

      return true;
   }


   // Setup device as primary receiver
   void configure_PRX(void)
   {
      char config = read_reg(CONFIG);
      config |= CONFIG_PRIM_RX;
      write_reg(CONFIG, config & ~CONFIG_PWR_UP);

      char buff[addr_len];
      memcpy(buff, master_addr, addr_len); // only TX to master
      write_reg(TX_ADDR, buff, addr_len);

      // pipe 0 is for receiving broadcast
      memcpy(buff, broadcast_addr, addr_len);
      write_reg(RX_ADDR_P0, buff, addr_len);

      // pipe 1 is this each slave's private address
      memcpy(buff, slave_addr, addr_len);
      write_reg(RX_ADDR_P1, buff, addr_len);

      // In case we want to receiver an ACK from the master
      memcpy(buff, master_addr, addr_len);
      write_reg(RX_ADDR_P2, buff, addr_len);

      write_reg(EN_RXADDR, EN_RXADDR_ERX_P0 | EN_RXADDR_ERX_P1);
      write_reg(EN_AA, EN_AA_ENAA_P1);  // auto ack on pipe 1 only

      // after power up, can't write to most registers anymore
      write_reg(CONFIG, config | CONFIG_PWR_UP);
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
   void configure_PTX()
   {
      clear_CE();
      char config = read_reg(CONFIG);
      config &= ~CONFIG_PRIM_RX;
      write_reg(CONFIG, config & ~CONFIG_PWR_UP); // power down

      // Use Pipe 1 for responses to the master
      char buff[addr_len];
      memcpy(buff, master_addr, addr_len);
      write_reg(RX_ADDR_P1, buff, addr_len);

      write_reg(EN_RXADDR, EN_RXADDR_ERX_P0 | EN_RXADDR_ERX_P1);
      flush_tx();
      write_reg(CONFIG, config | CONFIG_PWR_UP);
   }


   void write_tx_payload(void* data, const size_t len, const char slave_addr[], bool ack)
   {
      char config = read_reg(CONFIG);
      write_reg(CONFIG, config  & ~CONFIG_PWR_UP); // power down
      if (ack)
         write_reg(EN_AA, EN_AA_ENAA_P0);
      else
         write_reg(EN_AA, 0);
      write_reg(CONFIG, config | CONFIG_PWR_UP); // power back up
      // Really should have at least a 1.5 ms delay here but we don't seem to need it.

      //iobuff[0] = slave==0 ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD;;
      iobuff[0] = W_TX_PAYLOAD;
      memcpy(iobuff+1, data, len);
      write_data(iobuff, len+1);

      // Note the TX_ADDR must be equal the RX_ADDR_P0 in the PTX
      // for acks to work
      char buff[addr_len];
      memcpy(buff, slave_addr, addr_len);
      write_reg(TX_ADDR, buff, addr_len);

      memcpy(buff, slave_addr, addr_len);
      write_reg(RX_ADDR_P0, buff, addr_len);

      // pulse CE
      long long t0=runtime.usec();
      set_CE();
      delay_us(10);
      clear_CE();
      long long t1=runtime.usec();
      printf("CE dt: %lld \n", t1-t0);
   }


   void flush_tx(void)
   {
      char buff=FLUSH_TX;
      write_data(&buff, 1);
   }

}
