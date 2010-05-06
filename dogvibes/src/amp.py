import gst
import hashlib
import random
import time
import logging

from track import Track
from playlist import Playlist

class DogError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class Amp():
    def __init__(self, dogvibes, id):
        self.dogvibes = dogvibes
        self.pipeline = gst.Pipeline("amppipeline" + id)

        # create the tee element
        self.tee = gst.element_factory_make("tee", "tee")
        self.pipeline.add(self.tee)

        # listen for EOS
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message', self.pipeline_message)

        # Create amps playqueue
        if Playlist.name_exists(dogvibes.ampdbname + id) == False:
            self.dogvibes.API_createPlaylist(dogvibes.ampdbname + id)
        tqplaylist = Playlist.get_by_name(dogvibes.ampdbname + id)
        self.tmpqueue_id = tqplaylist.id

        self.active_playlist_id = self.tmpqueue_id
        if (tqplaylist.length() > 0):
            self.active_playlists_track_id = tqplaylist.get_track_nbr(0).ptid
        else:
            self.active_playlists_track_id = -1
        self.fallback_playlist_id = -1
        self.fallback_playlists_track_id = -1

        # sources connected to the amp
        self.sources = []

        # the gstreamer source that is currently used for playback
        self.src = None

        self.needs_push_update = False

    # Soon to be API
    def connect_source(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.dogvibes.sources) - 1:
            logging.warning ("Connect source - source does not exist")
            return

        if self.dogvibes.sources[nbr].amp != None:
            logging.warning ("Connect source - source is already connected to amp")
            return

        # Add amp as owner of source
        self.dogvibes.sources[nbr].amp = self
        self.sources.append(self.dogvibes.sources[nbr])

    def disconnect_source(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.dogvibes.sources) - 1:
            logging.warning ("Disonnect source - source does not exist")
            return

        if self.dogvibes.sources[nbr].amp == None:
            logging.warning ("Source has no owner")
            return      
        if self.dogvibes.sources[nbr].amp != self:
            logging.warning ("Amp not owner of this source")
            return

        self.sources.remove(self.dogvibes.sources[nbr])

    # API
    def API_connectSpeaker(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.dogvibes.speakers) - 1:
            logging.warning ("Connect speaker - speaker does not exist")

        speaker = self.dogvibes.speakers[nbr]

        if self.pipeline.get_by_name(speaker.name) == None:
            self.sink = self.dogvibes.speakers[nbr].get_speaker()
            self.pipeline.add(self.sink)
            self.tee.link(self.sink)
        else:
            logging.debug ("Speaker %d already connected" % nbr)
        #self.needs_push_update = True
        # FIXME: activate when client connection has been fixed!

    def API_disconnectSpeaker(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.dogvibes.speakers) - 1:
            logging.warning ("disconnect speaker - speaker does not exist")

        speaker = self.dogvibes.speakers[nbr];

        if self.pipeline.get_by_name(speaker.name) != None:
            (pending, state, timeout) = self.pipeline.get_state()
            self.set_state(gst.STATE_NULL)
            rm = self.pipeline.get_by_name(speaker.name)
            self.pipeline.remove(rm)
            self.tee.unlink(rm)
            self.set_state(state)
        else:
            logging.warning ("disconnect speaker - speaker not found")
        self.needs_push_update = True

    def API_getAllTracksInQueue(self):
        return self.dogvibes.API_getAllTracksInPlaylist(self.tmpqueue_id)

    def API_getPlayedMilliSeconds(self):
        (pending, state, timeout) = self.pipeline.get_state ()
        if (state == gst.STATE_NULL):
            logging.debug ("getPlayedMilliseconds in state==NULL")
            return 0
        try:
            pos = (pos, form) = self.pipeline.query_position(gst.FORMAT_TIME)
        except:
            pos = 0
        # We get nanoseconds from gstreamer elements, convert to ms
        return pos / 1000000

    def API_getStatus(self):
        status = {}

        # FIXME this should be speaker specific
        status['volume'] = self.dogvibes.speakers[0].get_volume()
        status['playlistversion'] = Playlist.get_version()

        playlist = self.fetch_active_playlist()

        # -1 is in tmpqueue
        if self.is_in_tmpqueue():
            status['playlist_id'] = -1
        else:
            status['playlist_id'] = self.active_playlist_id

        track = self.fetch_active_track()            
        if track != None:
            status['uri'] = track.uri
            status['title'] = track.title
            status['artist'] = track.artist
            status['album'] = track.album
            status['duration'] = int(track.duration)
            status['elapsedmseconds'] = self.API_getPlayedMilliSeconds()
            status['id'] = self.active_playlists_track_id
            status['index'] = track.position - 1
        else:
            status['uri'] = "dummy"

        (pending, state, timeout) = self.pipeline.get_state()
        if state == gst.STATE_PLAYING:
            status['state'] = 'playing'
        elif state == gst.STATE_NULL:
            status['state'] = 'stopped'
        else:
            status['state'] = 'paused'

        return status

    def API_nextTrack(self):
        self.change_track(1, True)
        self.needs_push_update = True

    def API_playTrack(self, playlist_id, nbr):
        nbr = int(nbr)
        playlist_id = int(playlist_id)

        # -1 is tmpqueue
        if (playlist_id == -1):
            # Save last known playlist that is not the tmpqueue
            if (not self.is_in_tmpqueue()):
                self.fallback_playlist_id = self.active_playlist_id
                self.fallback_playlists_track_id = self.active_playlists_track_id
            self.active_playlist_id = self.tmpqueue_id
        else:
            self.active_playlist_id = playlist_id

        self.change_track(nbr, False)
        self.needs_push_update = True

    def API_previousTrack(self):
        self.change_track(-1, True)
        self.needs_push_update = True

    def API_play(self):
        playlist = self.fetch_active_playlist()
        track = self.fetch_active_track()
        if track != None:
            self.play_only_if_null(track)      
        self.needs_push_update = True

    def API_pause(self):
        self.set_state(gst.STATE_PAUSED)
        self.needs_push_update = True

    def API_queue(self, uri):
        track = self.dogvibes.create_track_from_uri(uri)
        playlist = Playlist.get(self.tmpqueue_id)
        playlist.add_track(track)
        self.needs_push_update = True

    def API_removeTrack(self, nbr):
        nbr = int(nbr)

        # For now if we are trying to remove the existing playing track. Do nothing.
        if (nbr == self.active_playlist_id):
            logging.warning("Not allowed to remove playing track")
            return

        playlist = Playlist.get(self.tmpqueue_id)
        playlist.remove_track_id(nbr)
        self.needs_push_update = True

    def API_seek(self, mseconds):
        if self.src == None:
            return 0
        ns = int(mseconds) * 1000000
        logging.debug("Seek with time to ns=%d" %ns)
        self.pipeline.seek_simple (gst.FORMAT_TIME, gst.SEEK_FLAG_FLUSH, ns)
        self.needs_push_update = True

    def API_setVolume(self, level):
        level = float(level)
        if (level > 1.0 or level < 0.0):
            raise DogError, 'Volume must be between 0.0 and 1.0'
        self.dogvibes.speakers[0].set_volume(level)
        self.needs_push_update = True

    def API_stop(self):
        self.set_state(gst.STATE_NULL)
        self.needs_push_update = True

    # Internal functions

    def pad_added(self, element, pad, last):
        logging.debug("Lets add a speaker we found suitable elements to decode")
        pad.link(self.tee.get_pad("sink"))

    def change_track(self, tracknbr, relative):
        tracknbr = int(tracknbr)

        if relative and (tracknbr > 1 or tracknbr < -1):
            raise DogError, "Relative change track greater/less than 1 not implemented"

        playlist = self.fetch_active_playlist()

        track = self.fetch_active_track()
        if track == None:
            logging.warning("Could not find any active track")
            return

        # If we are in tmpqueue either removetrack or push it to the top
        if self.is_in_tmpqueue():
            if relative and (tracknbr == 1):
                # Remove track and goto next track
                playlist.remove_track_id(self.active_playlists_track_id)
                next_position = 0
            elif relative and (tracknbr == -1):
                # Do nothing since we are always on top in playqueue
                return
            else:
                # Move requested track to top of tmpqueue and play it
                self.active_playlists_track_id = tracknbr
                playlist.move_track(self.active_playlists_track_id, 1)
                next_position = 0

            # Check if tmpqueue no longer exists (all tracks has been removed)
            if playlist.length() <= 0:
                # Check if we used to be in a playlist
                if self.fallback_playlist_id != -1:
                    # Change one track forward in the playlist we used to be in
                    self.active_playlist_id = self.fallback_playlist_id
                    self.active_playlists_track_id = self.fallback_playlists_track_id
                    playlist = Playlist.get(self.active_playlist_id)
                    next_position = playlist.get_track_id(self.active_playlists_track_id).position - 1
                    next_position = next_position + 1
                    if next_position >= playlist.length():
                        # We were the last song in the playlist we used to be in, just stop everyting
                        self.set_state(gst.STATE_NULL)
                        return
                else:
                    # We have not entered any playlist yet, just stop playback
                    self.set_state(gst.STATE_NULL)
                    return
        elif (Playlist.get(self.tmpqueue_id).length() > 0) and relative:
            # Save the playlist that we curently are playing in for later use
            self.fallback_playlist_id = self.active_playlist_id
            self.fallback_playlists_track_id = self.active_playlists_track_id
            # Switch to playqueue
            self.active_playlist_id = self.tmpqueue_id
            playlist = Playlist.get(self.active_playlist_id)
            next_position = 0
        else:
            # We are inside a playlist
            if relative:
                next_position = track.position - 1 + tracknbr
            else:
                try:
                    next_position = playlist.get_track_id(tracknbr).position - 1
                except:
                    self.set_state(gst.STATE_NULL)
                    self.active_playlists_track_id = -1
                    logging.warning("Could not find this id in the active playlist")
                    self.set_state(gst.STATE_NULL)
                    return                

        try:
            track = playlist.get_track_nbr(next_position)
        except:
            self.set_state(gst.STATE_NULL)
            self.active_playlists_track_id = -1
            logging.debug("Could not get to next posiiton in the active playlist")
            return

        self.active_playlists_track_id = track.ptid
        self.set_state(gst.STATE_NULL)
        self.play_only_if_null(playlist.get_track_id(self.active_playlists_track_id))

    def pipeline_message(self, bus, message):
        t = message.type
        if t == gst.MESSAGE_EOS:
            self.API_nextTrack()
            self.needs_push_update = True
            # TODO: is this enough? An update is pushed to the clients
            # but will the info be correct?

    def play_only_if_null(self, track):
        (pending, state, timeout) = self.pipeline.get_state()
        if state != gst.STATE_NULL:
            self.set_state(gst.STATE_PLAYING)

        if self.src:
            self.pipeline.remove(self.src)
            if self.pipeline.get_by_name("decodebin2") != None:
                self.pipeline.remove(self.decodebin)

        self.src = None

        for source in self.sources:
            if source.uri_matches(track.uri):
                self.src = source.get_src()
                source.set_track(track)
                self.pipeline.add(self.src)
                self.src.link(self.tee)

        # Try decode bin if there where no match within the sources
        if self.src == None:
            logging.debug ("Decodebin is taking care of this uri")
            self.src = gst.element_make_from_uri(gst.URI_SRC, track.uri, "source")
            self.decodebin = gst.element_factory_make("decodebin2", "decodebin2")
            self.decodebin.connect('new-decoded-pad', self.pad_added)
            self.pipeline.add(self.src)
            self.pipeline.add(self.decodebin)
            self.src.link(self.decodebin)

        self.set_state(gst.STATE_PLAYING)

    def set_state(self, state):
        logging.debug("set state try: "+str(state))
        res = self.pipeline.set_state(state)
        if res != gst.STATE_CHANGE_FAILURE:
            (pending, res, timeout) = self.pipeline.get_state()
            while (res != state):
                print res
                time.sleep(0.1)
                (pending, res, timeout) = self.pipeline.get_state()
            logging.debug("set state success: "+ str(state))
        else:
            logging.warning("set state failure: "+ str(state))
        return res

    def is_in_tmpqueue(self):
        return (self.tmpqueue_id == self.active_playlist_id)

    def fetch_active_playlist(self):
        try:
            playlist = Playlist.get(self.active_playlist_id)
            return playlist
        except:
            # The play list have been removed or disapperd use tmpqueue as fallback
            self.active_playlist_id = self.tmpqueue_id
            self.active_playlists_track_id = 0
            playlist = Playlist.get(self.active_playlist_id)
            return playlist

    def fetch_active_track(self):
        # Assume that fetch active playlist alreay been run
        playlist = Playlist.get(self.active_playlist_id)
        if playlist.length() <= 0:
            return None

        if self.active_playlists_track_id != -1:
            try:
                track = playlist.get_track_id(self.active_playlists_track_id)
                return track
            except:
                logging.debug("Could not get active track")
                return None
        else:
            # Try the first active_play_list id
            track = playlist.get_track_nbr(0)
            self.active_playlists_track_id = track.ptid
            return track

