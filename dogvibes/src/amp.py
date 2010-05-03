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
        self.pipeline = gst.Pipeline ("amppipeline")

        # create the tee element
        self.tee = gst.element_factory_make("tee", "tee")
        self.pipeline.add(self.tee)

        # listen for EOS
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message', self.pipeline_message)

        self.tmpqueue = []
        self.intmpqueue = True

        # sources connected to the amp
        self.sources = []

        # the gstreamer source that is currently used for playback
        self.src = None

        self.active_playlist_id = -1
        self.active_playlist_position = 0

        self.needs_push_update = False

        # set playlist mode to normal
        self.playlist_mode = "normal"

    # Soon to be API
    def connect_source(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.dogvibes.sources) - 1:
            logging.warning ("Connect source - source does not exist")

        if self.dogvibes.sources[nbr].amp != None:
            logging.warning ("Connect source - source is already connected to amp")

        # Add amp as owner of source
        self.dogvibes.sources[nbr].amp = self
        self.sources.append(self.dogvibes.sources[nbr])

    def disconnect_source(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.dogvibes.sources) - 1:
            logging.warning ("Disonnect source - source does not exist")
        # FIXME implament me

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
        return [track.__dict__ for track in self.tmpqueue]

    def API_getPlayedMilliSeconds(self):
        (pending, state, timeout) = self.pipeline.get_state ()
        if (state == gst.STATE_NULL):
            return 0
        try:
            pos = (pos, form) = self.pipeline.query_position(gst.FORMAT_TIME)
        except:
            pos = 0
        return pos / 1000 # / gst.MSECOND # FIXME: something fishy here...

    def API_getStatus(self):
        status = {}

        # FIXME this should be speaker specific
        status['volume'] = self.dogvibes.speakers[0].get_volume()
        status['playqueuehash'] = self.get_hash_from_play_queue()

        track = None

        if self.intmpqueue:
            if len(self.tmpqueue) > 0:
                track = self.tmpqueue[0]
                status['index'] = 0
                status['playlist_id'] = -1
        else:
            playlist = Playlist.get(self.active_playlist_id)
            if self.active_playlist_id != -1 and playlist.length() > 0:
                track = playlist.get_track_nbr(self.active_playlist_position)
                status['index'] = self.active_playlist_position
                status['playlist_id'] = self.active_playlist_id

        status['uri'] = "dummy"

        if track != None:
            status['uri'] = track.uri
            status['title'] = track.title
            status['artist'] = track.artist
            status['album'] = track.album
            status['duration'] = int(track.duration)
            status['elapsedmseconds'] = self.API_getPlayedMilliSeconds()

        (pending, state, timeout) = self.pipeline.get_state()
        if state == gst.STATE_PLAYING:
            status['state'] = 'playing'
        elif state == gst.STATE_NULL:
            status['state'] = 'stopped'
        else:
            status['state'] = 'paused'

        return status

    def API_getQueuePosition(self):
        return self.active_playlist_position

    def API_nextTrack(self):
        if (len(self.tmpqueue) > 0 and self.intmpqueue):
            self.tmpqueue.remove(self.tmpqueue[0])

        if (len(self.tmpqueue) > 0):
            self.intmpqueue = True
            (pending, state, timeout) = self.pipeline.get_state()
            self.set_state(gst.STATE_NULL)
            self.play_only_if_null(self.tmpqueue[0])
        else:
            if (self.active_playlist_id != -1):
                self.change_track(self.active_playlist_position + 1)
            else:
                self.API_stop()
        self.needs_push_update = True

    def API_playTrack(self, playlist_id, nbr):
        # playlist_id=-1 means play queue
        nbr = int(nbr)
        playlist_id = int(playlist_id)
        if playlist_id == -1:
            if (nbr > (len(self.tmpqueue) - 1)):
                raise DogError, 'Trying to play none existing track from tmpqueue'
            self.intmpqueue = True
            for i in range(0, nbr):
                self.tmpqueue.remove(self.tmpqueue[0])
            self.set_state(gst.STATE_NULL)
            self.play_only_if_null(self.tmpqueue[0])
        else:
            self.active_playlist_id = playlist_id
            self.change_track(nbr)
            self.set_state(gst.STATE_PLAYING)
        self.needs_push_update = True

    def API_previousTrack(self):
        # TODO: stay on same place if in play queue?
        if (self.intmpqueue):
            self.change_track(self.active_playlist_position)
        else:
            self.change_track(self.active_playlist_position - 1)
        self.needs_push_update = True

    def API_play(self):
        if (len(self.tmpqueue) > 0 and (self.intmpqueue or self.active_playlist_id == -1)):
            self.intmpqueue = True
            self.play_only_if_null(self.tmpqueue[0])
        else:
            playlist = Playlist.get(self.active_playlist_id)
            if self.active_playlist_id == -1:
                pass
            elif self.active_playlist_position > playlist.length() - 1:
                raise DogError, 'Trying to play an empty tmpqueue'
            else:
                self.play_only_if_null(playlist.get_track_nbr(self.active_playlist_position))
        self.needs_push_update = True

    def API_pause(self):
        self.set_state(gst.STATE_PAUSED)
        self.needs_push_update = True

    def API_queue(self, uri):
        track = self.dogvibes.create_track_from_uri(uri)
        self.tmpqueue.append(track)
        self.needs_push_update = True

    def API_removeTrack(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.tmpqueue):
            raise DogError, 'Track not removed, tmpqueue is not that big'
        self.tmpqueue.remove(self.tmpqueue[nbr])
        self.needs_push_update = True

    def API_seek(self, mseconds):
        if self.src == None:
            return 0
        # FIXME: this *1000-hack only works for Spotify?
        self.pipeline.seek_simple (gst.FORMAT_TIME, gst.SEEK_FLAG_FLUSH, int(mseconds) * 1000);
        self.needs_push_update = True

    def API_setPlayQueueMode(self, mode):
        if (mode != "normal" and mode != "random" and mode != "repeat" and mode != "repeattrack"):
            raise DogError, "Unknown tmpqueue mode:" + mode
        self.playlist_mode = mode
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

    def change_track(self, tracknbr):
        tracknbr = int(tracknbr)

        self.intmpqueue = False

        playlist = Playlist.get(self.active_playlist_id)

        if playlist == None:
            self.set_state(gst.STATE_NULL)
            return
        elif (self.playlist_mode == "random"):
            self.active_playlist_position = random.randint(0, playlist.length() - 1)
        elif (self.playlist_mode == "repeattrack"):
            pass
        elif (tracknbr >= 0) and (tracknbr < playlist.length()):
            self.active_playlist_position = tracknbr
        elif tracknbr < 0:
            self.active_playlist_position = 0
        elif (tracknbr >= playlist.length()) and (self.playlist_mode == "repeat"):
            self.active_playlist_position = 0
        else:
            self.active_playlist_position = (playlist.length() - 1)
            self.set_state(gst.STATE_NULL)
            return

        (pending, state, timeout) = self.pipeline.get_state()
        self.set_state(gst.STATE_NULL)
        self.play_only_if_null(playlist.get_track_nbr(self.active_playlist_position))

    def get_hash_from_play_queue(self):
        ret = "dummy"
        for track in self.tmpqueue:
            ret += track.uri
        return hashlib.md5(ret).hexdigest()

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


