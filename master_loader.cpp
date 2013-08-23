/*
  master_loader.cpp copyright Jon Little (2013)

  This is the programmer side of the boot loader implemented in nrf_boot.cpp.
  It is written to run on a raspberry pi with a nRF24L01+ radio conected to
  its SPI interface. See nrf_boot.cpp for details about the protocol that
  is used across the radio. This program bascially reads in an SREC file,
  chopps it up and feeds it to the slave to be programmed.

  http://en.wikipedia.org/wiki/SREC_(file_format)

  While this code uses some constants from nrf24l01_defines it also uses
  some rourintes from nrf24l01.cpp. The latter needs to be fixed so the
  boot loader implementation stands apart from the regular code.
*/


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <sys/time.h>
#include <time.h>

#include <bcm2835.h>

#include "nrf24l01_defines.hpp"
#include "ensemble.hpp"
#include "nrf_boot.h"


bool nrf_tx(uint8_t* buff, size_t len);
void nrf_rx(void);
void write_reg(uint8_t reg, const uint8_t* data, const size_t len);
void write_reg(uint8_t reg, uint8_t data);
uint8_t read_reg(uint8_t reg);
void read_reg(uint8_t reg, uint8_t* data, const size_t len);
void clear_CE(void);
void set_CE(void);
std::string timestamp(void);  // A string timestam for logs
void nrf_setup(int slave);


using namespace std;
using namespace nRF24L01;


int main(int argc, char **argv)
{
   int verbose=1,debug=1;
   string input_fn("slave_main.hex");
   unsigned slave_no=2;

   if (slave_no == 0)
   {
      cout << "We don't program slave 0 (everybody at once) yet. Specify slave." << endl;
      exit(1);
   }
   else if (slave_no >= ensemble::num_slaves)
   {
      cout << "Invalid slave number: " << slave_no << ". Terminating" << endl;
      exit(-1);
   }

   cout << timestamp() <<  " programming slave " << slave_no << " [";
   for (int i=0;i<4;i++)
      printf("%02x", (int)ensemble::slave_addr[slave_no][i]);
   printf("]\n");

   nrf_setup(slave_no);

   struct timeval tv;
   gettimeofday(&tv, NULL);
   double t0 = tv.tv_sec + 1e-6*tv.tv_usec;
   double t_hb = t0;
      
   for (long i=0; i < 200000; i++)
   {
      gettimeofday(&tv, NULL);
      double t = tv.tv_sec + 1e-6*tv.tv_usec;

      if (t - t_hb >= 1.0)
      {
         uint8_t *p = buff;
         buff[0] = 0xff & (boot_magic_word >> 8);
         buff[1] = 0xff & (boot_magic_word);
         buff[2] = bl_no_op;
         bool ack = false;
         if (debug) cout << timestamp();
         ack = nrf_tx(buff, boot_message_size);
         if (!ack)
            cout << " x";
         else
            cout << " !";
         if (debug) cout << endl;
         t_hb = t;
      }
      bcm2835_delayMicroseconds(10000);
   }

   bcm2835_spi_end();
   cout << "Done programming" << endl;
   return 0;
}


void nrf_setup(int slave_no)
{
   if (!bcm2835_init())
   {
      cout << "Cound not initialize bcm2835 interface. Got root?" << endl;
      exit(-1);
   }

   // This sets up P1-15 as the CE for the n24L01
   bcm2835_gpio_fsel(RPI_GPIO_P1_15, BCM2835_GPIO_FSEL_OUTP);
   clear_CE(); // Make sure the chip is quiet until told otherwise

   bcm2835_spi_begin();
   bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
   bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
   bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);    // 3.9 MHz
   bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
   bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

   uint8_t buff[boot_message_size];

   if (read_reg(CONFIG) == 0xff || read_reg(STATUS) == 0xff)
   {
      cout << "Failed to find nRF24L01. Exiting." << endl;
      return -1;
   }

   const uint8_t cfg=CONFIG_EN_CRC | CONFIG_CRCO | CONFIG_MASK_TX_DS | CONFIG_MASK_MAX_RT;
   write_reg(CONFIG, cfg);
   clear_CE();
      
   write_reg(SETUP_RETR, SETUP_RETR_ARC_4); // auto retransmit 3 x 250us
   write_reg(SETUP_AW, SETUP_AW_4BYTES);  // 4 byte addresses
   write_reg(RF_SETUP, 0b00001110);  // 2Mbps data rate, 0dBm
   write_reg(RF_CH, default_channel); // use channel 2
   write_reg(RX_PW_P0, boot_message_size);

   // Clear the various interrupt bits
   write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);

   write_reg(RX_ADDR_P0, ensemble::master_addr, ensemble::addr_len);
   write_reg(TX_ADDR, ensemble::slave_addr[slave_no], ensemble::addr_len);
   write_reg(EN_AA, EN_AA_ENAA_P0); // don't know if this is needed on the PTX
   write_reg(EN_RXADDR, EN_RXADDR_ERX_P0);

   buff[0] = FLUSH_RX;
   bcm2835_spi_transfern((char*)&buff, 1);
   buff[0] =FLUSH_TX;
   bcm2835_spi_transfern((char*)&buff, 1);
      
   write_reg(CONFIG, cfg | CONFIG_PWR_UP);
}


