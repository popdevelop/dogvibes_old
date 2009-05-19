import dbus
import gst
from track import Track

class SpotifySource:
    def __init__(self, name, user, passw):
        self.name = name
        self.passw = passw
        self.user = user
        self.created = False

        bus = dbus.SystemBus()
        self.proxy = bus.get_object('com.Dogvibes',
                                    '/com/dogvibes/dogvibes')

    def get_src(self):
        if self.created == False:
            self.bin = gst.Bin(self.name)
            print "Logging on to spotify"
            self.spotify = gst.element_factory_make("spotify", "spotify")
            self.spotify.set_property ("user", self.user);
            self.spotify.set_property ("pass", self.passw);
            self.spotify.set_property ("buffer-time", 100000000);
            self.bin.add(self.spotify)
            gpad = gst.GhostPad("src", self.spotify.get_static_pad("src"))
            self.bin.add_pad(gpad)
            self.created = True
        return self.bin

    def search (self, query):
        a = self.proxy.Search(query, dbus_interface='com.Dogvibes.SpotifySearch', utf8_strings = True)
        r = []
        for i in a:
            k = map(lambda x: str(x), i.keys())
            v = map(lambda x: str(x), i.values())
            d = dict(zip(k,v))
            r.append(d)
        return r

    def set_track (self, track):
        self.spotify.set_property ("spotifyuri", track.uri)
