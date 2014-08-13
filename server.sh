#!/bin/bash
base=/home/littlej/dev/bling
/usr/local/bin/node $base/js/server.js >$base/js/server.log &
$base/master_server --port 9321 -d 8
