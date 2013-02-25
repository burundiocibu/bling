// An interface to a nrf24l01 chip
#ifndef _NRF24L01_HPP
#define _NRF24L01_HPP

#include <stddef.h>

namespace nRF24L01
{
   ////////////////////////////////////////////////////////////////////////////////////
   // Commands (table 20)
   const char R_REGISTER = 0x00;
   const char W_REGISTER = 0x20;
   const char R_RX_PAYLOAD = 0x61;
   const char W_TX_PAYLOAD = 0xA0;
   const char FLUSH_TX = 0xE1;
   const char FLUSH_RX = 0xE2;
   const char REUSE_TX_PL = 0xE3;
   const char NOP = 0xFF;

   //SPI command data mask defines
   const char R_REGISTER_DATA = 0x1F;
   const char W_REGISTER_DATA = 0x1F;

   ////////////////////////////////////////////////////////////////////////////////////
   // Register definitions (table 28)
   const char CONFIG = 0x00;
   const char EN_AA = 0x01;
   const char EN_RXADDR = 0x02;
   const char SETUP_AW = 0x03;
   const char SETUP_RETR = 0x04;
   const char RF_CH = 0x05;
   const char RF_SETUP = 0x06;
   const char STATUS = 0x07;
   const char OBSERVE_TX = 0x08;
   const char CD = 0x09;
   const char RX_ADDR_P0 = 0x0A;
   const char RX_ADDR_P1 = 0x0B;
   const char RX_ADDR_P2 = 0x0C;
   const char RX_ADDR_P3 = 0x0D;
   const char RX_ADDR_P4 = 0x0E;
   const char RX_ADDR_P5 = 0x0F;
   const char TX_ADDR = 0x10;
   const char RX_PW_P0 = 0x11;
   const char RX_PW_P1 = 0x12;
   const char RX_PW_P2 = 0x13;
   const char RX_PW_P3 = 0x14;
   const char RX_PW_P4 = 0x15;
   const char RX_PW_P5 = 0x16;
   const char FIFO_STATUS = 0x17;

   ////////////////////////////////////////////////////////////////////////////////////
   // Register bitwise definitions (table 28)
   //CONFIG register bitwise definitions
   const char CONFIG_RESERVED = 0x80;
   const char CONFIG_MASK_RX_DR = 0x40;
   const char CONFIG_MASK_TX_DS = 0x20;
   const char CONFIG_MASK_MAX_RT = 0x10;
   const char CONFIG_EN_CRC = 0x08;
   const char CONFIG_CRCO = 0x04;
   const char CONFIG_PWR_UP = 0x02;
   const char CONFIG_PRIM_RX = 0x01;

   //EN_AA register bitwise definitions
   const char EN_AA_RESERVED = 0xC0;
   const char EN_AA_ENAA_ALL = 0x3F;
   const char EN_AA_ENAA_P5 = 0x20;
   const char EN_AA_ENAA_P4 = 0x10;
   const char EN_AA_ENAA_P3 = 0x08;
   const char EN_AA_ENAA_P2 = 0x04;
   const char EN_AA_ENAA_P1 = 0x02;
   const char EN_AA_ENAA_P0 = 0x01;
   const char EN_AA_ENAA_NONE = 0x00;

   //EN_RXADDR register bitwise definitions
   const char EN_RXADDR_RESERVED = 0xC0;
   const char EN_RXADDR_ERX_ALL = 0x3F;
   const char EN_RXADDR_ERX_P5 = 0x20;
   const char EN_RXADDR_ERX_P4 = 0x10;
   const char EN_RXADDR_ERX_P3 = 0x08;
   const char EN_RXADDR_ERX_P2 = 0x04;
   const char EN_RXADDR_ERX_P1 = 0x02;
   const char EN_RXADDR_ERX_P0 = 0x01;
   const char EN_RXADDR_ERX_NONE = 0x00;

   //SETUP_AW register bitwise definitions
   const char SETUP_AW_RESERVED = 0xFC;
   const char SETUP_AW_5BYTES = 0x03;
   const char SETUP_AW_4BYTES = 0x02;
   const char SETUP_AW_3BYTES = 0x01;
   const char SETUP_AW_ILLEGAL = 0x00;

