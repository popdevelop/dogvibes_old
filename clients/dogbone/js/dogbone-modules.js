/*
 * Modules for handling playlists, searches, etc...
 */

var Config = {
  resizeable: true
};

var UI = {
  titlebar: "#titlebar",
  navigation: "#navigation",
  searchform: "#Search-form",
  init: function() {

  }
};

var Titlebar = {
  set: function(text) {
    $(UI.titlebar).empty();
    $(UI.titlebar).append($("<li class='selected'>"+text+"</li>"));
  }
};

String.prototype.removePrefix = function(prefix) {
  if(this.indexOf(prefix) == 0) {
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

var NavList = {
  /* Store all sections globally */
  sections: Array(),
  /* Section object */  
  Section: function(type) {
    this.ul = $("<ul class='"+type+"'></ul>");
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
      this.items[id].addClass('selected');
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


var Main = {
  ui: Array(),
  pages: [{id: "home", title: "Home"}, 
          {id: "playqueue", title: "Play queue"}],
  init: function() {
    Main.ui.list = new NavList.Section('');
    $(Main.pages).each(function(i, el){
      Main.ui.list.addItem(el.id, $("<a href='#"+el.id+"'>"+el.title+"</a>"));
      $(document).bind("Page."+el.id, Main.setPage);
    });
  },
  setPage: function() {
    Titlebar.set(Dogbone.page.title);
    Main.ui.list.selectItem(Dogbone.page.id);
  }
};

var TrackInfo = {
  myName: "#TrackInfo",
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
    $(TrackInfo.ui.artist).val(Dogvibes.status.artist);
    $(TrackInfo.ui.title).val(Dogvibes.status.title);    
  }
}

var Playlist = {
  ui: {
    content: "#Playlist-content",
    items:   "#Playlist-items"
  },
  fields: [],
  param: "",
  init: function() {
    Playlist.ui.list = new NavList.Section('playlists');
    Playlist.ui.newList = new NavList.Section('last');
    Playlist.ui.newBtn  = $("<a>New playlist</a>");
    Playlist.ui.newBtn.click(function() {
      var name = prompt("Enter new playlist name");
      if(name) {
        Dogvibes.createPlaylist(name, Playlist.fetchAll);
      }
    });
    Playlist.ui.newList.addItem('newlist', Playlist.ui.newBtn);
    
    /* Configure playlist item table fields by looking for table headers */
    $(Playlist.ui.content + " th").each(function(i, el){
      if("id" in el) {
        var field = el.id.removePrefix("Playlist-");
        if(field !== false) {
          Playlist.fields.push(field);
        }
      }
    });
    
    /* Setup events */
    $(document).bind("Page.playlist", Playlist.setPage);
    $(document).bind("Status.playlist", Playlist.fetchAll);
    $(document).bind("Server.connected", Playlist.fetchAll);     
  },
  setPage: function() {
    Playlist.ui.list.selectItem(Dogbone.page.param);
    Titlebar.set(Dogbone.page.title);
    if(Playlist.param != Dogbone.page.param) {
      Playlist.param = Dogbone.page.param;
      $(Playlist.ui.items).empty();
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
    Playlist.result = json.result;
    Playlist.displayResult();
  },
  displayResult: function() {
    $(Playlist.result).each(function(i, el) {
      var tr = $("<tr></tr>");
      for(var i in Playlist.fields) {
        var content = $("<td></td>");
        if(Playlist.fields[i] in el) {
          var value = el[Playlist.fields[i]];
          /* FIXME: assumption: Convert to time string if value is numeric */
          if(typeof(value) == "number") {
            value = value.msec2time();
            content.addClass("time");
          }        
          content.append(value);
        }
        tr.append(content);
      }
      $(Playlist.ui.items).append(tr);
    });
    $(Playlist.ui.items + " tr:odd").addClass("odd");  
  }
};

var Search = {
  ui: {
    content: "#Search-content",
    items:   "#Search-items"
  },
  fields: [],
  pages: ["search"],
  searches: Array(),
  param: "",
  result: false,
  init: function() {
    /* Init search navigation section */
    Search.ui.list = new NavList.Section('search');
    
    $(document).bind("Page.search", Search.setPage);
    $(UI.searchform).submit(function(e) {
      Search.doSearch($("#Search-input").val());
      e.preventDefault();
      return false;
    });
    /* Load searches from cookie */
    var temp;
    for(var i = 0; i < 6; i++){
      if((temp = getCookie("Dogvibes.search" + i)) != "") {
        Search.searches[i] = temp;
      }
    }
    Search.draw(); 
    
    /* Configure search item table fields by looking for table headers */
    $(Search.ui.content + " th").each(function(i, el){
      if("id" in el) {
        var field = el.id.removePrefix("Search-");
        if(field !== false) {
          Search.fields.push(field);
        }
      }
    });     
       
  },
  setPage: function() {
    /* See if search parameter has changed. If so, reload */
    if(Dogbone.page.param != Search.param) {
      Search.param = Dogbone.page.param;
      Search.searches.push(Search.param);
      $(Search.ui.items).empty();
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
    if(json.error !== 0) {
      alert("Search error!");
      return;
    }
    Search.result = json.result;
    Search.displayResult();
  },
  displayResult: function() {
    $(Search.result).each(function(i, el) {
      var tr = $("<tr></tr>");
      for(var i in Search.fields) {
        var content = $("<td></td>");
        if(Search.fields[i] in el) {
          var value = el[Search.fields[i]];
          /* FIXME: assumption: Convert to time string if value is numeric */
          if(typeof(value) == "number") {
            value = value.msec2time();
            content.addClass("time");
          }
          content.append(value);
        }
        tr.append(content);
      }
      $(Search.ui.items).append(tr);
    });
    $(Search.ui.items + " tr:odd").addClass("odd");  
  }
};


$(document).ready(function() {
  UI.init();
  Dogbone.init("content");
  TrackInfo.init();
  /* Init in correct order */
  Main.init();
  Search.init();
  Playlist.init();
  /* Start server connection */
  Dogvibes.init('http://192.168.0.60:2000');
});