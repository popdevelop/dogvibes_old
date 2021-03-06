/*
 * Modules for handling playlists, searches, etc...
 */

var Config = {
  defaultUser: "",
  defaultServer: "http://localhost:2000",
  resizeable: true,
  draggableOptions: {
    revert: 'invalid', 
    scroll: false,
    revertDuration: 100, 
    helper: 'clone', 
    cursorAt: { left: 5 },
    appendTo: "#drag-dummy", 
    zIndex: 1000,
    addClasses: false,
    start: function() { $(this).click(); }
  },
  sortableOptions: {
    revert: 100, 
    scroll: false, 
    helper: 'clone', 
    appendTo: "#drag-dummy", 
    zIndex: 1000,
    addClasses: false,
  }  
};


/* TODO: remove when all dependencies are solved */
var UI = {
  titlebar: "#titlebar",
  navigation: "#navigation",
  trackinfo: "#trackinfo",
  currentsong: "#currentsong"
};

/* TODO: Find out a good way to handle titlebar */
var Titlebar = {
  set: function(text) {
    $(UI.titlebar).empty();
    $(UI.titlebar).append($("<li class='selected'>"+text+"</li>"));
  }
};

/**************************
 * Prototype extensions
 **************************/

/* remove a prefix from a string */
String.prototype.removePrefix = function(prefix) {
  if(this.indexOf(prefix) === 0) {
    return this.substr(prefix.length);
  }
  return false;
};

/* Create a function for converting msec to time string */
Number.prototype.msec2time = function() {
  var ts = Math.floor(this / 1000);
  if(!ts) { ts=0; }
  if(ts===0) { return "0:00"; }
  var m = Math.round(ts/60 - 0.5);
  var s = Math.round(ts - m*60);
  if (s<10 && s>=0){
    s="0" + s;
  }
  return m + ":" + s;
};

/******************
 * Helper classes
 ******************/

var ResultTable = function(config) {
  var self = this;
  /* Default configuration */
  this.options = {
    name: "Table",
    idTag: "id",
    highlightClass: "playing",
    sortable: false,
    click: function() {},
    dblclick: function() {},
    callbacks: {
      album: function(element) {
        var a = $('<a/>').attr('href', '#album/' + element.text());
        element.contents().wrap(a);
      },      
      artist: function(element) {
        var a = $('<a/>').attr('href', '#artist/' + element.text());
        element.contents().wrap(a);
      }    
    }
  };
  
  /* Set user configuration */
  $.extend(true, this.options, config);
  
  this.ui = {
    content: "#" + self.options.name + "-content",
    items  : "#" + self.options.name + "-items"
  };  
  
  /* Some properties */
  this.items = [];
  this.selectedItem = false;
  this.fields = [];
  this.selectMulti = false;
  
  /* Configure table fields by looking for table headers */
  $(".template", self.ui.content).children().each(function(i, el) {
    if("id" in el) {
      var field = el.id.removePrefix(self.options.name + "-");
      if(field !== false) {
        self.fields.push(field);
      }
    }
  });
  
  if(self.options.sortable) {
    $(self.ui.content).tablesorter();
  }
  
  /***** Methods *****/
  this.display = function() {
    var self = this;
    $(self.ui.items).empty();
    $(self.items).each(function(i, el) {
      var tr = $("<tr></tr>");
      var id = self.options.idTag in el ? el[self.options.idTag] : i;
      tr.attr("id", self.options.name+"-item-id-"+id);
      tr.attr("name", self.options.name+"-item-no-"+i);
      /* always add uri */
      if("uri" in el) { tr.attr("uri", el.uri); }
      $(self.fields).each(function(i, field) {
        var content = $("<td></td>");
        if(field in el) {
          var value = el[field];
          /* FIXME: assumption: Convert to time string if value is numeric */
          if(typeof(value) == "number") {
            value = value.msec2time();
            content.addClass("time");
          }
          content.append(value);
        }
        if(field in self.options.callbacks) {
          content.id = id;
          content.nbr = i;
          self.options.callbacks[field](content);
        }        
        tr.append(content);
      });
      tr.click(self.options.click);
      tr.dblclick(self.options.dblclick);
      $(self.ui.items).append(tr);
    });
    
    $("tr:visible",this.ui.items).filter(":odd").addClass("odd");
        
    /* Update tablesorter */
    $(self.ui.content).trigger("update");
  };
  
  this.empty = function() {
    $(this.ui.items).empty();
  };
  
  this.selectItem = function(index) {
    index = parseInt(index);
    this.deselectAll();
    if(index > this.items.length) return;
    
    if(!this.selectMulti ||
       !this.selectedItem) {
      this.selectedItem = index;
    }
    
    /* Create the selection range */
    var min = this.selectedItem < index ? this.selectedItem : index;
    var max = this.selectedItem < index ? index : this.selectedItem;

    for(var i = min; i <= max; i++) {
      $('tr[name="'+this.options.name+'-item-no-'+i+'"]').addClass("selected");
    }
  };
  this.deselectAll = function() {
    this.selectedItem = false;
    $("tr", this.ui.items).removeClass("selected");
  };
  this.clearHighlight = function() {
    $("tr", this.ui.items).removeClass(this.options.highlightClass);  
  };
  this.highlightItem = function(index) {
    var item = $('tr[name="'+this.options.name+'-item-no-'+index+'"]');
    item.addClass(this.options.highlightClass);
  };
}

