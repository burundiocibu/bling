// <!-- -*- mode: javascript; -*- -->

if (typeof dcodeIO === 'undefined' || !dcodeIO.ProtoBuf)
{
    throw(new Error("ProtoBuf.js is not present. Please see www/index.html for manual setup instructions."));
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
        var b2 = body.toArrayBuffer();
        header.len = b2.byteLength;
        var b1 = header.toArrayBuffer();
        var b3 = appendBuffer(b1, b2);
        socket.send(b3);
        //log.value += "Sent " + b3.byteLength + " bytes\n";
        //log.value += hexy(ab2str(msg_buff)) + "\n";
    }
    else
    {
        log.value += "Not connected\n";
    }
}

function get_message(socket, body)
{
    log.value += "Received "+evt.data.byteLength+" bytes\n";
    var hdr = Header.decode(evt.data.slice(0,7));
    log.value += "Received msg_id:" + hdr.msg_id + "\n";
    if (hdr.msg_id == Msg_Ids.SLAVE_LIST)
    {
        var slave_list = Slave_List.decode(evt.data.slice(7));
        log.value += "slaves found: ";
        for (var i=0; i<slave_list.slave.length; i++)
        {
            slave=slave_list.slave[i]
            log.value += slave.slave_id+"("+slave.soc.toFixed(2)+"%) ";
        }
        log.value += "\n";
    }
}
