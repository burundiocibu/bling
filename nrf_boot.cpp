/*
  nrf_boot.c copyright Jon Little (2013)
  
  This is a boot loader that was derived from kavr by Frank Edelhaeuser.
  
  It uses allows reprogramming via an nRF24L01+ that is attached to the
  SPI interface as follows:
  AVR    nRF24L01
  ----   --------
  MISO	 MISO
  MOSI	 MOSI
  SCK    SCK
  SS	 CSN
  PCINT4 IRQ
  PD3    CE

  The slave is the AVR being programmed while the master is the device
  sending the program to the slave. This bootloader is intended to be
  used with a bunch of slaves available to a given master. Part of this
  bootloader is some slave-unique data that is stored in the EEPROM. This
  data includes a serial number, a slave unique nRF address, a common
  master address, a broadcast address that all slaves may listen to, and
  possibilly some application specific data.

  Each slave will be programmed individually by the master using the slave's
  unique address. (Maybe later I'll be motivated to make a broadcast reprogramming
  but dealing with the retransmissions won't be trivial).

  The master will signal that it wants to program a slave by sending page write
  commands to the slave's unique address. This first write command should be
  addressed to 0x0000 on the AVR and will be a dummy page that will have
  the slave do a boot reset. This will keep the slave in the bootloader until
  this page is successfully written at the end of a programming cycle.

  The nRF's checksum will be use to verify data transmission and the slave
  will use the nRF's ACK to communicate successfull reception of each packet.
  After the write command is received, the slave should turn off auto ack on
  the radio until the write has finished.

  data protocol:
chunk=16 bytes
chunk number:  offset into page chunk / 16
nRF message size: 22 bytes

common:
  buff[0,2] = frame word: 0xbabe
  buff[2,1] = id

id=0: no-op

id=1: load page chunk
  buff[3,1] = chunk number
  buff[4,2] = dest page address
  buff[6,16] = chunk

id=2: write page buffer
  buff[3,1] = # of chunks that should have been loaded
  buff[4,2] = dest page address

id=3: check write complete
  buff[3,1] = unused
  buff[4,2] = address that should have been written

id=5: start app
  buff[3,1] = alt addr flag, 1 = jump to address below
  buff[4,2] = jump address

id=4: eeprom write
  buff[3,1] = length (must be less than 16) 
  buff[4,2] = address
  buff[6,length] = eeprom data to be written

id=12: read flash request
  buff[3,1] = # of chunks that are requested
  buff[4,2] = source page address

id=11:
  buff[3,1] = chunk number
  buff[4,2] = source page address
  buff[6,16] = chunk


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

#include <avr/boot.h>
#include <avr/common.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "slave_eeprom.h"
#include "nrf24l01_defines.hpp"

// macros
#define PACKED          __attribute__((__packed__))
#define NOINLINE        __attribute__((noinline))
#define NAKED           __attribute__((naked))
#define SECTION(a)      __attribute__((section(a)))



// here are some defines and such that need to be shared with the
// master
const size_t addr_len = 4;
const size_t message_size = 22;
const size_t chunk_size = 16;
const uint16_t frame_word = 0xbabe;
enum bootloader_msg_ids
{
   no_op = 0,
   load_flash_chunk = 1,
   write_flash_page = 2,
   check_write_complete = 3,
   write_eeprom = 4,
   start_app = 5,
   read_flash_request = 12,
   read_flash_chunk = 13,
   read_eeprom = 14
};   


// Some local buffers
uint8_t channel;
char master_addr[addr_len];
char broadcast_addr[addr_len];
char slave_addr[addr_len];


void flash_page_write(uint16_t addr, uint8_t* buff);
void spi_transact(uint8_t* p_rtx, const short len);   
void clear_CE(void);
void set_CE(void);
void nrf_setup(void);
void write_reg(uint8_t reg, uint8_t* data, const size_t len);
void write_reg(uint8_t reg, uint8_t data);
char read_reg(uint8_t reg);
void read_reg(uint8_t reg, uint8_t* data, const size_t len);
uint8_t read_payload(uint8_t* buff, const size_t len);



// defined in linker script
extern void* __bss_end;

// It appears that when the -nostartfiles option is given
// to the loader, this doesn't need to be called 'main'
void kavr(void) NAKED SECTION(".vectors");
void kavr(void)
{
   // Instead of linking __init:
   // (how to avoid this assembly code? Anyone?)
   asm volatile ("clr r1");
   asm volatile ("ldi r28, lo8(%0)" :: "i" (&__bss_end));
   asm volatile ("ldi r29, hi8(%0)" :: "i" (&__bss_end));

   // we don't need no stinking interrupts
   cli();

   // Turn off the 12V supply
   DDRB |= _BV(PB1);
   PORTB &= ~_BV(PB1);

   // Set the tlc5940 BLANK line to to disable PWM output
   DDRC  |= _BV(PC2);
   PORTC |= _BV(PC2);

   // For now, simply jump right into the user app.
   typedef void APP(void);
   ((APP*)0)();
   
   // Initialize the nRF interface
   nrf_setup();
   uint8_t rx_buff[message_size];

   // Initialize flash programming state
   uint32_t curr_page = 0xffff; // Address of page we are working on
   uint8_t page_buff[SPM_PAGESIZE]; // where we build up this page
   uint8_t last_chunk=0; // last chunk that was wrtten into the page_buff

   // Countdown till giving up and jumping to the user app
   const unsigned timeout_lim = 1000;
   unsigned timeout_cnt=0;
   
   while (timeout_cnt < timeout_lim)
   {
      // check to see if we received any data
      uint8_t status=read_reg(nRF24L01::STATUS);
      if (status == 0x0e)
      {
         timeout_cnt++;
         _delay_us(1000);
         continue;
      }

      //read_payload(rx_buff, message_size);

      const uint16_t rx_frame_word = rx_buff[0] << 8 | rx_buff[1];
      if (rx_frame_word != frame_word)
         continue;

      // Since we received a valid boot loader packet, reset the timeout
      timeout_cnt = 0;
      const uint8_t id=rx_buff[2];
      const uint8_t rx_chunk = rx_buff[3];
      const uint16_t pg_addr =  rx_buff[4] << 8 | rx_buff[5];
      switch (id)
      {
         // This command simply loads more data into the page buffer
         case load_flash_chunk:
            if (pg_addr != curr_page)
            {
               curr_page = pg_addr;
               memset(page_buff, 0, sizeof(page_buff));
            }
            memcpy(page_buff + rx_chunk*chunk_size, &rx_buff[6], chunk_size);
            last_chunk = rx_chunk;
            break;
               
         // This command writes whatever is in the flash page buffer
         case write_flash_page:
            if (last_chunk-1 != SPM_PAGESIZE/chunk_size)
               ; // not sure what to do here...
            // Turn off the radio so we don't ACK during the write.
            clear_CE();
            flash_page_write(curr_page, page_buff);
            set_CE();  // Turn the radio back on
            break;

         case start_app:
            boot_rww_enable();
            ((APP*)0)();

         case check_write_complete:
            // really don't have to do anything here
            // the ACK will be sufficient to tell the master
            // that the write is complete
            break;

         case write_eeprom:
         case read_flash_request:
         case read_flash_chunk:
         case read_eeprom:
         case no_op:
            ;
      }
   }

   // Start the user app
   boot_rww_enable();
   ((APP*)0)();
}


void flash_page_write(uint16_t addr, uint8_t* buff)
{
    if (addr >= BOOTADDR)
       return;  //NO!

    uint16_t  dst = addr;
    uint16_t* src = (uint16_t*) buff;
    for (uint8_t i=0; i<SPM_PAGESIZE/2; i++)
    {
       boot_page_fill(dst, *src++);
       dst += 2;
    }

    boot_page_erase(addr);
    boot_spm_busy_wait();
    
    boot_page_write(addr);
    boot_spm_busy_wait();
    
}


void spi_transact(uint8_t* p_rtx, const short len)
{
   PORTB &= ~_BV(PB2); //Drive SS low
   for(int i=0; i<len; i++)
   {
      SPDR = *p_rtx;
      while (!(SPSR & _BV(SPIF)));
      *p_rtx++ = SPDR;
   }
   PORTB |= _BV(PB2); // Drive SS back high
}


inline void clear_CE(void)
{
   PORTD &= ~_BV(PD3);
}


inline void set_CE(void)
{
   PORTD |= _BV(PD3);
}


void nrf_setup(void)
{
   using namespace nRF24L01;
   channel  = eeprom_read_byte((const uint8_t*)EE_CHANNEL);
   eeprom_read_block((void*)master_addr,    (const void*)EE_MASTER_ADDR,    4);
   eeprom_read_block((void*)broadcast_addr, (const void*)EE_BROADCAST_ADDR, 4);
   eeprom_read_block((void*)slave_addr,     (const void*)EE_SLAVE_ADDR,     4);

   // Setup up the SPI inerface
   // MOSI, SCK, and SS are outputs
   DDRB |= _BV(PB2) | _BV(PB3) | _BV(PB5);
   // Turn on SPI as master @ Fosc/4
   SPCR = _BV(SPE) | _BV(MSTR);
   PORTB |= _BV(PB2); // Make sure SS is high

   // use D3 as the CE to the nrf24l01
   DDRD |= _BV(PD3);
   clear_CE();

   // Note this leaves the nRF powered down
   const uint8_t config=CONFIG_PRIM_RX | CONFIG_EN_CRC | CONFIG_CRCO | CONFIG_MASK_TX_DS | CONFIG_MASK_MAX_RT;
   write_reg(CONFIG, config);
   
   write_reg(SETUP_RETR, SETUP_RETR_ARC_4); // auto retransmit 3 x 250us
   
   write_reg(SETUP_AW, SETUP_AW_4BYTES);  // 4 byte addresses
   write_reg(RF_SETUP, 0b00001110);  // 2Mbps data rate, 0dBm
   write_reg(RF_CH, channel);
   
   write_reg(RX_PW_P0, message_size);
   write_reg(RX_PW_P1, message_size);
   write_reg(RX_PW_P2, message_size);
   
   // Clear the various interrupt bits
   write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);
   
   uint8_t io_buff[addr_len+1];

   // Flush the RX/TX FIFOs
   io_buff[0] = FLUSH_RX;
   spi_transact(io_buff, 1);
   io_buff[0] = FLUSH_TX;
   spi_transact(io_buff, 1);

   memcpy(io_buff, master_addr, addr_len); // only TX to master
   write_reg(TX_ADDR, io_buff, addr_len);

   // pipe 0 is for receiving broadcast 
   memcpy(io_buff, broadcast_addr, addr_len);
   write_reg(RX_ADDR_P0, io_buff, addr_len);

   // pipe 1 is this each slave's private address
   memcpy(io_buff, slave_addr, addr_len);
   write_reg(RX_ADDR_P1, io_buff, addr_len);

   write_reg(EN_RXADDR, EN_RXADDR_ERX_P0 | EN_RXADDR_ERX_P1);
   write_reg(EN_AA, EN_AA_ENAA_P1);  // auto ack on pipe 1 only

   // after power up, can't write to most registers anymore
   write_reg(CONFIG, config | CONFIG_PWR_UP);
   _delay_us(1500);
   set_CE();
}

   
void write_reg(uint8_t reg, uint8_t* data, const size_t len)
{
   uint8_t io_buff[len+1];
   io_buff[0]=nRF24L01::W_REGISTER | reg;
   memcpy(io_buff+1, data, len);
   spi_transact(io_buff, len+1);
}

   
void write_reg(char reg, char data)
{
   uint8_t io_buff[2];
   io_buff[0]=nRF24L01::W_REGISTER | reg;
   io_buff[1]=data;
   spi_transact(io_buff, 2);
}


char read_reg(char reg)
{
   uint8_t io_buff[2];
   io_buff[0]=nRF24L01::R_REGISTER | reg;
   io_buff[1] = 0;
   spi_transact(io_buff, 2);
   return io_buff[1];
}


void read_reg(char reg, uint8_t* data, const size_t len)
{
   uint8_t io_buff[len+1];
   io_buff[0]=nRF24L01::R_REGISTER | reg;
   io_buff[1]=0;
   memcpy(io_buff+1, data, len);
   spi_transact(io_buff, len+1);
}


uint8_t read_payload(uint8_t* buff, const size_t len)
{
   uint8_t pipe = (read_reg(nRF24L01::STATUS) & 0x0e) >> 1;
   uint8_t io_buff[len+1];

   // read the packet out of the nRF's FIFO
   clear_CE();
   io_buff[0]=nRF24L01::R_RX_PAYLOAD;
   spi_transact(io_buff, len+1);
   set_CE();
   
      // clear all the IRQ bits!
   write_reg(nRF24L01::STATUS,
             nRF24L01::STATUS_RX_DR | nRF24L01::STATUS_TX_DS | nRF24L01::STATUS_MAX_RT);

   memcpy(buff, &io_buff[1], len);
   return pipe;
}