var NavList = {
  /* Store all sections globally */
  sections: Array(),
  /* Section object */  
  Section: function(container, type) {
    this.ul = $(container);
    this.ul.addClass(type);
    NavList.sections.push(this);
    this.items = Array();
    $(UI.navigation).append(this.ul);    
    this.addItem = function(id, item) {
      this.items[id] = $(item);
      this.ul.append(this.items[id]);
    };
    this.selectItem= function(id) {
      $(NavList.sections).each(function(i ,el) {
        el.deselect();
      });
      if(typeof(this.items) != "undefined" && id in this.items) {
        this.items[id].addClass('selected');
      }
    };
    this.deselect = function() {
      for(var i in this.items) {
        this.items[i].removeClass('selected');
      }
    };
    this.empty = function() {
      this.items = Array();
      this.ul.empty();
    };
  }
};


/**************************
 * Modules 
 **************************/

var Main = {
  ui: {
    page   : "#playqueue",
    section: "#Main-section"
  },
  init: function() {
    /* Bug in jQuery: you can't have same function attached to multiple events! */    
    Main.ui.list = new NavList.Section(Main.ui.section, '');
    Main.ui.list.addItem("home", $("<li class='home'><a href='#home'>Home</a><li>"));
    $(document).bind("Page.home", Main.setHome);
    Main.ui.playqueue = $("<li class='playqueue'><a href='#playqueue'>Play queue</a></li>");
    Main.ui.playqueue.droppable({
      hoverClass: 'drophover',
      tolerance: 'pointer',
      drop: function(event, ui) {
        uri = ui.draggable.attr("uri");
        Dogvibes.queue(uri);
      }
    });
    Main.ui.list.addItem("playqueue", Main.ui.playqueue);
    $(document).bind("Page.playqueue", Main.setQueue); 
    
    /* Online / Offline */
    $(document).bind("Server.error", function() {
      $(Main.ui.page).removeClass();
      $(Main.ui.page).addClass("disconnected");
    });
    $(document).bind("Server.connected", function() {
      $(Main.ui.page).removeClass("disconnected");
    });
  },
  setQueue: function() {
    Titlebar.set(Dogbone.page.title);
    Main.ui.list.selectItem(Dogbone.page.id);
    Playqueue.fetch();
  },
  setHome: function() {
    Titlebar.set(Dogbone.page.title);
    Main.ui.list.selectItem(Dogbone.page.id);
  }  
};

