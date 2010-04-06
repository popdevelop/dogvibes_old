/* Config */
var default_server = "http://dogvibes.com:2000";
var server = false;
var poll_interval = 2000; /* ms */
var connection_timeout = 5000; /* ms */
var time_interval = 2000; /* ms */
/* Misc variables */
var wait_req = 0;
var track_list_table = "<table cellspacing=\"0\" cellpadding=\"0\"><thead><tr><th id=\"indicator\">&nbsp;<th id=\"track\"><a href=\"#\">Track</a><th id=\"artist\"><a href=\"#\">Artist</a><th id=\"time\"><a href=\"#\">Time</a><th id=\"album\"><a href=\"#\">Album</a></thead><tbody id=\"s-results\"></tbody></table>";
var search_summary = "<div id=\"s-artists\" class=\"grid_5\"></div><div id=\"s-albums\" class=\"grid_4\"></div><div class=\"clear\">&nbsp;</div><div id=\"s-tracks\" class=\"grid_9\"></div><div class=\"clear\">&nbsp;</div>";
var loading_small = "<img src=\"img/loading_small.gif\">";
var welcome_text = "<h1>Welcome to dogbone!</h1> <p>The first HTML-client to <a href=\"http://www.dogvibes.com\" target=\"_new\">Dogvibes</a>.</p>";
var poll_handle = false;
var current_song = {
    album: false,
    index: false ,
    playqueuehash: false,
    artist: false,
    title: false,
    uri: false,
    state: false,
    duration: false,
    elapsedmseconds: false
};
var current_page = false;
var current_search_results = false;
var time_count;
var request_in_progress = false;
var seek_in_progress = false; /* FIXME: */
var vol_in_progress = false; /* FIXME: */
var use_websocket = 0;
var ws;
var synced = 0;

/* These are all the commands available to the server */
var command = {
        /* playqueue */
        add: "/amp/0/queue?uri=",
        get: "/amp/0/getAllTracksInQueue",
        remove: "/amp/0/removeTrack?nbr=",
        /* Playlists */
        getplaylists: "/dogvibes/getAllPlaylists",
        getplaylisttracks: "/dogvibes/getAllTracksInPlaylist?playlist_id=",
        playlistadd: "/dogvibes/createPlaylist?name=",
        playlistremove: "/dogvibes/removePlaylist?id=",
        addtoplaylist: "/dogvibes/addTrackToPlaylist?playlist_id=",
        removefromplaylist: "/dogvibes/removeTrackFromPlaylist?playlist_id=",
        /* playback control */
        next: "/amp/0/nextTrack",
        play: "/amp/0/play",
        playtrack: "/amp/0/playTrack?nbr=",
        pause: "/amp/0/pause",
        prev: "/amp/0/previousTrack",
        seek: "/amp/0/seek?mseconds=",
        volume: "/amp/0/setVolume?level=", 
        /* other */

        status: "/amp/0/getStatus",
        getmseconds: "/amp/0/getPlayedMilliSeconds",
        search: "/dogvibes/search?query=",
        albumart: "/dogvibes/getAlbumArt?size=159&uri=",
        list: "/dogvibes/list?type="
};

/*
 * Misc. Handy functions
 */

function setWait(){
    if(wait_req == 0){
        $("#wait_req").html(loading_small);
    }
    wait_req++;
};
function clearWait(){
    wait_req--;
    if(wait_req == 0){
        $("#wait_req").empty();
    }
};

/* Pad time with zeros */
function checkTime(i){
    if (i<10 && i>=0){
        i="0" + i;
    }
    return i;
}

var successGetMSeconds = function(data){
    if(data.error != 0){
        connectionBad("Server error! (" + data.error + ")");
    }

    // this is different from the normal case
    current_song.elapsedmseconds = data.result;

    new_time = Math.round(data.result / 1000 -0.5);
    percent = (current_song.elapsedmseconds/current_song.duration)*100;
    $("#playback_time").html(timestamp_to_string(new_time));
    if(!seek_in_progress) {
        $('#playback_seek').slider('option', 'value', percent);
    }
};

