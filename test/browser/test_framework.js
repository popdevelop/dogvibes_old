//if (!('WebSocket' in window)) {
//    alert('WebSocket not available in your browser. Use Google Chrome')
//}



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
    this.result = function(color, command, reply, startTime, endTime, firstTime) {
        document.getElementById("log").innerHTML += '<font color="' + color + '"><b>'+command+'</b><br>'+(startTime-firstTime) + 'ms' + ' (' + (endTime-startTime) +'ms): '+reply+'<i></i></font><br>';
    };
    return this;
}
var log = new Log();

var testers = [];

colors_nbr = 0;
colors = []
colors[0] = '#ff0000'
colors[1] = '#0000ff'
colors[2] = '#00dd00'
colors[3] = '#ffff00'

function pushHandler(d) {
    // dummy
}

function Tester(name, uri)
{
    this.name = name;
    this.color = colors[colors_nbr];

    this.msg_id = 0;

    this.ws = new WebSocket(uri);
    this.ws.parent = this;

    this.commands = []
    this.replies = []
    this.firstTime = 0;

//    function getAllPlaylists(json) {
//        //document.getElementById('log').innerHTML += json['result'][0]['name'] + '<br>';
//    }
    this.ws.onopen = function() {};
    this.ws.onclose = function() {};
    this.ws.onmessage = function (e) {
        var reply = eval('('+e.data+')');
        var t = new Date().getTime()
        this.parent.replies.push({c: e.data, t: t, i: reply.msg_id});
        //log.reply(this.parent.name, this.parent.color, e.data)
    };

    this.sendWS = function(cmd) {
        if (cmd.indexOf('?') == -1) {
            cmd += "?msg_id=" + this.msg_id;
        } else {
            cmd += "&msg_id=" + this.msg_id;
        }
        var t = new Date().getTime();
        if (this.firstTime == 0)
            this.firstTime = t;
        log.command(this.name, this.color, cmd)
        this.commands.push({c: cmd, t: t, i: this.msg_id});
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
    setTimeout('start()', 500);
}

function findReplyFromCommand(command) {
    for (var j = 0; j < testers.length; ++j) {
        var t = testers[j];
        for (var i = 0; i < t.replies.length; ++i) {
            if (command.i == t.replies[i].i) {
                return t.replies[i]
            }
        }
    }
    return null;
}
function showResults() {
    setTimeout('x_showResults()', 2000);
}
function x_showResults() {
    document.getElementById("log").innerHTML += '<h3>Results</h3>';
    for (var j = 0; j < testers.length; ++j) {
        var t = testers[j];
        for (var i = 0; i < t.commands.length; ++i) {
            var command = t.commands[i];
            reply = findReplyFromCommand(command);
            if (reply == null)
                alert("Did not recieve reply! (Write test for this)");
            log.result(t.color, command.c, reply.c, command.t, reply.t, t.firstTime);
        }
    }
}