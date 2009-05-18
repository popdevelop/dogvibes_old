#!/usr/bin/env python
import os
import hashlib
import gobject
import gst
# import spources
from spotifysource import SpotifySource

# import speakers
from devicespeaker import DeviceSpeaker

# import track
from track import Track

import BaseHTTPServer
from urlparse import urlparse
import cgi
import json

# method redirects
class APIDistributor:
    def dogvibes_search(self, id, params):
        global dogvibes
        return dogvibes.search(params.get('query')[0])
    def amp_connectSpeaker(self, id, params):
        global dogvibes
        return dogvibes.amps[id].connectSpeaker(int(params.get('nbr')[0]))
    def amp_disconnectSpeaker(self, id, params):
        global dogvibes
        return dogvibes.amps[id].disconnectSpeaker(int(params.get('nbr')[0]))
    def amp_getAllTracksInQueue(self, id, params):
        global dogvibes
        return dogvibes.amps[id].getAllTracksInQueue()
    def amp_getPlayedMilliSeconds(self, id, params):
        global dogvibes
        return dogvibes.amps[id].getPlayedMilliSeconds()
    def amp_getStatus(self, id, params):
        global dogvibes
        return dogvibes.amps[id].getStatus()
    def amp_getQueuePosition(self, id, params):
        global dogvibes
        return dogvibes.amps[id].getQueuePosition()
    def amp_nextTrack(self, id, params):
        global dogvibes
        return dogvibes.amps[id].nextTrack()
    def amp_playTrack(self, id, params):
        global dogvibes
        return dogvibes.amps[id].playTrack(int(params.get('nbr')[0]))
    def amp_previousTrack(self, id, params):
        global dogvibes
        return dogvibes.amps[id].previousTrack()
    def amp_play(self, id, params):
        global dogvibes
        return dogvibes.amps[id].play()
    def amp_pause(self, id, params):
        global dogvibes
        return dogvibes.amps[id].pause()
    def amp_queue(self, id, params):
        global dogvibes
        return dogvibes.amps[id].queue(params.get('uri')[0])
    def amp_removeFromQueue(self, id, params):
        global dogvibes
        return dogvibes.amps[id].removeFromQueue(int(params.get('nbr')[0]))
    def amp_seek(self, id, params):
        global dogvibes
        return dogvibes.amps[id].seek(int(params.get('mseconds')[0]))
    def amp_setVolume(self, id, params):
        global dogvibes
        return dogvibes.amps[id].setVolume(float(params.get('level')[0]))
    def amp_stop(self, id, params):
        global dogvibes
        return dogvibes.amps[id].stop()

# web server
class APIHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        api = APIDistributor()

        u = urlparse(self.path)
        c = u.path.split('/')
        method = c[-1]
        object = c[1]
        id = c[2]

        print dogvibes

        if hasattr(api, object + "_" + method):
            params = cgi.parse_qs(u.query)

            if 'id' in params:  id = params.get('id')
            else:               id = 0
            
            id = 0 # TODO: remove when more amps are supported

            data = getattr(api, object + "_" + method).__call__(id, params)
            if data == None:  data = dict(error = 0)
            else:             data = dict(error = 0, result = data)
            data = json.write(data)

            self.send_response(200)

            if 'callback' in params:
                data = params['callback'][0] + '(' + data + ')'
                self.send_header("Content-type", "text/javascript")
            else:
                self.send_header("Content-type", "application/json")

            self.end_headers()
            self.wfile.write(data)

        else:
            self.send_error(404, 'Unsupported call')

class API:
    def __init__(self):
        httpserver = BaseHTTPServer.HTTPServer(("", 2000), APIHandler)
        httpserver.serve_forever()

class Dogvibes():
    def __init__(self):
        # add all sources
        self.sources = [SpotifySource("spotify", "gyllen", "bobidob20")]

        # add all speakers
        self.speakers = [DeviceSpeaker("devicesink")]

        # add all amps
        amp0 = Amp(self)
        amp0.connectSpeaker(0)
        self.amps = [amp0]

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

    # API

    def connectSpeaker(self, nbr):
        if nbr > len(self.dogvibes.speakers) - 1:
            print "Speaker does not exist"

        speaker = self.dogvibes.speakers[nbr];

        if (self.pipeline.get_by_name (speaker.name) == None):
            self.sink = self.dogvibes.speakers[nbr].get_speaker ();
            self.pipeline.add(self.sink)
            self.tee.link(self.sink)
        else:
            print "Speaker %d already connected" % nbr
        # FIXME: we never get here...

    def disconnectSpeaker(self, nbr):
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
        # FIXME: we never get here...

    def getAllTracksInQueue(self):
        ret = []
        for track in self.playqueue:
            ret.append(track.to_dict())
        return ret

    def getPlayedMilliSeconds(self):
        (pos, form) = self.pipeline.query_position(gst.FORMAT_TIME)
        return pos / gst.MSECOND

    def getStatus(self):
        uri = 'xxx'
        playqueuehash = 'xxx'
        if (len(self.playqueue) > 0):
            uri = self.playqueue[self.playqueue_position - 1].uri
            playqueuehash = self.GetHashFromPlayQueue()
        return {'uri': uri,
                'playqueuehash': playqueuehash,
                'title': 'dummy',
                'artist': 'dummy',
                'album': 'dummy',
                'duration': '0',
                'state': 'stopped'}

    def getQueuePosition(self):
        return self.playqueue_position

    def nextTrack(self):
        self.ChangeTrack(self.playqueue_position + 1)

    def playTrack(self, tracknbr):
        self.ChangeTrack(tracknbr)
        self.play()

    def previousTrack(self):
        self.ChangeTrack(self.playqueue_position - 1)

    def play(self):
        self.PlayOnlyIfNull(self.playqueue[self.playqueue_position])

    def pause(self):
        self.pipeline.set_state(gst.STATE_PAUSED)

    def queue(self, uri):
        print "Queued track:%s" % uri
        self.playqueue.append(Track(uri))
        return "trams"

    def removeFromQueue(self, nbr):
        if (nbr > len(self.playqueue)):
            print "Too big of a number for removing"
            return

        self.playqueue.remove(self.playqueue[nbr])

        if (nbr <= self.playqueue_position):
            self.playqueue_position = self.playqueue_position - 1

    def seek(self, mseconds):
        print "Implement me"
        # FIXME
        #    pipeline.seek_simple (Format.TIME, SeekFlags.NONE, ((int64) msecond) * MSECOND);
        # self.pipeline.seek_simple (Track(uri))

    def setVolume(self, vol):
        if (vol > 2 or vol < 0):
            print "Volume must be between 0.0 and 2.0"
        self.volume.set_property("volume", vol) # FIXME: this doesn't seem to return

    def stop(self):
        self.pipeline.set_state(gst.STATE_NULL)

    # Internal functions

    def ChangeTrack(self, tracknbr):
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
    os.system("../spotifysch&")
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
