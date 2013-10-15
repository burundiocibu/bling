#include <cstdlib>
#include <cstdio>
#include <ncurses.h>
#include <unistd.h>
#include <cmath>

#include <bcm2835.h>
#include "Lock.hpp"
#include "rt_utils.hpp"
#include "nrf24l01.hpp"
#include "messages.hpp"
#include "ensemble.hpp"

namespace msg=messages;
using namespace std;

RunTime runtime;

typedef struct HSV {
   HSV():h(0),s(0),v(0){};
   uint8_t h, s, v;
} hsv_t;

typedef struct RGB {
   RGB():r(0),g(0),b(0){};
   uint16_t r,g,b;
   void slide(unsigned ch, int dir)
   {
      uint16_t *v=&r;
      if (ch==1) v=&g;
      else if (ch==2) v=&b;
      if (dir>0)
      {
         if (*v==0)
            *v=1;
         else
            *v = (*v << 1) + 1;
         if (*v >=4096)
            *v=4095;
      }
      else
      {
         if (*v>0)
            *v >>= 1;
      }
   };
} rgb_t;

unsigned slave=0;
void nrf_tx(uint8_t *buff, size_t len, unsigned slave, unsigned repeat);
void nrf_rx();
void hexdump(uint8_t* buff, size_t len);
void set_rgb(rgb_t rgb);
void hsv2rgb(hsv_t hsv, rgb_t &rgb);

int debug;

int main(int argc, char **argv)
{
   Lock lock;

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
   hsv_t hsv;
   rgb_t rgb;

   uint8_t h=0, s=255, v=0;
   unsigned hb_count=0;
   uint32_t last_hb=0;
   mvprintw(0,0, "tx_fc  slave  R   G   B      j    tx_err  tx_obs ack_err");
   mvprintw(1, 6, "%3d   %03x %03x %03x", slave, rgb.r, rgb.g, rgb.b);
   mvprintw(2, 12, "%03x %03x %03x", hsv.h, hsv.s, hsv.v);
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
         msg::encode_heartbeat(buff, t);
         nrf_tx(buff, sizeof(buff), slave, 1);
         hb_count++;
         last_hb = t;
         mvprintw(1, 0, "%02x", msg::freshness_count);
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
         case 'w': rgb.slide(0, 1);rgb.slide(1, 1);rgb.slide(2, 1);set_rgb(rgb); break;
         case 'W': rgb.slide(0,-1);rgb.slide(1,-1);rgb.slide(2,-1); set_rgb(rgb); break;
         case 'r': rgb.slide(0, 1); set_rgb(rgb); break;
         case 'R': rgb.slide(0,-1); set_rgb(rgb); break;
         case 'g': rgb.slide(1, 1); set_rgb(rgb); break;
         case 'G': rgb.slide(1,-1); set_rgb(rgb); break;
         case 'b': rgb.slide(2, 1); set_rgb(rgb); break;
         case 'B': rgb.slide(2,-1); set_rgb(rgb); break;
         case 'v': hsv.v+=2; hsv2rgb(hsv, rgb); set_rgb(rgb); break;
         case 'V': hsv.v-=2; hsv2rgb(hsv, rgb); set_rgb(rgb); break;
         case 'h': hsv.h+=2; hsv2rgb(hsv, rgb); set_rgb(rgb); break;
         case 'H': hsv.h-=2; hsv2rgb(hsv, rgb); set_rgb(rgb); break;
         case 's': hsv.s+=2; hsv2rgb(hsv, rgb); set_rgb(rgb); break;
         case 'S': hsv.s-=2; hsv2rgb(hsv, rgb); set_rgb(rgb); break;
         case '0':
            msg::encode_start_effect(buff, 0, t, 750);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case '1':
            msg::encode_start_effect(buff, 1, t, 20000);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case '2':
            msg::encode_start_effect(buff, 2, t, 3000);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case '3':
            msg::encode_start_effect(buff, 3, t, 10000);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case '4':
            msg::encode_start_effect(buff, 4, t, 30000);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case '5':
            msg::encode_start_effect(buff, 5, t, 4000);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case '6':
            msg::encode_start_effect(buff, 6, t, 15000);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case '7':
            msg::encode_start_effect(buff, 7, t, 10000);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case '8':
            msg::encode_start_effect(buff, 8, t, 4500);
            nrf_tx(buff, sizeof(buff), slave, 25);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
            
         case 'x':
            msg::encode_all_stop(buff);
            nrf_tx(buff, sizeof(buff), slave, 5);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            hsv.v=0; hsv2rgb(hsv, rgb);
            break;
         case 'p':
            msg::encode_ping(buff);
            nrf_tx(buff, sizeof(buff), slave, 1);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            nrf_rx();
            break;
         case 'z':
            msg::encode_reboot(buff);
            nrf_tx(buff, sizeof(buff), slave, 1);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case 'n':
            msg::encode_sleep(buff);
            nrf_tx(buff, sizeof(buff), slave, 1);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
         case 'N':
            msg::encode_wake(buff);
            nrf_tx(buff, sizeof(buff), slave, 1);
            mvprintw(3, 2, "%8.3f tx ", 0.001*runtime.msec());
            break;
      }
      mvprintw(1, 6, "%3d   %03x %03x %03x", slave, rgb.r, rgb.g, rgb.b);
      mvprintw(2, 12, "%03x %03x %03x", hsv.h, hsv.s, hsv.v);
   }

   nRF24L01::shutdown();
   endwin();
   bcm2835_gpio_write(LED, HIGH);
   return 0;
}



