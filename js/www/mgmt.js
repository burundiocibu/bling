//<!-- -*- mode: javascript; -*- -->

var tlc=0;
document.getElementById("tlc").value = tlc;

function get_slave_list()
{
    var gsl = new Get_Slave_List();
    gsl.scan = "false";
    gsl.tries = 0;
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
    write_log("Scanning Slaves\n");
}

function all_stop()
{
    tlc=0;
    document.getElementById("tlc").value = tlc;
    var msg = new Send_All_Stop();
    msg.slave_id = 0;
    msg.tries = 10;
    var hdr = new Header();
    hdr.msg_id = "SEND_ALL_STOP";
    send_msg(socket, hdr, msg);
    write_log("All Stop\n");
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
    write_log("Start effect "+e+"\n");
}

function set_tlc(v)
{
    if (v>0)
    {
        if (tlc<=0)
            tlc = 1;
        else
            tlc = tlc << 1;
        if (tlc >= 4096)
            tlc = 4096-1;
    }
    else
    {
        tlc = tlc >>> 1;
    }
    var msg = new Set_Slave_Tlc();
    msg.slave_id = 0;
    msg.tlc = tlc;
    msg.repeat = 10;
    var hdr = new Header();
    hdr.msg_id = "SET_SLAVE_TLC";
    send_msg(socket, hdr, msg);
    document.getElementById("tlc").value = tlc;
}

function shutdown_master()
{
    var hdr = new Header();
    hdr.msg_id = "SHUTDOWN_MASTER";
    send_msg(socket, hdr);
    write_log("Shutting down master\n");
}

function get_master_status()
{
    var hdr = new Header();
    hdr.msg_id = "GET_MASTER_STATUS";
    send_msg(socket, hdr);
    write_log("Getting master status (no rx written yet)\n");
}

function program_slave()
{
    var msg = new Program_Slave();
    msg.slave_id = 0;
    var hdr = new Header();
    hdr.msg_id = "PROGRAM_SLAVE";
    send_msg(socket, hdr, msg);
    write_log("Programming slave:"+slave_id+"\n");
}
