import BaseHTTPServer

data = """{ error: 0 }"""

class APIHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/amp/0/getStatus":
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            self.wfile.write(data)
        else:
            self.send_error(404, 'Unsupported call')
            
httpserver = BaseHTTPServer.HTTPServer(("", 2000), APIHandler)
httpserver.serve_forever()