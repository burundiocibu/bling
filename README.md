bling
=====

...adding a little more bling to the field...

This is the code for a set of networked LED controllers (slaves) run by a central host (master). 
Communications are handled using the nordic semi nRF24L01+ chip. Each slave also has a ATmega328
micro-controller, and a TLC5940 PWM LED driver. The slaves are intended to be powerd by 6-8 VDC
from a pair of Li-Ion cells in series. The slave includes a MAX17041 fuel gauge IC to monitor the
battery.

Some day I should put the board design on github too.

The nRF24L01+ is a addressable packet oriented radio with mutiple addresses on
which it can 'listen'. Each of thes is called a pipe. A common address is used
as a broadcast to all the slaves. Each slave also has a unique private address
that the master can use to communicate with just that one slave.

