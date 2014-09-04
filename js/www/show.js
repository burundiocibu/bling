//<!-- -*- mode: javascript; -*- -->

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
}

function all_stop()
{
    var msg = new Send_All_Stop();
    msg.slave_id = 0;
    msg.tries = 10;
    var hdr = new Header();
    hdr.msg_id = "SEND_ALL_STOP";
    send_msg(socket, hdr, msg);
}

function start_effect()
{
    var eb = document.getElementById("play_effect");
    var msg = new Start_Effect();
    msg.slave_id = 0; // broadcast
    msg.effect_id = parseInt(eb.value);
    eb.value++;
    eb.textContent=eb.value
    msg.start_time = 0;
    msg.duration = 10000; // ms
    msg.repeat = 10;
    var hdr = new Header();
    hdr.msg_id = "START_EFFECT";
    send_msg(socket, hdr, msg);
}

function prev_effect()
{
    var eb = document.getElementById("play_effect");
    eb.value--;
    if (eb.value < 1)
    {
        eb.value = 1;
    }
    eb.textContent=eb.value;
}

function next_effect()
{
    var eb = document.getElementById("play_effect");
    eb.value++;
    eb.textContent=eb.value;
}
