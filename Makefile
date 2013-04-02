CPPFLAGS+=-std=c++0x
ALL_BIN=spidev_test spi-time tx-test rx-test

all: $(ALL_BIN)

clean:
	-rm -f $(ALL_BIN) *.o

spidev_test: spidev_test.c


spi-time: LDLIBS+=-lbcm2835
spi-time: spi-time.cpp

tx-test rx-test: LDLIBS+=-lbcm2835 -lstdc++
tx-test: tx-test.o rt_utils.o nrf24l01.o

rx-test: rx-test.o rt_utils.o nrf24l01.o