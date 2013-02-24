CPPFLAGS+=-std=c++0x
ALL_BIN=spidev_test spin spi-time tx-test

all: $(ALL_BIN)

clean:
	-rm -f $(ALL_BIN) *.o

spidev_test: spidev_test.c


spin: LDLIBS+=-lbcm2835
spin: spin.c


spi-time: LDLIBS+=-lbcm2835
spi-time: spi-time.cpp

#tx-test: CPPFLAGS+=-std=c++0x
tx-test: LDLIBS+=-lbcm2835 -lstdc++
tx-test: tx-test.o rt_utils.o nrf24l01.o