var Playqueue = {
  ui: {
    page: "#playqueue"
  },
  table: false,
  hash: false,
  init: function() {
    /* Create a table for our tracks */
    Playqueue.table = new ResultTable(
    {
      name: 'Playqueue', 
      click: function() {
        var index = $(this).attr("name").removePrefix('Playqueue-item-no-');
        Playqueue.table.selectItem(index);       
      },
      dblclick: function() {
        var id = $(this).attr("id").removePrefix('Playqueue-item-id-');
        Dogvibes.playTrack(id, "-1");
      },
      /* Add a remove-icon  */
      callbacks: {
        space: function(element) {        
          $('<span> remove </span>')
            .attr("id", "Remove-id-"+element.id)
            .attr("title", "remove track from playqueue")
            .click(function() {
              var id = $(this).attr("id").removePrefix("Remove-id-");
              Dogvibes.removeTrack(id);
          }).appendTo(element);
        }
      }
    });
    
    $(document).bind("Status.playlistchange", function() { Playqueue.fetch() });
    $(document).bind("Status.state", function() { Playqueue.set() });
    $(document).bind("Status.playlist", function() { Playqueue.set() });
    $(document).bind("Server.connected", Playqueue.fetch);
  },
  fetch: function() {
    if(Dogbone.page.id != "playqueue") return;
    if(Dogvibes.server.connected) { 
      Playqueue.hash = Dogvibes.status.playlistversion;
      Playqueue.table.empty();
      $(Playqueue.ui.page).addClass("loading");
      Dogvibes.getAllTracksInQueue("Playqueue.update");
    }
  },
  update: function(json) {
    $(Playqueue.ui.page).removeClass("loading");  
    if(json.error !== 0) {
      return;
    }
    Playqueue.table.items = json.result;
    Playqueue.table.display();
    Playqueue.set();
    /* Make draggable/sortable */
    $(function() {
      $("tr", Playqueue.table.ui.items).draggable(Config.draggableOptions);
    });     
  },
  set: function() {
    if(Dogvibes.status.state == "playing" &&
       Dogvibes.status.playlist_id == -1) {
      $("li.playqueue").addClass('playing'); 
      Playqueue.table.highlightItem(Dogvibes.status.index);      
    } 
    else {
      $("li.playqueue").removeClass('playing');    
      Playqueue.table.clearHighlight();     
    }
  }
};

var PlayControl = {
  ui: {
    controls: "#PlayControl",
    prevBtn : "#pb-prev",
    playBtn : "#pb-play",
    nextBtn : "#pb-next",
    volume  : "#Volume-slider",
    seek    : "#TimeInfo-slider",
    elapsed : "#TimeInfo-elapsed",
    duration: "#TimeInfo-duration"
  },
  volSliding: false,
  seekSlideing: false,
  init: function() {
    $(document).bind("Status.state", PlayControl.set);
    $(document).bind("Status.volume", PlayControl.setVolume);
    $(document).bind("Status.elapsed", PlayControl.setTime);
    
    $(PlayControl.ui.volume).slider( {
      start: function(e, ui) { PlayControl.volSliding = true; },
      stop: function(e, ui) { PlayControl.volSliding = false; },
      change: function(event, ui) { 
        Dogvibes.setVolume(ui.value/100);
      }
    });
    
    $(PlayControl.ui.seek).slider( {
      start: function(e, ui) { PlayControl.seekSliding = true; },
      stop: function(e, ui) { PlayControl.seekSliding = false; },
      change: function(event, ui) { 
        Dogvibes.seek(Math.round((ui.value*Dogvibes.status.duration)/100));
      }
    });    
    
    $(PlayControl.ui.nextBtn).click(function() {
      Dogvibes.next();
    });

    $(PlayControl.ui.playBtn).click(function() {
      PlayControl.toggle();
    });
    
    $(PlayControl.ui.prevBtn).click(function() {
      Dogvibes.prev();
    });
  },
  set: function() {
    $(PlayControl.ui.controls).removeClass();
    $(PlayControl.ui.controls).addClass(Dogvibes.status.state);
    switch(Dogvibes.status.state) {
      case "stopped":
        $(PlayControl.ui.seek).slider( "option", "disabled", true );
        break;
      default:
        $(PlayControl.ui.seek).slider( "option", "disabled", false );
        break;
    }
    //$(PlayControl.ui.duration).text(Dogvibes.status.duration.msec2time());    
  },
  toggle: function() {
    switch(Dogvibes.status.state) {
      case "playing":
        Dogvibes.pause();
        break;
      default:
        Dogvibes.play();
        break;
    }
    PlayControl.set();
  },
  setVolume: function() {
    if(PlayControl.volSliding) return;
    $(PlayControl.ui.volume).slider('option', 'value', Dogvibes.status.volume*100);  
  },
  setTime: function() {
    //$(PlayControl.ui.elapsed).text(Dogvibes.status.elapsedmseconds.msec2time());
    if(PlayControl.seekSliding) return;
    $(PlayControl.ui.seek).slider('option', 'value', (Dogvibes.status.elapsedmseconds/Dogvibes.status.duration)*100);  
  }  
};

