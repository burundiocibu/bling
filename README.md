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

Some day I should put the board design and the rpi hardware design on github too.

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
master (currently a raspberry pi). This requires the bcm2835 c
libraray to be installed along with the ncurses-dev package.

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

   * master_server -- Used to allow control of the show from a basic
     TCP/IP socket.
   * client_cmd -- Use to help ring out the TCP/IP protocol with the
     master_server program.
   * master_ws -- same as master_server but uses websockets (using the
     libwebsockets library) as opposed to raw bsd sockets.
   * http_server -- a simple webserver (also based upon libwebsockets)
     that will serve up a page to a client to allow the client to form
     a websocket to master_ws and run the show.
     

Network Control
--------------
It would be nice to be able to control the system from a remote
pad/computer. So turn the raspberry pi into a 'gateway' between the
nRF link to the slaves and a control client.

The general idea is to use google protocol.buffers to encode messages
that will be exchanged between a remote client and the master across a
websocket.

Then, to allow basically any device to control the system, have the
master server up a javascript program to the client as long as the
client can support websockets.


Setup of the master
----------------
debian packages required:
   gcc-avr, avr-libc, and binutils-avr
   gcc, g++, ncurses-dev
   libboost-asio, libboost-program-options
   libprotobuf-dev, protobuf-compiler
   python-protobuf (in case I write a python client)
   
Other libs required:
   git://git.libwebsockets.org/libwebsockets
   http://www.airspayce.com/mikem/bcm2835/



