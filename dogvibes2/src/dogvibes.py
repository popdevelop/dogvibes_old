import gobject
import gst

from amp import Amp

# import spources
from spotifysource import SpotifySource
from lastfmsource import LastFMSource

# import speakers
from devicespeaker import DeviceSpeaker

from track import Track

class Dogvibes():
    def __init__(self):
        # add all sources
        self.sources = [SpotifySource("spotify", "gyllen", "bobidob20"),
                        LastFMSource("lastfm", "dogvibes", "futureinstereo")]

        # add all speakers
        self.speakers = [DeviceSpeaker("devicesink")]

        # add all amps
        amp0 = Amp(self)
        amp0.API_connectSpeaker(0)
        self.amps = [amp0]

    def CreateTrackFromUri(self, uri):
        track = None
        for source in self.sources:
            track = source.CreateTrackFromUri(uri);
            if track != None:
                break
        return track

    # API

    def API_search(self, query):
        ret = []
        for source in self.sources:
            ret += source.search(query)
        return ret

    #import cStringIO
    #from PIL import Image

    import hashlib

    def API_getAlbumArt(self, uri):
        track = self.CreateTrackFromUri(uri)
        if (track == None):
            return -1 # could not queue, track not valid

        art_dir = 'albumart'
        img_hash = hashlib.sha224(uri).hexdigest()
        img_path = art_dir + '/' + img_hash
        if os.path.exists(img_path):
            f = open(img_path, 'rb')
            img_data = f.read()
        else:
            img_data = AlbumArtDownloader(track.artist, track.album).fetch()
            f = open(img_path, 'wb')
            f.write(img_data)

        f.close()
        return img_data

        #im = cStringIO.StringIO(img_data)
        #size = (159, 159)
        #img = Image.open(im)
        #img.thumbnail(size, Image.ANTIALIAS)
