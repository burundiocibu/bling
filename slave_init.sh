#!/bin/bash

eval `get_param $1`

if [ -z $slave_addr ]; then
    exit
fi

avrdude="avrdude -F -p m328p -c usbtiny"
avrterm="$avrdude -t -q"

echo "Initializing slave ${slave_no}, addr=${slave_addr}"

echo "Writing boot loader to flash ..."
$avrdude -q -e -U flash:w:nrf_boot.hex:i

echo "Writing slave number to eeprom ..."
echo "write eeprom 0 0" $slave_no | $avrterm

echo "Writing channel number to eeprom ..."
echo "write eeprom 2" $channel_no | $avrterm

echo "Writing slave unique addrss to eeprom ..."
echo "write eeprom 4" $slave_addr | $avrterm

echo "Writing broadcast address to eeprom ..."
echo "write eeprom 8" $broadcast_addr | $avrterm

echo "Writing master's address to eeprom ..."
echo "write eeprom 12" $master_addr | $avrterm

echo "Dumping eeprom ..."
echo "dump eeprom 0 16" | $avrterm
