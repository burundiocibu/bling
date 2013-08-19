/*
  master_loader.cpp copyright Jon Little (2013)

  This is the programmer side of the boot loader implemented in nrf_boot.cpp.
  It is written to run on a raspberry pi with a nRF24L01+ radio conected to
  its SPI interface. See nrf_boot.cpp for details about the protocol that
  is used across the radio. This program bascially reads in an SREC file,
  chopps it up and feeds it to the slave to be programmed.

  http://en.wikipedia.org/wiki/SREC_(file_format)

  While this code uses some constants from nrf24l01_defines it also uses
  some rourintes from nrf24l01.cpp. The latter needs to be fixed so the
  boot loader implementation stands apart from the regular code.
*/


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <boost/program_options.hpp>
#include <string>


#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "ensemble.hpp"
#include "nrf_boot.h"

RunTime runtime;


bool tx(uint8_t* buff, size_t len, unsigned slave_no);
void rx(void);


namespace po = boost::program_options;
using namespace std;


int main(int argc, char **argv)
{
   int verbose,debug;
   string input_fn;
   unsigned slave_no;
   po::options_description base_opts("General options");
   base_opts.add_options()
      ("help,h",  "Produce help message")
      ("hex,i",   po::value<string>(&input_fn), "Hex file to load.")
      ("slave,s",   po::value<unsigned>(&slave_no)->default_value(0), "Slave number to program.")
      ("verbose,v", po::value<int>(&verbose)->default_value(0), "Produce more verbose status messages.")
      ("debug,d", "Enable debug output.")
      ;

   try
   {
      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).options(base_opts).run(), vm);
      po::notify(vm);
      if (vm.count("help"))
      {
         cout << "Boot loader to work with the nRF" << endl
              << base_opts << endl;
         exit(1);
      }

      debug = vm.count("debug");
      if (debug)
         cout << "Reading image from " << input_fn << endl;

      if (slave_no == 0)
      {
         cout << "We don't program slave 0 (everybody at once) yet. Specify slave." << endl;
         exit(1);
      }
      else if (slave_no >= ensemble::num_slaves)
      {
         cout << "Invalid slave number: " << slave_no << ". Terminating" << endl;
         exit(-1);
      }

      nRF24L01::channel = 2;
      memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
      memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
      memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[slave_no], nRF24L01::addr_len);
      nRF24L01::setup();

      if (!nRF24L01::configure_base())
      {
         cout << "Failed to find nRF24L01. Exiting." << endl;
         return -1;
      }

      const uint8_t cfg = nRF24L01::read_reg(nRF24L01::CONFIG);
      nRF24L01::write_reg(nRF24L01::RX_PW_P0, 22);  // all messages 22 bytes wide
      nRF24L01::write_reg(nRF24L01::RX_PW_P1, 22);
      nRF24L01::write_reg(nRF24L01::RX_PW_P2, 22);

      nRF24L01::configure_PTX();
      nRF24L01::flush_tx();

      uint32_t last_hb = runtime.msec();
      uint8_t buff[message_size];
      
      for (long i=0; 20000; i++)
      {
         uint32_t t = runtime.msec();

         if (t - last_hb > 990)
         {
            uint8_t *p = buff;
            buff[0] = 0xff & (frame_word >> 8);
            buff[1] = 0xff & (frame_word);
            buff[2] = no_op;
            bool ack = tx(buff, message_size, slave_no);
            if (!ack)
               cout << "Packet lost" << endl;
            else
               cout << "Ping :-)" << endl;
            last_hb = t;
         }
      }

      nRF24L01::shutdown();
      cout << "Done programming" << endl;
   }
   catch (po::error& e)
   {
      cout << e.what() << endl
           << base_opts << endl;
      exit(-1);
   }

   return 0;
}


bool tx(uint8_t* buff, size_t len, unsigned slave_no)
{
   bool success=false;
   using namespace nRF24L01;
   static unsigned t_tx=0, nack_cnt=0, retry_cnt=0;
   static int dt=0;

   t_tx = runtime.msec();
   write_tx_payload(buff, len, ensemble::slave_addr[slave_no], true);

   uint64_t t0=runtime.usec();
   for (int i=0; i < 200; i++)
   {
      uint8_t status = read_reg(STATUS);
      if (status & STATUS_MAX_RT)
      {
         nack_cnt++;
         write_reg(STATUS, STATUS_MAX_RT);
         // data doesn't automatically removed...
         flush_tx();
         break;
      }
      else if (status & STATUS_TX_DS)
      {
         dt = runtime.usec() - t0;
         write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
         success=true;
         break;
      }
      delay_us(10);
   }

   uint8_t obs_tx = read_reg(OBSERVE_TX);
   retry_cnt += obs_tx & 0x0f;
   return success;
}


void rx(uint8_t* buff, size_t len)
{
   using namespace nRF24L01;

   int rx_dt;
   unsigned no_resp;
   uint8_t pipe;
   char config = read_reg(CONFIG);
   config |= CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   delay_us(120);  // Really should be 1.5 ms at least
   set_CE();

   uint32_t t0=runtime.usec();
   for (int i=0; i<1000; i++)
   {
      uint8_t status = read_reg(STATUS);
      if (status & STATUS_RX_DR)
      {
         read_rx_payload((char*)buff, len, pipe);
         rx_dt = runtime.usec() - t0;
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }

      if (runtime.usec() - t0 > 10000 && i>5)
      {
         no_resp++;
         rx_dt = runtime.usec() - t0;
         break;
      }
      delay_us(50);
   }

   // Config the nRF as PTX again
   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();
}
