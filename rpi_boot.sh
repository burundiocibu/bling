#!/bin/bash

d=/home/littlej/bling
logfile=$d/master_lcd.log

bin=master_lcd

echo Start: `date` >> $logfile
$d/$bin >>$logfile &