var ConnectionIndicator = {
  ui: {
    icon: "#ConnectionIndicator-icon"
  },
  icon: false,
  init: function() {
    ConnectionIndicator.icon = $(ConnectionIndicator.ui.icon);
    if(ConnectionIndicator.icon) {
      $(document).bind("Server.connecting", function() {
        ConnectionIndicator.icon.removeClass();
        //ConnectionIndicator.icon.addClass("connecting");
      });
      $(document).bind("Server.error", function() {
        ConnectionIndicator.icon.removeClass();
        ConnectionIndicator.icon.addClass("error");
      });
      $(document).bind("Server.connected", function() {
        ConnectionIndicator.icon.removeClass();
        ConnectionIndicator.icon.addClass("connected");
      });       
    }
  }
};

var TrackInfo = {
  ui: {
    artist  : "#TrackInfo-artist",
    title   : "#TrackInfo-title",
    albumArt: "#TrackInfo-albumArt"
  },
  init: function() {
    if($(TrackInfo.ui.artist) && $(TrackInfo.ui.title)) {
      $(document).bind("Status.songinfo", TrackInfo.set);
    }
  },
  set: function() {
    $(TrackInfo.ui.artist).text(Dogvibes.status.artist);
    $(TrackInfo.ui.title).text(Dogvibes.status.title);
    var img = Dogvibes.albumArt(Dogvibes.status.uri);
    /* Create a new image and crossfade over */
    var newImg = new Image();
    newImg.src = img;
    newImg.id  = 'TrackInfo-newAlbumArt';
    /* Don't show image until fully loaded */
    $(newImg).load(function() {
      $(newImg)
        .appendTo('#currentsong')
        .fadeIn(2000, function() {
          $(TrackInfo.ui.albumArt).remove();
          $(newImg).attr("id", "TrackInfo-albumArt");
        });
    });
  }
};

