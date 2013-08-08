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
  commands to the slave's unique address. This first write command will be
  addressed to 0x0000 on the AVR and will be a dummy page that will have
  the slave do a boot reset. This will keep the slave in the bootloader until
  this page is successfully written at the end of a programming cycle.

  The nRF's checksum will be use to verify data transmission and the slave
  will use the nRF's ACK to communicate successfull reception.


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

#include "slave_eeprom.h"
#include "nrf24l01_defines.hpp"

// macros
#define PACKED          __attribute__((__packed__))
#define NOINLINE        __attribute__((noinline))
#define NAKED           __attribute__((naked))
#define SECTION(a)      __attribute__((section(a)))


// data
struct HEX_RECORD
{
    uint8_t             length;
    uint16_t            address;
    uint8_t             type;
    uint8_t             data[255 + 1];
} PACKED record;

#define FIFOSIZE 64
uint8_t                 page[SPM_PAGESIZE];
uint8_t                 fifo[FIFOSIZE];
uint8_t                 fifo_fill;


// defined in linker script
extern void* __bss_end;


// prototypes
static void flash_wait_poll_uart(void);
static void flash_write(uint16_t page);


namespace nrf
{
   // This is just at SPI transaction
   void write_data(const char* p_tx, char* p_rx, const short len);   
   void clear_CE(void);
   void set_CE(void);
   void setup(void);
   void write_reg(char reg, char* data, const size_t len);
   void write_reg(char reg, char data);
   char read_reg(char reg);
   void read_reg(char reg, char* data, const size_t len);
}


// implementation
void kavr(void) NAKED SECTION(".vectors");
void kavr(void)
{
   uint32_t            timeout;
   uint16_t            new_page;
   uint16_t            cur_page;
   uint16_t            addr;
   uint8_t*            rx_ptr;
   uint8_t             rx_dat;
   uint8_t             rx_crc;
   uint8_t             rx_len;
   uint8_t             rx_val;
   uint8_t             i;
   uint8_t*            dst;


   // Instead of linking __init:
   // (how to avoid this assembly code? Anyone?)
   asm volatile ("clr r1");
   asm volatile ("ldi r28, lo8(%0)" :: "i" (&__bss_end));
   asm volatile ("ldi r29, hi8(%0)" :: "i" (&__bss_end));

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
   nrf::setup();

   // And listen for page write command directed at me.

   // Initialize overall state
   cur_page  = BOOTADDR;
   fifo_fill = 0;
   timeout   = 0;

   // (Re-)Initialize line state
  restart_line:
   rx_val = 0;
   rx_len = 0;
   rx_crc = 0;
   rx_ptr = (uint8_t*) &record;

   // Download data records until timeout
   while (1)
   {
      // Poll UART for receive characters
      flash_wait_poll_uart();
      if (fifo_fill)
      {
         // Extract oldest character from receive FIFO
         rx_dat = fifo[0];
         fifo_fill --;
         i = 0;
         do fifo[i] = fifo[i + 1]; while (++ i < fifo_fill);

         // Convert to binary nibble
         rx_dat -= '0';
         if (rx_dat > 9)
         {
            rx_dat -= 7;
            if ((rx_dat < 10) || (rx_dat > 15))
               goto restart_line;
         }

         // Valid hex character. Restart timeout.
         timeout = 0;

         // Two nibbles = 1 byte
         rx_val = (rx_val << 4) + rx_dat;
         if ((++ rx_len & 1) == 0)
         {
            rx_crc += rx_val;
            *rx_ptr ++ = rx_val;

            if (rx_ptr == (uint8_t*) &record.data[record.length + 1])
            {
               if (rx_crc)
               {
                  // Checksum error. Abort
                  //out_char('?');
                  break;
               }
               else if (record.type == 0)
               {
                  // Data record
                  addr = (record.address << 8) | (record.address >> 8);
                  for (i = 0; i < record.length; i ++)
                  {
                     // Determine page base for current address
                     new_page = addr & ~(SPM_PAGESIZE - 1);
                     if (new_page != cur_page)
                     {
                        // Write updated RAM page buffer into flash
                        flash_write(cur_page);

                        // Load page at new address into RAM page buffer
                        cur_page = new_page;

                        // memcpy_P(page, (PGM_P) new_page, SPM_PAGESIZE);
                        dst = page;
                        while (dst < &page[SPM_PAGESIZE])
                           *dst ++ = pgm_read_byte((PGM_P) new_page ++);
                     }

                     // Update RAM page buffer from data record
                     page[addr & (SPM_PAGESIZE - 1)] = record.data[i];
                     addr ++;
                  }

                  goto restart_line;
               }
               else if (record.type == 1)
               {
                  // End of file record. Finish
                  flash_write(cur_page);
                  //out_char('S');
                  break;
               }
            }
         }
      }

      timeout ++;
   }

   // Start the user app
   ((APP*)0)();
}


