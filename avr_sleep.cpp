void do_sleep()
{
   //avr_tlc5940::setup();
   // uses TIMER0 for GS clock, TIMER1 to drive blank (and an IRQ)
   // just disable TIMER0 using PRR
   PRR |= _BV(PRTIM2);
   PORTC |= _BV(PC2); // _BLANK


   // Set the CPU clock prescaler to be 1024. 8MHz / 1024 = 7.8125 kHz
   CLKPR = CLKPCE;
   CLKPR = CLKPS3;

   //avr_rtc::setup();
   // uses TIMER2 to generate a 1kHz interrupt
   // uses a prescaler of 
   // lets make this wake us up periodically from sleep
   // prescaler = 1024, f=7.8125 kHz/1024 = 7.6294 Hz
   // with OCR=125, 7.6294 / 125 = 0.061035 Hz = 16.38 seconds between IRQ
   TCCR2B = _BV(CS22|CS21|CS20) ;

   // Turn off 12V supply
   PORTB &= ~_BV(PB1);

   // Got to "power-save" mode
   SMCR = SM1 | SM0;   
   
   //nRF24L01::setup();
   // generates an INT0 on rx of data when powered on
   // leave this as is..
   using namespace nRF24L01;
   char config = read_reg(CONFIG);


   clear_CE();  // Turn off receiver
   write_reg(CONFIG, config & ~CONFIG_PWR_UP); // power down
   while(true)
      sleep_mode();

   while(true)
   {
      clear_CE();  // Turn off receiver
      write_reg(CONFIG, config & ~CONFIG_PWR_UP); // power down
      sleep_mode();

      write_reg(CONFIG, config & CONFIG_PWR_UP); // power up
      set_CE();
      
      sleep_mode();
      
      uint8_t status = nRF24L01::read_reg(nRF24L01::STATUS);
      if (status != 0x0e)
      {
         uint8_t buff[ensemble::message_size];
   
         uint8_t pipe;
         nRF24L01::read_rx_payload(buff, sizeof(buff), pipe);
         nRF24L01::clear_IRQ();
         if (messages::get_id(buff) ==  messages::wake_id)
            break;
      }
   }


   // need to turn back on all hardware here...

   // Turn back on the tlc5940 GS clock
   PRR &= ~_BV(PRTIM2);

   // Restore the prescaler for the tlc5940
   TCCR0B = _BV(CS00); // prescaler div = 1

   // Restore the prescaler for the rtc
   TCCR2B = _BV(CS22) ; // prescaler = 64, f=8e6/64=125kHz

   // Turn back on 12V supply
   PORTB |= _BV(PB1);

   return;
   // or maybe ((APP*)0)();
}
