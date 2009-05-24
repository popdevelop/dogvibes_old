
/* Config */
var default_server = "http://dogvibes.com:2000";
var server = false;
var poll_interval = 2000 /* ms */
var connection_timeout = 10000; /* ms */
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
var current_playlist = false;
var time_count;
var request_in_progress = false;

/* These are all the commands available to the server */
var command = {
   /* playqueue */
   add: "/amp/0/queue?uri=",
   get: "/amp/0/getAllTracksInQueue",
   remove: "/amp/0/removeTrack?nbr=",
   /* Playlists */
   getplaylists: "/dogvibes/getAllPlaylists",
   getplaylisttracks: "/dogvibes/getAllTracksInPlaylist?playlist_id=",
   /* playback control */
   next: "/amp/0/nextTrack",
   play: "/amp/0/play",
   playtrack: "/amp/0/playTrack?nbr=",
   pause: "/amp/0/pause",
   prev: "/amp/0/previousTrack",
   volume: "/amp/0/setVolume?level=", 
   /* other */
   status: "/amp/0/getStatus",
   search: "/dogvibes/search?query=",
   albumart: "/dogvibes/getAlbumArt?size=159&uri="
}

/*
 * Misc. Handy functions 
 */
 
function  setWait(){
	if(wait_req == 0){
		$("#wait_req").html(loading_small);
	}
	wait_req++;	
};
function clearWait(){
	wait_req--
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

/* Increase playback time counter */
function increaseCount(){
	current_song.elapsedmseconds += 1000;
   updateTimes();
}
/* playbacktime, seekbar */
function updateTimes(){
   new_time = Math.round(current_song.elapsedmseconds / 1000 -0.5);
   percent = (current_song.elapsedmseconds/current_song.duration) *100;
	$("#playback_time").html(timestamp_to_string(new_time));
	$('#playback_seek').slider('option', 'value', percent);
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
	m = Math.round(ts/60 - 0.5)
	s = ts - m*60;	
	s=checkTime(s);
	return m + ":" + s;
}

/* 
 * Status request loop and its handler 
 */
function requestStatus()
{
	if(!request_in_progress){
		connectionRequest();
         $.ajax({
            url: server + command.status,
            type: "GET",
            dataType: 'jsonp',
            success: function(data){
            if(data.error != 0){
               connectionBad("Server error! (" + data.error + ")");
            }
            connectionOK(); /* This will restore timeout aswell */    
            handleStatusResponse(data.result);
         }     
		});
	}
}
function handleStatusResponse(data)
{
	/* Check if song has switched */
	if(current_song.index != data.index){
		$("#row_" + current_song.index + " td:first a").removeClass("playing_icon");      
      $("#row_" + current_song.index + " td:first a").addClass("remButton"); 
		$("#row_" + current_song.index + " td").removeClass("playing");  	
		$("#album_art").html("<img src=\"" + server + command.albumart + data.uri + "\">");
	}
   /* Update playqueue if applicable */   
	if(data.playqueuehash != current_song.playqueuehash && current_page == "playqueue"){
		getPlayQueue();
	}

   current_song = data;   
   /* Update volume */
   if(data.volume){
      $('#playback_volume').slider('option', 'value', data.volume*100);
   }
	/* Update play status */
	if(data.state == "playing"){
		$("#pb-play > a").addClass("playing");
		increaseCount();
		clearInterval(time_count);
		time_count = setInterval(increaseCount, 1000);
	}
	else{
		$("#pb-play > a").removeClass("playing");
		clearInterval(time_count);
	}
   /* Update now playing field */
	if(data.state != "stopped"){
		$("#now_playing .artist").text(data.artist);
		$("#now_playing .title").text(data.title);
		$("#row_" + data.index + " td:first a").addClass("playing_icon"); 
      $("#row_" + data.index + " td:first a").removeClass("remButton");       
		$("#row_" + data.index + " td").addClass("playing");       
      updateTimes();
	}
	else {
		$("#now_playing .title").html("Nothing playing right now");
		$("#now_playing .artist").empty();
		$("#album_art").empty();
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
   poll_handle = setInterval(requestStatus,poll_interval);
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

/* Get playqueue or playlist */
function getPlayQueue(){
	$("#s-results").html("<tr><td colspan=6>"+ loading_small+ " <i>Fetching play queue...</i>");
   var getcommand;
   if(current_playlist == "playqueue"){
      getcommand = command.get;
   } else {
      getcommand = command.getplaylisttracks + current_playlist;
   }
	$.ajax({
      url: server + getcommand,
      type: "GET",
      dataType: "jsonp",
      success: function(data) {
      item_count = 0;
		$("#s-results").empty();
		$.each(data.result, function(i, song) {
            td = (i % 2 == 0) ? "<td class=\"odd\">" : "<td>";
            $("#s-results").append("<tr id=\"row_"+ i + "\">" + td + "<a href=\"#\" id=\"" + i + "\" class=\"remButton\" title=\"Remove from queue\">-</a>" + td + "<a href=\"#\" id=\"" + i + "\" class=\"playButton\">" + song.title + "</a>" + td + song.artist + td + timestamp_to_string(song.duration/1000) + td + song.album);
            item_count++;
         });
         $(".remButton").click(function () { 
            $.ajax({ 
               beforeSend: setWait(),
               type: "GET",
               dataType: 'jsonp',
               url: server + command.remove + this.id,
               success: function () {clearWait(); requestStatus(); }
            });
         });    
         $(".playButton").click(function () { 
            $.ajax({ 
               beforeSend: setWait(),
               type: "GET",
               dataType: 'jsonp',
               url: server + command.playtrack + this.id,
               success: function () {clearWait(); requestStatus(); }
            });
         });      
         if(item_count == 0){
            $("#s-results").html("<tr><td colspan=6><i>No tracks in this list</i>");
         }
         else{
            /* TODO: Make things sortable. Just dummy for now */
            $(function() {
               $("#s-results").sortable();
               $("#s-results").disableSelection();
            });
         }
         requestStatus();
      }
	});   
}

/* Playlists */
function getPlayLists(){
   $("#playlists-items").empty();
   $.ajax({
      url: server + command.getplaylists,
      type: "GET",
      dataType: "jsonp",
      success: function(data) {
         $.each(data.result, function(i, list){
            $("#playlists-items").append("<li id=\"pl-"+list.id+"\"><a href=\"#\" class=\"playlistClick\" name=\""+list.id+"\">"+list.name+"</a>");
         });
         $(".playlistClick").click(function() { 
            current_playlist = this.name;
            setPage("pl-" + this.name);
            $("#tab-title").text("Playlist");
            $("#playlist").html(track_list_table);
            getPlayQueue();
         });         
      }
   });
}

/* Searching */
var searchesArray = new Array();
function addSearch(keyword){
   tempArray = new Array();
   tempArray.unshift(jQuery.trim(keyword));
   $.each(searchesArray, function(i, entry){
      if(jQuery.trim(keyword) != entry){
         tempArray.push(entry);
      }
   });
   if(tempArray.length > 6){
      tempArray.pop();
   }
   $("#searches-items").empty();
   $.each(tempArray, function(i, entry) { 
      $("#searches-items").append("<li id=\"s-"+i+"\"><a href=\"#\" class=\"searchClick\">"+entry+"</a>");
   });
   $(".searchClick").click(function() { doSearchFromLink($(this).text()); });
   searchesArray = tempArray;
}

function doSearchFromLink(value) {
	$("#s-input").val(value);
	doSearch();
}

function doSearch() {
   addSearch($("#s-input").val());
	setPage("s-0");
	$("#playlist").html(search_summary + track_list_table);
	$("#s-results").html("<tr><td colspan=6>" + loading_small + " <i>Searching...</i>");
	$("#s-keyword").text($("#s-input").val());
	$("#tab-title").text("Search");
	$.ajax({
      url: server + command.search + $("#s-input").val(),
      type: "GET",
      dataType: "jsonp",
      success: function(data) {
         var artists = {};
         var albums  = {};
      	 item_count = 0;
          artist_count = 0;
          album_count = 0;
         $("#s-results").empty();
            $.each(data.result, function(i, song) {
               td = (i % 2 == 0) ? "<td class=\"odd\">" : "<td>";
               $("#s-results").append("<tr>" + td +"<a href=\"#\" id=\"" + song.uri + "\" class=\"addButton\" title=\"Add to play queue\">+</a>" + td + "<a href=\"#\" id=\"" + song.uri + "\" class=\"playButton\">" + song.title + td + "<a href=\"#\" id=\"" + song.artist + "\" class=\"searchArtistButton\">" + song.artist + td + timestamp_to_string(song.duration/1000) + td + "<a href=\"#\" id=\"" + song.album + "\" class=\"searchArtistButton\">" + song.album);
               item_count++;
               if(!artists[song.artist]) { artists[song.artist]=0; artist_count++; }
               artists[song.artist]++;
               if(!albums[song.album]) { albums[song.album]=song.artist; album_count++; }
            });
            /* Add click actions */
            $(".addButton").click(function () {
               $(this).effect('highlight');
               $.ajax({  
                  beforeSend: setWait(),
                  type: "GET",
                  dataType: 'jsonp',
                  url: server + command.add + this.id,
                  success: clearWait()
               });
            }); 
            $(".playButton").click(function () {
               $(this).effect('highlight');
               $.ajax({
                  beforeSend: setWait(),
                  type: "GET",
                  dataType: 'jsonp',
                  url: server + command.add + this.id,
                  success: function () {clearWait(); requestStatus(); }
               });
            });     
            $(".searchArtistButton").click(function () {
               doSearchFromLink(this.id);
            });
            if(item_count == 0){
               $("#s-results").html("<tr><td colspan=6><i>No results for '" + $("#s-input").val() + "'</i>");
               
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
         }
	});
}


/*
 * Register events
 */

/* Startup */
$("document").ready(function() { 
   $("#message .btn").hide();
	connectionBad("No server configured.");
   setPage("p-home");
	$("#playback_seek").slider();
	$("#playback_volume").slider();
	/* Do we have a server? otherwise prompt */ 
	if(!server){
		server = prompt("Enter Dogvibes server URL:", default_server);
	}
	if(server){
      connectionInit();
      getPlayLists(); /* TODO: move this when we have playlisthash */
		return;
	}
	connectionBad("No server configured. Press reload to set");
});

/* Playback control */
/* Play button */
$("#pb-play").click(function() {
	var action = $("#pb-play > a").hasClass("playing") ? command.pause : command.play;
	$.ajax({ 
		type: "GET",
		dataType: 'jsonp',
		url: server + action,
		success: function () { requestStatus(); }
	});
});
/* Next button */
$("#pb-next").click(function() {
	$.ajax({ 
		type: "GET",
		dataType: 'jsonp',
		url: server + command.next,
		success: function () { requestStatus(); }
	});
});

/* Prev button */
$("#pb-prev").click(function() {
	$.ajax({ 
		type: "GET",
		dataType: 'jsonp',
		url: server + command.prev,
		success: function () { requestStatus(); }
	});
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
$("#p-home").click(function () {
	setPage("p-home");
	$("#tab-title").text("Home");
	$("#playlist").html(welcome_text);
});

$("#p-local").click(function () {
	setPage("p-local");
	$("#tab-title").text("Local media");
	$("#playlist").html("<h1>Local files:</h1><div id=\"file_tree\"></div>");
	$('#file_tree').fileTree({
		root: '/',
		script: server + '/jqueryFileTree.php',
		multiFolder: true,
		loadMessage: loading_small
	}, function(file) {
		alert(file);
	});	
});

/* Play queue management */

$("#p-playqueue").click(function () {
	setPage("p-playqueue");
   current_playlist = "playqueue";
	$("#tab-title").text("Play queue");
	$("#playlist").html(track_list_table);
	getPlayQueue();
});

/* Searching */
$("#s-submit").click(doSearch);
$("#s-input").keypress(function (e) {
	if (e.which == 13)
		doSearch();
});

/* Clear search keyword field if nav links are clicked*/
$(".navlink").bind("click", function () {
	$("#s-keyword").empty();
});

$('#playback_volume').slider({
   change: function(event, ui) { 
	$.ajax({ 
		type: "GET",
		dataType: 'jsonp',
		url: server + command.volume + ui.value/100,
		success: function () { requestStatus(); }
	});		
   }
});