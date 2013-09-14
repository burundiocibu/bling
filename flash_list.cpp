/*
  flash_list.cpp copyright Jon Little (2013)

This program is used to program all slaves defined in nameList.hpp

*/


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <list>
#include <set>
#include <iomanip>

#include <bcm2835.h>

#include "nrf24l01_defines.hpp"
#include "ensemble.hpp"
#include "nrf_boot.h"
#include "Slave.hpp"
#include "nameList.hpp"

nameList::NameHatInfo testNameList[] =
{
   "Hingle McCringleberry", 207, 7, "F19",
   "Ebreham Moisesus", 208, 8, "F8",
   "Bob Mellon",    202, 2, "F1",
   "Rob Burgundy",  205, 5, "F5",
   "OneThirtyNine", 239, 139, "F6",
   "OneThirty",     230, 130, "F2",
   "OneTwentyEight",228, 128, "F3",
   "TwentyNine",    229, 29,  "F4",
   "OneTwentySix",  226, 126, "F7",
   "OneThirtyFive", 235, 135, "F9",
};

std::ostream& operator<<(std::ostream& s, const Slave& slave)
{
   s << std::left << std::setw(3) << slave.slave_no
     << " " << std::setw(3) << slave.drill_id
     << " " << std::right << std::setw(3) << slave.stateOfCharge << "%"
     << " " << std::left << slave.student_name;
   return s;
}

std::ostream& operator<<(std::ostream& s, const std::list<Slave>& slave_list)
{
   std::list<Slave>::const_iterator j;
   for (j=slave_list.begin(); j != slave_list.end(); j++)
      std::cout << *j << std::endl;
   return s;
}


// returns true for a successfull transmission
bool nrf_tx(uint8_t* buff, size_t len, const unsigned max_retry, unsigned &loss_count);
void nrf_rx(void);
bool prog_slave(uint16_t slave_no, uint8_t* image_buff, size_t image_size);


void write_reg(uint8_t reg, const uint8_t* data, const size_t len);
void write_reg(uint8_t reg, uint8_t data);
uint8_t read_reg(uint8_t reg);
void read_reg(uint8_t reg, uint8_t* data, const size_t len);
void clear_CE(void);
void set_CE(void);
std::string timestamp(void);  // A string timestam for logs
void nrf_setup(int slave);
void nrf_set_slave(int slave);

unsigned debug=0;

void hex_dump(const void* buff, size_t len)
{
   const uint8_t *p = static_cast<const uint8_t*>(buff);
   for (unsigned i=0; i<len; i++)
      printf("%02x", *p++);
}

using namespace std;
using namespace nRF24L01;

int main(int argc, char **argv)
{
   char *input_fn;
   bool test=false;
   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "di:s:t")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 'i': input_fn = optarg; break;
         case 't': test=true; break;
         default:
            printf("Usage %s -i fn [-d]\n", argv[0]);
            exit(-1);
      }

   if (input_fn==NULL)
   {
      printf("Please specify slave image hex file.\n");
      exit(-1);
   }

   FILE* fp = fopen(input_fn, "r");
   if (fp == NULL)
   {
      printf("Could not open %s. Terminating.\n", input_fn);
      exit(-1);
   }

   bool got_image=false;
   char line[80];
   size_t image_size=0;
   uint8_t *image_buff;
   uint8_t *p;

   const size_t ibsize = 0x8000;
   image_buff = (uint8_t*)malloc(ibsize);
   memset(image_buff, 0xff, ibsize);
   
   p = image_buff;
   while (!got_image)
   {
      if (fgets(line, sizeof(line), fp) == NULL)
         break;
      if (line[0] != ':')
         break;
      int len, addr, id;
      sscanf(line+1, "%02x%04x%02x", &len, &addr, &id);

      if (id==0)
      {
         for (int i=0; i<len; i++)
            sscanf(line+9+2*i, "%02x", p++);

         image_size+=len;
      }
      else if (id==1)
         got_image = true;
   }

   if (debug)
      printf("Image length is %d bytes\n", image_size);

   if (!got_image)
   {
      printf("Failed to read image to load. Exiting.\n");
      exit(-1);
   }

   nrf_setup(0);
 
   list<Slave> todo, done, all;
   if (!test)
   {
      for(int i = 0; i < nameList::numberEntries; i++)
         todo.push_back(Slave(nameList::nameList[i].circuitBoardNumber,
                              nameList::nameList[i].hatNumber,
                              nameList::nameList[i].drillId,
                              nameList::nameList[i].name));
   }
   else
   {
      size_t ll = sizeof(testNameList)/sizeof(nameList::NameHatInfo);
      for(int i = 0; i < ll; i++)
         todo.push_back(Slave(testNameList[i].circuitBoardNumber,
                              testNameList[i].hatNumber,
                              testNameList[i].drillId,
                              testNameList[i].name));
   }

   for (int pass=1; todo.size() > 0; pass++)
   {
      cout << "Pass " << pass << endl;
      list<Slave>::iterator i;
      for (i = todo.begin(); i != todo.end(); )
      {
         if (i->drill_id[0] != 'F')
         {
            done.push_back(*i);
            i=todo.erase(i);
         }
         else if (prog_slave(i->slave_no, image_buff, image_size))
         {
            done.push_back(*i);
            cout << timestamp() << " Programmed " << *i << endl;
            i = todo.erase(i);
         }
         else
         {
            cout << timestamp() << " Failed programming " << *i << endl;
            i++;
         }
      }
      if (todo.size())
         cout << "Boards remaining: " << endl
              << todo << endl;
   }

   cout << "All boards programmed." << endl;

   bcm2835_spi_end();
   fclose(fp);
   return 0;
}


