
/*************************************
 * Dogvibes server API
 * Requires jQuery and jsonp plugin 
 *************************************/


/* AJAX and websocket connection objects. A connection object is expected
 * to have the following members:
 * - status            : JS object with current server status
 * - start(server)     : function to start the connection to server
 * - stop              :
 * - send(URL, Success): function to send a command to the server
 *
 * Connection object is also expected to trigger the following events:
 * - "Server.connected": upon successful connection
 * - "Server.status"   : when server has new status
 * - "Server.error"    : when server error occurred (disconnected)
 */

/* Polling AJAX connection type */
var AJAX = {
  server: "",
  status: Array(),
  connected: false,
  interval: 1000,
  timer: false,
  
  start: function(server) {
    AJAX.server = server;
    AJAX.timer = setTimeout(AJAX.getStatus, 0);
  },
  stop: function() {  
    AJAX.connected = false;
    clearTimeout(AJAX.timer);
    $(document).trigger("Server.error");    
  },
  send: function(URL, Success) {
    /* Changing state? */
    if(!AJAX.status.connected) {
      $(document).trigger("Server.connecting");    
    }
    $.jsonp({
      url: AJAX.server + URL,
      error: AJAX.stop,
      success: Success,
      callbackParameter: "callback",
      timeout: 2000
    });
  },
  /* Private functions */
  getStatus: function() {
    /* TODO: avoid forward reference */
    clearTimeout(AJAX.timeout);
    AJAX.send(Dogvibes.defAmp + Dogvibes.cmd.status, AJAX.handleStatus);
  },
  handleStatus: function(data) {
    /* Changing state? */
    if(!AJAX.connected) {
      AJAX.connected = true;
      $(document).trigger("Server.connected");
    }
    /* Reload timer if not disconnected */
    if(data.result.state != "disconnected") {
      AJAX.timer = setTimeout(AJAX.getStatus, AJAX.interval);
    }
    AJAX.status = data;
    $(document).trigger("Server.status");
  }
};


/* TODO: Websockets connection type */
var WSocket = {
  status: Array(),
  ws: false,
  start: function(server) {
    if("WebSocket" in window) {
      ws = new WebSocket(server);
      ws.onopen = function() { $(document).trigger("Server.connected"); };
      ws.onmessage = function(e){ eval(e.data); };
      ws.onclose = function(){ $(document).trigger("Server.error"); }
    }
  },
  stop: function() {
  },
  send: function(URL, Success, Error) {
    if (URL.indexOf('?') == -1) {
      WSocket.ws.send(URL + "?callback=" + Success);
    } else {
      WSocket.ws.send(URL + "&callback=" + Success);
    }  
  },
  /* Private functions */
  handleStatus: function(json) {
  }
};
/* Workaround for server hard coded callback */
var pushHandler = WSocket.handleStatus;


/********************************
 * Dogvibes server API functions
 ********************************/
window.Dogvibes =  {
  server: false,
  status: false,
  defAmp: "/amp/0" , /* TODO: dynamic */
  cmd: {
    status: "/getStatus",
    prev:   "/previousTrack",
    play:   "/play",
    playTrack: "/playTrack?nbr=",
    pause:  "/pause",
    next:   "/nextTrack",
    seek:   "/seek?mseconds=",
    volume: "/setVolume?level=",
    albumArt: "/dogvibes/getAlbumArt?size=320&uri=",
    playlists: "/dogvibes/getAllPlaylists",
    playlist: "/dogvibes/getAllTracksInPlaylist?playlist_id=",
    createPlaylist: "/dogvibes/createPlaylist?name=",
    playqueue: "/getAllTracksInQueue",
    search: "/dogvibes/search?query="
  },
  /*****************
   * Initialization
   *****************/
  init: function(server) {
    $(document).bind("Server.status", Dogvibes.handleStatus);
    Dogvibes.server = AJAX;
    Dogvibes.server.start(server);
  },
  /* Handle new status event from connection object and dispatch events */
  handleStatus: function() {
    var data = Dogvibes.server.status;
    if(data.error != 0) {
      /* TODO: Notify */
    }
    var oldStatus = Dogvibes.status;
    Dogvibes.status = data.result;
    /* TODO: solve better. Fill in artist info when state is stopped.
     * since this info is not sent from server */
    if(data.result.state == "stopped") {
      data.result.artist = "";
      data.result.album  = "";
      data.result.title  = "";
    }

    /* Walk through interesting status fields and dispatch events if changed */
    if(Dogvibes.status.state != oldStatus.state) {
      $(document).trigger("Status.state");
    }

    if(Dogvibes.status.volume != oldStatus.volume) {
      $(document).trigger("Status.volume");
    }

    if(Dogvibes.status.artist != oldStatus.artist ||
       Dogvibes.status.title  != oldStatus.title  ||
       Dogvibes.status.album  != oldStatus.album  ||
       Dogvibes.status.albumArt != oldStatus.albumArt) {
      $(document).trigger("Status.songinfo");
    }

    if(Dogvibes.status.elapsedmseconds != oldStatus.elapsedmseconds ||
       Dogvibes.status.duration != oldStatus.duration) {
      $(document).trigger("Status.time");
    }

    if(Dogvibes.status.playlist_id != oldStatus.playlist_id ||
       Dogvibes.status.playqueuehash != oldStatus.playqueuehash) {
      $(document).trigger("Status.playlist");
    }  
    
    /* TODO: add more */
  },
  /****************
   * API functions 
   ****************/
  search: function(keyword, Success) {
    var URL = Dogvibes.cmd.search + keyword;
    Dogvibes.server.send(URL, Success);
  },
  getAllPlaylists: function(Success) {
    Dogvibes.server.send(Dogvibes.cmd.playlists, Success);
  },
  getAllTracksInPlaylist:function(id, Success) {
    Dogvibes.server.send(Dogvibes.cmd.playlist + id, Success);  
  },
  playTrack: function(id, pid) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.playTrack + id + "&playlist_id=" + pid;
    Dogvibes.server.send(URL, function() {});
  },
  createPlaylist: function(name, Success) {
    var URL = Dogvibes.cmd.createPlaylist + name;
    Dogvibes.server.send(URL, Success);  
  }
};