/* Increase playback time counter */
function increaseCount(){
    // websocket is fast enough to get this from the server
    if (use_websocket)
        sendCmd(command.getmseconds, "successGetMSeconds");
    else
        current_song.elapsedmseconds += 1000;

    updateTimes();
}
/* playbacktime, seekbar */
function updateTimes(){
    new_time = Math.round(current_song.elapsedmseconds / 1000 -0.5);
    percent = (current_song.elapsedmseconds/current_song.duration) *100;
    $("#playback_time").html(timestamp_to_string(new_time));
    if(!seek_in_progress) {
        $('#playback_seek').slider('option', 'value', percent);
    }
    $("#playback_total").html(timestamp_to_string(Math.round(current_song.duration/1000 - 0.5)));
    if(current_song.elapsedmseconds >= current_song.duration){
        clearInterval(time_count);
    }
}

/* Convert seconds to format M:SS */
function timestamp_to_string(ts)
{
    if(!ts) { ts=0; }
    if(ts==0) { return "0:00"; }
    m = Math.round(ts/60 - 0.5);
    s = Math.round(ts - m*60);
    s=checkTime(s);
    return m + ":" + s;
}

function sendCmd(url, successFunc) {
    if (use_websocket) {
        if (url.indexOf('?') == -1) {
            ws.send(url + "?callback=" + successFunc);
        } else {
            ws.send(url + "&callback=" + successFunc);
        } 
    } else {
        $.ajax({
            url: server + url,
            type: "GET",
            dataType: 'jsonp',
            success: eval(successFunc)
        });
    }
}

var pushHandler = function(data){
    handleStatusResponse(data.result);
}

var successGetStatus = function(data){
    if(data.error != 0){
        connectionBad("Server error! (" + data.error + ")");
    }

    synced = 1;
    connectionOK(); /* This will restore timeout aswell */    
    handleStatusResponse(data.result);
};

/* 
 * Status request loop and its handler 
 */
function requestStatus()
{
//    if (use_websocket && synced) {
//        // we don't poll status in websocket mode
//        return;
//    }

    if(!request_in_progress){
        connectionRequest();
        sendCmd(command.status, "successGetStatus");
    }
}

function isTheSonglist(page, data)
{
    var type = page.substring(0,3);
    var pl_id = playlists.selected == "playqueue" ? -1 : playlists.selected;
    return type == 'pl-' && data.playlist_id == pl_id;
}

function handleStatusResponse(data)
{
    /* Check if song has switched */
    if(current_song.index != data.index){
        if(isTheSonglist(current_page, data)){
            $("#row_" + current_song.index + " td:first a").removeClass("playing_icon");      
            $("#row_" + current_song.index + " td:first a").addClass("remButton"); 
            $("#row_" + current_song.index + " td").removeClass("playing");     
        }
        $("#album_art").html("<img src=\"" + server + command.albumart + data.uri + "\">");
    }
    /* Update playqueue if applicable */

    if(data.playqueuehash != current_song.playqueuehash && isTheSonglist(current_page, data)){
        getPlayQueue();
    }
    if(playlists.activeList != data.playlist_id) {
        playlists.setActive(data.playlist_id);
    }
    current_song = data;   
    /* Update volume */

    if(!vol_in_progress && data.volume){
        $('#playback_volume').slider('option', 'value', data.volume*100);
    }
    /* Update play status */
    if(data.state == "playing"){
        $("#pb-play > a").addClass("playing");
        increaseCount();
        clearInterval(time_count);
        time_count = setInterval(increaseCount, time_interval);
    }
    else{
        $("#pb-play > a").removeClass("playing");
        clearInterval(time_count);
    }
    /* Update now playing field */
    if(data.state != "stopped"){
        $("#now_playing .artist").text(data.artist);
        $("#now_playing .title").text(data.title);
        if(isTheSonglist(current_page, data)){
            $("#row_" + data.index + " td:first a").addClass("playing_icon"); 
            $("#row_" + data.index + " td:first a").removeClass("remButton");       
            $("#row_" + data.index + " td").addClass("playing");       
        }
        updateTimes();
    }
    else {
        $("#now_playing .title").html("Nothing playing right now");
        $("#now_playing .artist").empty();
        $("#album_art").empty();
        $("#playback_time").empty();
        $("#playback_total").empty();
    }
}

/* 
 * Connection states
 */
var connection_timer;
function connectionInit()
{
    $("#message .btn").hide();
    $("#message .text").addClass("loading");
    clearTimeout(connection_timer);      
    connectionBad("Connecting to server '" + server + "'...");
    //poll_handle = setInterval(requestStatus,poll_interval);
    requestStatus(); 
}

