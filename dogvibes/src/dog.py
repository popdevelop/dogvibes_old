from twisted.internet.protocol import Protocol, Factory
from twisted.internet import glib2reactor
glib2reactor.install()
from twisted.web import server, resource
from twisted.internet import reactor
import SocketServer
import socket
import logging
import threading
import re
import cjson
from urlparse import urlparse
import sys
from dogvibes import Dogvibes
from threading import Thread
import signal
import optparse
import gobject
import gst
import cgi
import inspect
import urllib
import config
import time

API_VERSION = '0.1'

LOG_LEVELS = {'0': logging.CRITICAL,
              '1': logging.ERROR,
              '2': logging.WARNING,
              '3': logging.INFO,
              '4': logging.DEBUG}

lock = threading.Lock()

def register_dog():
#    DEFAULT_EXT_IP = '0.0.0.0'
#
#    int_ip = socket.gethostbyname(socket.gethostname())
#
#    # Try fetching external IP more than once. Sometimes it fails
#    for i in range(0,3):
#        try:
#            response = urllib.urlopen('http://whatismyip.org')
#            ext_ip = response.read()
#            continue
#        except:
#            ext_ip = DEFAULT_EXT_IP
#            if i < 2:
#                time.sleep(1)

#    if ext_ip == DEFAULT_EXT_IP:
#        print 'Could not get external IP. No connection to Internet?'

    if cfg['MASTER_SERVER'] == 'dogvib.es':
        int_ip = 'dogvib.es'
    else:
        int_ip = socket.gethostbyname(socket.gethostname())

    try:
#        response = urllib.urlopen('http://dogvibes.com/registerDog?name=%s&password=%s&int_ip=%s&exp_ip=%s&ws_port=%s&http_port=%s&api_version=%s' % (cfg['DOGVIBES_USER'], cfg['DOGVIBES_PASS'], int_ip, ext_ip, cfg['WS_PORT'], cfg['HTTP_PORT'], API_VERSION))
        response = urllib.urlopen('http://dogvibes.com/registerDog?name=%s&password=%s&int_ip=%s&api_version=%s' % (cfg['DOGVIBES_USER'], cfg['DOGVIBES_PASS'], int_ip, API_VERSION))
    except:
        print 'Could access dogvibes.com'
        return

    reply = response.read()
    if reply == '0':
        print 'Registered a client at http://dogvibes.com/%s' % cfg['DOGVIBES_USER']
    elif reply == '1':
        print "Must specify a password to update client '%s'" % cfg['DOGVIBES_USER']
    elif reply == '2':
        print "Password do not match the registered one for client '%s'" % cfg['DOGVIBES_USER']
    else:
        print 'Unknown error when registering client'

class DogProtocol(Protocol):

    buf = '' # Save unprocessed data between reads

    def connectionMade(self):
        print "Connected to dogvibes.com"
        self.transport.write(cfg['DOGVIBES_USER'] + r'\\') # register username at dogvibes.com

    def connectionLost(self, reason):
        print "Disconnected to dogvibes.com"

    def dataReceived(self, command):

        print command

        u = urlparse(command)
        c = u.path.split('/')

        try:
            callback = None
            data = None
            error = 0
            raw = False

            params = cgi.parse_qs(u.query)
            # use only the first value for each key (feel free to clean up):
            params = dict(zip(params.keys(), map(lambda x: x[0], params.values())))

            if 'callback' in params:
                callback = params.pop('callback')
                if '_' in params:
                    params.pop('_')

            if 'msg_id' in params:
                msg_id = params.pop('msg_id')

            if (len(c) < 3):
                raise NameError("Malformed command: %s" % u.path)

            method = 'API_' + c[-1]
             # TODO: this should be determined on API function return:
            raw = method == 'API_getAlbumArt'

            obj = c[1]
            #id = c[2]
            id = 0 # TODO: remove when more amps are supported

            if obj == 'dogvibes':
                klass = dogvibes
            elif obj == 'amp':
                klass = dogvibes.amps[id]
            else:
                raise NameError("No such object '%s'" % obj)

            # strip params from paramters not in the method definition
            args = inspect.getargspec(getattr(klass, method))[0]
            params = dict(filter(lambda k: k[0] in args, params.items()))
            # call the method
            data = getattr(klass, method).__call__(**params)
        except AttributeError as e:
            error = 1 # No such method
            logging.warning(e)
        except TypeError as e:
            error = 2 # Missing parameter
            logging.warning(e)
        except ValueError as e:
            error = 3 # Internal error, e.g. could not find specified uri
            logging.warning(e)
        except NameError as e:
            error = 4 # Wrong object or other URI error
            logging.warning(e)

        # Add results from method call only if there is any
        if data == None or error != 0:
            data = dict(error = error)
        else:
            data = dict(error = error, result = data)

        data = cjson.encode(data)

        # Wrap result in a Javascript function if a callback is present
        if callback != None:
            data = "%s(%s)" % (callback, data)

        # TODO: introduce a delay here to test api_server threading!
        if raw:
            self.transport.write(command + r'||' + '0' + r'\\')
        else:
            self.transport.write(command + r'||' + data + r'\\')
#        self.transport.write(data + r'\\')

if __name__ == "__main__":

    print "Running Dogvibes"
    print
    print "  Vibe the dog!"
    print "                 .--.    "
    print "                / \\aa\_  "
    print "         ,      \_/ ,_Y) "
    print "        ((.------`\"=(    "
    print "         \   \      |o   "
    print "         /)  /__\  /     "
    print "        / \ \_  / /|     "
    print "        \_)\__) \_)_)    "
    print "                         "
    print "                         "
    print "   uses SPOTIFY(R) CORE  "
    print "                         "
    print "This product uses SPOTIFY(R) CORE"
    print "but is not endorsed, certified or"
    print "otherwise approved in any way by "
    print "Spotify. Spotify is the registered"
    print "trade mark of the Spotify Group"
    print "                         "

    # Enable Ctrl-C
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    # Setup log
    parser = optparse.OptionParser()
    parser.add_option('-l', help='Log level', dest='log_level', default='2')
    parser.add_option('-f', help='Log file name', dest='log_file', default='/dev/stdout') # TODO: Windows will feel dizzy
    (options, args) = parser.parse_args()
    log_level = LOG_LEVELS.get(options.log_level, logging.NOTSET)
    logging.basicConfig(level=log_level, filename=options.log_file,
                        format='%(asctime)s %(levelname)s: %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')


    # Load configuration
    cfg = config.load("dogvibes.conf")

    global dogvibes
    dogvibes = Dogvibes()

    from twisted.internet.protocol import ClientFactory
    factory = ClientFactory()
    factory.protocol = DogProtocol
    reactor.connectTCP(cfg['MASTER_SERVER'], 11111, factory)

    register_dog()

    reactor.run()
