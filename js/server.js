#!/usr/local/bin/node
//
// Set up: npm install
var http = require("http"),
    fs = require("fs"),
    path = require("path"),
    open = require("open"),
    exec = require('child_process').exec;

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
            fs.writeFileSync(path.join(__dirname, "www", dep[0]),
                             fs.readFileSync(path.join(__dirname, dep[1])));
        } catch (err) {
            console.log("Copying failed: "+err.message);
            console.log("\nDid you run `npm install` ?");
            process.exit(1);
        }
    }
}

// HTTP server
var server = http.createServer(function(req, res) {
    var file = null,
        type = "text/html";
    if (req.url == "/")
    {
        file = "index.html";
    }
    else if (req.url == "/ensemble_master")
    {
        // This is how the client is told where to make the websocket connection
        res.writeHead(200, {"Content-Type":"text/javascript"});
        msg="ensemble_master=\"ws://"+req.connection.localAddress+":9321/ws\""
        res.write(msg);
        res.end();
        return;
    }
    else if (req.url == "/slave_main_version")
    {
        fs.readFile(path.join(__dirname, "..", req.url), function(err, data) {
            if (err)
            {
                res.writeHead(500, {"Content-Type": type});
                res.end("Internal Server Error: "+err);
            }
            else
            {
                res.writeHead(200, {"Content-Type":"text/javascript"});
                res.write("slave_main_version="+data);
                res.end();
            }
        });
        return;
    }
    else if (/^\/(\w+(?:\.min)?\.(?:js|html|proto))$/.test(req.url))
    {
        file = req.url.substring(1);
        if (/\.js$/.test(file))
        {
            type = "text/javascript";
        }
    }
    else if (/^\/(\w+(?:\.min)?\.(?:ico|png))$/.test(req.url))
    {
        file = req.url.substring(1);
        if (/\.js$/.test(file))
        {
        type = "image/x-icon";
        }
    }
    else if ( req.url == "/favicon.ico")
    {
        file = "favicon.ico";
        type = "image/x-icon";
    }
    else
    {
        console.log("Client requested "+req.url)
    }

    if (file)
    {
        fs.readFile(path.join(__dirname, "www", file), function(err, data) {
            if (err)
            {
                res.writeHead(500, {"Content-Type": type});
                res.end("Internal Server Error: "+err);
            }
            else
            {
                res.writeHead(200, {"Content-Type": type});
                res.write(data);
                res.end();
                //console.log("Served www/"+file);
            }
        });
    }
    else
    {
        console.log("Requested file not found:"+file)
        res.writeHead(404, {"Content-Type": "text/html"});
        res.end("Not Found");
    }
});

server.listen(80);
server.on("listening", function() {
    console.log("Server started");
    //open("http://localhost:8080/");
});

server.on("error", function(err) {
    console.log("Failed to start server:", err);
    process.exit(1);
});
