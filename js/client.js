#!/usr/bin/env node
// -*- mode: javascript; -*-
// A command line client for the master_ws
var http = require("http"),
    fs = require("fs"),
    path = require("path"),
    ws = require("ws"),
    open = require("open"),
    ProtoBuf = require("protobufjs"),
    nopt = require("nopt"),
    hexy = require('hexy');

// Initialize ProtoBuf.js
var builder = ProtoBuf.loadProtoFile(path.join(__dirname, "www", "server_msg.proto"));
var Header = builder.build("bling_pb.header");
var Msg_Ids = builder.build("bling_pb.header.msg_id_type");
var Ack = builder.build("bling_pb.slave_list.ack");
var Nak = builder.build("bling_pb.slave_list.nak");
var Send_All_Stop = builder.build("bling_pb.send_all_stop");
var Get_Slave_List = builder.build("bling_pb.get_slave_list");
var Slave_List = builder.build("bling_pb.slave_list");
var Slave_Info = builder.build("bling_pb.slave_list.slave_info");
var Set_Slave_Tlc = builder.build("bling_pb.set_slave_tlc");
var Start_Effect = builder.build("bling_pb.start_effect");
var Ping_Slave = builder.build("bling_pb.ping_slave");
var Reboot_Slave = builder.build("bling_pb.reboot_slave");
var Program_Slave = builder.build("bling_pb.program_slave");
// get_master_status and shutdown_master don't have unique messages


// Connect to the websocket server
var socket = new ws("ws://threepi:9321/ws");
socket.binaryType = "arraybuffer"; // We are talking binary


function appendBuffer( buffer1, buffer2 )
{
  var tmp = new Uint8Array( buffer1.byteLength + buffer2.byteLength );
  tmp.set( new Uint8Array( buffer1 ), 0 );
  tmp.set( new Uint8Array( buffer2 ), buffer1.byteLength );
  return tmp.buffer;
}

function ab2str(buf)
{
    return String.fromCharCode.apply(null, new Uint8Array(buf));
}

function send_msg(header, body)
{
    if (socket.readyState == ws.OPEN) 
    {
        var b2 = body.toArrayBuffer();
        header.len = b2.byteLength;
        var b1 = header.toArrayBuffer();
        var b3 = appendBuffer(b1, b2);
        socket.send(b3);
        //console.log("Sent " + b3.byteLength + " bytes");
        //console.log( hexy.hexy(ab2str(b3)) );
    }
    else
    {
        console.log("Not connected");
    }
}

function get_slave_list()
{
    var gsl = new Get_Slave_List();
    gsl.scan = "false";
    gsl.tries = 0;
    var hdr = new Header();
    hdr.msg_id = "GET_SLAVE_LIST";
    send_msg(hdr, gsl);
}

function scan_slaves()
{
    var gsl = new Get_Slave_List();
    gsl.scan = "true";
    gsl.tries = 1;
    var hdr = new Header();
    hdr.msg_id = "GET_SLAVE_LIST";
    send_msg(hdr, gsl);
}

function all_stop()
{
    var msg = new Send_All_Stop();
    msg.slave_id = 0;
    msg.tries = 10;
    var hdr = new Header();
    hdr.msg_id = "SEND_ALL_STOP";
    send_msg(hdr, msg);
}

function start_effect()
{
    var msg = new Start_Effect();
    msg.slave_id = 0; // broadcast
    msg.effect_id = parseInt(effect.value);
    //effect.value++;
    msg.start_time = 0;
    msg.duration = 10000; // ms
    msg.repeat = 10;
    var hdr = new Header();
    hdr.msg_id = "START_EFFECT";
    send_msg(hdr, msg);
}

socket.onerror = function(error)
{    
}

socket.onopen = function() 
{
    console.log("Connected");
    get_slave_list();
};

socket.onclose = function() 
{
    console.log("Disconnected");
};

socket.onmessage = function(evt) 
{
    try
    {
        console.log("Received "+evt.data.byteLength+" bytes");
        var hdr = Header.decode(evt.data.slice(0,7));
        console.log("Received msg_id:" + hdr.msg_id);
        if (hdr.msg_id == Msg_Ids.SLAVE_LIST)
        {
            var slave_list = Slave_List.decode(evt.data.slice(7));
            console.log("slaves found: ");
            for (var i=0; i<slave_list.slave.length; i++)
            {
                slave=slave_list.slave[i]
                console.log(slave.slave_id+"("+slave.soc.toFixed(2)+"%) ");
            }
        }
    }
    catch (err)
    {
        console.log("Error: "+err);
    }
    process.exit();
};
