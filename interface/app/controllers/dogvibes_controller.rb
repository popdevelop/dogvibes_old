class DogvibesController < ApplicationController
  require "dbus"

  def search
    render(:text => "Searched for <b>" + params[:query] + "</b>")
    get_dogvibes.Search(params[:query])[0].each { |song|
      unless(song.empty?)
        splitted = song.split(",")
        s = {:title => splitted[1], :artist => splitted[0], :key => splitted[2]}
        songs << s
      end
    }

    songs = [
             { :title => 'Wonderwall', :artist => 'Oasis', :key => "C:/Music/Oasis.mp3" },
             { :title => 'One', :artist => 'Metallica', :key => "C:/Music/Metallica.mp3" }
            ]

    # return in JSONP (JSON with padding) format
    render :json => params[:callback] + '(' + songs.to_json + ')'
  end

  private

  def get_dogvibes
    bus = DBus::SystemBus.instance
    service = bus.service("com.Dogvibes")
    obj = service.object("/com/dogvibes/dogvibes")
    obj.introspect
    obj.default_iface = "com.Dogvibes.Dogvibes"
    return obj
  end

end
