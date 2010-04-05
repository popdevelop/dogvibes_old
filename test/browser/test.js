if (!('WebSocket' in window)) {
    alert('WebSocket not available in your browser. Use Google Chrome')
}

var Log = function(){
    this.error = function(msg) {
        document.getElementById("log").innerHTML += '<font color="red"><b>' + msg + '</b></font>' + '<br>';
    };
    this.reply = function(name, color, msg) {
        document.getElementById('log').innerHTML += '&nbsp; <font color="' + color + '"><i>' + msg + '</i></font><br>';
    };
    this.command = function(name, color, msg) {
        document.getElementById('log').innerHTML += '<font color="' + color + '"><b>' + msg + '</b></font><br>';
    };
    return this;
}
var log = new Log();

var testers = [];

colors_nbr = 0;
colors = []
colors[0] = '#ff0000'
colors[1] = '#0000ff'

function Tester(name, uri)
{
    this.name = name;
    this.color = colors[colors_nbr];

    this.msg_id = 0;

    this.ws = new WebSocket(uri);
    this.ws.parent = this;
//    function getAllPlaylists(json) {
//        //document.getElementById('log').innerHTML += json['result'][0]['name'] + '<br>';
//    }
    this.ws.onopen = function() {};
    this.ws.onclose = function() {};
    this.ws.onmessage = function (e) {
        log.reply(this.parent.name, this.parent.color, e.data)
        //eval(e.data);
    };

    this.sendWS = function(cmd) {
        if (cmd.indexOf('?') == -1) {
            cmd += "?msg_id=" + this.msg_id;
        } else {
            cmd += "&msg_id=" + this.msg_id;
        }
        log.command(this.name, this.color, cmd)
        this.ws.send(cmd);
        this.msg_id++;
    }
    this.dogvibes = function(cmd) {
        this.sendWS('/dogvibes/' + cmd);
    };
    this.amp = function(cmd) {
        this.sendWS('/amp/0/' + cmd);
    };

    testers.push(this);
    colors_nbr++;

    return this;
}

function start() {
    for (var i = 0; i < testers.length; ++i) {
        if (testers[i].ws.readyState != WebSocket.OPEN) {
            log.error('Could not open WebSocket');
            return;
        }
    }
    run();
}
function initTest() {
    setTimeout('start()', 100);
}

var t1 = new Tester('one', 'ws://localhost:9999/');
var t2 = new Tester('two', 'ws://localhost:9999/');

initTest();

function run() {
    t1.amp('getQueuePosition');
    t2.amp('getQueuePosition');
    t1.amp('setVolume?level=0.1')
    t1.amp('getQueuePosition');
    t2.amp('getQueuePosition');
    t1.amp('setVolume?level=0.1')
    t1.amp('getQueuePosition');
    t2.amp('getQueuePosition');
    t1.amp('setVolume?level=0.1');
}
