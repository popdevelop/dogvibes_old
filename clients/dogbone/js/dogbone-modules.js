/*
 * Modules for handling playlists, searches, etc...
 */

var Config = {
  resizeable: true
};

var UI = {
  titlebar: "#titlebar",
  navigation: "#navigation",
  searchform: "#searchform",
  init: function() {

  }
};

var Titlebar = {
  set: function(text) {
    $(UI.titlebar).empty();
    $(UI.titlebar).append($("<li class='selected'>"+text+"</li>"));
  }
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

var Playlist = {
  ui: Array(),
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
    $(document).bind("Page.playlist", Playlist.setPage);
    $(document).bind("Status.playlist", Playlist.fetchAll);
    $(document).bind("Server.connected", Playlist.fetchAll);     
  },
  setPage: function() {
    Playlist.ui.list.selectItem(Dogbone.page.param);
    Titlebar.set(Dogbone.page.title);
    if(Playlist.param != Dogbone.page.param) {
      Playlist.param = Dogbone.page.param;
      $('#playlist').empty();
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
  /* FIXME:  */
  handleResponse: function(json) {
    if(json.error !== 0) {
      alert("Couldn't get playlist");
      return;
    }
    Playlist.result = json.result;
    var tbl = $("<table></table>");
    $(Playlist.result).each(function(i, el) {
      tbl.append($("<tr><td>"+el.title+"</td></tr>"));
    });
    $("#playlist").append(tbl);   
  }
};

var Search = {
  ui: Array(),
  pages: ["search"],
  searches: Array(),
  param: "",
  result: false,
  init: function() {
    /* Init search navigation section */
    Search.ui.list = new NavList.Section('search');
    
    $(document).bind("Page.search", Search.setPage);
    $(UI.searchform).submit(function(e) {
      Search.doSearch($("#searchfield").val());
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
  },
  setPage: function() {
    /* See if search parameter has changed. If so, reload */
    if(Dogbone.page.param != Search.param) {
      Search.param = Dogbone.page.param;
      Search.searches.push(Search.param);
      $("#search").empty();
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
  /* FIXME:  */  
  handleResponse: function(json) {
    if(json.error !== 0) {
      alert("Error in search: " + Search.param);
      return;
    }
    Search.result = json.result;
    var tbl = $("<table></table>");
    $(Search.result).each(function(i, el) {
      tbl.append($("<tr><td>"+el.title+"</td></tr>"));
    });
    $("#search").append(tbl);
  }
};


$(document).ready(function() {
  UI.init();
  Dogbone.init("content");
  /* Init in correct order */
  Main.init();
  Search.init();
  Playlist.init();
  /* Start server connection */
  Dogvibes.init('http://192.168.0.60:2000');
});