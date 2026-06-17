import http.server
import sys

class H(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/test.html":
            import os
            base = os.path.dirname(os.path.abspath(__file__))
            path = "/tmp/sf-test/test.html" if os.path.exists("/tmp/sf-test/test.html") else os.path.join(base, "test.html")
            body = open(path, "rb").read()
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        elif self.path == "/blob.bin":
            body = bytes(bytearray(i & 0xFF for i in range(1048576)))
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        elif self.path == "/bench.html":
            # MSE append/remove micro-benchmark page. Served from the gen
            # output dir if present, else this script's directory.
            import os
            base = os.path.dirname(os.path.abspath(__file__))
            path = "/tmp/sf-test/bench.html" if os.path.exists("/tmp/sf-test/bench.html") else os.path.join(base, "bench.html")
            body = open(path, "rb").read()
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        elif self.path.endswith(".webm"):
            # WebM init/media segments produced by gen_webm.py into /tmp/sf-test.
            import os
            path = os.path.join("/tmp/sf-test", os.path.basename(self.path))
            if not os.path.exists(path):
                self.send_response(404)
                self.end_headers()
                return
            body = open(path, "rb").read()
            self.send_response(200)
            self.send_header("Content-Type", "video/webm")
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
        with open("/tmp/sf-test/results.txt", "a") as f:
            f.write(data + "\n")
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def log_message(self, fmt, *args):
        print("REQ>>> " + (fmt % args), flush=True)
        with open("/tmp/sf-test/results.txt", "a") as f:
            f.write("REQ " + (fmt % args) + "\n")

http.server.HTTPServer(("127.0.0.1", 8799), H).serve_forever()
