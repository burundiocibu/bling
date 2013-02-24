
#include <bcm2835.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include <ctime>

using namespace std;

double now()
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (double)tv.tv_sec + (double)tv.tv_usec/1e6;
}


int main(int argc, char **argv)
{
   
   if (!bcm2835_init())
      return 1;

   double t0, dt;
   t0 = now();
   bcm2835_spi_begin();
   bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
   bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
   bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);
   bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
   bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
   dt = now() - t0;
   cout << "Setup took " << setprecision(3) << 1e6*dt << " us." << endl;
   
   
   // Send a some bytes to the slave and simultaneously read 
   // some bytes back from the slave
   // Most SPI devices expect one or 2 bytes of command, after which they will send back
   // some data. In such a case you will have the command bytes first in the buffer,
   // followed by as many 0 bytes as you expect returned data bytes. After the transfer, you 
   // Can the read the reply bytes from the buffer.
   // If you tie MISO to MOSI, you should read back what was sent.
   char buf[] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0x40, 0x00, 0x00, 0x00, 0x00, 0x95,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xAD,
      0xF0, 0x0D,
   };
   
   t0 = now();
   bcm2835_spi_transfern(buf, sizeof(buf));
   dt = now() - t0;
   cout << "Xfer took " << setprecision(3) << 1e6*dt << " us." << endl;

   // buf will now be filled with the data that was read from the slave
   for (int ret = 0; ret < sizeof(buf); ret++)
   {
      if (!(ret % 6))
         puts("");
      printf("%.2X ", buf[ret]);
   }
   puts("");
   
   bcm2835_spi_end();
   bcm2835_close();
   return 0;
}