function connectionRequest()
{
    request_in_progress = true;
    connection_timer = setTimeout(connectionTimeout, connection_timeout);
}

function connectionOK()
{
    request_in_progress = false;        
    $("#shade").hide();
    $("#message").hide();
    clearTimeout(connection_timer);
}

function connectionBad(msg)
{
    $("#shade").show();
    $("#message").show(); 
    $("#message .text").html(msg);
    /* Try reconnect */
    request_in_progress = false;
}

/* This is the "end" state of the connection */
function connectionTimeout()
{
    clearInterval(poll_handle); 
    $("#message .btn").show();
    $("#shade").show();
    $("#message").show(); 
    $("#message .text").removeClass("loading");
    $("#message .text").html("Connection timed out! Check server and reload page to try again.");
}


var successRemove = function(data) {
    clearWait();
    /* TODO: change back this when we have playlisthash */
    getPlayQueue();
};

var successPlayButton = function(data) {
    clearWait();
    requestStatus();
};

var successGetCommand = function(data) {
    item_count = 0;
    $("#s-results").empty();
    $.each(data.result, function(i, song) {
        td = (i % 2 == 0) ? "<td class=\"odd\">" : "<td>";
        id = i;
        $("#s-results").append("<tr id=\"row_"+ i + "\" class=\"pl_row\">" + td + "<a href=\"#\" id=\"" + id + "\" class=\"remButton\" title=\"Remove from queue\">-</a>" + td + "<a href=\"#\" id=\"" + id + "\" name=\""+song.uri+"\" class=\"playButton\">" + song.title + "</a>" + td + song.artist + td + timestamp_to_string(song.duration/1000) + td + song.album);
        item_count++;
    });
    $(".remButton").click(function () {
        setWait();
        sendCmd(removecommand + this.id, "successRemove");
    });    
    $(".playButton").dblclick(function () {
        var pl_id = playlists.selected == "playqueue" ? -1 : playlists.selected;
        var data = this.id + "&playlist_id=" + pl_id;

        setWait();
        sendCmd(addcommand + data, "successPlayButton");
    });
    if(item_count == 0){
        $("#s-results").html("<tr><td colspan=6><i>No tracks in this list</i>");
    }
    else{
        $(".pl_row").click(function(){
            $(".pl_row").find("td").removeClass("selected");
            $(this).find("td").addClass("selected");
        });         
        /* TODO: Make things sortable. Just dummy for now */
        $(function() {
            $("#s-results").sortable({appendTo: "body", scroll: false });
            $("#s-results").disableSelection();
        });
    }
    clearWait();
    requestStatus();
};

var getcommand, addcommand, removecommand;
/* Get playqueue or playlist */
function getPlayQueue(){
    setWait();
    if(playlists.selected == "playqueue"){
        getcommand = command.get;
        addcommand = command.playtrack;
        removecommand = command.remove;
    } else {
        getcommand = command.getplaylisttracks + playlists.selected;
        addcommand = command.playtrack;
        removecommand = command.removefromplaylist + playlists.selected + "&track_id=";
    }
    sendCmd(getcommand, "successGetCommand");
}

var successGetPlaylists = function(data) {
    playlists.items = data.result;
    playlists.draw();
};

