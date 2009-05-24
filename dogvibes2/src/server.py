#!/usr/bin/env python
import os
import inspect
import hashlib
import gobject
import gst

#import dogvibes object
from dogvibes import Dogvibes

# import track
from track import Track
from collection import Collection

import BaseHTTPServer
from urlparse import urlparse
import cgi
import json
import sys

import urllib

# web server
class APIHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        u = urlparse(self.path)
        c = u.path.split('/')
        method = 'API_' + c[-1]

         # TODO: this should be determined on API function return:
        raw = method == 'API_getAlbumArt'

        obj = c[1]
        id = c[2]
        id = 0 # TODO: remove when more amps are supported

        if obj == 'dogvibes':
            klass = dogvibes
        else:
            klass = dogvibes.amps[id]

        callback = None
        data = None
        error = 0

        params = cgi.parse_qs(u.query)
        # use only the first value for each key (feel free to clean up):
        params = dict(zip(params.keys(), map(lambda x: x[0], params.values())))

        if 'callback' in params:
            callback = params.pop('callback')

        try:
            # strip params from paramters not in the method definition
            args = inspect.getargspec(getattr(klass, method))[0]
            params = dict(filter(lambda k: k[0] in args, params.items()))
            # call the method
            data = getattr(klass, method).__call__(**params)
            if data == -1: error = 4
            # TODO: must check for functions returning errors
        except AttributeError:
            error = 1 # No such method
        except TypeError:
            error = 2 # Unsupported parameter
        except NameError:
            error = 3 # Missing parameter

        self.send_response(400 if error else 200) # Bad request or OK

        if raw:
            self.send_header("Content-type", "image/jpeg")
        else:
            # Add results from method call only if there is any
            if data == None or error != 0:
                data = dict(error = error)
            else:
                data = dict(error = error, result = data)

            # Different JSON syntax in different versions of python
            if sys.version_info[0] >= 2 and sys.version_info[1] >= 6:
                data = json.dumps(data)
            else:
                data = json.write(data)

            # Wrap result in a Javascript function if a callback was submitted
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


if __name__ == '__main__':
    if os.path.exists('dogvibes.db'):
        os.remove('dogvibes.db')
        print '''REMOVING DATABASE! DON'T DO THIS IF YOU WANNA KEEP YOUR PLAYLISTS'''

    os.system("./spotifysch&")
    os.system("sleep 1")

    # create the dogvibes object
    global dogvibes
    dogvibes = Dogvibes()

    print "Running Dogvibes."
    print "   ->Vibe the dog!"
    print "                 .--.    "
    print "                / \\aa\_  "
    print "         ,      \_/ ,_Y) "
    print "        ((.------`\"=(    "
    print "         \   \      |o   "
    print "         /)  /__\  /     "
    print "        / \ \_  / /|     "
    print "        \_)\__) \_)_)    "

    api = API()
