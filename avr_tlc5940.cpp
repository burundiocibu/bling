#include <util/delay.h> // F_CPU should come from the makefile...
#include <avr/io.h>
#include <util/twi.h>
#include <avr/interrupt.h>

#include "avr_tlc5940.hpp"

// This is an AVR interface to a TLC5940
// Uses:
//  TIMER0 to generate the GSCLOCK on PB7
//  TIMER1 to generate the BLANK strobe (clocked by GSCLOCK on the T1 pin)
//  USART0 to clock the 'greyscale' data out to the chip (in MSPI mode)
//  PC3 for TLC5940 XLAT signal (to write the data into the PWM registers)
//  PD4 for the greyscale data clock (SCLK)
//  PD5 clock input to TIMER1, from GSCLOCK
//  PC2 for the BLANK signal (to shut off output and to start the next PWM cycle)
//  PD7 for the VPRG
//
// Note that it appears that the unit goes to 100% on at 1023, not 4095.
// This is probably a bug in the way I am communicating to the. Everything
// else looks good when I look at it on the scope but I just need to move
// on...
//
namespace avr_tlc5940
{
   uint8_t gsdata[24];
   bool pending_gsdata;
   bool need_xlat;

   void setup(void)
   {
      //==============================================================
      // tlc5940 interface
      // use Timer0 to generate a 4 MHz GSCLK on OC0A
      TCCR0A = _BV(COM0A0) | _BV(WGM01); // Clear Timer on Compare match mode, toggle OC0A on match
      TCCR0B = _BV(CS00); // prescaler div = 1
      TIMSK0 = 0; // make sure it generates no interrupts
      OCR0A = 0;  // f=8e6/(2*(0+1)) = 4 MHz
      DDRD |= _BV(PD6); // enable the OC0A output
      
      // use Timer1 to count up 4096 clocks to restart the gsdata cycle
      // ext clock source on T1, rising edge, CTC mode; reset on OCR1A match
      TCCR1A = 0;
      TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS11) | _BV(CS10);
      OCR1A = 4096;
      TIMSK1 |= _BV(OCIE1A); // Interrupt on output compare on A  
      
      //         BLANK      XLAT
      DDRC  |= _BV(PC2) | _BV(PC3);
      PORTC |= _BV(PC2); // start with blank high to keep lights off
      PORTC &= ~_BV(PC3);
      //       VPRG
      DDRD  |= _BV(PD7);
      PORTD &= ~_BV(PD7);

      // Set up USART0/MSPI
      DDRD  |= _BV(PD4); // use XCK for the SCLK
      UBRR0 = 0; // baud=8e6/(2*(0+1)) = 4 MHz baud rate
      UCSR0C = _BV(UMSEL01) | _BV(UMSEL00); // operate USART 0 in MSPIM mode, MSB first
      UCSR0B = _BV(RXEN0) | _BV(TXEN0); // give the pins the correct dir..
      UBRR0 = 0; // set the baud /again/. cause they say to

      for (unsigned i=0; i<sizeof(gsdata); i++)
         gsdata[i]=0;
      output_gsdata();
      PORTD |= _BV(PD4);
      PORTD &= ~_BV(PD4); // additional SCLK pulse

      sei();
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
      pending_gsdata=true;
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
      if (!pending_gsdata)
         return;

      need_xlat=false;
      // Now to send out the grayscale data
      // 24 bytes at 4 MHz takes 24 us + setup time
      for (unsigned i=0; i<sizeof(gsdata); i++)
      {
         while ( !(UCSR0A & _BV(UDRE0)) ) ; // wait for tx buffer to be clear
         UDR0 = gsdata[i];
         while ( !(UCSR0A & _BV(RXC0))  ) ;
      }

      pending_gsdata=false;
      need_xlat=true; // indicate that there is new data to be latched
   }

   // Pulse BLANK  every 4096 GSCLK cycles
   // should go off 4096 / 4 MHz = 1.024 ms
   // While BLANK is active, pulse XLAT if needed
   ISR (TIMER1_COMPA_vect)
   {
      PORTC |= _BV(PC2);  // set BLANK

      if (need_xlat)
      {
         PORTC |= _BV(PC3); // pulse XLAT
         PORTC &= ~_BV(PC3);
         need_xlat = false;
      }

      PORTC &= ~_BV(PC2); // clear BLANK
   }
}
