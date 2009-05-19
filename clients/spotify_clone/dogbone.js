
/* Config */
var default_server = "http://dogvibes.com:2000"
var server = false;
var poll_interval = 5000 /* ms */
var connection_timeout = 10000; /* ms */
/* Misc variables */
var wait_req = 0;
var track_list_table = "<table cellspacing=\"0\" cellpadding=\"0\"><thead><tr><th id=\"indicator\">&nbsp;<th id=\"track\"><a href=\"#\">Track</a><th id=\"artist\"><a href=\"#\">Artist</a><th id=\"time\"><a href=\"#\">Time</a><th id=\"rating\"><a href=\"#\">Rating</a><th id=\"album\"><a href=\"#\">Album</a></thead><tbody id=\"s-results\"></tbody></table>";
var loading_small = "<img src=\"img/loading_small.gif\">";
var welcome_text = "<h1>Welcome to dogbone!</h1> <p>The first HTML-client to <a href=\"http://www.dogvibes.com\" target=\"_new\">Dogvibes</a>.</p>";
var poll_handle = false;
var current_time;
var current_song = false;
var current_playqueue = false;
var current_page = "home";
var time_count;
var request_in_progress = false;
var dobj = new Date();

/* These are all the commands available to the server */
var command = 
{
   /* playqueue */
   add: "/amp/0/queue?uri=",
   get: "/amp/0/getAllTracksInQueue",
   remove: "/amp/0/removeTrack?nbr=",
   /* playback control */
   next: "/amp/0/nextTrack",
   play: "/amp/0/play",
   playtrack: "/amp/0/playTrack?nbr=",
   pause: "/amp/0/pause",
   prev: "/amp/0/previousTrack",
   /* other */
   status: "/amp/0/getStatus",
   search: "/dogvibes/search?query="
}

/* Handy functions */
function  setWait(){
	if(wait_req == 0)
	{
		$("#wait_req").html(loading_small);
	}
	wait_req++;	
};
function clearWait(){
	wait_req--
	if(wait_req == 0)
	{
		$("#wait_req").empty();
	}
};

function setPlayStatus(data)
{
   /* Check if song has switched */
   if(current_song != data.index){
      if(current_song){
         $("#row_" + current_song + " td:first").removeClass("playing_icon");      
         $("#row_" + current_song + " td").removeClass("playing");    
      }
      $("#playback_total").html(timestamp_to_string(data.duration/1000));
      current_song = data.index;
   }
   /* Update play status */
	if(data.state == "playing"){
		$("#pb-play > a").addClass("playing");
      $("#row_" + current_song + " td:first").addClass("playing_icon");      
      $("#row_" + current_song + " td").addClass("playing"); 
		current_time = data.duration/1000;
		increaseCount();
		clearInterval(time_count);
		time_count = setInterval(increaseCount, 1000);
	}
	else{
		$("#pb-play > a").removeClass("playing");
		clearInterval(time_count);
	}
   /* Update playqueue if applicable */
   if(current_playqueue && data.playqueuehash != current_playqueue && current_page == "playqueue")
   {
      getPlayQueue();
   }
}

function checkTime(i)
{
	if (i<10 && i>=0)
  	{
  		i="0" + i;
  	}
	return i;
}

function increaseCount()
{
	current_time = current_time + 1;
	$("#playback_time").html(timestamp_to_string(current_time));
}

function timestamp_to_string(ts)
{
   if(!ts) { ts=0; }
	m = Math.round(ts/60 - 0.5)
	s = ts - m*60;	
	m=checkTime(m);
	s=checkTime(s);
   return m + ":" + s;
}

