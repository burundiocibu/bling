#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ncurses.h>
#include <boost/program_options.hpp>
#include <string>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"

#include "Slave.hpp"
#include "ensemble.hpp"

RunTime runtime;

namespace po = boost::program_options;


int main(int argc, char **argv)
{

   int verbose,debug;
   std::string input_fn;
   std::vector<std::string> eph_files;
   po::options_description base_opts("General options");
   base_opts.add_options()
      ("help,h",  "Produce help message")
      ("hex,i",   po::value<std::string>(&input_fn), "Hex file to load.")
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
         std::cout << "Boot loader to work with the nRF" << std::endl
              << base_opts << std::endl;
         exit(1);
      }

      debug = vm.count("debug");
      if (debug)
         std::cout << "Reading from " << input_fn << std::endl;

      WINDOW *win;
      int prev_curs;

      win = initscr();
      cbreak();
      nodelay(win, true);
      noecho();
      nonl();
      intrflush(win, true);
      keypad(win, true);
      prev_curs = ::curs_set(0);   // we want an invisible cursor.
      mvprintw(1, 0, "  #      t_tx   tx_dt   tx_cnt   RTC   NACK    \%%          t_rx   rx_dt   no_resp");

      nRF24L01::channel = 2;
      memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
      memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
      memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[2], nRF24L01::addr_len);
      nRF24L01::setup();

      if (!nRF24L01::configure_base())
      {
         printf("Failed to find nRF24L01. Exiting.\n");
         return -1;
      }

      const uint8_t cfg = read_reg(CONFIG);
      write_reg(nRF24L01::RX_PW_P0, 22);  // all messages 22 bytes wide
      write_reg(nRF24L01::RX_PW_P1, 22);
      write_reg(nRF24L01::RX_PW_P2, 22);

      nRF24L01::configure_PTX();
      nRF24L01::flush_tx();

      uint32_t last_hb=0;
      Slave ship[ensemble::num_slaves];
      for (int i=0; i < ensemble::num_slaves; i++)
         ship[i].slave_no = i;

      for (long i=0; ; i++)
      {
         char key = getch();
         if (key=='q')
            break;
         else if (key==' ')
         {
            while(true)
            {
               key = getch();
               if (key==' ')
                  break;
            }
         }

         uint32_t t = runtime.msec();

         if (t - last_hb > 990)
         {
            messages::encode_heartbeat(ship[0].buff, t);
            ship[0].tx();
            last_hb = t;
         }

         for (int j=1; j <4; j++)
         {
            Slave& slave = ship[j];
            messages::encode_ping(slave.buff);
            if (slave.tx())
               slave.rx();
            mvprintw(1+slave.slave_no, 0, slave.status().c_str());
         }
      }

      nRF24L01::shutdown();
      endwin();

   }
   catch (po::error& e)
   {
      std::cout << e.what() << std::endl
           << base_opts << std::endl;
      exit(-1);
   }

   return 0;
}
