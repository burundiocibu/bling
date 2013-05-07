// An i2c interface for both the raspberry pi and avr-libc

#include <string.h>
#include <stdio.h>

#ifdef AVR
#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h>
#include <avr/interrupt.h>
#else
#include <bcm2835.h>
#endif


namespace i2c
{

#ifdef AVR
// avr-libc interfaced
#define MAX_ITER 200

   uint8_t twst;

   void setup(void)
   {
      TWSR = 0;  // set prescaler of 0
      TWBR = 12; // Set SCL frequency to 400 kHz
   }

   void shutdown(void)
   {
   }

   // Waits for the current tx to complete
   // at SCL frequency of 400 kHz, this should be
   // at most 9 x 1/4e5 = 22.5 us
   // Returns the status with the prescaler masked out
   inline uint8_t wait_for_tx(void)
   {
      while ((TWCR & _BV(TWINT)) == 0) ;
      twst = (TW_STATUS & 0xf8);
      return twst;
   }

   // Send the start condition and wait for 
   /// the operation to compelte
   inline  uint8_t send_start(void)
   {
      TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
      return wait_for_tx();
   }

   // Send the stop condition
   inline void send_stop(void)
   {
      TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
   }

   // Start tx/rx of byte and wait for 
   // the operation to complete
   inline uint8_t send_data(const uint8_t data)
   {
      TWDR = data;
      TWCR = _BV(TWINT) | _BV(TWEN);
      return wait_for_tx();
   }

   // Start tx/rx of byte and wait for 
   // the operation to complete
   inline uint8_t receive_data(uint8_t& data, bool ack=false)
   {
      if (ack)
         TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
      else
         TWCR = _BV(TWINT) | _BV(TWEN);
      wait_for_tx();
      data = TWDR;
      return twst;
   }


   int read(const uint8_t slave_addr, const uint8_t reg_addr, uint8_t& data)
   {
      return read(slave_addr, reg_addr, &data, 1);
   }


   int write(const uint8_t slave_addr, const uint8_t cmd, const uint8_t data)
   {
      return write(slave_addr, cmd, &data, 1);
   }


   //=============================================================================
   int write(const uint8_t slave_addr, const uint8_t cmd, const uint8_t* data, const size_t len)
   {
      uint8_t n = 0;
      int rv = 0;
      
     restart:
      if (n++ >= MAX_ITER)
         return -1;

     begin:
      switch (send_start())
      {
         case TW_REP_START:
         case TW_START:       break;
         case TW_MT_ARB_LOST: goto begin;
         default:             return -1;
      }

      // WTF : without this line, this function hangs after a couple calls.
      // inserting a delay or messing with other bits on this port doesn't work
      // doing a read/write of the bit (i.e. PORTC ^= _BV(PC7) ) doesn't work either
      // Oh, its noise on the 5V rail used to pull up SCL & SDA
      //PORTC &= ~_BV(PC7);

      switch (send_data(slave_addr | TW_WRITE))
      {
         case TW_MT_SLA_ACK:  break;
         case TW_MT_SLA_NACK: goto restart;
         case TW_MT_ARB_LOST: goto begin;
         default:             goto error;
      }

      switch (send_data(cmd))
      {
         case TW_MT_DATA_ACK:  break;
         case TW_MT_DATA_NACK: goto quit;
         case TW_MT_ARB_LOST:  goto begin;
         default:              goto error;
      }

      for (int i=len; i > 0; i--)
      {
         switch (send_data(*data++))
         {
            case TW_MT_DATA_NACK: goto error;
            case TW_MT_DATA_ACK:  rv++; break;
            default:              goto error;
         }
      }

     error:
      PORTB ^= _BV(PB7);
      rv = -1;

     quit:
      send_stop();
      return rv;
   }


   //=============================================================================
   int read(const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* data, const size_t len)
   {
      int rv = 0;
      int n=0;

     restart:
      if (n++ >= MAX_ITER)
         return -1;

     begin:
      switch(send_start())
      {
         case TW_REP_START:
         case TW_START:       break;
         case TW_MT_ARB_LOST: goto begin;
         default:             return -1;
      }

      switch (send_data(slave_addr | TW_WRITE))
      {
         case TW_MT_SLA_ACK:  break;
         case TW_MT_SLA_NACK: goto restart;
         case TW_MT_ARB_LOST: goto begin;
         default:             goto error;
      }

      PORTB ^= _BV(PB7);
      switch (send_data(reg_addr))
      {
         case TW_MT_DATA_ACK:  break;
         case TW_MT_DATA_NACK: goto quit;
         case TW_MT_ARB_LOST:  goto begin;
         default:              goto error;
      }

      switch (send_start())
      {
         case TW_START:
         case TW_REP_START:    break;
         case TW_MT_ARB_LOST:  goto begin;
         default:              goto error;
      }

      switch (send_data(slave_addr | TW_READ))
      {
         case TW_MR_SLA_ACK:   break;
         case TW_MR_SLA_NACK:  goto quit;
         case TW_MR_ARB_LOST:  goto begin;
         default:              goto error;
      }

      for (int i=len; i > 0; i--)
      {
         switch (receive_data(*data++, (i==1)))
         {
            case TW_MR_DATA_NACK: goto quit;
            case TW_MR_DATA_ACK:  break;
            default:              goto error;
         }
         rv++;
      }

     error:
      rv = -1;

     quit:
      send_stop();
      return rv;
   }


   void delay_us(uint32_t us)
   {
      for (uint16_t i=0; i<us; i++)
         _delay_us(1);
   }

#else
// bcm2835 based interface

   void setup(void)
   {
      bcm2835_init();
      bcm2835_i2c_begin();
      // default should be 100kHz
   }

   void shutdown(void)
   {
      bcm2835_i2c_end();
   }

   //=============================================================================
   int write(const uint8_t slave_addr, const uint8_t cmd, const uint8_t* data, const size_t len)
   {
      bcm2835_i2c_setSlaveAddress(slave_addr);
      int rc;
      char buff[len+1];
      buff[0]=(char)cmd;
      memcpy(buff+1, data, len);
      rc = bcm2835_i2c_write(buff, len+1);
      if (rc != BCM2835_I2C_REASON_OK)
         printf("write rc: %d\n", rc);
      return rc;
   }

   int write(const uint8_t slave_addr, const uint8_t cmd, const uint8_t data)
   {
      return write(slave_addr, cmd, &data, 1);
   }


   //=============================================================================
   int read(const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* data, const size_t len)
   {
      bcm2835_i2c_setSlaveAddress(slave_addr);
      int rc;

      rc = bcm2835_i2c_write((char*)&reg_addr, 1);
      if (rc != BCM2835_I2C_REASON_OK)
      {
         printf("write reg rc: %d\n", rc);
         return rc;
      }
      rc = bcm2835_i2c_read((char*)data, len);
      if (rc != BCM2835_I2C_REASON_OK)
         printf("read rc: %d\n", rc);

      return rc;
   }


   int read(const uint8_t slave_addr, const uint8_t reg_addr, uint8_t& data)
   {
      return read(slave_addr, reg_addr, &data, 1);
   }


   void delay_us(uint32_t us)
   {
      bcm2835_delayMicroseconds(us);
   }


#endif
}
