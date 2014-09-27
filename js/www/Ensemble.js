// <!-- -*- mode: javascript; -*- -->

if (typeof dcodeIO === 'undefined' || !dcodeIO.ProtoBuf)
{
    throw(new Error("ProtoBuf.js is not present. Please check server."));
}

// Initialize ProtoBuf.js
var ProtoBuf = dcodeIO.ProtoBuf;
var builder = ProtoBuf.loadProtoFile("./server_msg.proto");
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
var Master_Status = builder.build("bling_pb.master_status");


// Connect to the websocket server
var socket = new WebSocket(ensemble_master);
//var socket = new WebSocket("ws://fourpi:9321/ws");
socket.binaryType = "arraybuffer"; // We are talking binary

function log(msg)
{
    if (document.getElementById("log") !== null)
    {
        var p = document.getElementById('log');
        p.innerHTML = p.innerHTML + msg  + "\n";
    }
}


function set_status(s)
{
    if (document.getElementById("status_region") !== null)
    {
        document.getElementById("status_region").value = s;
    }
}

set_status("Disconnected");


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

function send_msg(socket, header, body)
{
    if (socket.readyState == WebSocket.OPEN) 
    {
        if (typeof body === 'undefined')
        {
            header.len = 0;
            var b1 = header.toArrayBuffer();
            socket.send(b1);
        }
        else
        {
            var b2 = body.toArrayBuffer();
            header.len = b2.byteLength;
            var b1 = header.toArrayBuffer();
            var b3 = appendBuffer(b1, b2);
            socket.send(b3);
        }
    }
    else
    {
        log("Send failed: not connected");
    }
}

socket.onerror = function(error)
{
    log("Failed connecting to " + ensemble_master);
}

socket.onopen = function() 
{
    log("Connected to " + ensemble_master);
    set_status("Connected");
};

socket.onclose = function() 
{
    log("Disconnected");
    set_status("Disconnected");
};

socket.onmessage = function(evt) 
{
    try
    {
        var hdr = Header.decode(evt.data.slice(0,7));
        if (hdr.msg_id == Msg_Ids.SLAVE_LIST)
        {
            slave_list = Slave_List.decode(evt.data.slice(7));
            log(slave_list.slave.length+" Slaves:");
            for (var i=0; i<slave_list.slave.length; i++)
            {
                var slave=slave_list.slave[i]
                log(slave.slave_id+", soc="+slave.soc.toFixed(1)+                    
                    "%, vcell="+slave.vcell.toFixed(3)+
                    "V, v="+slave.version);
            }
        }
        else if (hdr.msg_id == Msg_Ids.MASTER_STATUS)
        {
            var ms = Master_Status.decode(evt.data.slice(7));
            log("Master temperature: "+ms.temperature);
        }
    }
    catch (err)
    {
        log("Error: "+err);
    }
};
