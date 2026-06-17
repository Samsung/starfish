import http.server
import os

BASE = os.path.dirname(os.path.abspath(__file__))


class H(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/page.html":
            path = ("/tmp/sf-measure/page.html"
                    if os.path.exists("/tmp/sf-measure/page.html")
                    else os.path.join(BASE, "page.html"))
            body = open(path, "rb").read()
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        n = int(self.headers.get("Content-Length", 0))
        data = self.rfile.read(n).decode("utf-8", "replace")
        print("RESULT>>> " + data, flush=True)
        with open("/tmp/sf-measure/results.txt", "a") as f:
            f.write(data + "\n")
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def log_message(self, fmt, *args):
        pass


http.server.HTTPServer(("127.0.0.1", 8802), H).serve_forever()