var Playlist = {
  ui: {
    section : "#Playlist-section",
    newPlist: "#Newlist-section"
  },
  table: false,
  selectedList: "",
  init: function() {
    Playlist.ui.list =    new NavList.Section(Playlist.ui.section, 'playlists');
    Playlist.ui.newList = new NavList.Section(Playlist.ui.newPlist, 'last');
    Playlist.ui.newBtn  = $("<li class='newlist'><a>New playlist</a></li>");
    Playlist.ui.newBtn.click(function() {
      var name = prompt("Enter new playlist name");
      if(name) {
        Dogvibes.createPlaylist(name, "Playlist.fetchAll");
      }
    });
    Playlist.ui.newList.addItem('newlist', Playlist.ui.newBtn);

    /* Handle offline/online */
    $(document).bind("Server.error", function() {
      $(Playlist.table.ui.content).hide();
      $("#playlist").addClass("disconnected");
    });
    $(document).bind("Server.connected", function() {
      $(Playlist.table.ui.content).show();
      $("#playlist").removeClass("disconnected");
    });
    
    /* Create a table for the tracks */
    Playlist.table = new ResultTable(
    {
      name: 'Playlist',
      highlightClass: "listplaying",
      /* Click events */
      click: function() {
        var index = $(this).attr("name").removePrefix('Playlist-item-no-');
        Playlist.table.selectItem(index);    
      },
      dblclick: function() {
        var index = $(this).attr('id').removePrefix('Playlist-item-id-');
        Playlist.playItem(index);
      },
      /* Add a remove-icon  */
      callbacks: {
        space: function(element) {        
          $('<span> remove </span>')
            .attr("id", "Remove-id-"+element.id)
            .attr("title", "remove track from playlist")
            .click(function() {
              var id = $(this).attr("id").removePrefix("Remove-id-");
              var pid = Playlist.selectedList;
              Dogvibes.removeFromPlaylist(id, pid, "Playlist.setPage");
          }).appendTo(element);
        }
      }
    });

    /* Setup events */
    $(document).bind("Page.playlist", Playlist.setPage);
    $(document).bind("Status.playlistchange", function() { Playlist.setPage(); });   
    $(document).bind("Server.connected", function() { Playlist.fetchAll(); });
    
    $(document).bind("Status.state", function() { Playlist.set(); });    
    $(document).bind("Status.songinfo", function() { Playlist.set(); });    
    $(document).bind("Status.playlist", function() { Playlist.set(); });       
    /* Handle sorts */
    $(Playlist.table.ui.items).bind("sortupdate", function(event, ui) {
      var items = $(this).sortable('toArray');
      var trackID = $(ui.item).attr("id");
      var position;
      for(var i = 0; i < items.length; i++) {
        if(items[i] == trackID) {
          position = i;
          break;
        }
      }
      trackID = trackID.removePrefix("Playlist-item-id-");
      Dogvibes.move(Playlist.selectedList, trackID, (position+1), "Playlist.setPage");
    });               
  },
  setPage: function() {
    if(Dogbone.page.id != "playlist") return;
    Playlist.ui.list.selectItem(Dogbone.page.param);
    Titlebar.set(Dogbone.page.title);
    
    if(Dogvibes.server.connected) {
      /* Save which list that is selected */
      Playlist.selectedList = Dogbone.page.param;
      /* Load new items */
      Dogvibes.getAllTracksInPlaylist(Playlist.selectedList, "Playlist.handleResponse");
    }
  },
  fetchAll: function() {
    Dogvibes.getAllPlaylists("Playlist.update");
  },
  update: function(json) {
    Playlist.ui.list.empty();
    if(json.error !== 0) {
      alert("Couldn't get playlists");
      return;
    }
    $(json.result).each(function(i, el) {
      /* Create list item */
      var item = 
      $('<li></li>')
      .attr("id", "Playlist-"+el.id)
      .append(
        $('<a></a>')
        .attr("href", "#playlist/"+el.id)
        .text(el.name)
        .click(function() {
          $(this).blur();
        })
      );
      /* Make droppable */
      item.droppable({
        hoverClass: 'drophover',
        tolerance: 'pointer',
        drop: function(event, ui) {
          id = $(this).attr("id").removePrefix("Playlist-");
          uri = ui.draggable.attr("uri");
          Dogvibes.addToPlaylist(id, uri);
        }
      });
      /* Remove-button */
      $('<span> remove </span>')
      .attr("id", "Remove-id-" + el.id)
      .attr("title", "remove this playlist")
      .click(function() {
        if(confirm("Do you want to remove this playlist?")) {
          var id = $(this).attr("id").removePrefix("Remove-id-");
          Dogvibes.removePlaylist(id, "Playlist.fetchAll");
          /* FIXME: solve this nicer */
          if(id == Playlist.selectedList) {
            location.hash = "#home";
          }
        }
      }).appendTo(item);
      /* Double click to start playing */
      item.dblclick(function() {
        id = $(this).attr("id").removePrefix("Playlist-id-");
        Dogvibes.playTrack(0, id);
      });
      Playlist.ui.list.addItem(el.id, item);
    });
    Playlist.setPage();
  },

  handleResponse: function(json) {
    if(json.error !== 0) {
      alert("Couldn't get playlist");
      return;
    }
    Playlist.table.items = json.result;
    Playlist.table.display();    
    Playlist.set();
    $(function() {
      $(Playlist.table.ui.items).sortable(Config.sortableOptions);
    });     
  },
  playItem: function(id) {
    if(parseInt(id) != NaN) {
      Dogvibes.playTrack(id, Playlist.selectedList);
    }
  },
  set: function() {  
    Playlist.table.clearHighlight();
    $('li', Playlist.ui.list.ul).removeClass('playing');    
    if(Dogvibes.status.state == "playing") {
       $("#Playlist-"+Dogvibes.status.playlist_id).addClass('playing');
       
      if(Dogvibes.status.playlist_id == Playlist.selectedList) {    
        Playlist.table.highlightItem(Dogvibes.status.index);       
      }
    }
  } 
};

