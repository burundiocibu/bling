bling
=====

...adding a little more bling to the field...

This is the code for a set of networked LED controllers run by a central host.

A single master node broadcasts commands to a set of slave LED controllers.

Each slave has a AVR micro-controller with an nRF24L01+
2.5GHz radio and at least one TLC5940 PWM LED driver.

The nRF24L01+ is a addressable packet oriented radio with mutiple addresses on
which it can 'listen'. This project uses two of these channels
