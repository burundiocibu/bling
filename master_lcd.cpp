#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <time.h>
#include <sys/mman.h> // for settuing up no page swapping
#include <bcm2835.h>
#include <chrono>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "lcd_plate.hpp"

#include "messages.hpp"

RunTime runtime;

void nrf_tx(unsigned slave, uint8_t *buff, size_t len);
void nrf_rx(unsigned slave);
void slider(unsigned slave, uint8_t ch, uint16_t &v, int dir);
void hexdump(uint8_t* buff, size_t len);

void shutdown(int param);

using namespace std;
ofstream logfile;

int main(int argc, char **argv)
{
   string log_fn="master_lcd.log";

   signal(SIGTERM, shutdown);
   signal(SIGINT, shutdown);

   if (false)
   {
      struct sched_param sp;
      memset(&sp, 0, sizeof(sp));
      sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
      sched_setscheduler(0, SCHED_FIFO, &sp);
      mlockall(MCL_CURRENT | MCL_FUTURE);
   }

   time_t rawtime;
   struct tm * timeinfo;
   char now [80];
   time(&rawtime);
   timeinfo = localtime (&rawtime);
   strftime (now, 80, "%b %d %02H:%02M:%02S", timeinfo);

   logfile.open(log_fn, std::ofstream::app);
   logfile << now << " master_lcd start " << endl;

   lcd_plate::setup(0x20);
   lcd_plate::clear();
   lcd_plate::set_cursor(0,0);
   lcd_plate::set_backlight(lcd_plate::VIOLET);
   lcd_plate::puts("master_lcd");
   logfile << fixed << setprecision(3) << 1e-3*runtime.msec() << hex << endl;

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
   {
      logfile << "Failed to find nRF24L01. Exiting." << endl;
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

   uint8_t buff[::messages::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   uint16_t red=0,green=0,blue=0;
   unsigned hb_count=0;
   uint32_t last_hb=0;
   unsigned slave=0;

   uint8_t button=0;
   for (int i=0; ; i++)
   {
      uint32_t t = runtime.msec();

      if (t - last_hb > 990)
      {
         ::messages::encode_heartbeat(buff, t);
         nrf_tx(slave, buff, sizeof(buff));
         hb_count++;
         last_hb = t;
      }

      uint8_t b = 0x1f & ~lcd_plate::read_buttons();
      if (b!=button)
      {
         button = b;
         logfile << 1e-3*runtime.msec() << " lcd keypress: " << hex << int(b) << endl;
         if (b & lcd_plate::LEFT)
         {
            slave++;
            if (slave>3)
               slave=0;
            logfile << 1e-3*runtime.msec() << " slave " << slave << endl;
         }
         else if (b & lcd_plate::RIGHT)
         {
         }
         else if (b & lcd_plate::UP)
         {
            slider(slave, 0, red,   1);
            slider(slave, 1, green, 1);
            slider(slave, 2, blue,  1);
         }
         else if (b & lcd_plate::DOWN)
         {
            slider(slave, 0, red,   -1);
            slider(slave, 1, green, -1);
            slider(slave, 2, blue,  -1);
         }
         else if (b & lcd_plate::SELECT)
         {
            ::messages::encode_start_effect(buff, 0, t, 1000);
            nrf_tx(slave, buff, sizeof(buff));
            logfile << 1e-3*runtime.msec() << " effect " << 0 << endl;
         }
      }
      bcm2835_delayMicroseconds(5000);
   }
}


void shutdown(int param)
{
   nRF24L01::shutdown();
   lcd_plate::clear();
   lcd_plate::set_backlight(lcd_plate::OFF);
   lcd_plate::shutdown();
   bcm2835_close();
   logfile << 1e-3*runtime.msec() << " master_lcd stop" << endl;
   logfile.close();
   exit(0);
}


void nrf_tx(unsigned slave, uint8_t *buff, size_t len)
{
   using namespace nRF24L01;

   static unsigned ack_err=0;
   static unsigned tx_err=0;

   write_tx_payload(buff, len, slave);

   uint8_t status;
   int j;
   for(j=0; j<100; j++)
   {
      status = read_reg(STATUS);
      if (status & STATUS_TX_DS)
         break;
      delay_us(5);
   }

   if (status & STATUS_MAX_RT)
   {
      ack_err++;
      write_reg(STATUS, STATUS_MAX_RT);
      // data doesn't automatically removed...
      flush_tx();
      logfile << 1e-3*runtime.msec() << " ack_err " << ack_err << endl;
   }
   else if (status & STATUS_TX_DS)
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
   else
      logfile << 1e-3*runtime.msec() << " tx_err " << ++tx_err << endl;
}



void nrf_rx(unsigned slave)
{
   using namespace nRF24L01;
   uint8_t buff[::messages::message_size];
   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   delay_us(1000);
   set_CE();

   int i;
   for (i=0; i<100; i++)
   {
      if (read_reg(STATUS) & STATUS_RX_DR)
      {
         read_rx_payload((char*)buff, ::messages::message_size, pipe);
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }
      delay_us(200);
   }

   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();

   logfile << 1e-3*runtime.msec() << " rx: ";
   for(auto i:buff)
      logfile << i << " ";
   logfile << endl;
}



void slider(unsigned slave, uint8_t ch, uint16_t &v, int dir)
{
   if (dir>0)
   {
      if (v==0)
         v=1;
      else
         v = (v << 1) + 1;
      if (v >=4096)
         v=4095;
   }
   else
   {
      if (v>0)
         v >>= 1;
   }

   uint8_t buff[::messages::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   ::messages::encode_set_tlc_ch(buff, ch, v);
   nrf_tx(slave, buff, sizeof(buff));
}