var Search = {
  ui: {
    form:    "#Search-form",
    input:   "#Search-input",
    section: "#Search-section",
    page   : "#search"
  },
  searches: Array(),
  param: "",
  table: false,
  init: function() {
    /* Init search navigation section */
    Search.ui.list = new NavList.Section(Search.ui.section,'search');
    $(document).bind("Page.search", Search.setPage);
    
    /* Handle offline/online */
    $(document).bind("Server.error", function() {
      $(Search.table.ui.content).hide();
      $(Search.ui.page).removeClass();
      $(Search.ui.page).addClass("disconnected");
    });
    $(document).bind("Server.connected", function() {
      Search.setPage();
      $(Search.table.ui.content).show();
      $(Search.ui.page).removeClass("disconnected");
    });

    $(Search.ui.form).submit(function(e) {
      Search.doSearch($("#Search-input").val());
      e.preventDefault();
      return false;
    });
    
    /* Create result table */
    Search.table = new ResultTable(
    {
      name: "Search",
      idTag: "uri",
      sortable: true,
      click: function() {
        var index = $(this).attr("name").removePrefix('Search-item-no-');
        Search.table.selectItem(index);
      },
      dblclick: function() {
        var uri = $(this).attr("id").removePrefix('Search-item-id-');
        Dogvibes.queue(uri);
        Search.table.deselectAll();
        $(this).effect("highlight");
        $(this).addClass("queued");
      }
    });
  
    /* Load searches from cookie */
    var temp;
    for(var i = 0; i < 6; i++){
      if((temp = getCookie("Dogvibes.search" + i)) != "") {
        Search.searches[i] = temp;
      }
    }
    Search.draw();
  },
  setPage: function() {
    if(Dogbone.page.id != "search") return;
    /* See if search parameter has changed. If so, reload */
    if(Dogvibes.server.connected &&
       Dogbone.page.param != Search.param) {
      Search.param = Dogbone.page.param;
      Search.searches.push(Search.param);
      Search.table.empty();
      $(Search.ui.page).addClass("loading");
      Search.addSearch(Search.param);
      
      Dogvibes.search(Search.param, "Search.handleResponse");
    }
    Search.setTitle();    
    Search.ui.list.selectItem(Dogbone.page.param);
  },
  addSearch: function(keyword) {
    var tempArray = Array();
    tempArray.unshift($.trim(keyword));
    $.each(Search.searches, function(i, entry){
      if($.trim(keyword) != entry){
        tempArray.push(entry);
      }
    });
    if(tempArray.length > 6){
      tempArray.pop();
    }
    Search.searches = tempArray;
    for(var i = 0; i < tempArray.length; i++) {
      setCookie("Dogvibes.search" + i, tempArray[i]);
    }
    Search.draw();  
  },
  draw: function() {
    Search.ui.list.empty();
    $(Search.searches).each(function(i, el) {
      Search.ui.list.addItem(el,"<li class='"+el+"'><a href='#search/"+el+"'>"+el+"</a></li>");
    });
  },
  setTitle: function() {
    $(UI.titlebar).empty();
    $(UI.titlebar).append($("<li class='selected'>Search</li>"));
    $(UI.titlebar).append($("<li class='keyword'>"+Search.param+"</li>"));    
  },
  doSearch: function(keyword) {
    window.location.hash = "#search/"+keyword;
  },
 
  handleResponse: function(json) {
    $(Search.ui.page).removeClass("loading");  
    if(json.error !== 0) {
      alert("Search error!");
      return;
    }
    Search.table.items = json.result;
    Search.table.display();
    $(function() {
      $(Search.table.ui.items + " tr").draggable(Config.draggableOptions);
    });    
  }
};