bool nrf_tx(uint8_t* data, size_t len)
{
   bool success=false;
   static unsigned t_tx=0, nack_cnt=0, retry_cnt=0;
   static int dt=0;
   uint8_t iobuff[len+1];

   iobuff[0] = W_TX_PAYLOAD;
   memcpy(iobuff+1, data, len);
   bcm2835_spi_transfern((char*)iobuff, len+1);

   set_CE();
   bcm2835_delayMicroseconds(10);
   clear_CE();

   for (int i=0; i < 200; i++)
   {
      uint8_t status = read_reg(STATUS);
      if (status & STATUS_MAX_RT)
      {
         nack_cnt++;
         write_reg(STATUS, STATUS_MAX_RT);  // clear IRQ
         iobuff[0] = FLUSH_TX;
         bcm2835_spi_transfern((char*)&iobuff, 1);
         break;
      }
      else if (status & STATUS_TX_DS)
      {
         write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
         success=true;
         break;
      }
      bcm2835_delayMicroseconds(10);
   }

   uint8_t obs_tx = read_reg(OBSERVE_TX);
   retry_cnt += obs_tx & 0x0f;
   cout << " retry: " << retry_cnt;
   return success;
}


void nrf_rx(uint8_t* buff, size_t len)
{
   using namespace nRF24L01;

   int rx_dt;
   unsigned no_resp;
   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   bcm2835_delayMicroseconds(120);  // Really should be 1.5 ms at least
   set_CE();

   for (int i=0; i<100; i++)
   {
      uint8_t status = read_reg(STATUS);
      if (status & STATUS_RX_DR)
      {
         uint8_t iobuff[len+1];
         iobuff[0]=R_RX_PAYLOAD;
         clear_CE();
         bcm2835_spi_transfern((char*)iobuff, len+1);
         set_CE();
         memcpy(buff, iobuff+1, len);
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }
   }

   // Config the nRF as PTX again
   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();
}


void clear_CE(void)
{
   bcm2835_gpio_write(RPI_GPIO_P1_15, LOW);
}


void set_CE(void)
{
   bcm2835_gpio_write(RPI_GPIO_P1_15, HIGH);
}


void write_reg(uint8_t reg, const uint8_t* data, const size_t len)
{
   char iobuff[len+1];
   iobuff[0]=W_REGISTER | reg;
   memcpy(iobuff+1, data, len);
   bcm2835_spi_transfern(iobuff, len+1);
}


void write_reg(uint8_t reg, uint8_t data)
{
   char iobuff[2];
   iobuff[0]=W_REGISTER | reg;
   iobuff[1]=data;
   bcm2835_spi_transfern(iobuff, 2);
}


uint8_t read_reg(uint8_t reg)
{
   char iobuff[2];
   iobuff[0]=R_REGISTER | reg;
   iobuff[1] = 0;
   bcm2835_spi_transfern((char*)iobuff, 2);
   return iobuff[1];
}


void read_reg(uint8_t reg, uint8_t* data, const size_t len)
{
   char iobuff[2];
   iobuff[0]=R_REGISTER | reg;
   iobuff[1]=0;
   memcpy(iobuff+1, data, len);
   bcm2835_spi_transfern((char*)iobuff, len+1);
}


std::string timestamp(void)
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   time_t now_time = tv.tv_sec;
   struct tm now_tm;
   localtime_r((const time_t*)&tv.tv_sec, &now_tm);
   char b1[32], b2[32];
   strftime(b1, sizeof(b1), "%H:%M:%S", &now_tm);
   snprintf(b2, sizeof(b2), "%s.%03d", b1, tv.tv_usec/1000);
   return string(b2);
}
