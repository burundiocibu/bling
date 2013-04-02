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


//===============================================================================================
// This part of the namespace is the hardware-dependant part of the interface
#define nRF24L01_IO_DEBUG1(str) //RunTime::puts(); printf("  %s %.2x\n", str, data)
#define nRF24L01_IO_DEBUG2(str) //RunTime::puts(); printf("  %s ",str); dump(data, len)
#define nRF24L01_IO_DEBUG3(str) //RunTime::puts(); printf("  %s\n",str)


namespace nRF24L01
{
const size_t addr_len=3;
const char ptx_addr[]={0xE1, 0xE3, 0xE5};
const char prx_addr[]={0xB1, 0xB3, 0xB5};
char iobuff[16];



void write_data(char* data, size_t len)
{
   nRF24L01_IO_DEBUG2("tx");
#ifdef AVR
   avr_spi::tx(data, data, len);
#else
   bcm2835_spi_transfern(data, len);
#endif
   nRF24L01_IO_DEBUG2("rx");
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


void clear_CE()
{
#ifdef AVR
   PORTB &= ~_BV(PB6);
#else
   bcm2835_gpio_write(RPI_GPIO_P1_15, LOW);
#endif
}


void set_CE()
{
#ifdef AVR
   PORTB |= _BV(PB6);
#else
   bcm2835_gpio_write(RPI_GPIO_P1_15, HIGH);
#endif
}

bool nrf_rx_flag=false;

#ifdef AVR
int rx_flag=0;

ISR(PCINT0_vect)
{
   rx_flag++;
}
#endif


bool setup()
{
#ifdef AVR
   avr_spi::setup();

   // use B.6 as the CE to the nrf24l01
   DDRB |= _BV(PB6);

   rx_flag=0;
   // use PCINT4 (PB4) to signal the MCU we has data.
   cli();
   PCICR |= _BV(PCIE0); // enable pin change interrupts
   PCMSK0 |= _BV(PCINT4); // enable PCINT4
   PCIFR &= ~_BV(PCIF0); // clearn any pending PCINT interrupt
   sei();

   return true;
#else
   if (!bcm2835_init())
      return false;

   // This sets up P1-15 as the CE for the n24L01
   bcm2835_gpio_fsel(CE, BCM2835_GPIO_FSEL_OUTP);
   clear_ce(); // Make sure the chip is quiet until told otherwise

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

void shutdown()
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
   write_reg(SETUP_RETR, SETUP_RETR_ARC_0); // auto retransmit off
   write_reg(SETUP_AW, SETUP_AW_3BYTES);  // 3 byte addresses
   write_reg(RF_SETUP, 0x07);  // 1Mbps data rate, 0dBm
   write_reg(RF_CH, 0x02); // use channel 2

   // Clear the various interrupt bits
   write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);

   //shouldn't have to do this, but it won't TX if you don't
   write_reg(EN_AA, 0x00); //disable auto-ack, RX mode
   return true;
}


// Setup device as primary receiver
void configure_PRX(void)
{
   nRF24L01_IO_DEBUG3("config PRX");

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
	  iobuff[0]=FLUSH_RX;
      write_data(iobuff, 1);
      char config = read_reg(CONFIG);
      nRF24L01_IO_DEBUG3("powering up");
      config |= CONFIG_PWR_UP;
      write_reg(CONFIG, config);
      delay_us(1500);
      set_CE();
   }


   void read_rx_payload(void* data, const size_t len)
   {
      iobuff[0]=R_RX_PAYLOAD;
      clear_CE();
      write_data(iobuff, len+1);
      set_CE();
      memcpy(data, iobuff+1, len);
   }


   // Setup device as the primary transmitter
   void configure_PTX(void)
   {
      nRF24L01_IO_DEBUG3("config PTX");

      // Note the TX_ADDR must be equal the RX_ADDR_P0 in the PTX
      char buff[addr_len];
      memcpy(buff, ptx_addr, addr_len);
      write_reg(TX_ADDR, buff, addr_len);

      memcpy(buff, ptx_addr, addr_len);
      write_reg(RX_ADDR_P0, buff, addr_len);
   }


   void power_up_PTX()
   {
	  iobuff[0]=FLUSH_TX;
      write_data(iobuff, 1);
      char config = read_reg(CONFIG);
      nRF24L01_IO_DEBUG3("powering up");
      config |= CONFIG_PWR_UP;
      write_reg(CONFIG, config);
   }


   void write_tx_payload(void* data, const size_t len)
   {
      iobuff[0]=W_TX_PAYLOAD;
      memcpy(iobuff+1, data, len);
      write_data(iobuff, len+1);
   }


   void pulse_CE()
   {
      nRF24L01_IO_DEBUG3("CE high");
      set_CE();
      delay_us(10);
      nRF24L01_IO_DEBUG3("CE low");
      clear_CE();
   }
}
