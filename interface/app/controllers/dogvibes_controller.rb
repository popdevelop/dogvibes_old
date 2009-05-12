class DogvibesController < ApplicationController
  require "dbus"

  def search
    #get_dogvibes.Search(params[:query])[0].each { |track|
    #  unless(track.empty?)
    #    splitted = track.split(",")
    #    s = {:title => splitted[1], :artist => splitted[0], :key => splitted[2]}
    #    tracks << s
    #  end
    #}

    tracks = [
             { :title => 'Wonderwall', :artist => 'Oasis', :uri => "spotify:track:75UqWU4Y0YdCB9MrnKZZnC", :album => 'Svens Kladdlåda', :duration => 103000}
             { :title => 'One', :artist => 'Metallica', :uri => "spotify:track:7kXmJwrZGIhDaLT9sNo3ut", :album => 'Nils Kottes Bravader', :duration => 200000}
            ]

    # return in JSONP (JSON with padding) format
    if params[:callback].nil?
      render :json => tracks.to_json
    else
      render :json => params[:callback] + '(' + tracks.to_json + ')'
    end
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
