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
from threading import Thread
import signal
import optparse
import gobject
import gst
import cgi
import inspect
import urllib
import time
from albumart import AlbumArt

API_VERSION = '0.1'

LOG_LEVELS = {'0': logging.CRITICAL,
              '1': logging.ERROR,
              '2': logging.WARNING,
              '3': logging.INFO,
              '4': logging.DEBUG}

DOG_LOCK = threading.Lock()

dogs = []

# A connection to a Dog
class Dog():
    def __init__(self, sock, username):
        self.sock = sock
        self.username = username

        dog = Dog.find(username)
        DOG_LOCK.acquire()
        if dog != None:
            dogs.remove(dog)
        dogs.append(self)
        DOG_LOCK.release()

    # Send a command on a socket and wait for reply, blocking
    def send_cmd(self, cmd):
        self.sock.send(cmd)
        tmp = ''

        # if this fails, the socket is broken
        try:
            tmp += self.sock.recv(4000)
        except:
            DOG_LOCK.acquire()
            dogs.remove(self)
            DOG_LOCK.release()
            print "Dog '%s' has disconnected" % self.username
            return "{error: 4}" # Yuck!

        self.sock.setblocking(0)
        try:
            while True:
                tmp += self.sock.recv(4000)
        except:
            pass
        self.sock.setblocking(1)

        return tmp

    @classmethod
    def find(self, username):
        DOG_LOCK.acquire()
        for dog in dogs:
            if dog.username == username:
                DOG_LOCK.release()
                return dog
        DOG_LOCK.release()


def handle_request(path):

    print path
    error = 0
    raw = False
    data = ''

    u = urlparse(path)
    c = u.path.split('/')

    if (len(c) < 4):
        #raise NameError("Malformed command: %s" % u.path)
        return (data, raw, error)

    raw = c[-1] == 'getAlbumArt'

    s = path.split('/', 2)
    username = s[1]
    cmd = '/' + s[2]

    dog = Dog.find(username)
    if (dog == None):
        raise Exception
    else:
        if raw:
            # FIXME: need to get real album and artist here
            data = AlbumArt.get_image("oasis", "wonderwall", 300)
        else:
            data = dog.send_cmd(cmd)

    return (data, raw, error)

class HTTPServer(resource.Resource):
    isLeaf = True
    def render_GET(self, request):
        (data, raw, error) = handle_request(request.uri)

        if raw:
            request.setHeader("Content-type", "image/png")
        else:
            request.setHeader("Content-type", "application/json")
            #request.setHeader("Content-type", "text/javascript") # no callback

        #self.send_response(400 if error else 200) # Bad request or OK

        return data


SERVER_HANDSHAKE = '\
HTTP/1.1 101 Web Socket Protocol Handshake\r\n\
Upgrade: WebSocket\r\n\
Connection: Upgrade\r\n\
WebSocket-Origin: %s\r\n\
WebSocket-Location: %s/\r\n\r\n\
'

class WebSocket(Protocol):

    handshaken = False # Indicates if initial setup has been done
    buf = '' # Save unprocessed data between reads

    def handshake(self, data):

        # FIXME: if smaller than header size, we risk missing some initial commands!!

        shake, self.buf = data.split('\r\n\r\n')

        # extract info to send back to client according to the websocket proto
        host = re.findall("Host: ([a-zA-Z0-9\.:/]*)", shake)
        origin = re.findall("Origin: ([a-zA-Z0-9\.:/]*)", shake)

        # re.findall always return an array
        if host == [] or origin == []:
            logging.error("Websocket handshake is wrong, check incoming request " +  self.transport.client)
            logging.error(shake)
            self.transport.loseConnection()

        # compile an answer and send back to the client
        new_handshake = SERVER_HANDSHAKE % (origin[0], "ws://" + host[0])

        self.transport.write(new_handshake)

        self.handshaken = True

    def sendWS(self, data):
        print data
        logging.debug("sending to %d: %s" % (self.transport.client[1], data))
        self.transport.write('\x00' + data.encode('utf-8') + '\xff')

    def dataReceived(self, data):

        # Handle Flash plugin
        if data.find("<policy-file-request/>") != -1:
            self.transport.write('<?xml version="1.0"?>\n')
            self.transport.write('<!DOCTYPE cross-domain-policy SYSTEM "http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd">\n')
            self.transport.write('<cross-domain-policy>\n')
            self.transport.write('  <allow-access-from domain="*" to-ports="*"/>\n')
            self.transport.write('</cross-domain-policy>')
            self.transport.loseConnection()
            return

        self.buf += data.strip();

        if self.handshaken == False:
            self.handshake(data)

        cmds = []

        msgs = self.buf.split('\xff')
        self.buf = msgs.pop()

        for msg in msgs:
            if msg[0] == '\x00':
                cmds.append(msg[1:])

        for cmd in cmds:
            (data, raw, error) = handle_request(cmd)
            self.sendWS(data);

# Loop over all registered sockets and remove them if dead
def clean_sockets():
    while True:
        DOG_LOCK.acquire()
        for dog in dogs:
            try:
                pass
                # TODO: detect here that socket is dead
            except:
                "Dog %s has disconnected" % dog.username
                dogs.remove(dog)
        DOG_LOCK.release()
        time.sleep(0.25)

# Responsible for accepting new connections from Dogs
class CliThread(Thread):
    def __init__(self):
        Thread.__init__( self )
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("", 11111))
        self.sock.listen(5)

    def run(self):
        while 1:
            cli, address = self.sock.accept()
            username = cli.recv(4096)
            print "connected user: " + username
            Dog(cli, username)


if __name__ == "__main__":

    print "Dogvibes API"

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


    factory = Factory()
    factory.protocol = WebSocket

    reactor.listenTCP(9999, factory)
    reactor.listenTCP(2000, server.Site(HTTPServer()))

    CliThread().start()
    threading.Thread(target = clean_sockets).start()

    reactor.run()
