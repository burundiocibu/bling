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
var Get_Slave_List = builder.build("bling_pb.get_slave_list");
var Slave_List = builder.build("bling_pb.slave_list");
var Slave_Info = builder.build("bling_pb.slave_list.slave_info");
var Master_Status = builder.build("bling_pb.master_status");


// Connect to the websocket server
var socket = new WebSocket(ensemble_master);
socket.binaryType = "arraybuffer"; // We are talking binary

setInterval(function() {get_slave_list()}, 10000)

function log(msg)
{
    if (document.getElementById("log") !== null)
    {
        var p = document.getElementById('log');
        p.innerHTML = p.innerHTML + msg  + "\n";
    }
}

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
        log("Socket not open...");
    }
}

socket.onerror = function(error)
{
    log("Failed connecting to " + ensemble_master);
}

socket.onopen = function()
{
    log("Connected to " + ensemble_master);
    get_slave_list();
};

socket.onclose = function() 
{
    log("Disconnected");
    set_status("Disconnected");
};

function timeStamp()
{
    var now = new Date();
    var date = [ now.getMonth() + 1, now.getDate(), now.getFullYear() ];
    var time = [ now.getHours(), now.getMinutes(), now.getSeconds() ];
    var suffix = ( time[0] < 12 ) ? "AM" : "PM";
    time[0] = ( time[0] < 12 ) ? time[0] : time[0] - 12;
    time[0] = time[0] || 12;
 
    for ( var i = 1; i < 3; i++ ) {
        if ( time[i] < 10 ) {
            time[i] = "0" + time[i];
        }
    }
    return date.join("/") + " " + time.join(":") + " " + suffix;
}

socket.onmessage = function(evt) 
{
    try
    {
        var hdr = Header.decode(evt.data.slice(0,7));
        if (hdr.msg_id == Msg_Ids.SLAVE_LIST)
        {
            slave_list = Slave_List.decode(evt.data.slice(7));
            for (var i=0; i<slave_list.slave.length; i++)
            {
                var slave=slave_list.slave[i];
                row = Math.floor(i/4)
                col = i % 4
                if (row <= 9)
                {
                    doc_id="r"+row+"c"+col
                    element = document.getElementById(doc_id)
                    element.value =
                        slave.slave_id+" "+slave.version+"\n"+
                        slave.vcell.toFixed(2)+"v";
                    if (slave.vcell < 7.4)
                        element.style.backgroundColor="#ffb9b9"
                    else
                        element.style.backgroundColor="LightGreen"
                }
            }
            document.getElementById("refresh_time").value = timeStamp();
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


function get_slave_list(active)
{
    var gsl = new Get_Slave_List();
    gsl.scan = "true";
    gsl.tries = 10;
    gsl.active = active;
    var hdr = new Header();
    hdr.msg_id = "GET_SLAVE_LIST";
    send_msg(socket, hdr, gsl);
}

function get_master_status()
{
    var hdr = new Header();
    hdr.msg_id = "GET_MASTER_STATUS";
    send_msg(socket, hdr);
}
