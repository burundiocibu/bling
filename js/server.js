#!/usr/bin/env node
//
// Set up: npm install
var http = require("http"),
    fs = require("fs"),
    path = require("path"),
    ws = require("ws"),
    open = require("open"),
    ProtoBuf = require("protobufjs"),
    hexy = require('hexy');

// Copy dependencies to "www/" (example specific, you usually don't have to care
var deps = [
    ["Long.min.js", "./node_modules/protobufjs/node_modules/bytebuffer/node_modules/long/dist/Long.min.js"],
    ["ByteBufferAB.min.js", "./node_modules/protobufjs/node_modules/bytebuffer/dist/ByteBufferAB.min.js"],
    ["ProtoBuf.min.js", "./node_modules/protobufjs/dist/ProtoBuf.min.js"],
    ["hexy.js", "./node_modules/hexy/hexy.js"],
];
for (var i=0, dep, data; i<deps.length; i++)
{
    dep = deps[i];
    if (!fs.existsSync(path.join(__dirname, "www", dep[0])))
    {
        console.log("Copying "+dep[0]+" from "+dep[1]);
        try
        {
            fs.writeFileSync(path.join(__dirname, "www", dep[0]), fs.readFileSync(path.join(__dirname, dep[1])));
        } catch (err) {
            console.log("Copying failed: "+err.message);
            console.log("\nDid you run `npm install` ?");
            process.exit(1);
        }
    }
}

// Initialize from .proto file
var builder = ProtoBuf.loadProtoFile(path.join(__dirname, "www", "server_msg.proto"));
var Header = builder.build("bling_pb.header");
var Msg_Ids = builder.build("bling_pb.header.msg_id_type");
var Get_Slave_List = builder.build("bling_pb.get_slave_list");
var Slave_List = builder.build("bling_pb.slave_list");
var Slave_Info = builder.build("bling_pb.slave_list.slave_info");

function appendBuffer( buffer1, buffer2 ) {
  var tmp = new Uint8Array( buffer1.byteLength + buffer2.byteLength );
  tmp.set( new Uint8Array( buffer1 ), 0 );
  tmp.set( new Uint8Array( buffer2 ), buffer1.byteLength );
  return tmp.buffer;
}

function ab2str(buf) {
    return String.fromCharCode.apply(null, new Uint8Array(buf));
}

// HTTP server
var server = http.createServer(function(req, res) {
    var file = null,
        type = "text/html";
    if (req.url == "/") {
        file = "index.html";
    } else if (/^\/(\w+(?:\.min)?\.(?:js|html|proto))$/.test(req.url)) {
        file = req.url.substring(1);
        if (/\.js$/.test(file)) {
            type = "text/javascript";
        }
    } else if ( req.url == "/favicon.ico") {
        file = "favicon.ico";
        type = "image/x-icon";
    } else {
        console.log("Client requested "+req.url)
    }

    if (file) {
        fs.readFile(path.join(__dirname, "www", file), function(err, data) {
            if (err) {
                res.writeHead(500, {"Content-Type": type});
                res.end("Internal Server Error: "+err);
            } else {
                res.writeHead(200, {"Content-Type": type});
                res.write(data);
                res.end();
                console.log("Served www/"+file);
            }
        });
    } else {
        console.log("Requested file not found:"+file)
        res.writeHead(404, {"Content-Type": "text/html"});
        res.end("Not Found");
    }
});

server.listen(8080);
server.on("listening", function() {
    console.log("Server started");
    open("http://localhost:8080/");
});
server.on("error", function(err) {
    console.log("Failed to start server:", err);
    process.exit(1);
});
    
// WebSocket adapter
var wss = new ws.Server({server: server});
wss.on("connection", function(socket) {
    console.log("New WebSocket connection");
    socket.on("close", function() {
        console.log("WebSocket disconnected");
    });
    socket.on("message", function(data, flags) {
        if (flags.binary)
        {
            try
            {
                // Decode the Message
                console.log("Received " + data.length + " bytes");
                console.log(hexy.hexy(data));
                var hdr = Header.decode(data.slice(0,7));
                console.log("msg_id:"+hdr.msg_id);
                if (hdr.msg_id == Msg_Ids.GET_SLAVE_LIST)
                {
                    var gsl = Get_Slave_List.decode(data.slice(7));
                    console.log("gsl.scan:"+gsl.scan);
                    var slave_list = new Slave_List();
                    slaves = [];
                    for (var i=0; i<5; i++)
                    {
                        var si = new Slave_Info();
                        si.slave_id = i;
                        slaves.push(si);
                    }
                    slave_list.set_slave(slaves);
                    var sl_buff = slave_list.toArrayBuffer();
                    hdr.msg_id = "SLAVE_LIST";
                    hdr.len = sl_buff.byteLength;
                    var hdr_buff = hdr.toArrayBuffer();
                    var msg_buff = appendBuffer(hdr_buff, sl_buff);
                    socket.send(msg_buff);
                    console.log("Sent " + msg_buff.byteLength + " bytes\n");
                    console.log(hexy.hexy(ab2str(msg_buff)) + "\n");

                }
                // Transform the text to upper case
                //msg.text = msg.text.toUpperCase();
                // Re-encode it and send it back
                //socket.send(msg.toBuffer());
                //console.log("Sent: "+msg.text);
            } catch (err) {
                console.log("Processing failed:", err);
            }
        } else {
            console.log("Not binary data");
        }
    });
});
