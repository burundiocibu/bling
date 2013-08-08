#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
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
#include "ensemble.hpp"

void nrf_tx(unsigned slave, uint8_t *buff, size_t len);
void slider(unsigned slave, uint8_t ch, uint16_t &v, int dir);
void hexdump(uint8_t* buff, size_t len);
void heartbeat(int slave);
void shutdown(int param);
void print_slave(int slave);
void process_ui();

using namespace std;

// Two global objects. sorry
RunTime runtime;
ofstream logfile;


int main(int argc, char **argv)
{
   string log_fn="master_lcd.log";

   signal(SIGTERM, shutdown);
   signal(SIGINT, shutdown);

   if (true)
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

   logfile << fixed << setprecision(3) << 1e-3*runtime.msec() << hex << endl;

   nRF24L01::channel = 2;
   memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
   memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
   memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[2], nRF24L01::addr_len);

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
   {
      logfile << "Failed to find nRF24L01. Exiting." << endl;
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

   while(true)
   {
      heartbeat(0);
      process_ui();
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
   bool ack = slave != 0;

   write_tx_payload(buff, len, ensemble::slave_addr[slave], ack);

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
      lcd_plate::set_cursor(0,12);
      lcd_plate::puts(to_string(ack_err).c_str());
   }
   else if (status & STATUS_TX_DS)
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
   else
      logfile << 1e-3*runtime.msec() << " tx_err " << ++tx_err << endl;
}


// This is intended to mimic a slider control that can ramp up/down
// the intensity of an LED
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

void print_slave(int slave)
{
   lcd_plate::set_cursor(0,13);
   lcd_plate::puts(to_string(slave).c_str());
}


void heartbeat(int slave)
{
   static uint32_t last_hb=0;
   if (runtime.msec() - last_hb < 1000)
      return;
   uint8_t buff[::messages::message_size];
   ::messages::encode_heartbeat(buff, runtime.msec());
   nrf_tx(slave, buff, sizeof(buff));
   last_hb = runtime.msec();
}


class Effect
{
public:
   Effect()
      : state(idle)
   {}

   Effect(unsigned st, unsigned d, unsigned s) 
      : start_time(st), duration(d), slave(s),
        state(pending)
   {}

   virtual void process(void) = 0;
   virtual void stop(void) = 0;

   enum State
   {
      idle, pending, started
   } state;

   unsigned start_time;
   unsigned duration;
   unsigned slave;
};


class Throb : public Effect
{
public:
   virtual void process(void)
   {
      if (state==idle)
         return;

      uint32_t ms = runtime.msec();
      if (ms < start_time)
         return;

      if (ms > start_time + duration)
      {
         state=idle;
         return;
      }

      state=started;
      unsigned sec = ms/1000;
      ms -= sec*1000;

      int v = ms;
      if (!(sec & 1))
         v = 1000-ms;
      uint8_t buff[::messages::message_size];
      ::messages::encode_set_tlc_ch(buff, 0, v);
      nrf_tx(slave, buff, sizeof(buff));
   }
};


void process_ui(void)
{
   static unsigned slave=0;
   static uint16_t red=0,green=0,blue=0;
   typedef map<string, void *> MenuMap;
   static MenuMap mm;
   static MenuMap::const_iterator curr;
   static uint8_t button=0;

   static bool first_time = true;
   if (first_time)
   {
      lcd_plate::puts("master");
      print_slave(slave);
      first_time=false;
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
         print_slave(slave);
         logfile << 1e-3*runtime.msec() << " slave " << slave << endl;
      }
      else if (b & lcd_plate::RIGHT)
      {
         //eff.start();
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
         uint8_t buff[::messages::message_size];
         ::messages::encode_start_effect(buff, 0, runtime.msec(), 1000);
         nrf_tx(slave, buff, sizeof(buff));
         logfile << 1e-3*runtime.msec() << " effect " << 0 << endl;
      }
   }
}