static void flash_write(uint16_t addr)
{
    uint16_t  dst;
    uint16_t* src;
    uint8_t   i;

    if (addr < BOOTADDR)
    {
       //out_char(XOFF);

        // Copy RAM addr buffer into SPM addr buffer
        dst = addr;
        src = (uint16_t*) page;
        i = 0;
        do
        {
            boot_page_fill(dst, *src ++);
            dst += 2;
        }
        while (++ i < SPM_PAGESIZE / 2);

        // Erase and program
        boot_page_erase(addr);
        flash_wait_poll_uart();

        boot_page_write(addr);
        flash_wait_poll_uart();

        boot_rww_enable();

        //out_char(XON);
    }
}


static void flash_wait_poll_uart(void)
{
   do
   {
      if (fifo_fill < FIFOSIZE)
         // fifo[fifo_fill ++] = UDR;
         // add data to fifo
         ;
      wdt_reset();
   }
   while (boot_spm_busy());
}



namespace nrf
{

   // need to do this to pull in the defines
   using namespace nRF24L01;

   uint8_t channel;
   const size_t addr_len = 4;
   const size_t message_size = 32;
   char master_addr[addr_len];
   char broadcast_addr[addr_len];
   char slave_addr[addr_len];
   char iobuff[message_size+1];

   // This is just at SPI transaction
   void write_data(const char* p_tx, char* p_rx, const short len)
   {
      PORTB &= ~_BV(PB2); //Drive SS low
      int i;
      for(i=0; i<len; i++)
      {
         SPDR = *p_tx++;
         while (!(SPSR & _BV(SPIF)));
         *p_rx++ = SPDR;
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


   void setup(void)
   {
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
   
      // Flush the RX/TX FIFOs
      char buff=FLUSH_RX;
      write_data(&buff, &buff, 1);
      buff=FLUSH_TX;
      write_data(&buff, &buff, 1);

      memcpy(iobuff, master_addr, addr_len); // only TX to master
      write_reg(TX_ADDR, iobuff, addr_len);

      // pipe 0 is for receiving broadcast 
      memcpy(iobuff, broadcast_addr, addr_len);
      write_reg(RX_ADDR_P0, iobuff, addr_len);

      // pipe 1 is this each slave's private address
      memcpy(iobuff, slave_addr, addr_len);
      write_reg(RX_ADDR_P1, iobuff, addr_len);

      write_reg(EN_RXADDR, EN_RXADDR_ERX_P0 | EN_RXADDR_ERX_P1);
      write_reg(EN_AA, EN_AA_ENAA_P1);  // auto ack on pipe 1 only

      // after power up, can't write to most registers anymore
      write_reg(CONFIG, config | CONFIG_PWR_UP);
      _delay_us(1500);
      set_CE();

   }

   
   void write_reg(char reg, char* data, const size_t len)
   {
      iobuff[0]=W_REGISTER | reg;
      memcpy(iobuff+1, data, len);
      write_data(iobuff, iobuff, len+1);
   }

   
   void write_reg(char reg, char data)
   {
      iobuff[0]=W_REGISTER | reg;
      iobuff[1]=data;
      write_data(iobuff, iobuff, 2);
   }


   char read_reg(char reg)
   {
      iobuff[0]=R_REGISTER | reg;
      iobuff[1] = 0;
      write_data(iobuff, iobuff, 2);
      return iobuff[1];
   }


   void read_reg(char reg, char* data, const size_t len)
   {
      iobuff[0]=R_REGISTER | reg;
      iobuff[1]=0;
      memcpy(iobuff+1, data, len);
      write_data(iobuff, iobuff, len+1);
   }

}