/* Connection */
var connection_timer;
function connectionOK()
{
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

function connectionTimeout()
{
   $("#shade").show();
   $("#message").show(); 
   $("#message .text").removeClass("loading");
   $("#message .text").html("Connection timed out! Check server and reload page to try again.");
}

function getNowPlaying()
{
   if(!request_in_progress)
   {
   /* TODO: make connection state machine more clear... */
   request_in_progress = true;
   connection_timer = setTimeout(connectionTimeout, connection_timeout);
	$.ajax({
      url: server + command.status,
      type: "GET",
      dataType: 'jsonp',
      success: function(data){
            if(data.error != 0){
               connectionBad("Server error! (" + data.error + ")");
            }
             request_in_progress = false;
             connectionOK(); /* This will restore timeout aswell */    
             data = data.result;
             beg = "<ul>";
             sepa = "<li class=\"artist\">";
             sept = "<li class=\"title\">";
             end = "</ul>";
             setPlayStatus(data);             
             if(data.artist){
                 $("#now_playing").html(beg + sepa + data.title + sept + data.artist + end);
                 if(data.albumart){
                  $("#album_art").html("<img src=\"" + server + data.albumart + "\">");
                 }
                 else {
                  $("#album_art").empty();
                 }
             }
             else{
                 $("#now_playing").html(beg + sepa + "Nothing playing right now" + end);
                 $("#album_art").empty();
             }
             if(data.playqueuehash){
               current_playqueue = data.playqueuehash;
             }
         }
         /* error handling is not working when using jsonp, but anyway.... */
      /*error:function (xhr, ajaxOptions, thrownError){
            connectionBad("Server not responding! (" + thrownError + ": " + xhr.statusText + ") Trying to reconnect...");
         }  */       
      });
   }
}

function getPlayQueue(){
   $("#s-results").html("<tr><td colspan=6>"+ loading_small+ " <i>Fetching play queue...</i>");
     $.getJSON(server + command.get + "?callback=?", function(data) {
       $("#s-results").empty();
       $.each(data.result, function(i, song) {
         td = (i % 2 == 0) ? "<td class=\"odd\">" : "<td>";
         $("#s-results").append("<tr id=\"row_"+ i + "\">" + td + "<a href=\"#\" id=\"" + i + "\" class=\"remButton\" title=\"Remove from queue\">-</a>" + td + "<a href=\"#\" id=\"" + i + "\" class=\"playButton\">" + song.title + "</a>" + td + song.artist + td + timestamp_to_string(song.duration/1000) + td + "&nbsp;" +td + song.album);
       });
       $(".remButton").click(function () { 
           $.ajax({ 
               beforeSend: setWait(),
               type: "GET",
               dataType: 'jsonp',
               url: server + command.remove + this.id,
               success: function () {clearWait(); getNowPlaying(); }
         });
         $("#row_" + this.id).remove(); /* FIXME: do this more data driven, we dont know if the entry actually was removed */
         });    
       $(".playButton").click(function () { 
           $.ajax({ 
               beforeSend: setWait(),
               type: "GET",
               dataType: 'jsonp',
               url: server + command.playtrack + this.id,
               success: function () {clearWait(); getNowPlaying(); }
         });
         });      
       if(data.count == 0)
       {
         $("#s-results").html("<tr><td colspan=6><i>No stuff in play queue</i>");
       }
       getNowPlaying();
     });   
}

/* Startup: check server playback status. Fetch track if playing */
$("document").ready(function() { 
   connectionBad("No server configured.");
   $("#playback_seek").slider();
   $("#playback_volume").slider();
   /* Do we have a server? otherwise prompt */ 
   $.ajaxSetup({ error: function() { connectionBad("Connection lost! Trying to reconnect..."); } });
   if(!server){
      server = prompt("Enter Dogvibes server URL:", default_server);
   }
   if(server)
   {
      $("#message .text").addClass("loading");
      clearTimeout(connection_timer);      
      connectionBad("Connecting to server '" + server + "'...");
      poll_handle = setInterval(getNowPlaying,poll_interval);
      getNowPlaying();
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
  		success: function () { getNowPlaying(); }
	});
});
/* Next button */
$("#pb-next").click(function() {
    $.ajax({ 
        type: "GET",
        dataType: 'jsonp',
  		url: server + command.next,
  		success: function () { getNowPlaying(); }
	});
});

/* Prev button */
$("#pb-prev").click(function() {
    $.ajax({ 
        type: "GET",
        dataType: 'jsonp',
  		url: server + command.prev,
  		success: function () { getNowPlaying(); }
	});
});


/* Sections */
$("#p-home").click(function () {
   current_page = "home";
	$("#tab-title").text("Home");
	$("#playlist").html(welcome_text);
});

$("#p-local").click(function () {
   current_page = "local";
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
   current_page = "playqueue";
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

function doSearchFromLink(value) {
  $("#s-input").val(value);
  doSearch();
}

function doSearch() {
   current_page = "search";
  $("#playlist").html(track_list_table);	
  $("#s-results").html("<tr><td colspan=6>" + loading_small + " <i>Searching...</i>");
  $("#s-keyword").text($("#s-input").val());
  $("#tab-title").text("Search");
  $.getJSON(server + command.search + $("#s-input").val() + "&callback=?", function(data) {
    $("#s-results").empty();
    $.each(data.result, function(i, song) {
      td = (i % 2 == 0) ? "<td class=\"odd\">" : "<td>";
      $("#s-results").append("<tr>" + td +"<a href=\"#\" id=\"" + song.uri + "\" class=\"addButton\" title=\"Add to play queue\">+</a>" + td + "<a href=\"#\" id=\"" + song.uri + "\" class=\"playButton\">" + song.title + td + "<a href=\"#\" id=\"" + song.artist + "\" class=\"searchArtistButton\">" + song.artist + td + song.time + td + "&nbsp;" +td + "<a href=\"#\" id=\"" + song.album + "\" class=\"searchArtistButton\">" + song.album);
    });
    $(".addButton").click(function () {
      $.ajax({  
         beforeSend: setWait(),
         type: "GET",
         dataType: 'jsonp',
    		url: server + command.add + this.id,
    		success: clearWait()
  		});
    }); 
    $(".playButton").click(function () {
        $.ajax({
            beforeSend: setWait(),
            type: "GET",
            dataType: 'jsonp',
            url: server + command.playtrack + this.id,
            success: function () {clearWait(); getNowPlaying(); }
      });
      });     
    $(".searchArtistButton").click(function () {
      doSearchFromLink(this.id);
/*
      $.ajax({
         beforeSend: setWait(),
         type: "GET",
         dataType: 'jsonp',
    		url: server + command.add + this.id,
    		success: clearWait()
  		});
*/
    });
    if(data.count == 0)
    {
    	$("#s-results").html("<tr><td colspan=6><i>No results for '" + $("#s-input").val() + "'</i>");
    }
  });
}

/* Clear search keyword field if nav links are clicked*/
$(".navlink").bind("click", function () {
   $("#s-keyword").empty();
});