bool prog_slave(const uint16_t slave_no, uint8_t* image_buff, size_t image_size)
{
   uint8_t buff[ensemble::message_size];
   buff[0] = boot_magic_word >> 8;
   buff[1] = 0xff & boot_magic_word;
   buff[2] = bl_no_op;

   unsigned loss_count=0;
   unsigned lc;

   nrf_set_slave(slave_no);

   if (debug) 
      printf("%s Looking for slave %d.\n", timestamp().c_str(), slave_no);
   if (!nrf_tx(buff, ensemble::message_size, 5, loss_count))
   {
      if (debug>1)
         printf("%s Slave %d Not Found (loss=%d).\n", timestamp().c_str(), slave_no, loss_count);
      return false;
   }
   if (debug)
      printf("%s Slave %d Found (loss=%d).\n", timestamp().c_str(), slave_no, loss_count);
   
   const unsigned num_pages = image_size/boot_page_size + 1;
   const unsigned chunks_per_page = boot_page_size/boot_chunk_size;

   if (debug)
   {
      printf("%s Programming slave %d [", timestamp().c_str(), slave_no);
      for (int i=0; i<4; i++)
         printf("%02x", (int)ensemble::slave_addr[slave_no][i]);
      printf("]\n");
   }

   nrf_set_slave(slave_no);

   if (debug)
      printf("%s Writing pages ", timestamp().c_str());

   for (int page=0; page < num_pages; page++)
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
            printf("%s load pg:%02x chunk:%02x\n", timestamp().c_str(), page, chunk);
         if (!nrf_tx(buff, ensemble::message_size, 10, loss_count))
            return false;
      }
      buff[2] = bl_write_flash_page;
      if (debug>1)
         printf("%s Write page at %04x\n", timestamp().c_str(), page_addr);
      if (!nrf_tx(buff, ensemble::message_size, 50, loss_count))
         return false;

      buff[2] = bl_check_write_complete;
      if (debug>1)
         printf("%s Write page complete\n", timestamp().c_str());
      if (!nrf_tx(buff, ensemble::message_size, 50, loss_count))
         return false;
   }

   buff[2] = bl_start_app;
   if (debug>2) 
      printf("%s Starting app\n", timestamp().c_str());
   if (!nrf_tx(buff, ensemble::message_size, 10, loss_count))
      return false;

   if (debug)
      printf("%s Slave %d Programmed. (loss=%d)\n", timestamp().c_str(), slave_no, loss_count);

   return true;
}


void nrf_setup(int slave_no)
{
   if (!bcm2835_init())
   {
      printf("Cound not initialize bcm2835 interface. Got root?\n");
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

   uint8_t buff[ensemble::message_size];

   if (read_reg(CONFIG) == 0xff || read_reg(STATUS) == 0xff)
   {
      printf("Failed to find nRF24L01. Exiting.\n");
      exit(-1);
   }

   const uint8_t cfg=CONFIG_EN_CRC | CONFIG_CRCO | CONFIG_MASK_TX_DS | CONFIG_MASK_MAX_RT;
   write_reg(CONFIG, cfg);
   clear_CE();
      
   write_reg(SETUP_RETR, SETUP_RETR_ARC_10 | SETUP_RETR_ARD_1000); // auto retransmit 10 x 750us
   write_reg(SETUP_AW, SETUP_AW_4BYTES);  // 4 byte addresses
   write_reg(RF_SETUP, 0b00001110);  // 2Mbps data rate, 0dBm
   write_reg(RF_CH, ensemble::default_channel); // use channel 2
   write_reg(RX_PW_P0, ensemble::message_size);

   // Clear the various interrupt bits
   write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);
   
   write_reg(TX_ADDR, ensemble::slave_addr[slave_no], ensemble::addr_len);
   write_reg(RX_ADDR_P0, ensemble::slave_addr[slave_no], ensemble::addr_len);
   write_reg(EN_AA, EN_AA_ENAA_P0); // don't know if this is needed on the PTX
   write_reg(EN_RXADDR, EN_RXADDR_ERX_P0);

   buff[0] = FLUSH_RX;
   bcm2835_spi_transfern((char*)&buff, 1);
   buff[0] =FLUSH_TX;
   bcm2835_spi_transfern((char*)&buff, 1);
      
   write_reg(CONFIG, cfg | CONFIG_PWR_UP);
}


void nrf_set_slave(int slave_no)
{
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

   buff[0] = FLUSH_RX;
   bcm2835_spi_transfern((char*)&buff, 1);
   buff[0] =FLUSH_TX;
   bcm2835_spi_transfern((char*)&buff, 1);
      
   write_reg(CONFIG, cfg | CONFIG_PWR_UP);
}


bool nrf_tx(uint8_t* data, size_t len, const unsigned max_retry, unsigned &loss_count)
{
   uint8_t iobuff[len+1];

   for (int j=0; j<max_retry; j++)
   {
      if (debug>1)
         cout << "T";

      iobuff[0] = W_TX_PAYLOAD;
      memcpy(iobuff+1, data, len);
      bcm2835_spi_transfern((char*)iobuff, len+1);
      set_CE();
      bcm2835_delayMicroseconds(10);
      clear_CE();

      for (int i=0; i < 1000; i++)
      {
         uint8_t status = read_reg(STATUS);
         if (status & STATUS_TX_DS)
         {
            write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
            return true;
         }
         else if (status & STATUS_MAX_RT)
         {
            cout << "x" << flush;
            loss_count += read_reg(OBSERVE_TX) & 0x0f;
            write_reg(STATUS, STATUS_MAX_RT);  // clear IRQ
            iobuff[0] = FLUSH_TX;
            bcm2835_spi_transfern((char*)&iobuff, 1);
            break;
         }
         bcm2835_delayMicroseconds(50);
      }
   }

   cout << "X" << endl;
   return false;
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