   //SETUP_RETR register bitwise definitions
   const char SETUP_RETR_ARD = 0xF0;
   const char SETUP_RETR_ARD_4000 = 0xF0;
   const char SETUP_RETR_ARD_3750 = 0xE0;
   const char SETUP_RETR_ARD_3500 = 0xD0;
   const char SETUP_RETR_ARD_3250 = 0xC0;
   const char SETUP_RETR_ARD_3000 = 0xB0;
   const char SETUP_RETR_ARD_2750 = 0xA0;
   const char SETUP_RETR_ARD_2500 = 0x90;
   const char SETUP_RETR_ARD_2250 = 0x80;
   const char SETUP_RETR_ARD_2000 = 0x70;
   const char SETUP_RETR_ARD_1750 = 0x60;
   const char SETUP_RETR_ARD_1500 = 0x50;
   const char SETUP_RETR_ARD_1250 = 0x40;
   const char SETUP_RETR_ARD_1000 = 0x30;
   const char SETUP_RETR_ARD_750 = 0x20;
   const char SETUP_RETR_ARD_500 = 0x10;
   const char SETUP_RETR_ARD_250 = 0x00;
   const char SETUP_RETR_ARC = 0x0F;
   const char SETUP_RETR_ARC_15 = 0x0F;
   const char SETUP_RETR_ARC_14 = 0x0E;
   const char SETUP_RETR_ARC_13 = 0x0D;
   const char SETUP_RETR_ARC_12 = 0x0C;
   const char SETUP_RETR_ARC_11 = 0x0B;
   const char SETUP_RETR_ARC_10 = 0x0A;
   const char SETUP_RETR_ARC_9 = 0x09;
   const char SETUP_RETR_ARC_8 = 0x08;
   const char SETUP_RETR_ARC_7 = 0x07;
   const char SETUP_RETR_ARC_6 = 0x06;
   const char SETUP_RETR_ARC_5 = 0x05;
   const char SETUP_RETR_ARC_4 = 0x04;
   const char SETUP_RETR_ARC_3 = 0x03;
   const char SETUP_RETR_ARC_2 = 0x02;
   const char SETUP_RETR_ARC_1 = 0x01;
   const char SETUP_RETR_ARC_0 = 0x00;

   //RF_CH register bitwise definitions
   const char RF_CH_RESERVED = 0x80;

   //RF_SETUP register bitwise definitions
   const char RF_SETUP_RESERVED = 0xE0;
   const char RF_SETUP_PLL_LOCK = 0x10;
   const char RF_SETUP_RF_DR = 0x08;
   const char RF_SETUP_RF_PWR = 0x06;
   const char RF_SETUP_RF_PWR_0 = 0x06;
   const char RF_SETUP_RF_PWR_6 = 0x04;
   const char RF_SETUP_RF_PWR_12 = 0x02;
   const char RF_SETUP_RF_PWR_18 = 0x00;
   const char RF_SETUP_LNA_HCURR = 0x01;

   //STATUS register bitwise definitions
   const char STATUS_RESERVED = 0x80;
   const char STATUS_RX_DR = 0x40;
   const char STATUS_TX_DS = 0x20;
   const char STATUS_MAX_RT = 0x10;
   const char STATUS_RX_P_NO = 0x0E;
   const char STATUS_RX_P_NO_RX_FIFO_NOT_EMPTY = 0x0E;
   const char STATUS_RX_P_NO_UNUSED = 0x0C;
   const char STATUS_RX_P_NO_5 = 0x0A;
   const char STATUS_RX_P_NO_4 = 0x08;
   const char STATUS_RX_P_NO_3 = 0x06;
   const char STATUS_RX_P_NO_2 = 0x04;
   const char STATUS_RX_P_NO_1 = 0x02;
   const char STATUS_RX_P_NO_0 = 0x00;
   const char STATUS_TX_FULL = 0x01;

   //OBSERVE_TX register bitwise definitions
   const char OBSERVE_TX_PLOS_CNT = 0xF0;
   const char OBSERVE_TX_ARC_CNT = 0x0F;

   //CD register bitwise definitions
   const char CD_RESERVED = 0xFE;
   const char CD_CD = 0x01;

   //RX_PW_P0 register bitwise definitions
   const char RX_PW_P0_RESERVED = 0xC0;

   //RX_PW_P1 register bitwise definitions
   const char RX_PW_P1_RESERVED = 0xC0;

   //RX_PW_P2 register bitwise definitions
   const char RX_PW_P2_RESERVED = 0xC0;

   //RX_PW_P3 register bitwise definitions
   const char RX_PW_P3_RESERVED = 0xC0;

   //RX_PW_P4 register bitwise definitions
   const char RX_PW_P4_RESERVED = 0xC0;

   //RX_PW_P5 register bitwise definitions
   const char RX_PW_P5_RESERVED = 0xC0;

   //FIFO_STATUS register bitwise definitions
   const char FIFO_STATUS_RESERVED = 0x8C;
   const char FIFO_STATUS_TX_REUSE = 0x40;
   const char FIFO_STATUS_TX_FULL = 0x20;
   const char FIFO_STATUS_TX_EMPTY = 0x10;
   const char FIFO_STATUS_RX_FULL = 0x02;
   const char FIFO_STATUS_RX_EMPTY = 0x01;

   //===============================================================================================
   // This part of the namespace is the hardware-dependant part of the interface
   char write_data(char data);
   void write_data(char* data, size_t len);
   void delay_us(uint32_t us);
   void clear_ce();
   void set_ce();
   void rpi_setup();
   void rpi_shutdown();

   //===============================================================================================
   //Code below should be low-level hardware independant
   static const size_t addr_len=3;
   static const char ptx_addr[addr_len]={0xE1, 0xE3, 0xE5};
   static const char prx_addr[addr_len]{0xB1, 0xB3, 0xB5};

   void write_reg(char reg, char data);
   char read_reg(char reg);
   void write_reg(char reg, char* data, size_t len);
   void read_reg(char reg, char* data, size_t len);

   void configure_base();

   void configure_PRX();
   void power_up_PRX();

   void configure_PTX();
   void power_up_PTX();

   void write_tx_payload(void* data, unsigned int len);
   void read_rx_payload(void* data, unsigned int len);
   void pulse_CE();

}

#endif
