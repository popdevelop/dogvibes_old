/* Some configuration options */
var Config =  {
	defServer: "http://dogvibes.com",
	defAmp: "/amp/0/",
	defAlbumArtURL: "img/michael.jpg",
	pollIntervall: 2000 /* ms */
}

/* Shortcuts to save some typing */
var d = document;
d.cE  = document.createElement;
d.cT  = document.createTextNode;
d.gID = document.getElementById;

/* UI container objects */
var UI = {
	info: null,
	track: null,
	bottom: null,
	albArt: null,
	/* Initialize all objects */
	init: function() {
		UI.info    = d.gID("info");
		UI.track   = d.gID("track");
		UI.bottom  = d.gID("bottom");
		UI.albArt  = d.gID("container");
	},
	/* Misc handy function for creating elements */
	newElement: function (tag, content) {
   var el = d.cE(tag);
   var c;
   if(typeof(content) == "object") {
      c = content;
   }else {
      c = d.cT(content+"");
   }
   el.appendChild(c);
   return el;
	},
	setText: function(obj, text) {
		if(typeof(obj.childNodes[0]) == "undefined") {
			obj.appendChild(d.cT(text));
		}
		else {
			obj.childNodes[0].value = text+"";
		}
	}
}

/* The status fields this application is interested in. Also default values */
var defStatus = {
	volume: 0,
	index: 0,
	state: "",
	track: "Not connected",
	artist: "",
	album: "",
	albumArt: "",
	elapsedTime: 0
}

/* Status:
 * 
 * Object that fetches server data and dispatches events. This object
 * always contains the latest status data in Status.data */
var Status = {
	timer: null,
	data: defStatus,
	init: function() {
		/* Listen and respond to server connection events */
		$(document).bind("Server.connected", Status.run);
		$(document).bind("Server.error", Status.stop);	
		
		/* Also respond to immediate status request */
		$(document).bind("Status.invalid", Status.get);
		
		/* Trigger update of info when started */
		$(document).trigger("Status.songinfo");
	},
	run: function() {
		Status.timer = setInterval(Config.pollInterval, Status.get);
	},
	stop: function() {
		clearInterval(Status.timer);
		Status.handle(defStatus);
	},
	get: function() {
		Server.request(Server.cmd.status, Status.handle);
	},
	handle: function(data) {
		/* Walk through interesting status fields and dispatch events if changed*/
		/* Need to save old data before dispatching events */
		var oldStatus = Status.data;
		Status.data = data;
		
		if(data.state != oldStatus.state) {
			$(document).trigger("Status.state");
		}		
		
		if(data.volume != oldStatus.volume) {
			$(document).trigger("Status.volume");
		}
		
		if(data.artist != oldStatus.artist ||
		   data.track  != oldStatus.track  ||
			 data.album  != oldStatus.album  ||
			 data.albumArt != oldStatus.albumArt) {
			$(document).trigger("Status.songinfo");
		}
		
		if(data.elapsedTime != oldStatus.elapsedTime) {
			$(document).trigger("Status.time");
		}
	}
}

/* Server:
 * 
 * Object that manages server connection. */
var Server = {
	url: null, /* TODO: load from cookie */
	cmd: {
		status: Config.defAmp + "/getStatus",
		prev:   Config.defAmp + "/previousTrack",
		play:   Config.defAmp + "/play",
		pause:  Config.defAmp + "/pause",
		next:   Config.defAmp + "/nextTrack",
		seek:   Config.defAmp + "/seek?mseconds=",
		volume: Config.defAmp + "/setVolume?level=",
		albumArt: "/dogvibes/getAlbumArt?size=320&uri="
	},
	init: function() {
		/* Default state */
		$(document).trigger("Server.error");	
		
		/* Do we have a server? Otherwise ask user to enter URL */
		if(!Server.url) {
			Server.url = prompt("Enter Dogvibes server URL:", Config.defServer);
		}
		/* Try to connect */		
		Server.request(Server.cmd.status, Server.connected);
	},
	request: function(Command, Success, Error) {
		/* Setup default callbacks */
		if(typeof(Success == "undefined")) {
			Success = Status.get; 
		}	
		if(typeof(Error == "undefined")) {
			Error = Server.error; 
		}
    $.jsonp({
			url: Server.url + Command,
      //data: options,
      error: Error,
      callbackParameter: "callback",
      success: Success
		});	
	},	
	connected: function() {
		/* Let people know that we have working connection */
		$(document).trigger("Server.connected");
		
	},
	error: function() {
		$(document).trigger("Server.error");	
		/* */
		alert("Ooops! No server!");		
	}
}

/* SongInfo:
 *
 * Handles artist/trackname updates (upper bar) in the UI etc. */

