#! /usr/bin/env python3

# Formatted by black.

from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from contextlib import contextmanager
from functools import partial
from urllib.parse import urlparse, parse_qs
from collections import Counter
import json
import re
import signal
import errno
import sys
import os
import threading

# A request to this path is answered with a description of that request instead
# of a file. Serving files can only show a test what is on disk; a test that
# checks what its own request put on the wire -- the bytes of a fetch() body,
# the method it went out with -- needs the request itself reflected back.
ECHO_PATH = "/echo"

# /status/<code> answers with that status and no Location, so a 3xx from it is
# a final response, not a redirect; "?acao" lets any origin read it.
# "?loc=<kind>" adds a malformed Location field, one curl does not follow.
STATUS_PATH = "/status/"
MALFORMED_LOCATIONS = {
    "empty": [("Location", "")],
    "twoempty": [("Location", ""), ("Location", "")],
    "space": [("Location ", "/status/200")],  # space before the colon
    "cr": [("Location", "\r/status/200")],  # bare CR before the value
}

# A request to /hit/<key> is counted, and /hits/<key> answers how many arrived:
# only the server can show a test that a request was never sent.
HIT_PATH = "/hit/"
HITS_PATH = "/hits/"
hits = Counter()
hits_lock = threading.Lock()

# /revalidate/<key> serves a script, revalidated(<key>), that the HTTP cache
# stores and must revalidate on every use (Last-Modified, or an ETag with
# "?etag"). A conditional request gets a 304 that repeats no validator and says
# "Content-Length: 0", neither of which may change the stored response; with
# "?moved" it gets a redirect to /revalidate/<key>-moved?etag instead.
# /hits/<key> counts the full requests and /hits/<key>-304 the conditional ones.
REVALIDATE_PATH = "/revalidate/"