/* FIXME: correct artist/album handler in future */
var Artist = {
  albumItems: [
    {track: "1", title: "Maja min maja", duration: "3:54" },
    {track: "2", title: "Jag vill bo i en svamp", duration: "3:24" },
    {track: "3", title: "Olyckan", duration: "2:59" },
    {track: "4", title: "Maja min maja", duration: "3:54" },
    {track: "5", title: "Maja min maja", duration: "3:54" },
    {track: "6", title: "Maja min maja", duration: "3:54" }                 
  ],
  init: function() {
    Artist.table = new ResultTable({ name: 'Album' });
    Artist.table.empty();
    $(document).bind("Page.artist", Artist.setPage);
    $(document).bind("Page.album", function() { Artist.setPage(); });
  },
  setPage: function() {
    Titlebar.set(Dogbone.page.title);
    $("#"+Dogbone.page.title + "-title").text(Dogbone.page.param);
    if(Dogbone.page.title == "Album") {
      Artist.table.items = Artist.albumItems;
      Artist.table.display();
      var url = Dogvibes.albumArt("spotify://spotify:track:5ZINxPiu71f39pOp0qbTl4");
      $("#Album-art").attr('src', url);
    }
  }
}

/***************************
 * Keybindings 
 ***************************/
 
/* CTRL+s for searching */ 
$(document).bind("keyup", "ctrl+s", function() {
  $(Search.ui.input).focus();
});

$(document).bind("keyup", "ctrl+p", function() {
  PlayControl.toggle();
});

$(document).dblclick(function() {
  var sel;
  if(document.selection && document.selection.empty){
    document.selection.empty() ;
  } else if(window.getSelection) {
    sel=window.getSelection();
  }
  if(sel && sel.removeAllRanges) {
    sel.removeAllRanges();
  }
});

/***************************
 * Startup 
 ***************************/
$(document).ready(function() {
  
  /* Zebra stripes for all tables */
  $.tablesorter.defaults.widgets = ['zebra'];

  Dogbone.init("content");
  ConnectionIndicator.init();
  PlayControl.init();
  TrackInfo.init();
  /* Init in correct order */
  Playqueue.init();
  Main.init();
  Search.init();
  Playlist.init();
  Artist.init();
  /* Start server connection */
  Dogvibes.init(Config.defaultServer, Config.defaultUser);
  
  /****************************************
   * Misc. behaviour. Application specific
   ****************************************/
   
  /* FIXME:  */
  $(UI.trackinfo).click(function() {
    $(UI.navigation).toggleClass('fullHeight');
    $(UI.currentsong).toggleClass('minimized');
  });
  
  /* Splitter */
  $("#separator").draggable( {
    containment: [150, 0, 300, 0],
    axis: 'x',
    drag: PanelSplit.drag
  });
  
}); 

var PanelSplit = {
  left: 180,
  drag: function(event, ui) {
    PanelSplit.set(ui.position.left);
  },
  set: function(left) {
    $(".resizable-right").css("width", left+"px");
    $(".resizable-left").css("left", left+"px");
    $(".resizable-top").each(function(i, el) {
      var height = $(el).height();
      $(el).height(height + (left - PanelSplit.left));
    });    
    $(".resizable-bottom").each(function(i, el) {
      var height = $(el).css("bottom");
      height = parseInt(height.substring(0, height.indexOf("px")));
      $(el).css("bottom", (height + (left - PanelSplit.left)) + "px");
    });
    PanelSplit.left = left;
  }
};

window.onbeforeunlod = function() {
  return "Are you sure you want to leave?";
}
