#!/bin/bash

eval `get_param $1`

loader_hex=slave/nrf_boot.hex
show_hex=slave/slave_main.hex

if [ -z "$slave_addr" ]; then
    exit -1
fi

if [ ! -f "$loader_hex" ]; then
    printf "Could not find $loader_hex"
    exit -1
fi

if [ ! -f "$show_hex" ]; then
    printf "Could not find $show_hex"
    exit -1
fi

avrdude="sudo avrdude -p m328 -c usbtiny"
avrterm="$avrdude -t -qq"

echo "Initializing slave ${slave_no}, addr=${slave_addr}"
$avrdude -U lfuse:w:0xe2:m -U hfuse:w:0xd0:m -U efuse:w:0x07:m

echo "Writing boot loader to flash ..."
$avrdude -U flash:w:$loader_hex:i

echo "Writing slave number to eeprom ..."
echo "write eeprom 0" $slave_no "0" | $avrterm

echo "Writing channel number to eeprom ..."
echo "write eeprom 2" $channel_no | $avrterm

echo "Writing slave unique addrss to eeprom ..."
echo "write eeprom 4" $slave_addr | $avrterm

echo "Writing broadcast address to eeprom ..."
echo "write eeprom 8" $broadcast_addr | $avrterm

echo "Writing master's address to eeprom ..."
echo "write eeprom 12" $master_addr | $avrterm

echo "Dumping eeprom ..."
echo "dump eeprom 0 32" | $avrterm

echo "Dumping bootloader ..."
echo "dump flash 0x7000 32" | $avrterm

echo " "

sudo ./master_loader -s $1 -i $show_hex

echo "Dumping app ..."
echo "dump flash 0 32" | $avrterm

echo "Slave $1 done."