class RequestHandler(SimpleHTTPRequestHandler):
    def is_echo_request(self):
        return urlparse(self.path).path == ECHO_PATH

    # --inject-script/--inject-into: a served file whose URL path ends with one
    # of the configured suffixes is answered with the inject script prepended
    # to it. This is how a vendor suite's unmodified harness (served straight
    # out of a submodule) gets wired to the test shell: the bridge runs before
    # the harness, in the same script, so nothing in the checkout is edited and
    # no HTML rewriting is needed (see tool/khronos/inject_report.js).
    def inject_target(self):
        if not options.inject_script:
            return None
        url_path = urlparse(self.path).path
        if not any(url_path.endswith(s) for s in options.inject_into):
            return None
        local = self.translate_path(url_path)
        return local if os.path.isfile(local) else None

    # --alias /prefix/=dir: a URL path under /prefix/ is served out of dir,
    # relative to --directory. Gecko's mochitests load their harness from
    # /tests/SimpleTest/, which its source tree keeps elsewhere.
    def translate_path(self, path):
        for prefix, target in options.alias:
            if path.startswith(prefix):
                path = "/" + target.strip("/") + "/" + path[len(prefix) :]
                break
        return super().translate_path(path)

    def serve_injected(self, local, head_only=False):
        with open(local, "rb") as f:
            payload = options.inject_payload + b"\n" + f.read()
        self.send_response(200)
        self.send_header("Content-Type", self.guess_type(local))
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        if not head_only:
            self.wfile.write(payload)

    def echo_request(self):
        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length) if length else b""
        payload = json.dumps(
            {
                "method": self.command,
                "contentType": self.headers.get("Content-Type"),
                "contentLength": self.headers.get("Content-Length"),
                "bodyHex": body.hex(),
                "bodyLen": len(body),
            }
        ).encode("utf-8")

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        # Test documents load over file://, so every echo request is
        # cross-origin and its response is unreadable without this.
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(payload)

    def answer(self, code, payload, acao, fields=()):
        self.send_response(code)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(payload)))
        if acao:
            self.send_header("Access-Control-Allow-Origin", "*")
        for name, value in fields:
            self.send_header(name, value)
        self.end_headers()
        self.wfile.write(payload)

    def answer_test_path(self):
        url = urlparse(self.path)
        if url.path.startswith(STATUS_PATH):
            code = int(url.path[len(STATUS_PATH) :])
            # 204 and 304 responses have no body.
            payload = b"" if code in (204, 304) else b"status %d" % code
            loc = parse_qs(url.query).get("loc", [""])[0]
            fields = MALFORMED_LOCATIONS.get(loc, ())
            self.answer(code, payload, "acao" in url.query, fields)
        elif url.path.startswith(HIT_PATH):
            with hits_lock:
                hits[url.path[len(HIT_PATH) :]] += 1
            self.answer(200, b"", False)
        elif url.path.startswith(HITS_PATH):
            with hits_lock:
                count = hits[url.path[len(HITS_PATH) :]]
            self.answer(200, str(count).encode("ascii"), True)
        elif url.path.startswith(REVALIDATE_PATH):
            key = url.path[len(REVALIDATE_PATH) :]
            conditional = (
                "If-None-Match" in self.headers or "If-Modified-Since" in self.headers
            )
            with hits_lock:
                hits[key + "-304" if conditional else key] += 1
            # super().end_headers() leaves out the no-cache headers this server
            # adds by default (see end_headers()), which would keep the script
            # out of the cache.
            if conditional and "moved" in url.query:
                self.send_response(302)
                self.send_header("Location", REVALIDATE_PATH + key + "-moved?etag")
                self.send_header("Content-Length", "0")
                super().end_headers()
                return True
            if conditional:
                self.send_response(304)
                self.send_header("Content-Length", "0")
                super().end_headers()
                return True
            payload = b"revalidated(%s);" % json.dumps(key).encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/javascript")
            self.send_header("Content-Length", str(len(payload)))
            self.send_header("Cache-Control", "max-age=60, no-cache")
            if "etag" in url.query:
                self.send_header("ETag", '"v1"')
            else:
                self.send_header("Last-Modified", "Mon, 05 Oct 2026 10:00:00 GMT")
            super().end_headers()
            self.wfile.write(payload)
        else:
            return False
        return True

    # A single "bytes=start-end" Range (RFC 9110 14.1.2): a media demuxer
    # reading over HTTP seeks with it, and without it an MP4 whose "moov" atom
    # follows the media data (the suite's video assets) cannot be probed, so
    # every test that uploads a video frame fails before it starts. The
    # injected files are served whole; nothing seeks in a script.
    def serve_range(self, local, head_only=False):
        match = re.fullmatch(r"bytes=(\d*)-(\d*)", self.headers.get("Range", ""))
        if not match or not (match.group(1) or match.group(2)):
            return False
        size = os.path.getsize(local)
        if match.group(1):
            start = int(match.group(1))
            end = int(match.group(2)) if match.group(2) else size - 1
        else:
            start = max(0, size - int(match.group(2)))
            end = size - 1
        end = min(end, size - 1)
        if start > end:
            self.send_response(416)
            self.send_header("Content-Range", "bytes */%d" % size)
            self.send_header("Content-Length", "0")
            self.end_headers()
            return True
        self.send_response(206)
        self.send_header("Content-Type", self.guess_type(local))
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("Content-Range", "bytes %d-%d/%d" % (start, end, size))
        self.send_header("Content-Length", str(end - start + 1))
        self.end_headers()
        if not head_only:
            with open(local, "rb") as f:
                f.seek(start)
                self.wfile.write(f.read(end - start + 1))
        return True

    def do_GET(self):
        if self.is_echo_request():
            self.echo_request()
            return
        if self.answer_test_path():
            return
        target = self.inject_target()
        if target:
            self.serve_injected(target)
            return
        local = self.translate_path(urlparse(self.path).path)
        if os.path.isfile(local) and self.serve_range(local):
            return
        super().do_GET()

    def do_HEAD(self):
        # Same Content-Length a GET of the injected file would carry.
        target = self.inject_target()
        if target:
            self.serve_injected(target, head_only=True)
            return
        local = self.translate_path(urlparse(self.path).path)
        if os.path.isfile(local) and self.serve_range(local, head_only=True):
            return
        super().do_HEAD()

    def do_POST(self):
        if self.is_echo_request():
            self.echo_request()
            return
        self.send_error(405)

    do_PUT = do_POST

    def do_OPTIONS(self):
        # A PUT is not a CORS-safelisted method, so a conforming engine sends
        # a preflight before it and drops the request if this does not answer.
        if not self.is_echo_request():
            self.send_error(405)
            return
        self.send_response(204)
        self.send_header("Content-Length", "0")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def end_headers(self):
        if options.no_cache:
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
            self.send_header("Pragma", "no-cache")
            self.send_header("Expires", "0")
        super().end_headers()

    def log_message(self, format, *args):
        if options.silent:
            return
        super().log_message(format, *args)


