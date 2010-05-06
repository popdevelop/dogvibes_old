import tornado.ioloop
import tornado.web

import errno
import functools
import socket
from tornado import ioloop, iostream, httpserver, websocket

import re

dogs = []

class Dog():
    def __init__(self, stream):
        self.stream = stream
        self.current_request = None

    def set_username(self, username):
        username = username[:-2]
        dog = Dog.find(username)
        if dog != None:
            # TODO: close stream
            dogs.remove(dog)
        dogs.append(self)

        print "Hello, %s!" % username
        self.username = username

    active_requests = []

    def command_callback(self, data):
        data = data[:-2]
        command, result = data.split('||')
        found = None
        for req in self.active_requests:
            print "%s : %s" % (req.request.uri, command)
            if req.request.uri[-len(command):] == command:
                self.active_requests.remove(req)
                found = req
                break

        req = found

        # TODO: add more headers?
        print self.active_requests
        print req
        req.set_header("Content-Type", "text/javascript")
        req.write(result)
        req.finish()

        if self.active_requests != []:
            self.stream.read_until(r'\\', self.command_callback)

    def send_command(self, command, request):
        self.current_request = request
        try:
            self.stream.write(command)
        except IOError:
            print "Bye, %s!" % self.username
            self.stream.close()
            dogs.remove(self)
            return

        if self.active_requests == []:
            self.stream.read_until(r'\\', self.command_callback)

        self.active_requests.append(request)

    @classmethod
    def find(self, username): # TODO: faster approach?
        for dog in dogs:
            if dog.username == username:
                return dog
        return None

def process_command(handler, username):
    command = handler.request.uri[len(username)+1:]
    print command
    dog = Dog.find(username)
    if dog == None:
        return "ERROR!" # FIXME
    dog.send_command(command, handler)

def connection_ready(sock, fd, events):
    while True:
        try:
            connection, address = sock.accept()
        except socket.error, e:
            if e[0] not in (errno.EWOULDBLOCK, errno.EAGAIN):
                raise
            return
        connection.setblocking(0)
        stream = iostream.IOStream(connection)

        dog = Dog(stream)
        stream.read_until(r'\\', dog.set_username)

class HTTPHandler(tornado.web.RequestHandler):
    @tornado.web.asynchronous
    def get(self, username):
        process_command(self, username)

class WSHandler(websocket.WebSocketHandler):
    def open(self, username):
        self.receive_message(self.on_message)

    def on_message(self, command):
        process_command(self.username)
        self.receive_message(self.on_message)

    def send_result(self, data):
        self.send_message(data)

def setup_dog_socket(io_loop):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setblocking(0)
    sock.bind(("", 11111))
    sock.listen(5000)

    callback = functools.partial(connection_ready, sock)
    io_loop.add_handler(sock.fileno(), callback, io_loop.READ)

if __name__ == '__main__':
    application = tornado.web.Application([
            (r"/([a-zA-Z0-9]+)/stream", WSHandler),
            (r"/([a-zA-Z0-9]+).*", HTTPHandler), # TODO: split only on '/', avoids favicon
            ])

    http_server = httpserver.HTTPServer(application)
    http_server.listen(2000)

    io_loop = ioloop.IOLoop.instance()
    setup_dog_socket(io_loop)

    try:
        io_loop.start()
    except KeyboardInterrupt:
        io_loop.stop()
        print "exited cleanly"
