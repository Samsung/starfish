#! /usr/bin/env python3

# Formatted by black.

from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from contextlib import contextmanager
from functools import partial
from urllib.parse import urlparse
import json
import signal
import errno
import sys
import os

# A request to this path is answered with a description of that request instead
# of a file. Serving files can only show a test what is on disk; a test that
# checks what its own request put on the wire -- the bytes of a fetch() body,
# the method it went out with -- needs the request itself reflected back.
ECHO_PATH = "/echo"


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

    def do_GET(self):
        if self.is_echo_request():
            self.echo_request()
            return
        target = self.inject_target()
        if target:
            self.serve_injected(target)
            return
        super().do_GET()

    def do_HEAD(self):
        # Same Content-Length a GET of the injected file would carry.
        target = self.inject_target()
        if target:
            self.serve_injected(target, head_only=True)
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
                 inject_script=None, inject_into=()):
    from subprocess import Popen, PIPE

    process = None
    command = [sys.executable, __file__, "-d", directory, str(port)]
    command += ["--bind", address] if address else []
    command += ["--silent"] if silent == True else []
    if inject_script:
        command += ["--inject-script", inject_script]
        for suffix in inject_into:
            command += ["--inject-into", suffix]

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

    global options
    options = parser.parse_args()
    # Read once; it is prepended to every matching response.
    options.inject_payload = b""
    if options.inject_script:
        with open(options.inject_script, "rb") as f:
            options.inject_payload = f.read()

    start_server(options)
