import gobject
import gst
import hashlib
import os

from amp import Amp

# import spources
from spotifysource import SpotifySource
from lastfmsource import LastFMSource
#from filesource import FileSource

# import speakers
from devicespeaker import DeviceSpeaker

from track import Track
from playlist import Playlist

class Dogvibes():
    def __init__(self):
        # add all sources
        self.sources = [SpotifySource("spotify", "gyllen", "bobidob20"),
                        LastFMSource("lastfm", "dogvibes", "futureinstereo")]
                        #FileSource("filesource", "../testmedia/")]

        # add all speakers
        self.speakers = [DeviceSpeaker("devicesink")]

        # add all amps
        amp0 = Amp(self)
        amp0.API_connectSpeaker(0)
        self.amps = [amp0]

    def create_track_from_uri(self, uri):
        track = None
        for source in self.sources:
            track = source.create_track_from_uri(uri);
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
        track = self.create_track_from_uri(uri)
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

    def API_createPlaylist(self, name):
        Playlist.create(name)

    def API_addTrackToPlaylist(self, playlist_id, uri):
        track = self.create_track_from_uri(uri)
        if track == None:
            return -1 # could not queue, track not valid

        playlist = Playlist.get(playlist_id)
        if playlist == None:
            return -1 # no playlist with that id
        playlist.add_track(track)
        #playlist.save()

    def API_removeTrackFromPlaylist(self, playlist_id, track_id):
        playlist = Playlist.get(playlist_id)
        if playlist == None:
            return -1 # no playlist with that id
        # TODO: return error if track_id isn't present in database
        playlist.remove_track(track_id)

    def API_getAllPlaylists(self):
        return [playlist.to_dict() for playlist in Playlist.get_all()]

    def API_getAllTracksInPlaylist(self, playlist_id):
        playlist = Playlist.get(playlist_id)
        if playlist == None:
            return -1 # no playlist with that id

        # TODO: (track_id, uri) are returned from playlist. Should return tracks.
        tracks = []
        for uri in playlist.get_all_tracks():
            track = self.create_track_from_uri(uri[1]).__dict__
            track['id'] = uri[0]
            tracks.append(track)
        return tracks


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
