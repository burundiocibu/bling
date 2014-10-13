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
events.push(new Event("One On",       3, 11000)); // B
events.push(new Event("Intro On",      4, 6000));  // 2 bars before C
events.push(new Event("Intro Sparkle", 5, 13000)); // C
events.push(new Event("Massive On",      4, 100*1000)); // Massive
events.push(new Event("Balad On",      6, 4*60000)); // Start of movement 2? 
events.push(new Event("All Off",       0, 1000));

ep=0;

function update_effect()
{
    document.getElementById("play_effect").textContent = ep;
    document.getElementById("effect_region").value = events[ep].name+
        " ("+events[ep].id+","+events[ep].duration/1000+"s)";
}

update_effect();

document.onkeydown = function kph(e)
{
    k = e.keyCode;
    switch (e.keyCode)
    {
        case 37: prev_effect(); break; // left arrow
        case 39: next_effect(); break; // right arrow
        case 32: start_effect(); break; // space key
        case 13: start_effect(); break; // enter key
        case 88: all_stop();    break; // x
    }

    document.getElementById("all_stop").textContent = k;
};

new FastButton(document.getElementById('play_effect'), function() { start_effect(); });
new FastButton(document.getElementById('prev_effect'), function() { prev_effect(); });
new FastButton(document.getElementById('next_effect'), function() { next_effect(); });
new FastButton(document.getElementById('all_stop'),    function() { all_stop(); });

function all_stop()
{
    var msg = new Send_All_Stop();
    msg.slave_id = 0;
    msg.repeat = 50;
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
    msg.repeat = 25;
    var hdr = new Header();
    hdr.msg_id = "START_EFFECT";
    send_msg(socket, hdr, msg);
    next_effect();
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
