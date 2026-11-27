"""Referer sent for requests made by a file: document."""

import json
from urllib.parse import parse_qs, urlencode, urlsplit

from . import Case, Suite

PAGE = "assertion/security/file-referrer.html"


def cases(context):
    def echo(handler):
        query = parse_qs(urlsplit(handler.path).query)
        report = json.dumps({
            "policy": query.get("policy", [""])[0],
            "referer": handler.headers.get("Referer", ""),
        }).replace("<", "\\u003c")
        body = ("<!doctype html><script>parent.postMessage(%s, '*');</script>"
                % report).encode()
        handler.send_response(200)
        handler.send_header("Content-Type", "text/html")
        handler.send_header("Content-Length", str(len(body)))
        handler.end_headers()
        handler.wfile.write(body)

    context.add_route("/file-referrer/echo", echo)
    yield Case("frame", context.file(PAGE) + "?" + urlencode(
        {"echo": context.http("/file-referrer/echo")}))


SUITE = Suite("file-referrer", "Referer of requests from a file: document",
              cases)
