#!/bin/bash
base=/home/littlej/dev/bling
log=/log
/usr/local/bin/node $base/js/server.js >>$log/server.js.log &
$base/master_server --port 9321 -d 8 >>$log/master_server.log
