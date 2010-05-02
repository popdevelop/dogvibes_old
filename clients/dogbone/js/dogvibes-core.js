
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
  interval: 500,
  request: false,
  timer: false,
  
  start: function(server) {
    AJAX.server = server;
    AJAX.timer = setTimeout(AJAX.getStatus, 0);
  },
  stop: function() {  
    AJAX.connected = false;
    clearTimeout(AJAX.timer);
    AJAX.status = Dogvibes.defaultStatus;
    $(document).trigger("Server.error");
    $(document).trigger("Server.status");
    AJAX.request.abort();
  },
  send: function(URL, Success) {
    /* Changing state? */
    if(!AJAX.status.connected) {
      $(document).trigger("Server.connecting");    
    }
    AJAX.request = $.jsonp({
      url: AJAX.server + URL,
      error: AJAX.stop,
      success: eval(Success),
      callbackParameter: "callback",
      timeout: 5000
    });
  },
  /* Private functions */
  getStatus: function() {
    /* TODO: avoid forward reference */
    clearTimeout(AJAX.timeout);
    AJAX.send(Dogvibes.defAmp + Dogvibes.cmd.status, "AJAX.handleStatus");
  },
  handleStatus: function(data) {
    /* Changing state? */
    if(!AJAX.connected) {
      AJAX.connected = true;
      $(document).trigger("Server.connected");
    }
    
    AJAX.timer = setTimeout(AJAX.getStatus, AJAX.interval);
    
    AJAX.status = data;
    $(document).trigger("Server.status");
  }
};


/* TODO: Websockets connection type */
var WSocket = {
  status: {},
  ws: false,
  connected: false,
  start: function(server) {
    if("WebSocket" in window) {
      WSocket.ws = new WebSocket(server);
      WSocket.ws.onopen = function() { 
        WSocket.connected = true;
        $(document).trigger("Server.connected"); 
      };
      WSocket.ws.onmessage = function(e){ eval(e.data); };
      WSocket.ws.onclose = WSocket.stop;
      WSocket.ws.onerror = WSocket.stop;
    }
  },
  stop: function() {
    WSocket.connected = false;
    WSocket.status = Dogvibes.defaultStatus;
    $(document).trigger("Server.error");
    $(document).trigger("Server.status");    
  },
  send: function(URL, Success, Error) {
    Success = typeof(Success) == "undefined" ? "WSocket.getStatus" : Success;
    try {
      if (URL.indexOf('?') == -1) {
        WSocket.ws.send(URL + "?callback="+Success);
      } else {
        WSocket.ws.send(URL + "&callback="+Success);
      }
    }
    catch (e){
      
    }
  },
  getStatus: function() {
    WSocket.send(Dogvibes.defAmp + Dogvibes.cmd.status, "WSocket.handleStatus");
  },
  /* Private functions */
  handleStatus: function(json) {
    WSocket.status = json;
    $(document).trigger("Server.status");
    
  }
};
/* Workaround for server hard coded callback */
var pushHandler = WSocket.handleStatus;


/********************************
 * Dogvibes server API functions
 ********************************/
