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

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "lcd_plate.hpp"
#include "Lock.hpp"

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

// Two global objects. sorry
RunTime runtime;
bool time_to_die=false;

int main(int argc, char **argv)
{
   Lock lock;
   signal(SIGTERM, shutdown);
   signal(SIGINT, shutdown);

   // lock this process into memory
   if (true)
   {
      struct sched_param sp;
      memset(&sp, 0, sizeof(sp));
      sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
      sched_setscheduler(0, SCHED_FIFO, &sp);
      mlockall(MCL_CURRENT | MCL_FUTURE);
   }

   lcd_plate::setup(0x20);
   lcd_plate::clear();
   lcd_plate::set_cursor(0,0);
   lcd_plate::set_backlight(lcd_plate::YELLOW);

   nRF24L01::channel = 2;
   memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
   memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
   memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[0], nRF24L01::addr_len);

   nRF24L01::setup();
   if (!nRF24L01::configure_base())
   {
      cout << "Failed to find nRF24L01. Exiting." << endl;
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

   while(!time_to_die)
   {
      heartbeat(0);
      process_ui();
      bcm2835_delayMicroseconds(5000);
   }

   nRF24L01::shutdown();
   lcd_plate::clear();
   lcd_plate::set_backlight(lcd_plate::OFF);
   lcd_plate::shutdown();
   bcm2835_close();
}


void shutdown(int param)
{
   time_to_die=true;
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
void nrf_tx(unsigned slave, uint8_t *buff, size_t len, bool updateDisplay, int resendCount)
{
   using namespace nRF24L01;

   static unsigned ack_err=0;
   static unsigned tx_err=0;
   bool ack = slave != 0;

   int numXmit;
   for (numXmit=0; numXmit <= resendCount; numXmit++)
   {
      if(numXmit != 0)
      {
         // except first time through loop, delay 5 mS
         bcm2835_delay(2);
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
   
   static list<Event> event_list, test_list;
   static list<Event>::const_iterator current_event,test_event;
   static uint8_t prev_button=0;

   static bool first_time = true;
   if (first_time)
   {
      int bpm=170; // beats per minute
      int mpb = 1000 * 60 / bpm; // ms per beat
      event_list.push_back(Event("set13",        8, 4500));
      event_list.push_back(Event("flash",        0, 4100));
      event_list.push_back(Event("pre hit",      3, 10000));
      event_list.push_back(Event("sparkle",      4, 30000));
      event_list.push_back(Event("sparkle fade", 5, 4000));
      event_list.push_back(Event("dim red",      6, 8000));
      event_list.push_back(Event("R->L fade",    7, 4500));
      event_list.push_back(Event("M3 A1",        9, 5   * mpb));
      event_list.push_back(Event("M3 A2",        9, 5   * mpb));
      event_list.push_back(Event("M3 A3",        9, 1   * mpb));
      event_list.push_back(Event("M3 A4",        9, 6   * mpb));
      event_list.push_back(Event("M3 A5",        9, 4.5 * mpb));
      event_list.push_back(Event("M3 A6",        9, 1   * mpb));
      event_list.push_back(Event("M3 A7",        9, 4.5 * mpb));
      event_list.push_back(Event("M3 A8",        9, 1   * mpb));
      event_list.push_back(Event("M3 A9",        9, 4   * mpb));
      event_list.push_back(Event("M3 B1",       10, 60  * mpb));

      event_list.push_back(Event("M3 D1",       11, 14 * mpb)); // this is intended to overlap the next
      event_list.push_back(Event("M3 D2",       12, 14 * mpb)); // ditto
      event_list.push_back(Event("M3 D3",       13, 14 * mpb));

      event_list.push_back(Event("M3 F1",       14, 10 * mpb));
      event_list.push_back(Event("M3 F2",       15, 12 * mpb));
      event_list.push_back(Event("M3 F3",       16, 10 * mpb));
      event_list.push_back(Event("M3 F4",       17, 15 * mpb));
      event_list.push_back(Event("M3 F5",       18, 21 * mpb));
      event_list.push_back(Event("M3 F6",       19, 30 * mpb));
      event_list.push_back(Event("M3 F7",       20, 30000));

      //test_list.push_back(Event("Flash",    0,  1500));
      test_list.push_back(Event("RGB Test", 1, 20000));

      current_event = event_list.begin();
      test_event = test_list.begin();

      print_effect_name(current_event->name);
      first_time=false;
   }

   uint8_t b = 0x1f & ~lcd_plate::read_buttons();
   if (b!=prev_button)
   {
      prev_button = b;
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
         uint8_t buff[ensemble::message_size];
         print_effect_name(test_event->name);
         msg::encode_start_effect(buff, test_event->id, runtime.msec(), test_event->duration);
         nrf_tx(BROADCAST_ADDRESS, buff, sizeof(buff), true, 50);
         test_event++;
         if (test_event==test_list.end())
            test_event = test_list.begin();
         // reprint the event that pressing select will execute
         print_effect_name(current_event->name);
      }
      else if (b & lcd_plate::DOWN)
      {
         print_effect_name("All Stop");
         uint8_t buff[ensemble::message_size];
         msg::encode_all_stop(buff);
         nrf_tx(BROADCAST_ADDRESS, buff, sizeof(buff), true, 50);
         print_effect_name(current_event->name);
      }
      else if (b & lcd_plate::SELECT)
      {
         uint8_t buff[ensemble::message_size];
         msg::encode_start_effect(buff, current_event->id, runtime.msec(), current_event->duration);
         nrf_tx(BROADCAST_ADDRESS, buff, sizeof(buff), true, 50);
         current_event++;
         if (current_event==event_list.end())
            current_event = event_list.begin();
         print_effect_name(current_event->name);
      }
   }
}
