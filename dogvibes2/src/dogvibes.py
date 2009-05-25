import gobject
import gst
import os

from amp import Amp

# import spources
from spotifysource import SpotifySource
from lastfmsource import LastFMSource
from filesource import FileSource
from albumart import AlbumArt

# import speakers
from devicespeaker import DeviceSpeaker

from track import Track
from playlist import Playlist

class DogError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class Dogvibes():
    def __init__(self):
        # add all sources
        self.sources = [SpotifySource("spotify", "gyllen", "bobidob20"),
                        LastFMSource("lastfm", "dogvibes", "futureinstereo"),
                        FileSource("filesource", "../testmedia/")]

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
                return track
        raise DogError, 'Could not create track from URI'

    # API

    def API_search(self, query):
        ret = []
        for source in self.sources:
            ret += source.search(query)
        return ret

    def API_getAlbumArt(self, uri, size = 0):
        try:
            track = self.create_track_from_uri(uri)
            return AlbumArt.get_image(track.artist, track.album, size)
        except DogError:
            return AlbumArt.get_standard_image(size)

    def API_createPlaylist(self, name):
        Playlist.create(name)

    def API_removePlaylist(self, id):
        Playlist.remove(id)

    def API_addTrackToPlaylist(self, playlist_id, uri):
        track = self.create_track_from_uri(uri)
        playlist = Playlist.get(playlist_id)
        return playlist.add_track(track)

    def API_removeTrackFromPlaylist(self, playlist_id, track_id):
        playlist = Playlist.get(playlist_id)
        playlist.remove_track(track_id)

    def API_getAllPlaylists(self):
        return [playlist.to_dict() for playlist in Playlist.get_all()]

    def API_getAllTracksInPlaylist(self, playlist_id):
        playlist = Playlist.get(playlist_id)

        # TODO: (track_id, uri) are returned from playlist. Should return tracks.
        tracks = []
        for uri in playlist.get_all_tracks():
            track = self.create_track_from_uri(uri[1]).__dict__
            track['id'] = uri[0]
            tracks.append(track)
        return tracks
