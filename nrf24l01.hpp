#ifndef _NRF24L01_HPP
#define _NRF24L01_HPP

// An interface to a nrf24l01 chip

#include <stddef.h>
#include <stdint.h>

namespace nRF24L01
{
   ////////////////////////////////////////////////////////////////////////////////////
   // Commands (table 20)
   static const char R_REGISTER = 0x00;
   static const char W_REGISTER = 0x20;
   static const char R_RX_PAYLOAD = 0x61;
   static const char W_TX_PAYLOAD = 0xA0;
   static const char FLUSH_TX = 0xE1;
   static const char FLUSH_RX = 0xE2;
   static const char REUSE_TX_PL = 0xE3;
   static const char NOP = 0xFF;

   //SPI command data mask defines
   static const char R_REGISTER_DATA = 0x1F;
   static const char W_REGISTER_DATA = 0x1F;

   ////////////////////////////////////////////////////////////////////////////////////
   // Register definitions (table 28)
   static const char CONFIG = 0x00;
   static const char EN_AA = 0x01;
   static const char EN_RXADDR = 0x02;
   static const char SETUP_AW = 0x03;
   static const char SETUP_RETR = 0x04;
   static const char RF_CH = 0x05;
   static const char RF_SETUP = 0x06;
   static const char STATUS = 0x07;
   static const char OBSERVE_TX = 0x08;
   static const char CD = 0x09;
   static const char RX_ADDR_P0 = 0x0A;
   static const char RX_ADDR_P1 = 0x0B;
   static const char RX_ADDR_P2 = 0x0C;
   static const char RX_ADDR_P3 = 0x0D;
   static const char RX_ADDR_P4 = 0x0E;
   static const char RX_ADDR_P5 = 0x0F;
   static const char TX_ADDR = 0x10;
   static const char RX_PW_P0 = 0x11;
   static const char RX_PW_P1 = 0x12;
   static const char RX_PW_P2 = 0x13;
   static const char RX_PW_P3 = 0x14;
   static const char RX_PW_P4 = 0x15;
   static const char RX_PW_P5 = 0x16;
   static const char FIFO_STATUS = 0x17;

   ////////////////////////////////////////////////////////////////////////////////////
   // Register bitwise definitions (table 28)
   //CONFIG register bitwise definitions
   static const char CONFIG_RESERVED = 0x80;
   static const char CONFIG_MASK_RX_DR = 0x40;
   static const char CONFIG_MASK_TX_DS = 0x20;
   static const char CONFIG_MASK_MAX_RT = 0x10;
   static const char CONFIG_EN_CRC = 0x08;
   static const char CONFIG_CRCO = 0x04;
   static const char CONFIG_PWR_UP = 0x02;
   static const char CONFIG_PRIM_RX = 0x01;

   //EN_AA register bitwise definitions
   static const char EN_AA_RESERVED = 0xC0;
   static const char EN_AA_ENAA_ALL = 0x3F;
   static const char EN_AA_ENAA_P5 = 0x20;
   static const char EN_AA_ENAA_P4 = 0x10;
   static const char EN_AA_ENAA_P3 = 0x08;
   static const char EN_AA_ENAA_P2 = 0x04;
   static const char EN_AA_ENAA_P1 = 0x02;
   static const char EN_AA_ENAA_P0 = 0x01;
   static const char EN_AA_ENAA_NONE = 0x00;

   //EN_RXADDR register bitwise definitions
   static const char EN_RXADDR_RESERVED = 0xC0;
   static const char EN_RXADDR_ERX_ALL = 0x3F;
   static const char EN_RXADDR_ERX_P5 = 0x20;
   static const char EN_RXADDR_ERX_P4 = 0x10;
   static const char EN_RXADDR_ERX_P3 = 0x08;
   static const char EN_RXADDR_ERX_P2 = 0x04;
   static const char EN_RXADDR_ERX_P1 = 0x02;
   static const char EN_RXADDR_ERX_P0 = 0x01;
   static const char EN_RXADDR_ERX_NONE = 0x00;

   //SETUP_AW register bitwise definitions
   static const char SETUP_AW_RESERVED = 0xFC;
   static const char SETUP_AW_5BYTES = 0x03;
   static const char SETUP_AW_4BYTES = 0x02;
   static const char SETUP_AW_3BYTES = 0x01;
   static const char SETUP_AW_ILLEGAL = 0x00;

