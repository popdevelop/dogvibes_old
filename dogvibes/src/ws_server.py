#!/usr/bin/env python
import os
import inspect
import hashlib
import gobject
import gst

import threading
from threading import Thread
import socket
import select
import signal
import re
import cgi

#import dogvibes object
from dogvibes import Dogvibes

# import track
from track import Track
from collection import Collection

from urlparse import urlparse
import cjson
import sys

import urllib

import signal

class DogError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)


server_handshake = '\
HTTP/1.1 101 Web Socket Protocol Handshake\r\n\
Upgrade: WebSocket\r\n\
Connection: Upgrade\r\n\
WebSocket-Origin: %s\r\n\
WebSocket-Location: %s/\r\n\r\n\
'

class Server: 
    def __init__(self): 
        self.host = '' 
        self.port = 9999
        self.nbr_connections = 5 
        self.size = 1024 
        self.server = None 
        self.threads = [] 

    def open_socket(self):
        try: 
            self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
            self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server.bind((self.host,self.port)) 
            self.server.listen(self.nbr_connections) 
        except socket.error, (value,message): 
            if self.server:
                self.server.close()
            print "Could not open socket: " + message 
            sys.exit(1)

    def run(self): 
        self.open_socket() 
        input = [self.server]
        running = 1 
        while running:
            # TODO: we could wait for other connections like HTTP on port 80
            inputready,outputready,exceptready = select.select(input,[],[]) 

            for s in inputready:
                if s == self.server: 
                    # handle the server socket
                    c = ClientConnection(self.server.accept(),self) 
                    c.start()
                    self.threads.append(c) 

        # close all threads 

        self.server.close() 
        for c in self.threads: 
            c.join()

    def pushStatus(self):
        # loop through all amps and look for status updates
        amp = dogvibes.amps[0]
        if amp.needs_push_update:
            data = dict(error = 0, result = amp.API_getStatus())
            data = cjson.encode(data)
            data = 'successGetStatus' + '(' + data + ')'
            amp.needs_push_update = False

            for c in self.threads: 
                c.client.send('\x00' + data + '\xff')



class ClientConnection(threading.Thread): 
    def __init__(self,(client,address),parent):
        threading.Thread.__init__(self)
        self.client = client
        self.address = address
        self.size = 1024
        self.data = ''
        self.parent = parent

    def handshake(self):
        shake = self.client.recv(512) # FIXME: if smaller than header size, we risk missing some initial commands!!
        a = re.search("(Host: )([a-zA-Z])*:+([0-9])*", shake)
        location = "ws://" + a.group().split()[1]
        a = re.search("(Origin: )([a-zA-Z/:])*:+([0-9])*", shake)
        origin = a.group().split()[1]
        new_handshake = server_handshake % (origin, location)
        print new_handshake
        print shake
        self.client.send(new_handshake)
    
    def interact(self):
        self.parent.pushStatus()

        try:
            tmp = self.client.recv(256)
        except: return

        self.data += tmp;

        cmds = []

        msgs = self.data.split('\xff')
        self.data = msgs.pop()

        for msg in msgs:
            if msg[0] == '\x00':
                cmds.append(msg[1:])

        for cmd in cmds:
            print "%s(%s): %s" % (self.address[0], self.address[1], cmd)

            # path can be like dogvibes/method or amp/0/method
            u = urlparse(cmd)
            c = u.path.split('/')
            method = 'API_' + c[-1] # last is always method
             # TODO: this should be determined on API function return:
            raw = method == 'API_getAlbumArt'

            obj = c[1] # first is always object
            id = c[2] # this will not work...
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
                if '_' in params:
                    params.pop('_')                

            try:
                # strip params from paramters not in the method definition
                args = inspect.getargspec(getattr(klass, method))[0]
                params = dict(filter(lambda k: k[0] in args, params.items()))        
                # call the method
                data = getattr(klass, method).__call__(**params)
            except AttributeError:
                error = 1 # No such method
            except TypeError:
                error = 2 # Missing parameter
            except DogError:
                error = 3 # Internal error, e.g. could not find specified uri

            #self.send_response(400 if error else 200) # Bad request or OK

            # Add results from method call only if there is any
            if data == None or error != 0:
                data = dict(error = error)
            else:
                data = dict(error = error, result = data)

            # Different JSON syntax in different versions of python
            data = cjson.encode(data)

            # Wrap result in a Javascript function if a callback was submitted
            if callback != None:
                data = callback + '(' + data + ')'

            # TODO: make sure data is utf-8
            self.client.send('\x00' + data + '\xff')


    def run(self):
        print "Running thread"
        running = 1 
        while running:
            print 'connection!' 
            self.handshake()
            print 'handshaken'
            self.client.setblocking(0)
            while True:
                self.interact()


class API(Thread):
    def __init__(self):
        Thread.__init__ (self)

    def run(self):
        global dogvibes
        dogvibes = Dogvibes()

        s = Server() 
        s.run()


if __name__ == '__main__':
    #if os.path.exists('dogvibes.db'):
    #    os.remove('dogvibes.db')
    #    print "REMOVING DATABASE! DON'T DO THIS IF YOU WANNA KEEP YOUR PLAYLISTS"

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

    # Enable Ctrl-C
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    gobject.threads_init()

    API().start()

    loop = gobject.MainLoop()
    loop.run()
