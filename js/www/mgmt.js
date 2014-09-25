//<!-- -*- mode: javascript; -*- -->

var tlc=0;
document.getElementById("tlc").value = tlc;

new FastButton(document.getElementById('up'),       function() { set_tlc(1); });
new FastButton(document.getElementById('down'),     function() { set_tlc(-1); });
new FastButton(document.getElementById('effect0'),  function() { start_effect(1); });
new FastButton(document.getElementById('effect1'),  function() { start_effect(2); });
new FastButton(document.getElementById('all_stop'), function() { all_stop(); });

document.onkeydown = function kph(e)
{
    k = e.keyCode;
    switch (e.keyCode)
    {
        case 38: set_tlc(1); break; // up arrow
        case 40: set_tlc(-1); break; // down arrow
    }

    document.getElementById("all_stop").textContent = k;
};

function get_slave_list(active)
{
    var gsl = new Get_Slave_List();
    gsl.scan = "false";
    gsl.tries = 0;
    gsl.active = active;
    var hdr = new Header();
    hdr.msg_id = "GET_SLAVE_LIST";
    send_msg(socket, hdr, gsl);
}

function scan_slaves()
{
    var gsl = new Get_Slave_List();
    gsl.scan = "true";
    gsl.tries = 1;
    var hdr = new Header();
    hdr.msg_id = "GET_SLAVE_LIST";
    send_msg(socket, hdr, gsl);
    log("scan_slaves");
}

function all_stop()
{
    tlc=0;
    document.getElementById("tlc").value = tlc;
    var msg = new Send_All_Stop();
    msg.slave_id = 0;
    msg.repeat = 50;
    var hdr = new Header();
    hdr.msg_id = "SEND_ALL_STOP";
    send_msg(socket, hdr, msg);
    log("all_stop");
}

function start_effect(e)
{
    var msg = new Start_Effect();
    msg.slave_id = 0; // broadcast
    msg.effect_id = e;
    msg.start_time = 0;
    msg.duration = 10000; // ms
    msg.repeat = 10;
    var hdr = new Header();
    hdr.msg_id = "START_EFFECT";
    send_msg(socket, hdr, msg);
    log("start_effect("+e+")");
}

function set_tlc(v)
{
    if (v>0)
    {
        tlc = (tlc << 1) | 1;
        if (tlc >= 4096)
            tlc = 4096-1;
    }
    else
    {
        tlc = tlc >>> 1;
    }
    var msg = new Set_Slave_Tlc();
    msg.slave_id = 0;
    for (i=0;i<15; i++)
        msg.tlc[i] = tlc;
    msg.repeat = 2;
    var hdr = new Header();
    hdr.msg_id = "SET_SLAVE_TLC";
    send_msg(socket, hdr, msg);
    document.getElementById("tlc").value = tlc;
    //log("set_tlc("+tlc+")");
}

function shutdown_master()
{
    var hdr = new Header();
    hdr.msg_id = "SHUTDOWN_MASTER";
    send_msg(socket, hdr);
    log("shutdown_master");
}

function program_slaves(force)
{
    if (typeof slave_list === "undefined")
    {
        log("No slaves found. Try 'List Slaves'");
        return;
    }

    log("slave_main_version is "+slave_main_version);

    for (var i=0; i<slave_list.slave.length; i++)
    {
        var slave=slave_list.slave[i]
        if (slave.age > 30)
            continue;
        if (slave.version == slave_main_version && !force)
        {
            log("Slave "+slave.slave_id+" already running version "+slave_main_version);
            continue;
        }
        var msg = new Program_Slave();
        msg.slave_id = slave.slave_id;
        var hdr = new Header();
        hdr.msg_id = "PROGRAM_SLAVE";
        send_msg(socket, hdr, msg);
        //log("program_slave:"+msg.slave_id);
    }
    log("Slaves being programmed...");
}

function get_master_status()
{
    var hdr = new Header();
    hdr.msg_id = "GET_MASTER_STATUS";
    send_msg(socket, hdr);
    //log("get_master_status");
}
