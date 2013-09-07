#include <cstdlib>
#include <cstdio>
#include <ncurses.h>
#include <unistd.h>

#include <bcm2835.h>

#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"
#include "ensemble.hpp"

using namespace std;

RunTime runtime;

unsigned slave=0;
void nrf_tx(uint8_t *buff, size_t len, unsigned slave);
void nrf_rx();
void slider(uint8_t ch, uint16_t &v, int dir);
void hexdump(uint8_t* buff, size_t len);

int debug;

int main(int argc, char **argv)
{
   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, "di:s:")) != -1)
      switch (c)
      {
         case 'd': debug++; break;
         case 's': slave = atoi(optarg); break;
         default:
            printf("Usage %s -i fn -s slave_no [-d]\n", argv[0]);
            exit(-1);
      }

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

   nRF24L01::channel = ensemble::default_channel;
   memcpy(nRF24L01::master_addr,    ensemble::master_addr,   nRF24L01::addr_len);
   memcpy(nRF24L01::broadcast_addr, ensemble::slave_addr[0], nRF24L01::addr_len);
   memcpy(nRF24L01::slave_addr,     ensemble::slave_addr[2], nRF24L01::addr_len);

   nRF24L01::setup();

   if (!nRF24L01::configure_base())
   {
      printf("Failed to find nRF24L01. Exiting.\n");
      return -1;
   }
   nRF24L01::configure_PTX();
   nRF24L01::flush_tx();

   const int LED=RPI_GPIO_P1_07;
   bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);

   uint8_t buff[ensemble::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   uint16_t red=0,green=0,blue=0;
   unsigned hb_count=0;
   uint32_t last_hb=0;
   mvprintw(0,0, "tx_fc  slave  R   G   B      j    tx_err  tx_obs ack_err");
   mvprintw(1, 6, "%3d   %03x %03x %03x", slave, red, green, blue);
   mvprintw(9, 0, "   t_rx   fc   #  ver   Vbat,soc     mmc");

   for (int i=0; ; i++)
   {
      uint32_t t = runtime.msec();
      if (t % 1000 <250)
         bcm2835_gpio_write(LED, LOW);
      else 
         bcm2835_gpio_write(LED, HIGH);

      if (t - last_hb > 990)
      {
         messages::encode_heartbeat(buff, t);
         nrf_tx(buff, sizeof(buff), slave);
         hb_count++;
         last_hb = t;
         mvprintw(1, 0, "%02x", messages::freshness_count);
      }

      char key = getch();
      if (key=='q')
         break;

      if (key == 0xff)
      {
         // sleep 10 ms
         bcm2835_delayMicroseconds(10000);
         continue;
      }

      switch(key)
      {
         case 'G': slider(0, red,  -1);  break;
         case 'g': slider(0, red,   1);  break;
         case 'R': slider(1, green, -1); break;
         case 'r': slider(1, green,  1); break;
         case 'B': slider(2, blue,  -1); break;
         case 'b': slider(2, blue,   1); break;
         case 'w': slider(0, red,  1); slider(1, green,  1); slider(2, blue,  1); break;
         case 'W': slider(0, red, -1); slider(1, green, -1); slider(2, blue, -1); break;
         case '0':
            messages::encode_start_effect(buff, 0, t, 1000);
            nrf_tx(buff, sizeof(buff), slave);
            break;
         case '1':
            messages::encode_start_effect(buff, 1, t, 20000);
            nrf_tx(buff, sizeof(buff), slave);
            break;
         case '2':
            messages::encode_start_effect(buff, 2, t, 20000);
            nrf_tx(buff, sizeof(buff), slave);
            break;

         case '3':
            // First real effect; 
            messages::encode_start_effect(buff, 3, t, 2000);
            nrf_tx(buff, sizeof(buff), slave);
            break;
            
         case 's':
         case 'S':
            messages::encode_all_stop(buff);
            nrf_tx(buff, sizeof(buff), slave);
            red=0;green=0;blue=0;
            break;
         case 'p':
            messages::encode_ping(buff);
            nrf_tx(buff, sizeof(buff), slave);
            nrf_rx();
            break;
         case 'z':
            messages::encode_reboot(buff);
            nrf_tx(buff, sizeof(buff), slave);
            break;
      }
      mvprintw(1, 6, "%3d   %03x %03x %03x", slave, red, green, blue);
   }

   nRF24L01::shutdown();
   endwin();
   bcm2835_gpio_write(LED, HIGH);
   return 0;
}



void nrf_tx(uint8_t *buff, size_t len, unsigned slave)
{
   using namespace nRF24L01;

   static unsigned ack_err=0;
   static unsigned tx_err=0;
   bool ack = slave != 0;

   write_tx_payload(buff, len, (const char*)ensemble::slave_addr[slave], ack);

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
   }
   else if (status & STATUS_TX_DS)
      write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
   else
      tx_err++;

   uint8_t obs_tx = read_reg(OBSERVE_TX);
   mvprintw(1, 27, "%3d  %3d       %02x    %3d", j, tx_err, obs_tx, ack_err);
}



void nrf_rx(void)
{
   using namespace nRF24L01;
   uint8_t buff[ensemble::message_size];
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
         read_rx_payload((char*)buff, ensemble::message_size, pipe);
         write_reg(STATUS, STATUS_RX_DR); // clear data received bit
         break;
      }
      delay_us(200);
   }

   config &= ~CONFIG_PRIM_RX;
   write_reg(CONFIG, config); // should still be powered on
   clear_CE();

   uint32_t t_rx;
   uint16_t slave_id, soc, vcell, missed_message_count;
   uint8_t msg_id, freshness_count;
   int8_t major_version, minor_version;

   messages::decode_status(buff, slave_id, t_rx, major_version, minor_version,
                           vcell, soc, missed_message_count, freshness_count);

   mvprintw(10, 0, "%8.3f ", 0.001*t_rx);
   soc = 0xff & (soc >> 8);
   printw(" %02x %3d %d.%d  %1.3fmV %d%%    %d            ",
          freshness_count, slave_id, major_version,
          minor_version, 1e-3*vcell,  soc,  missed_message_count);
}


void hexdump(uint8_t* buff, size_t len)
{
   for (int i = 0; i <len; i++)
      printw("%.2X ", buff[i]);
}

void slider(uint8_t ch, uint16_t &v, int dir)
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

   uint8_t buff[ensemble::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;
   messages::encode_set_tlc_ch(buff, ch, v);
   nrf_tx(buff, sizeof(buff), slave);
}
