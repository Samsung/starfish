#!/usr/bin/env python3

"""Serve the pages used by Chromium's CDP behavior tests over HTTP."""

import contextlib
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
import pathlib
import threading
from urllib.parse import quote

from . import launcher

# Chromium's behavior tests expect their pages at this address.
HOST = "127.0.0.1"
PORT = 8000
DOCUMENT_ROOT = launcher.REPOSITORY_DIR / "tests" / "cdp"
# Chromium's suite comes from the third_party/blink checkout, and its tests
# expect their pages under this URL path.
INSPECTOR_ROOT = (launcher.REPOSITORY_DIR /
                  "third_party/blink/third_party/blink/web_tests/http/tests"
                  "/inspector-protocol")
INSPECTOR_URL_PATH = "/inspector-protocol"


def target_base_url(test_path):
    """Return the HTTP directory that TestRunner.url() should resolve from."""
    parent = pathlib.Path(os.path.abspath(test_path)).parent
    if parent.is_relative_to(INSPECTOR_ROOT):
        relative = pathlib.PurePosixPath(
            INSPECTOR_URL_PATH.lstrip("/")
        ) / parent.relative_to(INSPECTOR_ROOT).as_posix()
    else:
        relative = parent.relative_to(DOCUMENT_ROOT)
    return "http://%s:%d/%s/" % (HOST, PORT,
                                 quote(relative.as_posix(), safe="/"))


class _QuietHandler(SimpleHTTPRequestHandler):
    """Serve files without logging each request."""

    def log_message(self, _format, *args):
        pass

    def translate_path(self, path):
        root = self.directory
        if path.startswith(INSPECTOR_URL_PATH + "/"):
            self.directory = str(INSPECTOR_ROOT)
            path = path[len(INSPECTOR_URL_PATH):]
        try:
            return super().translate_path(path)
        finally:
            self.directory = root


@contextlib.contextmanager
def running(document_root=DOCUMENT_ROOT):
    """Serve document_root on HOST:PORT until the block ends."""
    handler = partial(_QuietHandler, directory=str(document_root))
    server = ThreadingHTTPServer((HOST, PORT), handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield "http://%s:%d" % server.server_address
    finally:
        server.shutdown()
        server.server_close()
