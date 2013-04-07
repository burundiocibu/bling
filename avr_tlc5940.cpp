#include <util/delay.h> // F_CPU should come from the makefile...
#include <avr/io.h>
#include <util/twi.h>
#include <avr/interrupt.h>

#include "avr_tlc5940.hpp"

// This is an AVR interface to a TLC5940
// Uses:
//  TIMER0 to generate the GSCLOCK on PB7
//  TIMER1 to generate the BLANK strobe (clocked by GSCLOCK)
//  USART1 to clock the 'greyscale' data out to the chip (in MSPI mode)
//  PD4 for TLC5940 XLAT signal (to write the data into the PWM registers)
//  PD5 for the greyscale data clock (SCLK)
//  PD6 clock input to TIMER1, from GSCLOCK
//  PD7 for the BLANK signal (to shut off output and to start the next PWM cycle)
//  PC6 for the VPRG
//
// Note that it appears that the unit goes to 100% on at 1023, not 4095.
// This is probably a bug in the way I am communicating to the. Everything
// else looks good when I look at it on the scope but I just need to move
// on...
//
namespace avr_tlc5940
{
   uint8_t gsdata[24];
   uint8_t need_xlat;

   void setup(void)
   {
      //==============================================================
      // tlc5940 interface
      // use Timer0 to generate a 4 MHz GSCLK on PB7
      TCCR0A = _BV(COM0A0) | _BV(WGM01); // Clear Timer on Compare match mode, toggle OC0A on match
      TCCR0B = _BV(CS00); // prescaler div = 1
      TIMSK0 = 0; // make sure it generates no interrupts
      OCR0A = 1;  // f=16e6/(2*1*(1+1)) = 4 MHz
      DDRB |= _BV(PB7); // enable the OC0A output

      // use Timer1 to count up 4096 clocks to restart the gsdata cycle
      TCCR1A = _BV(WGM11) | _BV(WGM01); // CTC mode, reset on OCR1A
      TCCR1B = _BV(CS12) | _BV(CS11) | _BV(CS10); // ext clock source on T1 (PD6), falling edge
      OCR1A = 4096;
      TIMSK1 |= _BV(OCIE1A); // Interrupt on output compare on A  
      sei();

      //        XLAT       SCLK       BLANK   
      DDRD  |= _BV(PD4) | _BV(PD5) | _BV(PD7);
      //       VPRG
      DDRC |= _BV(PC6);
      PORTD |= _BV(PD7);
      PORTD &= ~(_BV(PD4) | _BV(PD5));
      // PD3 is TxD and configured with the USART 
      // PD7 is XERR, input, not used at the moment

      // Set up USART1/MSPI
      UBRR1 = 0; //16e6/(2*(0+1) = 8 MHz baud rate
      UCSR1C = _BV(UMSEL11) | _BV(UMSEL10); // operate USART 1 in MSPIM mode, MSB first
      UCSR1B = _BV(RXEN1) | _BV(TXEN1); // give the pins the correct dir..
      UBRR1 = 0; // set the baud /again/. cause they say to

      PORTC |= _BV(PC6);
      PORTC &= ~_BV(PC6);
      
      for (unsigned i=0; i<sizeof(gsdata); i++)
         gsdata[i]=0;

      PORTD &= ~_BV(PD7);
      output_gsdata();
      _delay_us(40);
      PORTD |= _BV(PD5);
      PORTD &= ~_BV(PD5); // additional SCLK pulse
   }

   void set_channel(int chan, int value)
   {
      size_t lb = 23 - ((3*chan)>>1);
      uint16_t pwm = (chan & 1) ? value<<4 : value;
      uint16_t mask = (chan & 1) ? ~0xfff0: ~0x0fff;
      uint8_t pwm_l = pwm;
      uint8_t pwm_h = pwm>>8;
      uint8_t mask_l = mask & 0xff;
      uint8_t mask_h = mask>>8;
      
      gsdata[lb] &= mask_l;
      gsdata[lb-1] &= mask_h;
      gsdata[lb] |= pwm_l;
      gsdata[lb-1] |= pwm_h;
   }

   unsigned get_channel(int chan)
   {
      size_t lb = 23 - ((3*chan)>>1);
      uint16_t pwm;
      pwm = gsdata[lb];
      pwm |= gsdata[lb-1] << 8;
      if (chan & 1)
         pwm >>= 4;
      return pwm & 0x0fff;
   }

   void output_gsdata(void)
   {
      need_xlat=0;// ignore any previous data

      // Now to send out the grayscale data
      // 24 bytes at 8 MHz takes 24 us + setup time
      for (unsigned i=0; i<sizeof(gsdata); i++)
      {
         while ( !(UCSR1A & _BV(UDRE1)) ) ; // wait for tx buffer to be clear
         UDR1 = gsdata[i];
         while ( !(UCSR1A & _BV(RXC1))  ) ;
      }

      _delay_us(5); //  This seems to make the flickering no occur

      need_xlat = 1; // indicate that there is new data to be latched
   }

   void output_dcdata(void)
   {
   }

   // Pulse BLANK  every 4096 GSCLK cycles
   // should go off 4096 / 4 MHz = 1.024 ms
   // While BLANK is active, pulse XLAT if needed
   ISR (TIMER1_COMPA_vect)
   {
      PORTD |= _BV(PD7);  // set BLANK
      if (need_xlat)      // xfer data only while blank is high
      {
         // pulse XLAT
         PORTD |= _BV(PD4);
         PORTD &= ~_BV(PD4);
         need_xlat=0;
      }  
      PORTD &= ~_BV(PD7); // clear BLANK
   }

}
