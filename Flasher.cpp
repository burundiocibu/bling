/*
  Flasher.cpp copyright Jon Little (2013)
  
  This is the implementation of the Flasher class.
  -----------------------------------------------------------------------------
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <list>
#include <iostream>
#include <iomanip>

#include <bcm2835.h>

#include "nrf24l01_defines.hpp"
#include "ensemble.hpp"
#include "nrf_boot.h"
#include "Flasher.hpp"
#include "messages.hpp"

namespace msg=messages;
using namespace std;
using namespace nRF24L01;

Flasher::Flasher(int debug_level)
   :log("flasher.log", std::ofstream::app),
    debug(debug_level)
{
   if (!bcm2835_init())
   {
      cout << "Cound not initialize bcm2835 interface. Got root?" << endl;
      exit(-1);
   }

   log << endl << timestamp() << " Initializing nRF";

   // This sets up P1-15 as the CE for the n24L01
   bcm2835_gpio_fsel(RPI_GPIO_P1_15, BCM2835_GPIO_FSEL_OUTP);
   clear_CE(); // Make sure the radio is quiet until told otherwise

   bcm2835_spi_begin();
   bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
   bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
   bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);    // 3.9 MHz
   bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
   bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

   uint8_t buff[ensemble::message_size];

   if (read_reg(CONFIG) == 0xff || read_reg(STATUS) == 0xff)
   {
      cout << "Failed to find nRF24L01. Exiting." << endl;;
      exit(-1);
   }

   const uint8_t cfg=CONFIG_EN_CRC | CONFIG_CRCO | CONFIG_MASK_TX_DS | CONFIG_MASK_MAX_RT;
   write_reg(CONFIG, cfg);
   clear_CE();
      
   write_reg(SETUP_RETR, SETUP_RETR_ARC_10 | SETUP_RETR_ARD_500); // auto retransmit 10 x 500us
   write_reg(SETUP_AW, SETUP_AW_4BYTES);  // 4 byte addresses
   write_reg(RF_SETUP, 0b00001110);  // 2Mbps data rate, 0dBm
   write_reg(RF_CH, ensemble::default_channel); // use channel 2
   write_reg(RX_PW_P0, ensemble::message_size);
   write_reg(RX_PW_P1, ensemble::message_size);

   // Clear the various interrupt bits
   write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);
   int slave_no = 0;
   write_reg(TX_ADDR, ensemble::slave_addr[slave_no], ensemble::addr_len);
   write_reg(RX_ADDR_P0, ensemble::slave_addr[slave_no], ensemble::addr_len);
   write_reg(RX_ADDR_P1, ensemble::master_addr, ensemble::addr_len);
   write_reg(EN_AA, EN_AA_ENAA_P0); // don't know if this is needed on the PTX
   write_reg(EN_RXADDR, EN_RXADDR_ERX_P0 | EN_RXADDR_ERX_P1);

   buff[0] = FLUSH_RX;
   bcm2835_spi_transfern((char*)&buff, 1);
   buff[0] =FLUSH_TX;
   bcm2835_spi_transfern((char*)&buff, 1);
      
   write_reg(CONFIG, cfg | CONFIG_PWR_UP);
}


bool Flasher::prog_slave(const uint16_t slave_no, uint8_t* image_buff, size_t image_size, string version)
{
   nrf_set_slave(slave_no);

   rx_version = "unk";
   rx_vcell = 0;
   rx_soc = 0;

   unsigned loss_count=0;
   uint8_t buff[ensemble::message_size];

   msg::encode_ping(buff);
   if (debug>1)
      log << endl << timestamp() << " Sending ping    ";
   if (nrf_tx(buff, sizeof(buff), 1, loss_count))
   {
      if (debug>1)
         log << endl << timestamp() << " Got ACK from ping of slave " << slave_no << "   ";
      nrf_rx();
      if (rx_version == version)
      {
         if (debug)
            log << endl << timestamp() << " Slave " << slave_no << " already has version " 
                << version << " - no programming needed   ";
         return true;
      }  
   }
   else
   {
      log << endl << timestamp() << " No ACK to ping of slave " << slave_no << "   ";
   }

   memset(buff, 0, sizeof(buff));
   buff[0] = boot_magic_word >> 8;
   buff[1] = 0xff & boot_magic_word;
   buff[2] = bl_no_op;

   if (debug>1)
      log << endl << timestamp() << " Looking for slave " << setw(3) << slave_no << "   ";
   if (!nrf_tx(buff, ensemble::message_size, 1, loss_count))
   {
      if (debug>1)
         log << endl << timestamp() << " Slave " << setw(3) << slave_no << " Not Found. lc=" << loss_count << "   ";
      return false;
   }
   
   const unsigned num_pages = image_size/boot_page_size + 1;
   const unsigned chunks_per_page = boot_page_size/boot_chunk_size;

   if (debug)
   {
      log << endl << timestamp() << " Found slave " << setw(3) << slave_no << " [" << hex << setfill('0');
      for (int i=0; i<4; i++)
         log << setw(2) << (int)ensemble::slave_addr[slave_no][i];
      log << dec << setfill(' ') << "] lc=" << loss_count << "   ";
   }

   // Start off with writing a page to address 0x0000 that will jump 
   // to the boot loader
   uint8_t safe_page[] = {0xE0, 0xE0, 0xF0, 0xE7, 0x09, 0x95};
   buff[2] = bl_load_flash_chunk;
   buff[4] = 0x00; 
   buff[5] = 0x00;
   memcpy(buff+6, safe_page, sizeof(safe_page));
   for (int chunk=0; chunk < chunks_per_page; chunk++)
   {
      buff[3] = chunk;
      if (!nrf_tx(buff, ensemble::message_size, 10, loss_count))
         return false;
      memset(buff+6, 0, sizeof(safe_page));
   }

   buff[2] = bl_write_flash_page;
   if (debug>1)
      log << endl << timestamp() << " Write safe page " << "   ";
   if (!nrf_tx(buff, ensemble::message_size, 50, loss_count))
      return false;

   buff[2] = bl_check_write_complete;
   if (debug>1)
      log << endl << timestamp() << " Write page complete " << "   ";
   if (!nrf_tx(buff, ensemble::message_size, 50, loss_count))
      return false;

   for (int page=1; page < num_pages; page++)
   {
      uint16_t page_addr=page*boot_page_size;
      for (int chunk=0; chunk < chunks_per_page; chunk++)
      {
         uint16_t chunk_start=page*boot_page_size + chunk*boot_chunk_size;
         buff[2] = bl_load_flash_chunk;
         buff[3] = chunk;
         buff[4] = page_addr>>8;
         buff[5] = 0xff & page_addr;
         memcpy(buff+6, image_buff + chunk_start, boot_chunk_size);
         if (debug>2)
            log << endl << timestamp() << hex
                 << " Load pg:" << setw(2) << page
                 << " chunk:" << setw(2) << chunk << dec << "   ";
         if (!nrf_tx(buff, ensemble::message_size, 10, loss_count))
            return false;
      }
      buff[2] = bl_write_flash_page;
      if (debug>1)
         log << endl << timestamp() << " Write page " << hex << page_addr << dec << "   ";
      if (!nrf_tx(buff, ensemble::message_size, 50, loss_count))
         return false;

      buff[2] = bl_check_write_complete;
      if (debug>1)
         log << endl << timestamp() << " Write page complete " << "   ";
      if (!nrf_tx(buff, ensemble::message_size, 50, loss_count))
         return false;
   }

   {
      int page=0;
      uint16_t page_addr=page*boot_page_size;
      for (int chunk=0; chunk < chunks_per_page; chunk++)
      {
         uint16_t chunk_start=page*boot_page_size + chunk*boot_chunk_size;
         buff[2] = bl_load_flash_chunk;
         buff[3] = chunk;
         buff[4] = page_addr>>8;
         buff[5] = 0xff & page_addr;
         memcpy(buff+6, image_buff + chunk_start, boot_chunk_size);
         if (debug>2)
            log << endl << timestamp() << hex
                 << " Load pg:" << setw(2) << page
                 << " chunk:" << setw(2) << chunk << dec << "   ";
         if (!nrf_tx(buff, ensemble::message_size, 10, loss_count))
            return false;
      }
      buff[2] = bl_write_flash_page;
      if (debug>1)
         log << endl << timestamp() << " Write page " << hex << page_addr << dec << "   ";
      if (!nrf_tx(buff, ensemble::message_size, 50, loss_count))
         return false;

      buff[2] = bl_check_write_complete;
      if (debug>1)
         log << endl << timestamp() << " Write page complete " << "   ";
      if (!nrf_tx(buff, ensemble::message_size, 50, loss_count))
         return false;
   }

   buff[2] = bl_start_app;
   if (debug>2) 
      log << endl << timestamp() << " Programming complete. Starting app" << "   ";
   if (!nrf_tx(buff, ensemble::message_size, 10, loss_count))
      return false;

   if (debug)
      log << endl << timestamp() << " Programming slave " << slave_no << " complete. lc= " << loss_count << "   ";


   bcm2835_delayMicroseconds(5000);
   msg::encode_ping(buff);

   if (debug>1)
      log << endl << timestamp() << " Sending ping    ";

   if (nrf_tx(buff, sizeof(buff), 1, loss_count))
      nrf_rx();
   else
      log << endl << timestamp() << " No ACK to ping of slave " << slave_no << "   ";

   return true;
}


void Flasher::nrf_set_slave(int slave_no)
{
   log << endl << timestamp() << " Selecting slave " << slave_no << "   "; 
   uint8_t buff[ensemble::message_size];

   const uint8_t cfg=read_reg(CONFIG);

   // power down radio
   write_reg(CONFIG, cfg & ~CONFIG_PWR_UP);
   clear_CE();

   // Clear the various interrupt bits
   write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);

   // Set the new address
   write_reg(TX_ADDR, ensemble::slave_addr[slave_no], ensemble::addr_len);
   write_reg(RX_ADDR_P0, ensemble::slave_addr[slave_no], ensemble::addr_len);

   write_reg(CONFIG, cfg | CONFIG_PWR_UP);
}


bool Flasher::nrf_tx(uint8_t* data, size_t len, const unsigned max_retry, unsigned &loss_count)
{
   uint8_t iobuff[len+1];

   for (int j=0; j<max_retry; j++)
   {
      // a 22 byte payload + 4 byte addr + 2 byte crc + 17 bits framing = 241 bits
      // at 2 MHz data rate = 120 uS
      // with ARD=500uS and ARC=10, MAX_RT should be asserted within 
      // (130 + 120 + 500 + 130) x 10 = 8.8 mS
      // after transmission.

      if (debug>1)
         log << "T" << j;

      iobuff[0] = W_TX_PAYLOAD;
      memcpy(iobuff+1, data, len);
      bcm2835_spi_transfern((char*)iobuff, len+1);
      set_CE();
      bcm2835_delayMicroseconds(10);
      clear_CE();

      int i;
      const unsigned tx_to = 90; // 8.8 mS / 100 uS plus a little slop
      for (i=0; i < tx_to; i++)
      {
         uint8_t status = read_reg(STATUS);
         if (status & STATUS_TX_DS)
         {
            if (debug>1)
               log << "!" << flush;
            write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
            loss_count += read_reg(OBSERVE_TX) & 0x0f;
            return true;
         }
         else if (status & STATUS_MAX_RT)
         {
            if (debug>1)
               log << "x";
            loss_count += read_reg(OBSERVE_TX) & 0x0f;
            write_reg(STATUS, STATUS_MAX_RT);  // clear IRQ
            iobuff[0] = FLUSH_TX;
            bcm2835_spi_transfern((char*)&iobuff, 1);
            break;
         }
         bcm2835_delayMicroseconds(100);
      }
      if (debug>1 && i>=tx_to)
         log << endl << timestamp() << " Missed MAX_RT    ";
   }

   if (debug>1) 
      log << "X" << flush;
   return false;
}


bool Flasher::nrf_rx(void)
{
   uint8_t buff[ensemble::message_size+1];
   uint8_t pipe;
   char config = read_reg(CONFIG);
   write_reg(CONFIG, config | CONFIG_PRIM_RX); // should still be powered on
   set_CE();

   int i;
   for (i=0; i<100; i++)
   {
      if (read_reg(STATUS) & STATUS_RX_DR)
      {
         buff[0]=R_RX_PAYLOAD;
         clear_CE();
         bcm2835_spi_transfern((char*)buff, sizeof(buff));
         set_CE();
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }
      bcm2835_delayMicroseconds(200);
   }

   write_reg(CONFIG, config); // should still be powered on
   clear_CE();
   bcm2835_delayMicroseconds(200);
   buff[0] = FLUSH_RX;
   bcm2835_spi_transfern((char*)&buff, 1);
   buff[0] = FLUSH_TX;
   bcm2835_spi_transfern((char*)&buff, 1);


   if (i==100)
   {
      log << endl << timestamp() << " Ping response not received.   ";
      return false;
   }


   uint32_t t_rx;
   uint16_t slave_id, missed_message_count, vcell, soc;
   uint8_t msg_id, freshness_count;
   int8_t major_version, minor_version;

   msg::decode_status(buff+1, slave_id, t_rx, major_version, minor_version,
                      vcell, soc, missed_message_count, freshness_count);

   rx_vcell = 1e-3 * vcell;
   rx_soc = int(0xff & (soc >> 8));
   rx_version = to_string((int)major_version) + "." + to_string((int)minor_version);
   log << endl << timestamp() << " Ping response: slave " << slave_id 
       << ", version " << rx_version
       << ", Vbatt " << rx_vcell
       << ", SOC " <<  rx_soc << "%"
       << ", i=" << i;
   return true;
}


void Flasher::clear_CE(void)
{
   bcm2835_gpio_write(RPI_GPIO_P1_15, LOW);
}


void Flasher::set_CE(void)
{
   bcm2835_gpio_write(RPI_GPIO_P1_15, HIGH);
}


void Flasher::write_reg(uint8_t reg, const uint8_t* data, const size_t len)
{
   char iobuff[len+1];
   iobuff[0]=W_REGISTER | reg;
   memcpy(iobuff+1, data, len);
   bcm2835_spi_transfern(iobuff, len+1);
}


void Flasher::write_reg(uint8_t reg, uint8_t data)
{
   char iobuff[2];
   iobuff[0]=W_REGISTER | reg;
   iobuff[1]=data;
   bcm2835_spi_transfern(iobuff, 2);
}


uint8_t Flasher::read_reg(uint8_t reg)
{
   char iobuff[2];
   iobuff[0]=R_REGISTER | reg;
   iobuff[1] = 0;
   bcm2835_spi_transfern((char*)iobuff, 2);
   return iobuff[1];
}


void Flasher::read_reg(uint8_t reg, uint8_t* data, const size_t len)
{
   char iobuff[2];
   iobuff[0]=R_REGISTER | reg;
   iobuff[1]=0;
   memcpy(iobuff+1, data, len);
   bcm2835_spi_transfern((char*)iobuff, len+1);
}


std::string Flasher::timestamp(void)
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   time_t now_time = tv.tv_sec;
   struct tm now_tm;
   localtime_r((const time_t*)&tv.tv_sec, &now_tm);
   char b1[32], b2[32];
   strftime(b1, sizeof(b1), "%m/%d %H:%M:%S", &now_tm);
   snprintf(b2, sizeof(b2), "%s.%03d", b1, tv.tv_usec/1000);
   return string(b2);
}