/* Playlists */
var playlists = {
    activeList: 0,
    listIds: new Array(),
    items: new Array(),
    selected: null,
    ui: {
        list: "#playlists-items",
        section: "#playlists"
    },
    setActive: function(id) {
        playlists.activeList = id;
        for(var i in playlists.listIds) {
            $("#pl-"+playlists.listIds[i]).removeClass("active");
        }
        $("#pl-"+id).addClass("active");
    },
    get: function() {
        sendCmd(command.getplaylists, "successGetPlaylists");
    },
    add: function() {
        newlist = prompt("Enter new playlist name:", "");
        if(newlist && newlist!=""){
            sendCmd(command.playlistadd + newlist, "playlists.get");
        }
    },
    remove: function(id) {
        if(confirm("Are you sure you want to delete this playlist?")) {
            sendCmd(command.playlistremove + id, "playlists.get");
        }
    },
    draw: function() {
        playlists.listIds = new Array();
        $(playlists.ui.list).empty();
        if(playlists.items.length > 0) {
            $(playlists.ui.section).show();
            $.each(playlists.items, function(i, list){
                playlists.listIds.push(list.id);
                $(playlists.ui.list).append("<li id=\"pl-"+list.id+"\"><a href=\"#playlist/"+list.id+"\" class=\"playlistClick\" name=\""+list.id+"\">"+list.name+"</a> <span onclick=\"playlists.remove("+list.id+")\">x</span>");
                $("#pl-"+list.id).droppable({
                    hoverClass: 'drophover',
                    drop: function(event, ui) {
                    id = $(this).find("a").attr("name");
                    uri = ui.draggable.attr("id");
                    sendCmd(command.addtoplaylist + id + "&uri=" + uri, '');
                    $(this).effect("highlight");
                }
                });            
            });
            $(".playlistClick").click(clickHandler);
            $(".playlistClick").dblclick(function() {
                sendCmd(command.playtrack + "0"+"&playlist_id="+ this.name);
            });
          playlists.setActive(playlists.activeList);  
        } else {
            $(playlists.ui.section).hide();
        }
    }
};

var search = {
items: new Array(),
ui: {
    list: "#searches-items",
    section: "#searches",
    cookie:"dogvibes.searches",
    len: 6
},
init: function() {
    /* Load searches from cookie */
    for(var i = 0; i < search.ui.len; i++){
        if((temp = getCookie(search.ui.cookie + i)) != "") {
            search.items[i] = temp;
        }
    }
    search.draw();
},
add: function(keyword) {
    var tempArray = new Array();
    tempArray.unshift(jQuery.trim(keyword));
    $.each(search.items, function(i, entry){
        if(jQuery.trim(keyword) != entry){
            tempArray.push(entry);
        }
    });
    if(tempArray.length > search.ui.len){
        tempArray.pop();
    }
    search.items = tempArray;
    for(var i = 0; i < tempArray.length; i++) {
        setCookie(search.ui.cookie + i, tempArray[i]);
    }
    search.draw();
},
draw: function() {
    $(search.ui.list).empty();
    if(search.items.length > 0)
    {
        $(search.ui.section).show();
        $.each(search.items, function(i, entry) { 
            $(search.ui.list).append("<li id=\"s-"+i+"\"><a href=\"#search/"+entry+"\" class=\"searchClick\">"+entry+"</a>");
        });
        $(".searchClick").click(clickHandler);
    } else {
        $(search.ui.section).hide();
    }
}
};

var nav = {
expectedHash: "",
init: function() {
    setInterval(nav.checkHash, 500);
},
getHash: function() {
    var hash = window.location.hash;
    return hash.substring(1); // remove #
},

getLinkTarget: function(link) {
    return link.href.substring(link.href.indexOf('#')+1);
},

parseHash: function(hash) {
    /* FIXME: cmd-extraction fails i IE8 */
    ppos = hash.indexOf("/");
    if(ppos >= 0) {
        cmd = hash.substring(0, ppos);
        prm = hash.substring(ppos+1);
    } else {
        cmd = hash;
        prm = "";
    }
    return { command: cmd, param: unescape(prm) };
},

checkHash: function() {
    theHash = nav.getHash();
    if(theHash != nav.expectedHash)
    {
        nav.expectedHash = theHash;
        action = nav.parseHash(theHash);
        nav.executeAction(action);
    }
},

executeAction: function(action) {
    if(action != null && action.command != "") {
        switch (action.command) {
        case "search":
            doSearchFromLink(action.param, true);
            break;
        case "playqueue":
            action.param = "playqueue";
            title = "Playqueue";
            /* Fall-through */
        case "playlist":
            playlists.selected = action.param;
            setPage("pl-" + action.param);
            if(typeof(title) == "undefined") { title = "Playlist"; }
            $("#tab-title").text(title);
            $("#playlist").html(track_list_table);
            getPlayQueue();
            break;
        case "home":
            setPage("p-home");
            $("#tab-title").text("Home");
            $("#playlist").html(welcome_text);
            break;
        case "local":
            break;
        default:
            alert("Unknown command '"+action.command+"'");
        break;
        }
    }
}
};

function clickHandler() {
    var hash = nav.getLinkTarget(this);
    nav.expectedHash = hash;
    var action = nav.parseHash(hash);
    nav.executeAction(action);
}