void nrf_tx(uint8_t *buff, size_t len, unsigned slave, unsigned repeat)
{
   using namespace nRF24L01;

   static unsigned ack_err=0;
   static unsigned tx_err=0;
   bool ack = slave != 0;
   int i,j;

   for (i=0; i<repeat; i++)
   {
      write_tx_payload(buff, len, (const char*)ensemble::slave_addr[slave], ack);

      uint8_t status;
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
      {
         write_reg(STATUS, STATUS_TX_DS); //Clear the data sent notice
         if (ack) 
            break;
      }
      else
         tx_err++;
      
      if (i<repeat)
         delay_us(2500);
   }

   uint8_t obs_tx = read_reg(OBSERVE_TX);
   mvprintw(1, 27, "%3d  %3d       %02x    %3d   %3d", j, tx_err, obs_tx, ack_err, i);
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

   if (i==100)
      return;


   uint32_t t_rx;
   uint16_t slave_id, soc, vcell, missed_message_count;
   uint8_t msg_id, freshness_count;
   int8_t major_version, minor_version;

   msg::decode_status(buff, slave_id, t_rx, major_version, minor_version,
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


/*******************************************************************************
* from http://www.ruinelli.ch/rgb-to-hsv
* Function HSV2RGB
* Description: Converts an HSV color value into its equivalen in the RGB color space.
* Copyright 2010 by George Ruinelli
* The code I used as a source is from http://www.cs.rit.edu/~ncs/color/t_convert.html
* Notes:
*  r, g, b values are from 0..255
*  h = [0,255], s = [0,255], v = [0,255]
*  NB: if s == 0, then h = 0 (undefined)
******************************************************************************/
void hsv2rgb(hsv_t hsv, rgb_t &rgb)
{
   int i;
   float f, p, q, t, h, s, v;

   h=(float)hsv.h;
   s=(float)hsv.s;
   v=(float)hsv.v;

   s /= 255;

   if( s == 0 )
   { // achromatic (grey)
      rgb.r = rgb.g = rgb.b = v;
      return;
   }

   h *= 0.0234375; // same as / 42.66666666;
   i = floor( h );
   f = h - i;
   p = (unsigned char)(v * ( 1 - s ));
   q = (unsigned char)(v * ( 1 - s * f ));
   t = (unsigned char)(v * ( 1 - s * ( 1 - f ) ));
   
   switch( i )
   {
      case 0: rgb.r = v; rgb.g = t; rgb.b = p; break;
      case 1: rgb.r = q; rgb.g = v; rgb.b = p; break;
      case 2: rgb.r = p; rgb.g = v; rgb.b = t; break;
      case 3: rgb.r = p; rgb.g = q; rgb.b = v; break;
      case 4: rgb.r = t; rgb.g = p; rgb.b = v; break;
      case 5: rgb.r = v; rgb.g = p; rgb.b = q; break;
   }
   rgb.r << 4;
   rgb.g << 4;
   rgb.b << 4;
}


void set_rgb(rgb_t rgb)
{
   uint8_t buff[ensemble::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;

   msg::encode_set_tlc_ch(buff, 2, rgb.g);
   nrf_tx(buff, sizeof(buff), slave,1);
   msg::encode_set_tlc_ch(buff, 1, rgb.r);
   nrf_tx(buff, sizeof(buff), slave, 1);
   msg::encode_set_tlc_ch(buff, 0, rgb.b);
   nrf_tx(buff, sizeof(buff), slave, 1);

   msg::encode_set_tlc_ch(buff, 5, rgb.g);
   nrf_tx(buff, sizeof(buff), slave, 1);
   msg::encode_set_tlc_ch(buff, 4, rgb.r);
   nrf_tx(buff, sizeof(buff), slave, 1);
   msg::encode_set_tlc_ch(buff, 3, rgb.b);
   nrf_tx(buff, sizeof(buff), slave, 1);
}


// This seems to do a good job of driving the LEDs with a hue, saturation, value
// triplet. But it uses lots of floating point. blech.
// from http://en.wikipedia.org/wiki/HSL_and_HSV
void hsv_float(uint8_t h, uint8_t s, uint8_t v)
{
   double hue = 360.0 * h/255.0;
   double sat = s / 255.0;
   double val = v / 255.0;

   double c = sat * val;
   double hue_ = hue/60.0;
   double x = c * (1 - abs(fmod(hue_, 2) - 1));

   double r,g,b;
   if ( 0 <= hue_ && hue_ < 1) {
      r=c; g=x; b=0;
   }
   else if (1 <= hue_ && hue_ < 2) {
      r=x; g=c; b=0;
   }
   else if (2 <= hue_ && hue_ < 3) {
      r=0; g=c; b=x;
   }
   else if (3 <= hue_ && hue_ < 4) {
      r=0; g=x; b=c;
   }
   else if (4 <= hue_ && hue_ < 5) {
      r=x; g=0; b=c;
   }
   else if (5 <= hue && hue_ <=6) {
      r=c; g=0; b=x;
   }

   double m = val - c;
   r+=m;
   g+=m;
   b+=m;

   uint16_t red = r * 4095;
   uint16_t green = g * 4095;
   uint16_t blue = b * 4095;

   mvprintw(2, 12, "%04x %04x %04x", red, green, blue);
   mvprintw(2, 30, "h:%02x s:%02x v:%02x", h, s, v);
   mvprintw(3, 1, "c=%f hue_=%f, x=%f, ", c, hue_, x);

   uint8_t buff[ensemble::message_size];
   for (int i=0; i<sizeof(buff); i++) buff[i]=0;

   msg::encode_set_tlc_ch(buff, 2, green);
   nrf_tx(buff, sizeof(buff), slave, 1);
   msg::encode_set_tlc_ch(buff, 1, red);
   nrf_tx(buff, sizeof(buff), slave, 1);
   msg::encode_set_tlc_ch(buff, 0, blue);
   nrf_tx(buff, sizeof(buff), slave, 1);
}