   //SETUP_RETR register bitwise definitions
   static const char SETUP_RETR_ARD = 0xF0;
   static const char SETUP_RETR_ARD_4000 = 0xF0;
   static const char SETUP_RETR_ARD_3750 = 0xE0;
   static const char SETUP_RETR_ARD_3500 = 0xD0;
   static const char SETUP_RETR_ARD_3250 = 0xC0;
   static const char SETUP_RETR_ARD_3000 = 0xB0;
   static const char SETUP_RETR_ARD_2750 = 0xA0;
   static const char SETUP_RETR_ARD_2500 = 0x90;
   static const char SETUP_RETR_ARD_2250 = 0x80;
   static const char SETUP_RETR_ARD_2000 = 0x70;
   static const char SETUP_RETR_ARD_1750 = 0x60;
   static const char SETUP_RETR_ARD_1500 = 0x50;
   static const char SETUP_RETR_ARD_1250 = 0x40;
   static const char SETUP_RETR_ARD_1000 = 0x30;
   static const char SETUP_RETR_ARD_750 = 0x20;
   static const char SETUP_RETR_ARD_500 = 0x10;
   static const char SETUP_RETR_ARD_250 = 0x00;
   static const char SETUP_RETR_ARC = 0x0F;
   static const char SETUP_RETR_ARC_15 = 0x0F;
   static const char SETUP_RETR_ARC_14 = 0x0E;
   static const char SETUP_RETR_ARC_13 = 0x0D;
   static const char SETUP_RETR_ARC_12 = 0x0C;
   static const char SETUP_RETR_ARC_11 = 0x0B;
   static const char SETUP_RETR_ARC_10 = 0x0A;
   static const char SETUP_RETR_ARC_9 = 0x09;
   static const char SETUP_RETR_ARC_8 = 0x08;
   static const char SETUP_RETR_ARC_7 = 0x07;
   static const char SETUP_RETR_ARC_6 = 0x06;
   static const char SETUP_RETR_ARC_5 = 0x05;
   static const char SETUP_RETR_ARC_4 = 0x04;
   static const char SETUP_RETR_ARC_3 = 0x03;
   static const char SETUP_RETR_ARC_2 = 0x02;
   static const char SETUP_RETR_ARC_1 = 0x01;
   static const char SETUP_RETR_ARC_0 = 0x00;

   //RF_CH register bitwise definitions
   static const char RF_CH_RESERVED = 0x80;

   //RF_SETUP register bitwise definitions
   static const char RF_SETUP_RESERVED = 0xE0;
   static const char RF_SETUP_PLL_LOCK = 0x10;
   static const char RF_SETUP_RF_DR = 0x08;
   static const char RF_SETUP_RF_PWR = 0x06;
   static const char RF_SETUP_RF_PWR_0 = 0x06;
   static const char RF_SETUP_RF_PWR_6 = 0x04;
   static const char RF_SETUP_RF_PWR_12 = 0x02;
   static const char RF_SETUP_RF_PWR_18 = 0x00;
   static const char RF_SETUP_LNA_HCURR = 0x01;

   //STATUS register bitwise definitions
   static const char STATUS_RESERVED = 0x80;
   static const char STATUS_RX_DR = 0x40;
   static const char STATUS_TX_DS = 0x20;
   static const char STATUS_MAX_RT = 0x10;
   static const char STATUS_RX_P_NO = 0x0E;
   static const char STATUS_RX_P_NO_RX_FIFO_NOT_EMPTY = 0x0E;
   static const char STATUS_RX_P_NO_UNUSED = 0x0C;
   static const char STATUS_RX_P_NO_5 = 0x0A;
   static const char STATUS_RX_P_NO_4 = 0x08;
   static const char STATUS_RX_P_NO_3 = 0x06;
   static const char STATUS_RX_P_NO_2 = 0x04;
   static const char STATUS_RX_P_NO_1 = 0x02;
   static const char STATUS_RX_P_NO_0 = 0x00;
   static const char STATUS_TX_FULL = 0x01;

   //OBSERVE_TX register bitwise definitions
   static const char OBSERVE_TX_PLOS_CNT = 0xF0;
   static const char OBSERVE_TX_ARC_CNT = 0x0F;

   //CD register bitwise definitions
   static const char CD_RESERVED = 0xFE;
   static const char CD_CD = 0x01;

   //RX_PW_P0 register bitwise definitions
   static const char RX_PW_P0_RESERVED = 0xC0;

   //RX_PW_P1 register bitwise definitions
   static const char RX_PW_P1_RESERVED = 0xC0;

   //RX_PW_P2 register bitwise definitions
   static const char RX_PW_P2_RESERVED = 0xC0;

   //RX_PW_P3 register bitwise definitions
   static const char RX_PW_P3_RESERVED = 0xC0;

   //RX_PW_P4 register bitwise definitions
   static const char RX_PW_P4_RESERVED = 0xC0;

   //RX_PW_P5 register bitwise definitions
   static const char RX_PW_P5_RESERVED = 0xC0;

   //FIFO_STATUS register bitwise definitions
   static const char FIFO_STATUS_RESERVED = 0x8C;
   static const char FIFO_STATUS_TX_REUSE = 0x40;
   static const char FIFO_STATUS_TX_FULL = 0x20;
   static const char FIFO_STATUS_TX_EMPTY = 0x10;
   static const char FIFO_STATUS_RX_FULL = 0x02;
   static const char FIFO_STATUS_RX_EMPTY = 0x01;

   //===============================================================================================
   // This part of the namespace is the hardware-defendant part of the interface
   void write_data(char* data, const size_t len);
   void delay_us(uint32_t us);
   void clear_CE(void);
   void set_CE(void);
   bool setup(void);
   void shutdown(void);

   //===============================================================================================
   //Code below should be low-level hardware independent

   void write_reg(char reg, char data);
   void write_reg(char reg, char* data, const size_t len);
   char read_reg(char reg);
   void read_reg(char reg, char* data, const size_t len);

   // returns true on success
   bool configure_base(void);

   void configure_PRX(void);
   void power_up_PRX(void);

   void configure_PTX(void);
   void power_up_PTX(void);

   void write_tx_payload(void* data, const unsigned int len);
   void read_rx_payload(void* data, const unsigned int len);
   void flush_tx(void);
   void pulse_CE(void);

#ifdef AVR
   extern uint8_t rx_flag;
   extern uint32_t t_rx;
#endif
}
#endif
