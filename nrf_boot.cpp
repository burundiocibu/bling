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
  buff[0,2] = magic: 0xbabe
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
#include "nrf_boot.h"
#include "ensemble.hpp"

// macros
#define PACKED          __attribute__((__packed__))
#define NOINLINE        __attribute__((noinline))
#define NAKED           __attribute__((naked))
#define SECTION(a)      __attribute__((section(a)))


uint8_t io_buff[33];
uint8_t rx_buff[33];

uint8_t page_buff[SPM_PAGESIZE]; // where we build up this page

typedef void APP(void);

void flash_page_write(uint16_t addr, uint8_t* buff);
void spi_transact(uint8_t* p_rtx, const short len);   
void clear_CE(void);
void set_CE(void);
void nrf_setup(void);
void write_reg(uint8_t reg, uint8_t* data, const size_t len);
void write_reg(uint8_t reg, uint8_t data);
char read_reg(uint8_t reg);
void read_reg(uint8_t reg, uint8_t* data, const size_t len);
static uint8_t read_payload(uint8_t* buff, const size_t len);
void blink(int n);

using namespace nRF24L01;

// defined in linker script
extern void* __bss_end;

// It appears that when the -nostartfiles option is given
// to the loader, this doesn't need to be called 'main'
void kavr(void) NAKED SECTION(".vectors");
void kavr(void)
{
   const unsigned timeout_lim = 400;
   unsigned timeout_cnt=0;
   uint32_t curr_page = 0xffff; // Address of page we are working on
   uint8_t last_chunk=0; // last chunk that was wrtten into the page_buff
   uint8_t status;
   uint16_t rx_magic_word;
   uint8_t id;
   uint8_t rx_chunk;
   uint16_t pg_addr;

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
   //PORTB |= _BV(PB1);

   // Set the tlc5940 BLANK line to to disable PWM output
   DDRC  |= _BV(PC2);
   PORTC |= _BV(PC2);

   // Initialize the nRF interface
   nrf_setup();

   // Countdown till giving up and jumping to the user app
   while (timeout_cnt < timeout_lim)
   {
      // check to see if we received any data
      status = read_reg(STATUS);
      if (status == 0x0e)
      {
         timeout_cnt++;
         _delay_us(5000);
         continue;
      }

      read_payload(rx_buff, boot_message_size);

      rx_magic_word = rx_buff[0] << 8 | rx_buff[1];
      // really shouldn't have to ignore the MSB of the magic word
      if ((0xff & rx_magic_word) != (0xff & boot_magic_word))
         continue;

      // Since we received a valid boot loader packet, reset the timeout
      timeout_cnt = 0;
      id=rx_buff[2];
      rx_chunk = rx_buff[3];
      pg_addr =  rx_buff[4] << 8 | rx_buff[5];
      switch (id)
      {
         // This command simply loads more data into the page buffer
         case bl_load_flash_chunk:
            if (pg_addr != curr_page)
            {
               curr_page = pg_addr;
               memset(page_buff, 0x00, sizeof(page_buff));
            }
            memcpy(page_buff + rx_chunk*boot_chunk_size, &rx_buff[6], boot_chunk_size);
            last_chunk = rx_chunk;
            break;
               
         // This command writes whatever is in the flash page buffer
         case bl_write_flash_page:
            // Turn off the radio so we don't ACK during the write.
            clear_CE();
            if (last_chunk+1 != SPM_PAGESIZE/boot_chunk_size)
               blink(1); // blink an error code
            flash_page_write(curr_page, page_buff);
            set_CE();  // Turn the radio back on
            break;

         case bl_start_app:
            boot_rww_enable();
            ((APP*)0)();

         case bl_check_write_complete:
            // really don't have to do anything here
            // the ACK will be sufficient to tell the master
            // that the write is complete
            blink(0);
            break;

         case bl_write_eeprom:
         case bl_read_flash_request:
         case bl_read_flash_chunk:
         case bl_read_eeprom:
         case bl_no_op:
            break;
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
   uint8_t config;
   uint8_t channel;
   uint8_t master_addr[ensemble::addr_len];
   uint8_t slave_addr[ensemble::addr_len];

   // Read the radio config from EEPROM
   channel  = eeprom_read_byte((const uint8_t*)EE_CHANNEL);
   eeprom_read_block((void*)master_addr, (const void*)EE_MASTER_ADDR,  ensemble::addr_len);
   eeprom_read_block((void*)slave_addr,  (const void*)EE_SLAVE_ADDR,   ensemble::addr_len);

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
   config=CONFIG_PRIM_RX | CONFIG_EN_CRC | CONFIG_CRCO |
      CONFIG_MASK_TX_DS | CONFIG_MASK_MAX_RT | CONFIG_MASK_RX_DR;
   write_reg(CONFIG, config);
   
   write_reg(SETUP_AW, SETUP_AW_4BYTES);  // 4 byte addresses
   write_reg(RF_SETUP, 0b00001110);  // 1Mbps data rate, 0dBm
   write_reg(RF_CH, channel);
   write_reg(RX_PW_P0, boot_message_size);
   
   // Clear the various interrupt bits
   write_reg(STATUS, STATUS_TX_DS|STATUS_RX_DR|STATUS_MAX_RT);
   
   write_reg(TX_ADDR, master_addr, addr_len);
   write_reg(RX_ADDR_P0, slave_addr, ensemble::addr_len);
   write_reg(EN_AA, EN_AA_ENAA_P0);  // auto ack on pipe 0
   write_reg(EN_RXADDR, EN_RXADDR_ERX_P0);

   // Flush the RX/TX FIFOs
   io_buff[0] = FLUSH_RX;
   spi_transact(io_buff, 1);
   io_buff[0] = FLUSH_TX;
   spi_transact(io_buff, 1);

   // after power up, can't write to most registers anymore
   write_reg(CONFIG, config | CONFIG_PWR_UP);
   _delay_us(1500);
   set_CE();
}

   
void write_reg(uint8_t reg, uint8_t* data, const size_t len)
{
   io_buff[0]=W_REGISTER | reg;
   memcpy(io_buff+1, data, len);
   spi_transact(io_buff, len+1);
}

   
void write_reg(uint8_t reg, uint8_t data)
{
   io_buff[0]=W_REGISTER | reg;
   io_buff[1]=data;
   spi_transact(io_buff, 2);
}


char read_reg(uint8_t reg)
{
   io_buff[0]=R_REGISTER | reg;
   io_buff[1] = 0;
   spi_transact(io_buff, 2);
   return io_buff[1];
}


void read_reg(uint8_t reg, uint8_t* data, const size_t len)
{
   io_buff[0]=R_REGISTER | reg;
   io_buff[1]=0;
   memcpy(io_buff+1, data, len);
   spi_transact(io_buff, len+1);
}


uint8_t read_payload(uint8_t* buff, const size_t len)
{
   uint8_t pipe = (read_reg(STATUS) & 0x0e) >> 1;

   // read the packet out of the nRF's FIFO
   clear_CE();
   io_buff[0]=R_RX_PAYLOAD;
   spi_transact(io_buff, len+1);
   set_CE();
   
      // clear all the IRQ bits!
   write_reg(STATUS, STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT);

   memcpy(buff, &io_buff[1], len);
   return pipe;
}

void blink(int n)
{
   for (int i=0; i<n; i++)
   {
      PORTB |= _BV(PB1);
      _delay_ms(100);
      PORTB &= ~_BV(PB1);
      _delay_ms(100);
   }
}
