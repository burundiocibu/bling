bling
=====

...adding a little more bling to a marching band...

This is the code for a set of networked LED controllers (slaves) run by a central host (master).
Communications are handled using the nordic semi nRF24L01+ chip. Each slave also has a ATmega328
micro-controller, and a TLC5940 PWM LED driver. The slaves are intended to be powerd by 6-8 VDC
from a pair of Li-Ion cells in series. The slave includes a MAX17041 fuel gauge IC to monitor the
battery.

The master that I use is a raspberry pi with a some additional hardware.
   * An LCD shield with 5 buttons (http://www.adafruit.com/products/1109)
   * An nRF24L01+ tranceiver (https://www.sparkfun.com/products/705)

Some day I should put the board design on github too.

The nRF24L01+ is a addressable packet oriented radio with mutiple addresses on
which it can 'listen'. Each of thes is called a pipe. A common address is used
as a broadcast to all the slaves. Each slave also has a unique private address
that the master can use to communicate with just that one slave.

Building the software
------------------

Makefile.avr is used to make the image that is loaded onto the
slaves. This uses the gcc-avr, avr-libc, and binutils-avr packages for cross
compiling the code.

$ make slave_main.hex
to build the application code for the AVR

$ make nrf_boot.hex to build the nrf-enabled bootloader.

Makefile.rpi is used to make the executables that are run on the
master (currently a raspberry pi). This requires the bcm2835
libraray to be installed (http://www.airspayce.com/mikem/bcm2835/)
along with the ncurses-dev package

Master executables:
-----------------

   * make_addr.py -- reads from /dev/random to make up address for the
         slaves.
   * make_sets.py -- writes the tables used by the sequencing code
   * master_lcd -- a simple UI that runs on a LCD shield connected to
     the rpi ()
   * master_loader -- used to reprogram the slaves via the nrf radio
   * master_probe -- used to scan for what boards are responding...
   * show -- an executable that uses the LCD shield to step through
     a performance
