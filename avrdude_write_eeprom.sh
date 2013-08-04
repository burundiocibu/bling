#!/bin/bash

# 8 bit channel number
echo "write eeprom 0x0c 2" | avrdude -F -c usbtiny -p m328p -t
# master's address
echo "write eeprom 0x08 0xA1 0xA3 0xA5 0xA6" | avrdude -F -c usbtiny -p m328p -t
# broadcast address (address 0)
echo "write eeprom 0x04 0x3F 0x5B 0x74 0x38" | avrdude -F -c usbtiny -p m328p -t


# 16 bit slave number
echo "write eeprom 0x0d 0 2" | avrdude -F -c usbtiny -p m328p -t

# slave's address goes to address 0
# slave 2 address
echo "write eeprom 0x00 0xe4 0x18 0xdd 0x0a" | avrdude -F -c usbtiny -p m328p -t

# slave 1 address 
#echo "write eeprom 0x00 0xcd 0x80 0x27 0x40" | avrdude -F -c usbtiny -p m328p -t

echo "dump eeprom 0 16" | avrdude -F -c usbtiny -p m328p -t