window.Dogvibes =  {
  server: false,
  serverURL: false,
  status: false,
  defAmp: "/amp/0" , /* TODO: dynamic */
  cmd: {
    status: "/getStatus",
    prev:   "/previousTrack",
    play:   "/play",
    playTrack: "/playTrack?nbr=",
    queue:  "/queue?uri=",
    pause:  "/pause",
    next:   "/nextTrack",
    seek:   "/seek?mseconds=",
    volume: "/setVolume?level=",
    albumArt: "/dogvibes/getAlbumArt?size=180&uri=",
    moveInPlaylist: "/dogvibes/moveTrackInPlaylist?playlist_id=",
    playlists: "/dogvibes/getAllPlaylists",
    playlist: "/dogvibes/getAllTracksInPlaylist?playlist_id=",
    addtoplaylist: "/dogvibes/addTrackToPlaylist?playlist_id=",    
    createPlaylist: "/dogvibes/createPlaylist?name=",
    playqueue: "/getAllTracksInQueue",
    search: "/dogvibes/search?query=",
    setVolume: "/setVolume?level=",
    seek: "/seek?mseconds="
  },
  /*****************
   * Initialization
   *****************/
  init: function(server, user) {
    if(typeof(user) != "undefined" && user.length > 0) {
      server = server + "/" + user;
    }  
    $(document).bind("Server.status", Dogvibes.handleStatus);
    Dogvibes.server = server.substring(0, 2) == 'ws' ? WSocket : AJAX;
    Dogvibes.server.start(server);
    Dogvibes.serverURL = server;
  },
  /* Handle new status event from connection object and dispatch events */
  handleStatus: function() {
    var data = Dogvibes.server.status;
    if(data.error != 0) {
      /* TODO: Notify */
      return;
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

    if(Dogvibes.status.playlist_id != oldStatus.playlist_id) {
      $(document).trigger("Status.playlist");
    }  
    
    if(Dogvibes.status.playqueuehash != oldStatus.playqueuehash) {
      $(document).trigger("Status.playqueue");
    }

    if(Dogvibes.status.elapsedmseconds != oldStatus.elapsedmseconds) {
      $(document).trigger("Status.elapsed");
    }
     
    
    /* TODO: add more */
  },
  /****************
   * API functions 
   ****************/
  search: function(keyword, Success) {
    var URL = Dogvibes.cmd.search + escape(keyword);
    Dogvibes.server.send(URL, Success);
  },
  getAllPlaylists: function(Success) {
    Dogvibes.server.send(Dogvibes.cmd.playlists, Success);
  },
  getAllTracksInPlaylist:function(id, Success) {
    Dogvibes.server.send(Dogvibes.cmd.playlist + id, Success);
  },
  addToPlaylist: function(id, uri, Success) {
    var URL = Dogvibes.cmd.addtoplaylist + id + "&uri=" + uri;
    Dogvibes.server.send(URL, Success);
  },
  playTrack: function(id, pid, Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.playTrack + id + "&playlist_id=" + pid;
    Dogvibes.server.send(URL, Success);
  },
  getAllTracksInQueue: function(Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.playqueue;
    Dogvibes.server.send(URL, Success);  
  },
  queue: function(uri, Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.queue + uri;
    Dogvibes.server.send(URL, Success);     
  },
  play: function(Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.play;
    Dogvibes.server.send(URL, Success)
  },
  prev: function(Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.prev;
    Dogvibes.server.send(URL, Success)
  },
  next: function(Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.next;
    Dogvibes.server.send(URL, Success)
  },    
  pause: function(Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.pause;
    Dogvibes.server.send(URL, Success)
  },  
  createPlaylist: function(name, Success) {
    var URL = Dogvibes.cmd.createPlaylist + name;
    Dogvibes.server.send(URL, Success);  
  },
  setVolume: function(vol, Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.setVolume + vol;
    Dogvibes.server.send(URL, Success);
  },
  seek: function(time, Success) {
    var URL = Dogvibes.defAmp + Dogvibes.cmd.seek + time;
    Dogvibes.server.send(URL, Success);
  },
  move: function(pid, tid, pos, Success) {
    var URL = Dogvibes.cmd.moveInPlaylist + pid + "&track_id=" + tid + "&position=" + pos;
    Dogvibes.server.send(URL, Success);
  },
  /* Returns an URL to the album art */
  albumArt: function(uri) {
    return Dogvibes.serverURL + Dogvibes.cmd.albumArt + uri;
  }
};

window.Dogvibes.defaultStatus = {
  result: {
    artist: "",
    title: "",
    album: "",
    uri: "",
    state: "stopped",
    playlist_id: ""
  },
  error: "0"
};