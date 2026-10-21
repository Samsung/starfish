#!/usr/bin/env python3

"""Serve the pages used by Chromium's CDP behavior tests over HTTP."""

import contextlib
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import pathlib
import threading
from urllib.parse import quote

from . import launcher

# Chromium's behavior tests expect their pages at this address.
HOST = "127.0.0.1"
PORT = 8000
DOCUMENT_ROOT = launcher.REPOSITORY_DIR / "test" / "cdp"


def target_base_url(test_path):
    """Return the HTTP directory that TestRunner.url() should resolve from."""
    relative = pathlib.Path(test_path).resolve().parent.relative_to(
        DOCUMENT_ROOT.resolve()
    )
    return "http://%s:%d/%s/" % (HOST, PORT,
                                 quote(relative.as_posix(), safe="/"))


class _QuietHandler(SimpleHTTPRequestHandler):
    """Serve files without logging each request."""

    def log_message(self, _format, *args):
        pass


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
