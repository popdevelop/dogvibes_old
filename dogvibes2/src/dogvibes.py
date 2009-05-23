import gobject
import gst
import hashlib
import os

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

    def API_getAlbumArt(self, uri):
        track = self.CreateTrackFromUri(uri)
        if (track == None):
            return -1 # could not queue, track not valid

        art_dir = 'albumart'
        if not os.path.exists(art_dir):
            os.mkdir(art_dir)

        img_hash = hashlib.sha224(track.artist + track.album).hexdigest()
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

    # FIXME: all playlist handling are just dummies for now

    playlist_tracks = []

    def API_createPlaylist(self, name):
        pass

    def API_addTrackToPlaylist(self, nbr, uri):
        nbr = int(nbr)

        track = self.CreateTrackFromUri(uri)
        if (track == None):
            return -1 # could not queue, track not valid

        self.playlist_tracks.append(track)


    def API_getAllPlaylists(self):
        return ['Favourites',
                'Roskilde 2009',
                'A very long name for being a playlist name',
                'Club',
                'House',
                'CD collection']

    def API_getAllTracksInPlaylist(self, nbr):
        nbr = int(nbr)
        return [track.to_dict() for track in self.playlist_tracks]


import re
import urllib

class AlbumArtDownloader(object):
  awsurl = "http://ecs.amazonaws.com/onca/xml"
  def __init__(self, artist, album):
    self.artist = artist
    self.album = album

  def fetch(self):
    url = self._GetResultURL(self._SearchAmazon())
    if not url:
      return None
    img_re = re.compile(r'''registerImage\("original_image", "([^"]+)"''')
    prod_data = urllib.urlopen(url).read()
    m = img_re.search(prod_data)
    if not m:
      return None
    img_url = m.group(1)
    return urllib.urlopen(img_url).read()

  def _SearchAmazon(self):
    data = {
      "Service": "AWSECommerceService",
      "Version": "2005-03-23",
      "Operation": "ItemSearch",
      "ContentType": "text/xml",
      "SubscriptionId": "AKIAIQ74I7SUW5COGZCQ",
      "SearchIndex": "Music",
      "ResponseGroup": "Small",
    }

    data["Artist"] = self.artist
    data["Keywords"] = self.album

    fd = urllib.urlopen("%s?%s" % (self.awsurl, urllib.urlencode(data)))

    return fd.read()

  def _GetResultURL(self, xmldata):
    url_re = re.compile(r"<DetailPageURL>([^<]+)</DetailPageURL>")
    m = url_re.search(xmldata)
    return m and m.group(1)
