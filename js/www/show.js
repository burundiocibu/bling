// -*- mode: javascript; -*-

new FastButton(document.getElementById('play_effect'), function() { start_effect(); });
new FastButton(document.getElementById('prev_effect'), function() { prev_effect(); });
new FastButton(document.getElementById('next_effect'), function() { next_effect(); });
new FastButton(document.getElementById('all_stop'),    function() { all_stop(); });

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
