import gobject
import gst
import os
import config
import sys

from amp import Amp

# import spources
from spotifysource import SpotifySource
from filesource import FileSource
from albumart import AlbumArt

# import speakers
from devicespeaker import DeviceSpeaker

from track import Track
from playlist import Playlist
from source import Source


class Dogvibes():
    ampdbname = "qurkloxuiikkolkjhhf"

    def __init__(self):
        try: cfg = config.load("dogvibes.conf")
        except Exception, e:
            print "ERROR: Cannot load configuration file\n"
            sys.exit(1)

        self.sources = []

        # Hackidooda and laziness to always create correct source, remove in real release
        if Source.length() > 0:
            allsources = Source.get_all()
            # FIXME this should be dynamic
            for source in allsources:
                if source.type == "spotify":
                    spotifysource = SpotifySource("spotify", source.user, source.passw)
                    self.sources.append(spotifysource)
        else:
            # This is just here because of laziness 
            if(cfg['ENABLE_SPOTIFY_SOURCE'] == '1'):
                spot_user = cfg["SPOTIFY_USER"]
                spot_pass = cfg["SPOTIFY_PASS"]
                self.API_createSpotifySource(spot_user, spot_pass)
            if(cfg['ENABLE_FILE_SOURCE'] == '1'):
                self.API_createFileSource(cfg["FILE_SOURCE_ROOT"])

        # add all speakers, should also be stored in database as sources
        self.speakers = [DeviceSpeaker("devicesink")]

        self.needs_push_update = False

        self.search_history = []

        # add all amps
        amp0 = Amp(self, "0")
        amp0.API_connectSpeaker(0)
        self.amps = [amp0]

        # add sources to amp, assume spotify source on first position, laziness
        amp0.connect_source(0)

    def create_track_from_uri(self, uri):
        track = None
        for source in self.sources:
            if source:
                track = source.create_track_from_uri(uri);
                if track != None:
                    return track
        raise ValueError('Could not create track from URI')

    # API

    def API_createSpotifySource(self, user, passw):
        spotifysource = SpotifySource("spotify", user, passw)
        # FIXME: this logs in to the spotify source for the moment
        spotifysource.get_src()
        self.sources.append(spotifysource)
        Source.add(user, passw, "spotify")

    def API_createFileSource(self, dir):
        #FIXME implement me
        #FileSource("filesource", cfg["FILE_SOURCE_ROOT"])
        return

    def API_getAllSources(self):
        pass

    def API_search(self, query):
        # FIXME: need to lock this section
        self.search_history.append(query)
        # Save 20 searches. You can then limit this in your API call
        if len(self.search_history) > 20:
            self.search_history.pop(0)
        ret = []
        self.needs_push_update = True

        for source in self.sources:
            if source:
                ret += source.search(query)
        return ret

    def API_list(self, type):
        ret = []
        for source in self.sources:
            if source:
                ret += source.list(type)
        return ret

    def API_getAlbumArt(self, uri, size = 0):
        try:
            track = self.create_track_from_uri(uri)
            return AlbumArt.get_image(track.artist, track.album, size)
        except ValueError:
            return AlbumArt.get_standard_image(size)

    def API_createPlaylist(self, name):
        Playlist.create(name)

    def API_removePlaylist(self, id):
        Playlist.remove(id)
        self.needs_push_update = True

    def API_addTrackToPlaylist(self, playlist_id, uri):
        track = self.create_track_from_uri(uri)
        try:
            playlist = Playlist.get(playlist_id)
        except ValueError as e:
            raise
        return playlist.add_track(track)
        self.needs_push_update = True

    # TODO: should not be named track_id since it's referring to the nbr in the list
    def API_removeTrackFromPlaylist(self, playlist_id, track_id):
        try:
            playlist = Playlist.get(playlist_id)
            playlist.remove_track_id(int(track_id))
        except ValueError as e:
            raise
        self.needs_push_update = True

    def API_getAllPlaylists(self):
        all_playlists = [playlist.to_dict() for playlist in Playlist.get_all()]
        all_playlists = filter(lambda x:x['name'][0:len(self.ampdbname)] != self.ampdbname,all_playlists)
        return all_playlists 

    def API_getAllTracksInPlaylist(self, playlist_id):
        try:
            playlist = Playlist.get(playlist_id)
        except ValueError as e:
            raise
        return [track.__dict__ for track in playlist.get_all_tracks()]

    def API_renamePlaylist(self, playlist_id, name):
        try:
            Playlist.rename(playlist_id, name)
        except ValueError as e:
            raise
        self.needs_push_update = True

    def API_moveTrackInPlaylist(self, playlist_id, track_id, position):
        try:
            playlist = Playlist.get(playlist_id)
            playlist.move_track(int(track_id), int(position))
        except ValueError as e:
            raise
        return 0
        self.needs_push_update = True

    def API_getSearchHistory(self, nbr):
        return self.search_history[-int(nbr):]