var SongInfo = {
	ui: Array(),
	init: function() {
		/* Create UI objects that this module controls */
		SongInfo.ui.trackNo = d.cE('strong');
		SongInfo.ui.time = d.cE('ul');
		var ul = d.cE('ul');
		
		/* Track data */
		SongInfo.ui.artist = d.cE('li');
		ul.appendChild(SongInfo.ui.artist);

		SongInfo.ui.track = d.cE('li');
		SongInfo.ui.track.className = "highlight";		
		ul.appendChild(SongInfo.ui.track);
		
		SongInfo.ui.album = d.cE('li');
		ul.appendChild(SongInfo.ui.album);
		
		UI.info.appendChild(ul);
		
		/* Time and slider */
		SongInfo.ui.elapsed = d.cE('li');
		SongInfo.ui.elapsed.className = "time";
		SongInfo.ui.time.appendChild(SongInfo.ui.elapsed);

		SongInfo.ui.slider = d.cE('li');
		SongInfo.ui.slider.className = "slider";
		SongInfo.ui.time.appendChild(SongInfo.ui.slider);		

		SongInfo.ui.total = d.cE('li');
		SongInfo.ui.total.className = "time";
		SongInfo.ui.time.appendChild(SongInfo.ui.total);
		
		UI.track.appendChild(SongInfo.ui.trackNo);
		UI.track.appendChild(SongInfo.ui.time);
		
		/* Setup event listeners */
		$(document).bind("Server.error", SongInfo.hide);
		$(document).bind("Server.connected", SongInfo.show);
		$(document).bind("Status.songinfo", SongInfo.set);
		$(document).bind("Status.time", SongInfo.time);
	},
	set: function() {
		/* Update UI labels with lastes song info  */
		UI.setText(SongInfo.ui.artist, Status.data.artist);
		UI.setText(SongInfo.ui.album, Status.data.album);
		UI.setText(SongInfo.ui.track, Status.data.track);
		/* Update album art */
		var imgUrl = typeof(Status.data.uri) == "undefined" ?
								 Config.defAlbumArtURL :
								 Server.url + Server.cmd.albumArt + Status.data.uri;
		$(UI.albArt).css('background-image', 'url(' + imgUrl + ')'); 
		
		/* TODO: Track number */
		
	},
	time: function() {
		/* TODO: implement */
	},
	show: function() {
		$(UI.track).show();
		UI.albArt.className = "";
	},
	hide: function() {
		$(UI.track).hide();	
		UI.albArt.className = "none";		
	}
}

/* PlayControl:
 * 
 * Handles playback and volume
 */
var PlayControl = {
	ui: Array(),
	init: function() {
		/* Create UI objects that this module controls */
		var li;
		PlayControl.ui.volume = d.cE('div');
		PlayControl.ui.volume.id = "volume";
		
		PlayControl.ui.ctrl = d.cE('ul');
		PlayControl.ui.ctrl.id = "playback";
		
		PlayControl.ui.prev = d.cE('a');
		PlayControl.ui.prev.className = "prev";
		PlayControl.ui.prev.onclick = function() { Server.request(Server.cmd.prev); };
		li = UI.newElement('li', PlayControl.ui.prev);
		PlayControl.ui.ctrl.appendChild(li);
		
		PlayControl.ui.play = d.cE('a');
		PlayControl.ui.play.className = "play";
		PlayControl.ui.play.onclick = function() { Server.request(Server.cmd.play); };
		li = UI.newElement('li', PlayControl.ui.play);
		PlayControl.ui.ctrl.appendChild(li);		
		
		PlayControl.ui.next = d.cE('a');
		PlayControl.ui.next.className = "next";
		PlayControl.ui.next.onclick = function() { Server.request(Server.cmd.next); };
		li = UI.newElement('li', PlayControl.ui.next);
		PlayControl.ui.ctrl.appendChild(li);	
		
		UI.bottom.appendChild(PlayControl.ui.ctrl);
		UI.bottom.appendChild(PlayControl.ui.volume);	
		
		/* Setup events */
		$(document).bind("Status.volume", PlayControl.volume);
		$(document).bind("Status.state", PlayControl.state);		
		$(document).bind("Server.connected", PlayControl.show);
		$(document).bind("Server.error", PlayControl.hide);				
	},
	volume: function() {
		/* TODO: Do something */
	},
	state: function() {
		/* TODO: implement */
	},
	show: function() {
		$(UI.bottom).show();
	},
	hide: function() {
		$(UI.bottom).hide();	
	}
}

/* Let's begin when document is ready */
window.onload = function() {
	/* UI needs to be initialized first! */
	UI.init();
	SongInfo.init();
	PlayControl.init();
	Status.init();	
	/* Finally start server connection */
	Server.init();	

	/* Hide iPhone status bar */
	window.scrollTo(0, 0.9);
}


