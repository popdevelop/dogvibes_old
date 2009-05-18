import BaseHTTPServer
from urlparse import urlparse
import cgi
import json

# dummy amp
class Amp:
    def getStatus(self, nbr):
        return dict(s="result!", t="more!")

amp = Amp()

# method redirects
class API:
    def amp_getStatus(self, id, params):
        return amp.getStatus(params.get('something')[0])

# web server
class APIHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        api = API()

        u = urlparse(self.path)
        c = u.path.split('/')
        method = c[-1]
        object = c[1]
        id = c[2]

        if hasattr(api, object + "_" + method):
            params = cgi.parse_qs(u.query)

            if 'id' in params:  id = params.get('id')
            else:               id = 0

            data = getattr(api, object + "_" + method).__call__(id, params)
            data = dict(error = 0, results = data)
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


httpserver = BaseHTTPServer.HTTPServer(("", 2000), APIHandler)
httpserver.serve_forever()
