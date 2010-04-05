from twisted.internet.protocol import Protocol, Factory
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

LOG_LEVELS = {'0': logging.CRITICAL,
              '1': logging.ERROR,
              '2': logging.WARNING,
              '3': logging.INFO,
              '4': logging.DEBUG}

lock = threading.Lock()

server_handshake = '\
HTTP/1.1 101 Web Socket Protocol Handshake\r\n\
Upgrade: WebSocket\r\n\
Connection: Upgrade\r\n\
WebSocket-Origin: %s\r\n\
WebSocket-Location: %s/\r\n\r\n\
'

clients = []

class QOTD(Protocol):

    handshaken = False # Indicates if initial setup has been done
    buf = '' # Save unprocessed data between reads

    def connectionMade(self):
        clients.append(self)
        #self.transport.write(server_handshake)

    def connectionLost(self, reason):
        clients.remove(self)
        print "Disconnected from", self.transport.client

    def handshake(self, data):
        # FIXME: if smaller than header size, we risk missing some initial commands!!
        shake, self.buf = data.split('\r\n\r\n')

        # extract info to send back to client according to the websocket proto
        host = re.findall("Host: ([a-zA-Z0-9\.:/]*)", shake)
        origin = re.findall("Origin: ([a-zA-Z0-9\.:/]*)", shake)

        # re.findall always return an array
        if host == [] or origin == []:
            print "Websocket handshake is wrong. Check incoming request"
            print shake
            self.transport.loseConnection()

        # compile an answer and send back to the client
        new_handshake = server_handshake % (origin[0], "ws://" + host[0])

        print shake
        print new_handshake
        self.transport.write(new_handshake)

        self.handshaken = True

    def sendWS(self, data):
        logging.debug("sending to %d: %s" % (self.transport.client[1], data))
        self.transport.write('\x00' + data.encode('utf-8') + '\xff')

    def pushStatus(self):
        # loop through all amps and look for status updates
        amp = dogvibes.amps[0]
        if amp.needs_push_update or dogvibes.needs_push_update:
            print "HEHEHEH"
            data = dict(error = 0, result = amp.API_getStatus())
            data = cjson.encode(data)
            data = 'pushHandler' + '(' + data + ')'

            amp.needs_push_update = False
            dogvibes.needs_push_update = False
            lock.acquire()

            for client in clients:
                client.sendWS(data)
            lock.release()

    def dataReceived(self, data):
        self.buf += data.strip();

        if self.handshaken == False:
            self.handshake(data)

        self.pushStatus()

        cmds = []

        msgs = self.buf.split('\xff')
        self.buf = msgs.pop()

        for msg in msgs:
            if msg[0] == '\x00':
                cmds.append(msg[1:])

        for cmd in cmds:
            logging.info("%s(%s): %s" % (self.transport.client[0], self.transport.client[1], cmd))

            # path can be like dogvibes/method or amp/0/method
            u = urlparse(cmd)
            c = u.path.split('/')

            method = 'API_' + c[-1] # last is always method
            obj = c[1] # first is always object
            id = c[2] # TODO: must check len of array for this to work
            id = 0 # TODO: remove when more amps are supported

            if obj == 'dogvibes':
                klass = dogvibes
            else:
                klass = dogvibes.amps[id]

            callback = None
            data = None
            error = 0
            msg_id = None

            params = cgi.parse_qs(u.query)
            # use only the first value for each key (feel free to clean up):
            params = dict(zip(params.keys(), map(lambda x: x[0], params.values())))
            if 'callback' in params:
                callback = params.pop('callback')
                # FIXME: should be allowed to send more parameters
                # than specified. But strip them
                if '_' in params: # TODO: this applies when not using callback as well?
                    params.pop('_')

            if 'callback' in params:
                callback = params.pop('callback')

            if 'msg_id' in params:
                msg_id = params.pop('msg_id')

            try:
                # strip params from paramters not in the method definition
                args = inspect.getargspec(getattr(klass, method))[0]
                params = dict(filter(lambda k: k[0] in args, params.items()))
                # call the method
                data = getattr(klass, method).__call__(**params)
            except AttributeError as e:
                error = 1 # No such method
                logging.info(e)
            except TypeError as e:
                error = 2 # Missing parameter
                logging.info(e)
            except ValueError as e:
                error = 3 # Internal error, e.g. could not find specified uri
                logging.info(e)

            # Add results from method call only if there are any
            if data == None or error != 0:
                data = dict(error = error)
            else:
                data = dict(error = error, result = data)

            # TODO: use '_' instead of 'msg_id'?
            if msg_id != None:
                data['msg_id'] = msg_id

            # Different JSON syntax in different versions of python
            data = cjson.encode(data)

            # Wrap result in a Javascript function if a callback was submitted
            if callback != None:
                #data = callback + '(' + data + ')'
                data = "%s(%s)" % (callback, data)

            self.sendWS(data);

if __name__ == "__main__":

    print "Running Dogvibes (Websocket edition)"
    print "   ->Vibe the dog!"
    print "                 .--.    "
    print "                / \\aa\_  "
    print "         ,      \_/ ,_Y) "
    print "        ((.------`\"=(    "
    print "         \   \      |o   "
    print "         /)  /__\  /     "
    print "        / \ \_  / /|     "
    print "        \_)\__) \_)_)    "

    # Enable Ctrl-C
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    # Setup log
    parser = optparse.OptionParser()
    parser.add_option('-l', help='Log level', dest='log_level', default=3)
    parser.add_option('-f', help='Log file name', dest='log_file', default='/dev/stdout') # TODO: Windows will feel dizzy
    (options, args) = parser.parse_args()
    log_level = LOG_LEVELS.get(options.log_level, logging.NOTSET)
    logging.basicConfig(level=log_level, filename=options.log_file,
                        format='%(asctime)s %(levelname)s: %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')

    gobject.threads_init()

    global dogvibes
    dogvibes = Dogvibes()

    factory = Factory()
    factory.protocol = QOTD
    reactor.listenTCP(9999, factory)
    reactor.run()

    loop = gobject.MainLoop()
    loop.run()
