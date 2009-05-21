#!/usr/bin/env python
import os
import hashlib
import gobject
import gst
# import spources
from spotifysource import SpotifySource
from lastfmsource import LastFMSource

# import speakers
from devicespeaker import DeviceSpeaker

# import track
from track import Track

import BaseHTTPServer
from urlparse import urlparse
import cgi
import json
import sys

# web server
class APIHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        u = urlparse(self.path)
        c = u.path.split('/')
        method = c[-1]
        obj = c[1]
        id = c[2]
        id = 0 # TODO: remove when more amps are supported

        if obj == 'dogvibes':
            klass = dogvibes
        else:
            klass = dogvibes.amps[id]

        callback = None

        if hasattr(klass, method):
            params = cgi.parse_qs(u.query)
            # use only the first value for each key (feel free to clean up):
            params = dict(zip(params.keys(), map(lambda x: x[0], params.values())))

            if 'callback' in params:
                callback = params.pop('callback')

            try:
                data = getattr(klass, method).__call__(**params)
            except TypeError:
                data = dict(error = 2) # Unsupported parameter
            except NameError:
                data = dict(error = 3) # Missing parameter

            if data == None:  data = dict(error = 0)
            else:             data = dict(error = 0, result = data)

            self.send_response(200)
        else:
            self.send_response(400) # Bad request
            data = dict(error = 1)

        if sys.version_info[0] >= 2 and sys.version_info[1] >= 6:
            data = json.dumps(data)
        else:
            data = json.write(data)

        if callback != None:
            data = callback + '(' + data + ')'
            self.send_header("Content-type", "text/javascript")
        else:
            self.send_header("Content-type", "application/json")

        self.end_headers()
        self.wfile.write(data)


class API:
    def __init__(self):
        httpserver = BaseHTTPServer.HTTPServer(("", 2000), APIHandler)
        httpserver.serve_forever()

class Dogvibes():
    def __init__(self):
        # add all sources
        self.sources = [SpotifySource("spotify", "gyllen", "bobidob20"),
                        LastFMSource("lastfm", "dogvibes", "futureinstereo")]

        # add all speakers
        self.speakers = [DeviceSpeaker("devicesink")]

        # add all amps
        amp0 = Amp(self)
        amp0.connectSpeaker(0)
        self.amps = [amp0]

    def CreateTrackFromUri(self, uri):
        track = None
        for source in self.sources:
            track = source.CreateTrackFromUri(uri);
            if track != None:
                break
        return track

    # API

    def search(self, query):
        ret = []
        for source in self.sources:
            ret += source.search(query)
        return ret

