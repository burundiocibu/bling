// -*- mode: javascript; -*-

function Event(name, id, duration)
{
    this.name = name;
    this.id = id;  // see Effect.(c|h)pp
    this.duration = duration;  // ms
}
bpm=170; // beats per minute
mpb = 1000 * 60 / bpm; // ms per beat
events = [];
events.push(new Event("set13",   8, 4500));
events.push(new Event("flash",   1, 4500));
events.push(new Event("sparkle", 4, 30000));
events.push(new Event("M3 A1",   9, 5 * mpb));
events.push(new Event("M3 A2",   9, 5 * mpb));

ep=0;

function update_effect()
{
    document.getElementById("play_effect").textContent = ep;
    document.getElementById("effect_region").value = events[ep].name+
        " ("+events[ep].id+","+events[ep].duration+")";
}

update_effect();

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
    var msg = new Start_Effect();
    msg.slave_id = 0; // broadcast
    msg.effect_id = events[ep].id;
    msg.start_time = 0;
    msg.duration = events[ep].duration; // ms
    msg.repeat = 10;
    var hdr = new Header();
    hdr.msg_id = "START_EFFECT";
    send_msg(socket, hdr, msg);

    ep++;
    update_effect();
}

function prev_effect()
{
    ep--;
    if (ep < 0)
    {
        ep = 0;
    }
    update_effect();
}

function next_effect()
{
    ep++;
    if (ep >= events.length)
    {
        ep = 0;
    }
    update_effect();
}