function doSearchFromLink(value, save) {
    $("#s-input").val(value);
    if(typeof(save) == "undefined") { save = false; }
    doSearch(save);
}

var successPlay = function(data) {
    clearWait();
    requestStatus();
};

var successSearch = function(data) {
    var artists = {};
    var albums  = {};
    current_search_results = {};
    item_count = 0;
    artist_count = 0;
    album_count = 0;
    $("#s-results").empty();
    $.each(data.result, function(i, song) {
        td = (i % 2 == 0) ? "<td class=\"odd\">" : "<td>";
        $("#s-results").append("<tr class=\"pl_row\">" + td + "<a href=\"#\" id=\"" + song.uri + "\" class=\"addButton\" title=\"Add to play queue\">+</a>" + td + "<a href=\"#\" id=\"" + song.uri + "\" class=\"playButton\">" + song.title + td + "<a href=\"#\" id=\"" + song.uri + "\" class=\"searchArtistButton\">" + song.artist + td + timestamp_to_string(song.duration/1000) + td + "<a href=\"#\" id=\"" + song.uri + "\" class=\"searchAlbumButton\">" + song.album);
        current_search_results[song.uri] = { artist:song.artist, album:song.album };
        item_count++;
        if(!artists[song.artist]) { artists[song.artist]=0; artist_count++; }
        artists[song.artist]++;
        if(!albums[song.album]) { albums[song.album]=song.artist; album_count++; }
    });
    /* Add click actions */
    $(".addButton").click(function () {
        $("#s-results .selected").effect('highlight');
        $("#pl-playqueue").effect('highlight');
        setWait();
        sendCmd(command.add + this.id, "clearWait");
    }); 
    $(".playButton").dblclick(function () {
        $("#s-results .selected").effect('highlight');
        $("#pl-playqueue").effect('highlight');
        setWait();
        sendCmd(command.add + this.id, "successPlay");
    });   
    $(".searchArtistButton").click(function () {
        doSearchFromLink("artist:"+current_search_results[this.id].artist);
    });
    $(".searchAlbumButton").click(function () {
        doSearchFromLink("artist:"+current_search_results[this.id].artist + " album:"+current_search_results[this.id].album);
    });            
    if(item_count == 0){
        $("#s-results").html("<tr><td colspan=6><i>No results for '" + $("#s-input").val() + "'</i>");
    } else {
        $(".pl_row").click(function(){
            $(".pl_row").find("td").removeClass("selected");
            $(this).find("td").addClass("selected");
        });   
        /* TODO: Make things sortable. Just dummy for now */
        $(function() {
            $(".playButton").draggable({ revert: 'invalid', scroll: false, revertDuration: 100, helper: 'clone', appendTo: "body" });
        });
    }
    /* Print summaries */
    $("#s-tracks").html("<span>Tracks</span> <span class=\"count\">(" + item_count + ")</span>");
    $("#s-artists").html("<span>Artists: </span><span class=\"count\">(" + artist_count + ")</span> ");
    count = 0;         
    jQuery.each(artists, function(i, artist){
        $("#s-artists").append(i + " <em>&diams;</em> ");
        if(count++ == 18){
            $("#s-artists").append("<span class=\"warn\"> and "+(artist_count-18)+" more... </span>");
            return false;
        }            
    });
    $("#s-albums").html("<span>Albums: </span><span class=\"count\">(" + album_count + ") ");
    count = 0;
    jQuery.each(albums, function(i, album){
        $("#s-albums").append(i + " <em>by "+album+" &diams;</em> ");
        if(count++ == 8){
            $("#s-albums").append("<span class=\"warn\"> and "+(album_count-8)+" more...  </span>");
            return false;
        }
    });                 
};


function doSearch(save) {
    if(save) {
        search.add($("#s-input").val());
    }
    setPage("s-0");
    $("#playlist").html(search_summary + track_list_table);
    $("#s-results").html("<tr><td colspan=6>" + loading_small + " <i>Searching...</i>");
    $("#s-keyword").text($("#s-input").val());
    $("#tab-title").text("Search");

    sendCmd(command.search + $("#s-input").val(), "successSearch");
}


/*
 * Register events
 */