class Amp():
    def __init__(self, dogvibes):
        self.dogvibes = dogvibes
        self.pipeline = gst.Pipeline ("amppipeline")

        # create volume element
        self.volume = gst.element_factory_make("volume", "volume")
        self.pipeline.add(self.volume)

        # create the tee element
        self.tee = gst.element_factory_make("tee", "tee")
        self.pipeline.add(self.tee)

        # link volume with tee
        self.volume.link(self.tee)

        # create the playqueue
        self.playqueue = []
        self.playqueue_position = 0

        self.src = None

        # spotify is special FIXME: not how its supposed to be
        self.spotify = self.dogvibes.sources[0].get_src ()
        self.lastfm = self.dogvibes.sources[1].get_src ()


    # API

    def connectSpeaker(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.dogvibes.speakers) - 1:
            print "Speaker does not exist"

        speaker = self.dogvibes.speakers[nbr];

        if (self.pipeline.get_by_name (speaker.name) == None):
            self.sink = self.dogvibes.speakers[nbr].get_speaker ();
            self.pipeline.add(self.sink)
            self.tee.link(self.sink)
        else:
            print "Speaker %d already connected" % nbr

    def disconnectSpeaker(self, nbr):
        nbr = int(nbr)
        if nbr > len(self.dogvibes.speakers) - 1:
            print "Speaker does not exist"

        speaker = self.dogvibes.speakers[nbr];

        if (self.pipeline.get_by_name (speaker.name) != None):
            (pending, state, timeout) = self.pipeline.get_state()
            self.pipeline.set_state(gst.STATE_NULL)
            rm = self.pipeline.get_by_name (speaker.name)
            self.pipeline.remove (rm)
            self.tee.unlink (rm)
            self.pipeline.set_state(state)
        else:
            print "Speaker not connected"

    def getAllTracksInQueue(self):
        ret = []
        for track in self.playqueue:
            ret.append(track.to_dict())
        return ret

    def getPlayedMilliSeconds(self):
        (pos, form) = self.pipeline.query_position(gst.FORMAT_TIME)
        return pos / gst.MSECOND

    def getStatus(self):
        if (len(self.playqueue) > 0):
            track = self.playqueue[self.playqueue_position]
            status = {'title': track.name,
                      'artist': track.artist,
                      'album': track.album,
                      'duration': track.duration}
            status['index'] = self.playqueue_position
        else:
            status = {}

        if len(self.playqueue) > 0:
            status['uri'] = self.playqueue[self.playqueue_position - 1].uri
            status['playqueuehash'] = self.GetHashFromPlayQueue()
        else:
            status['uri'] = "dummy"
            status['playqueuehash'] = "dummy"

        (pending, state, timeout) = self.pipeline.get_state()
        if state == gst.STATE_PLAYING:
            status['state'] = 'playing'
        elif state == gst.STATE_NULL:
            status['state'] = 'stopped'
        else:
            status['state'] = 'paused'
            
        return status

    def getQueuePosition(self):
        return self.playqueue_position

    def nextTrack(self):
        self.ChangeTrack(self.playqueue_position + 1)

    def playTrack(self, nbr):
        self.ChangeTrack(nbr)
        self.play()

    def previousTrack(self):
        self.ChangeTrack(self.playqueue_position - 1)

    def play(self):
        self.PlayOnlyIfNull(self.playqueue[self.playqueue_position])

    def pause(self):
        self.pipeline.set_state(gst.STATE_PAUSED)

    def queue(self, uri):
        track = self.dogvibes.CreateTrackFromUri(uri)
        if (track == None):
            return -1 #"could not queue, track not valid"
        self.playqueue.append(track)

    def removeFromQueue(self, nbr):
        nbr = int(nbr)
        if (nbr > len(self.playqueue)):
            print "Too big of a number for removing"
            return

        self.playqueue.remove(self.playqueue[nbr])

        if (nbr <= self.playqueue_position):
            self.playqueue_position = self.playqueue_position - 1

    def seek(self, mseconds):
        mseconds = int(mseconds)
        self.pipeline.seek_simple (gst.FORMAT_BYTES, gst.SEEK_FLAG_NONE, mseconds);

    def setVolume(self, level):
        level = float(level)
        if (level > 1.0 or level < 0.0):
            print "Volume must be between 0.0 and 1.0"
        self.volume.set_property("volume", level)

    def stop(self):
        self.pipeline.set_state(gst.STATE_NULL)

    # Internal functions

    def ChangeTrack(self, tracknbr):
        tracknbr = int(tracknbr)

        if (tracknbr > len(self.playqueue) - 1):
            return

        if (tracknbr == self.playqueue_position):
            return

        if (tracknbr < 0):
            tracknbr = 0

        self.playqueue_position = tracknbr
        (pending, state, timeout) = self.pipeline.get_state()
        self.pipeline.set_state(gst.STATE_NULL)
        self.PlayOnlyIfNull(self.playqueue[self.playqueue_position])
        self.pipeline.set_state(state)



    def GetHashFromPlayQueue(self):
        ret = ""
        for track in self.playqueue:
            ret += track.uri
        print hashlib.md5(ret).hexdigest()

        return hashlib.md5(ret).hexdigest()

    def PlayOnlyIfNull(self, track):
        (pending, state, timeout) = self.pipeline.get_state ()
        if (state != gst.STATE_NULL):
            self.pipeline.set_state(gst.STATE_PLAYING)
            return

        if (self.src):
            self.pipeline.remove (self.src)
            if (self.spotify_in_use == False):
                print "removed a decodebin"
                self.pipeline.remove (self.decodebin)

        if track.uri[0:7] == "spotify":
            print "It was a spotify uri"
            self.src = self.spotify
            # FIXME ugly
            self.dogvibes.sources[0].set_track(track)
            self.pipeline.add(self.src)
            self.src.link(self.volume)
            self.spotify_in_use = True
        elif track.uri == "lastfm":
            print "It was a lastfm uri"
            self.src = self.lastfm
            self.dogvibes.sources[1].set_track(track)
            self.decodebin = gst.element_factory_make ("decodebin2", "decodebin2")
            self.decodebin.connect('new-decoded-pad', self.PadAdded)
            self.pipeline.add(self.src)
            self.pipeline.add(self.decodebin)
            self.src.link(self.decodebin)
            self.spotify_in_use = False
        else:
            print "Decodebin is taking care of this uri"
            self.src = gst.element_make_from_uri(gst.URI_SRC, track.uri, "source")
            self.decodebin = gst.element_factory_make ("decodebin2", "decodebin2")
            self.decodebin.connect('new-decoded-pad', self.PadAdded)
            self.pipeline.add(self.src)
            self.pipeline.add(self.decodebin)
            self.src.link(self.decodebin)
            self.spotify_in_use = False

        self.pipeline.set_state(gst.STATE_PLAYING)

    def PadAdded(self, element, pad, last):
        print "Lets add a speaker we found suitable elements to decode"
        pad.link (self.volume.get_pad("sink"))

if __name__ == '__main__':
    os.system("./spotifysch&")
    os.system("sleep 1")

    # create the dogvibes object
    global dogvibes
    dogvibes = Dogvibes()

    print "Running Dogvibes."
    print "   ->Vibe the dog!"
    print "                 .--.    "
    print "                / \aa\_  "
    print "         ,      \_/ ,_Y) "
    print "        ((.------`\"=(    "
    print "         \   \      |o   "
    print "         /)  /__\  /     "
    print "        / \ \_  / /|     "
    print "        \_)\__) \_)_)    "

    api = API()