def start_server(options):
    try:
        httpd = ThreadingHTTPServer(
            (options.bind, options.port),
            partial(RequestHandler, directory=options.directory),
        )
    except OSError as e:
        if e.errno == errno.EADDRINUSE:
            print(f"{options.port} Port in use.")
            exit(1)
        else:
            raise e

    options.port = options.port or httpd.server_address[1]

    def signal_handler(signal_number, frame):
        httpd.server_close()
        print()
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    print(f"Running HTTPServer for {options} ..")

    httpd.serve_forever()


@contextmanager
def popen_server(directory, cwd, address=None, port=0, silent=False,
                 inject_script=None, inject_into=(), alias=()):
    from subprocess import Popen, PIPE

    process = None
    command = [sys.executable, __file__, "-d", directory, str(port)]
    command += ["--bind", address] if address else []
    command += ["--silent"] if silent == True else []
    if inject_script:
        command += ["--inject-script", inject_script]
        for suffix in inject_into:
            command += ["--inject-into", suffix]
    for mapping in alias:
        command += ["--alias", mapping]

    try:
        process = Popen(
            command,
            close_fds=True,
            cwd=cwd,
            stdout=PIPE if silent else None,
            stderr=PIPE if silent else None,
        )
        yield process
    finally:
        if not process:
            return
        process.terminate()
        process.wait()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--bind",
        "-b",
        default="0.0.0.0",
        metavar="ADDRESS",
        help="Set the bind address [default: %(default)s]",
    )
    parser.add_argument(
        "--directory",
        "-d",
        default=os.getcwd(),
        help="Set the directory. [Default: %(default)s]",
    )
    parser.add_argument(
        "port",
        action="store",
        default=0,
        type=int,
        nargs="?",
        help="Set the port [default: %(default)s (auto)]",
    )
    parser.add_argument(
        "--no-cache",
        "-nc",
        action="store_true",
        default=True,
        help="Set to add no-cache control headers [default: %(default)s]",
    )
    parser.add_argument(
        "--silent",
        "-s",
        action="store_true",
        default=False,
        help="Run in silent mode [default: %(default)s]",
    )
    parser.add_argument(
        "--inject-script",
        metavar="FILE",
        help="Script to prepend to every served file matching --inject-into",
    )
    parser.add_argument(
        "--inject-into",
        metavar="SUFFIX",
        action="append",
        default=[],
        help="URL path suffix (e.g. /js-test-pre.js) that gets --inject-script"
        " prepended; repeatable",
    )

    parser.add_argument(
        "--alias",
        metavar="PREFIX=DIR",
        action="append",
        default=[],
        help="Serve URL paths under PREFIX out of DIR (relative to --directory);"
        " repeatable",
    )

    global options
    options = parser.parse_args()
    options.alias = [tuple(a.split("=", 1)) for a in options.alias]
    # Read once; it is prepended to every matching response.
    options.inject_payload = b""
    if options.inject_script:
        with open(options.inject_script, "rb") as f:
            options.inject_payload = f.read()

    start_server(options)