/* Startup */
$("document").ready(function() {

    $("#message .btn").hide();
    search.init();
    nav.init();
    connectionBad("No server configured.");
    setPage("p-home");
    $("#playback_seek").slider();
    $("#playback_volume").slider();
    /* Do we have a server? otherwise prompt */
    if((temp = getCookie("dogvibes.server")) != ""){
        default_server = temp;
    }
    if(!server){
        server = prompt("Enter Dogvibes server URL:", default_server);
    }

    if (server.substring(0, 2) == 'ws') {
        // We use Flash WebSocket for those who can't handle the native one
        // I.e. skip this check for now
        if (1 || "WebSocket" in window) {
            if (server) {
                ws = new WebSocket(server);
            } else {
                ws = new WebSocket("ws://localhost:9999/");
            }
            ws.onopen = function(){
                use_websocket = 1;
                time_interval /= 8; // fetch timeupdates faster
                init();
            };
            ws.onmessage = function(e){
                eval(e.data);
            };
            ws.onclose = function(){
                connectionBad("Server (Websocket) stopped responding");
                use_websocket = 0;
                init();
            };
        }
    } else {
        init();
    }
});

function init() {
    if(server){
        setCookie("dogvibes.server", server, 365);
        connectionInit();
        playlists.get(); /* TODO: move this when we have playlisthash */     
        return;
    }
    connectionBad("No server configured. Press reload to set");
}

/* Playback control */
/* Play button */
$("#pb-play").click(function() {
    var action = $("#pb-play > a").hasClass("playing") ? command.pause : command.play;
    sendCmd(action, "requestStatus");
});
/* Next button */
$("#pb-next").click(function() {
    sendCmd(command.next, "requestStatus");
});

/* Prev button */
$("#pb-prev").click(function() {
    sendCmd(command.prev, "requestStatus");
});

$("#message .btn").click(function() {
    connectionInit();
});

/* Sections */
function setPage(name){
    $("#"+current_page).removeClass("selected");
    current_page = name;
    $("#"+current_page).addClass("selected");   
}
$("#link_home").click(clickHandler);

var successLocal = function(data) {
    obj = $("#s-results");
    obj.append("<thead><th id=\"indicator\">&nbsp;<th><a href=\"#\">Artist</a><th><a href=\"#\">Album</a></thead>");
    var prev_artist = false;
    $.each(data.result, function(i, song){
        td = (i % 2 == 0) ? "<td class=\"odd\">" : "<td>";
        current_artist = "";
        if(song.artist != prev_artist){
            current_artist = song.artist;
            prev_artist    = song.artist;
        }               
        obj.append("<tr>"+ td + td + current_artist + td + song.album);
    });
};

$("#p-local").click(function () {
    setPage("p-local");
    $("#tab-title").text("Local media");
    $("#playlist").html("<h1>Local music:</h1><table id=\"s-results\"></table>");

    sendCmd(command.list + "album", "successLocal");
});

/* Play queue management */

$("#link_playqueue").click(clickHandler);
$("#pl-playqueue").droppable({
    hoverClass: 'drophover',
    drop: function(event, ui) {
    id = $(this).find("a").attr("name");
    uri = ui.draggable.attr("id");
    sendCmd(command.add + uri, '');
    $(this).effect("highlight");
}
});


$("#new_playlist").click(playlists.add);

/* Searching */
$("#s-submit").click(function(){ doSearch(true); });
$("#s-input").keypress(function (e) {
    if (e.which == 13)
        doSearch(true);
});

/* Clear search keyword field if nav links are clicked*/
$(".navlink").bind("click", function () {
    $("#s-keyword").empty();
});

$('#playback_volume').slider({
    start: function(e, ui) { vol_in_progress = true; },
    stop: function(e, ui) { vol_in_progress = false; },
    change: function(event, ui) { 
        sendCmd(command.volume + ui.value/100, "requestStatus");
    },
    slide: function(event, ui) {
        if (use_websocket) {
            // update other clients in realtime
            sendCmd(command.volume + ui.value/100, '');
        }
    }
});

$('#playback_seek').slider({
    start: function(e, ui) { seek_in_progress = true; },
    stop: function(e, ui) { seek_in_progress = false; },
    change: function(event, ui) {
        sendCmd(command.seek + Math.round((ui.value*current_song.duration)/100), "requestStatus");
    }
});
