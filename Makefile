# Local Variables:
# mode: makefile
# End:

SLAVE_BIN=slave/slave_main.hex slave/nrf_boot.hex
MASTER_BIN=master_curses master_logger master_scan master_loader master_server\
 show get_param flash_list print_slave_version

all: $(MASTER_BIN) $(SLAVE_BIN)

clean:
	-rm -f $(MASTER_BIN) $(SLAVE_BIN) *.o *.pb.cc *.pb.h slave/*

include Makefile.rpi
include Makefile.avr

