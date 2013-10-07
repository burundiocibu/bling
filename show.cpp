#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <list>
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

void nrf_tx(unsigned slave, uint8_t *buff, size_t len, bool updateDisplay, int resendCount);
void slider(unsigned slave, uint8_t ch, uint16_t &v, int dir);
void hexdump(uint8_t* buff, size_t len);
void heartbeat(int slave);
void shutdown(int param);
void print_slave(int slave);
void print_effect_name(int effect);
void process_ui();

namespace msg=messages;
using namespace std;

const int BROADCAST_ADDRESS  = 0;
const int DELAY_BETWEEN_XMIT = 1000;
const int NUMBER_5MS_PER_SECOND = 200;

// Two global objects. sorry
RunTime runtime;
ofstream logfile;


int main(int argc, char **argv)
{
   string log_fn="show.log";

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

   lcd_plate::setup(0x20);
   lcd_plate::clear();
   lcd_plate::set_cursor(0,0);
   lcd_plate::set_backlight(lcd_plate::YELLOW);

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

/*
 * nrf_tx - Transmit data
 *
 * slave         - address to send to
 * buff          - char buffer to send to slave
 * len           - number of characters in buffer
 * updateDisplay - true to update send status on display, false otherwise
 * secondsToSendMsgFor   - How many seconds the command should be resent over (sends every 5 mSec).  0 to send once.
 */
void nrf_tx(unsigned slave, uint8_t *buff, size_t len, bool updateDisplay, int secondsToSendMsgFor)
{
   using namespace nRF24L01;

   static unsigned ack_err=0;
   static unsigned tx_err=0;
   bool ack = slave != 0;

   // determine number of times through loop for resend
   int resendCount;
   if(secondsToSendMsgFor == 0)
   {
      resendCount = 0;
   }
   else
   {
      resendCount = 1 * NUMBER_5MS_PER_SECOND;
   }

   int numXmit;
   for (numXmit=0; numXmit <= resendCount; numXmit++)
   {
      if(numXmit != 0)
      {
         // except first time through loop, delay 5 mS
         bcm2835_delay(5);
      }

      write_tx_payload(buff, len, (const char *) ensemble::slave_addr[slave], ack);

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
         // Must clear to enable further communcation
         flush_tx();
         logfile << 1e-3*runtime.msec() << " ack_err " << ack_err << endl;
         //lcd_plate::puts(to_string(ack_err).c_str());
         if(updateDisplay)
         {
            lcd_plate::set_cursor(0,14);
            lcd_plate::puts("--");
         }
      }
      else if (status & STATUS_TX_DS)
      {
         write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
         if(updateDisplay)
         {
            lcd_plate::set_cursor(0,14);
            lcd_plate::puts("++");
         }
      }
      else
      {
         logfile << 1e-3*runtime.msec() << " tx_err " << ++tx_err << endl;
         if(updateDisplay)
         {
            lcd_plate::set_cursor(0,14);
            lcd_plate::puts("--");
         }
      }
   }
}


void print_effect_name(string name)
{
   lcd_plate::clear();
   lcd_plate::set_cursor(0,0);
   lcd_plate::puts(name.c_str());
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
   uint8_t buff[ensemble::message_size];
   msg::encode_heartbeat(buff, runtime.msec());
   nrf_tx(slave, buff, sizeof(buff), false, 0);
   last_hb = runtime.msec();
}


void process_ui(void)
{
   struct Event
   {
      Event(string n, unsigned i, unsigned d) :
         name(n), id(i), duration(d)
      {};
      string name;        // Should be under 12 characters
      uint8_t id;         // effect id from Effect.cpp
      uint16_t duration;  // duration in ms
   };
   
   static list<Event> event_list;
   static list<Event>::const_iterator current_event;
   static uint8_t prev_button=0;

   static bool first_time = true;
   if (first_time)
   {
      event_list.push_back(Event("set13",        8, 4500));
      event_list.push_back(Event("flash",        0, 900));
      event_list.push_back(Event("pre hit",      3, 10000));
      event_list.push_back(Event("sparkle",      4, 30000));
      event_list.push_back(Event("sparkle fade", 5, 4000));
      event_list.push_back(Event("dim red",      6, 1000));
      event_list.push_back(Event("R->L fade",    7, 4500));

      current_event = event_list.begin();
      print_effect_name(current_event->name);
      first_time=false;
   }

   uint8_t b = 0x1f & ~lcd_plate::read_buttons();
   if (b!=prev_button)
   {
      prev_button = b;
      logfile << 1e-3*runtime.msec() << " lcd keypress: " << hex << int(b) << endl;
      if (b & lcd_plate::LEFT)
      {
         if (current_event==event_list.begin())
            --(current_event=event_list.end());
         else
            current_event--;
         print_effect_name(current_event->name);
      }
      else if (b & lcd_plate::RIGHT)
      {
         current_event++;
         if (current_event==event_list.end())
            current_event = event_list.begin();
         print_effect_name(current_event->name);
      }
      else if (b & lcd_plate::UP)
      {
         //TODO: for now, nothing
      }
      else if (b & lcd_plate::DOWN)
      {
         //TODO: for now, nothing
      }
      else if (b & lcd_plate::SELECT)
      {
         uint8_t buff[ensemble::message_size];
         msg::encode_start_effect(buff, current_event->id, runtime.msec(), current_event->duration);
         nrf_tx(BROADCAST_ADDRESS, buff, sizeof(buff), true, 200);
         current_event++;
         if (current_event==event_list.end())
            current_event = event_list.begin();
         print_effect_name(current_event->name);
      }
   }
}
