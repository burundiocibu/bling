#!/bin/bash
base=/home/littlej/dev/bling
log=/log
/usr/local/bin/node $base/js/server.js >>$log/server.js.log &
# 2 lib websockets warnings
# 8 base output of master_server
$base/master_server --port 9321 -d $((2+8)) >>$log/master_server.log
