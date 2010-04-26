/*
 * Modules for handling playlists, searches, etc...
 */

var Config = {
  defaultServer: "http://192.168.1.87:2000",
  resizeable: true
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

function ResultTable(name) {
  this.name = name;
  this.ui = {
    content: "#" + name + "-content",
    items  : "#" + name + "-items"
  };
  this.items = [];
  this.selectedItem = false;
  this.fields = [];
  this.idTag = "id";
  /* Methods */
  this.display = function() {
    for(var i in this.items) {
      var el = this.items[i];
      var tr = $("<tr></tr>");
      tr.attr("id", this.name+"-item-id-"+el[this.idTag]);
      tr.attr("name", this.name+"-item-no-"+i);
      for(var j in this.fields) {
        var content = $("<td></td>");
        if(this.fields[j] in el) {
          var value = el[this.fields[j]];
          /* FIXME: assumption: Convert to time string if value is numeric */
          if(typeof(value) == "number") {
            value = value.msec2time();
            content.addClass("time");
          }        
          content.append(value);
        }
        tr.append(content);
      }
      $(this.ui.items).append(tr);
    }
    /* Zebra stripes */
    $(this.ui.items + " tr:odd").addClass("odd");
    
    /* Attach behaviours */
    var rows = $(this.ui.items + " tr");
    rows.click(this.click);
    rows.dblclick(this.dblclick);  
  };
  
  this.empty = function() {
    $(this.ui.items).empty();
  };
  
  this.selectItem = function(index) {
    $(this.ui.items + " tr").removeClass("selected");  
    Playlist.selectedItem = false;
    if(index < this.items.length) {
      this.selectedItem = $('tr[name="'+this.name+'-item-no-'+index+'"]');
      this.selectedItem.addClass("selected");
    }  
  };
  
  /* Default click handler */
  this.click = function() {    
  };
  
  this.dblclick = function() {
  };
  
  /* Configure playlist item table fields by looking for table headers */
  var cols = $(this.ui.content + " th");
  for(var i = 0; i < cols.length; i++) {
    var el = cols[i];
    if("id" in el) {
      var field = el.id.removePrefix(this.name + "-");
      if(field !== false) {
        this.fields.push(field);
      }
    }
  }  
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
      this.items[id] = $('<li class="'+id+'"></li>');
      this.items[id].append(item);
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
      for( var i in this.items) {
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
    section: "#Main-section"
  },
  init: function() {
    /* Bug in jQuery: you can't have same function attached to multiple events! */    
    Main.ui.list = new NavList.Section(Main.ui.section, '');
    Main.ui.list.addItem("home", $("<a href='#home'>Home</a>"));
    $(document).bind("Page.home", Main.setHome);        
    Main.ui.list.addItem("playqueue", $("<a href='#playqueue'>Play queue</a>"));
    $(document).bind("Page.playqueue", Main.setQueue); 
    
    /* Online / Offline */
    $(document).bind("Server.error", function() {
      $("#playqueue").addClass("disconnected");
    });
    $(document).bind("Server.connected", function() {
      $("#playqueue").removeClass("disconnected");
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
    Playqueue.table = new ResultTable('Playqueue');
    Playqueue.table.idTag = "uri";
    Playqueue.table.click = function() {
      var index = $(this).attr("name").removePrefix('Playqueue-item-no-');
      Playqueue.table.selectItem(index);       
    };
    
    Playqueue.table.dblclick = function() {
      var id = $(this).attr("name").removePrefix('Playqueue-item-no-');
      Dogvibes.playTrack(id, "-1");
    };
    
    $(document).bind("Status.playqueue", Playqueue.fetch);
    $(document).bind("Status.state", function() { Playqueue.set() });    
    $(document).bind("Status.playlist", function() { Playqueue.set() });        
    
  },
  fetch: function() {
    if(Dogvibes.status.playqueuehash != Playqueue.hash) {
      Playqueue.hash = Dogvibes.status.playqueuehash;
      Playqueue.table.empty();
      $(Playqueue.ui.page).addClass("loading");
      Dogvibes.getAllTracksInQueue(Playqueue.update);
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
  },
  set: function() {
    if(Dogvibes.status.state == "playing" &&
       Dogvibes.status.playlist_id == -1) {
      $("li.playqueue").addClass('playing'); 
      $(Playqueue.table.ui.items + " tr:first").addClass("playing");
      $(Playqueue.table.ui.items + " tr:first td:first").addClass("playing");      
    } 
    else {
      $("li.playqueue").removeClass('playing');    
      $(Playqueue.table.ui.items + " tr:first").removeClass("playing");
      $(Playqueue.table.ui.items + " tr:first td:first").removeClass("playing");      
    }
  }
};

var PlayControl = {
  ui: {
    controls: "#PlayControl",
    prevBtn : "#pb-prev",
    playBtn : "#pb-play",
    nextBtn : "#pb-next"
  },
  init: function() {
    $(document).bind("Status.state", PlayControl.set);
    
    $(PlayControl.ui.nextBtn).click(function() {
      Dogvibes.next();
    });
    $(PlayControl.ui.prevBtn).click(function() {
      Dogvibes.prev();
    });    
  },
  set: function() {
    $(PlayControl.ui.controls).removeClass();
    $(PlayControl.ui.controls).addClass(Dogvibes.status.state);
    switch(Dogvibes.status.state) {
      case "playing":
        $(PlayControl.ui.playBtn).click(function() {
          Dogvibes.pause();
        });
        break;
      case "paused":
      case "stopped":
        $(PlayControl.ui.playBtn).click(function() {
          Dogvibes.play();
        });
        break;
    }
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
    artist: "#TrackInfo-artist",
    title:  "#TrackInfo-title"    
  },
  init: function() {
    if($(TrackInfo.ui.artist) && $(TrackInfo.ui.title)) {
      $(document).bind("Status.songinfo", TrackInfo.set);
    }
  },
  set: function() {
    $(TrackInfo.ui.artist).text(Dogvibes.status.artist);
    $(TrackInfo.ui.title).text(Dogvibes.status.title);    
  }
};

var Playlist = {
  ui: {
    section:  "#Playlist-section",
    newPlist: "#Newlist-section"
  },
  table: false,
  param: "",
  init: function() {
    Playlist.ui.list =    new NavList.Section(Playlist.ui.section, 'playlists');
    Playlist.ui.newList = new NavList.Section(Playlist.ui.newPlist, 'last');
    Playlist.ui.newBtn  = $("<a>New playlist</a>");
    Playlist.ui.newBtn.click(function() {
      var name = prompt("Enter new playlist name");
      if(name) {
        Dogvibes.createPlaylist(name, Playlist.fetchAll);
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
    
    Playlist.table = new ResultTable('Playlist');
    /* Setup table behaviours */
    Playlist.table.click = function() {
      var index = $(this).attr("name").removePrefix('Playlist-item-no-');
      Playlist.table.selectItem(index);    
    };
    Playlist.table.dblclick = function() {
      Playlist.playItem($(this).attr('id').removePrefix('Playlist-item-id-'));   
    };

    /* Setup events */
    $(document).bind("Page.playlist", Playlist.setPage);
    $(document).bind("Status.playlist", Playlist.fetchAll);
    $(document).bind("Server.connected", Playlist.fetchAll);
    
    $(document).bind("Status.state", function() { Playlist.set(); });    
    $(document).bind("Status.playlist", function() { Playlist.set(); });              
  },
  setPage: function() {
    Playlist.ui.list.selectItem(Dogbone.page.param);
    Titlebar.set(Dogbone.page.title);
    if(Playlist.param != Dogbone.page.param) {
      Playlist.param = Dogbone.page.param;
      $(Playlist.table.ui.items).empty();
      Playlist.selectedItem = false;
      Dogvibes.getAllTracksInPlaylist(Playlist.param, Playlist.handleResponse);
    }
  },
  fetchAll: function() {
    Playlist.ui.list.empty();
    Dogvibes.getAllPlaylists(Playlist.update);
  },
  update: function(json) {
    if(json.error !== 0) {
      alert("Couldn't get playlists");
      return;
    }
    $(json.result).each(function(i, el) {
      Playlist.ui.list.addItem(el.id, $('<a href="#playlist/'+el.id+'">'+el.name+'</a>'));
    });
  },

  handleResponse: function(json) {
    if(json.error !== 0) {
      alert("Couldn't get playlist");
      return;
    }
    Playlist.table.items = json.result;
    Playlist.table.display();
    Playlist.set();
  },
  playItem: function(id) {
    if(parseInt(id)) {
      Dogvibes.playTrack(id, Playlist.param);
    }
    else if(Playlist.selectedItem) {
      Playlist.selectedItem.dblclick();
    }
  },
  set: function() {
    if(Dogvibes.status.state == "playing" &&
       Dogvibes.status.playlist_id != -1) {
      $('tr[name="Playlist-item-no-'+Dogvibes.status.index+'"] td:first').addClass("listplaying");      
    } 
    else {
      $('tr td').removeClass("listplaying");      
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
      $(Search.table.ui.content).show();
      $(Search.ui.page).removeClass("disconnected");
    });

    $(Search.ui.form).submit(function(e) {
      Search.doSearch($("#Search-input").val());
      e.preventDefault();
      return false;
    });
    
    /* Create result table and setup click handlers */
    Search.table = new ResultTable("Search");
    Search.table.idTag = "uri";
    Search.table.click = function() {
      var index = $(this).attr("name").removePrefix('Search-item-no-');
      Search.table.selectItem(index);     
    };
    Search.table.dblclick = function() {
      var uri = $(this).attr("id").removePrefix('Search-item-id-');
      Dogvibes.queue(uri);
    };
    
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
    /* See if search parameter has changed. If so, reload */
    if(Dogbone.page.param != Search.param) {
      Search.param = Dogbone.page.param;
      Search.searches.push(Search.param);
      Search.table.empty();
      $(Search.ui.page).addClass("loading");
      Search.addSearch(Search.param);
      
      Dogvibes.search(Search.param, Search.handleResponse);      
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
      Search.ui.list.addItem(el,"<a href='#search/"+el+"'>"+el+"</a>");
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
  }
};


/***************************
 * Keybindings 
 ***************************/
 
/* CTRL+s for searching */ 
$(document).bind("keyup", "ctrl+s", function() {
  $(Search.ui.input).focus();
});

$(document).bind("keyup", "space", function() {
  PlayControl.toggle();
});

$(document).dblclick(function() {
  var sel ;
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
  Dogbone.init("content");
  ConnectionIndicator.init();
  PlayControl.init();
  TrackInfo.init();
  /* Init in correct order */
  Playqueue.init();
  Main.init();
  Search.init();
  Playlist.init();
  /* Start server connection */
  Dogvibes.init(Config.defaultServer);
  
  /****************************************
   * Misc. behaviour. Application specific
   ****************************************/
  $(UI.trackinfo).click(function() {
    $(UI.navigation).toggleClass('fullHeight');
    $(UI.currentsong).toggleClass('minimized');
  });  
  
});