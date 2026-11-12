"""File-origin policy cases and their loopback redirect fixture."""

import base64
import json
from urllib.parse import urlencode

from . import Case, Suite

PAGE = "assertion/security/file-origin-policy.html"
TARGET = "fixtures/security/file-origin-target.html"
READABLE = "fixtures/security/file-origin-readable.txt"
TARGET_MARKER = "FILE_ORIGIN_TARGET_LOADED"


def cases(context):
    target = context.file(TARGET)
    readable = context.file(READABLE)

    def redirect(handler):
        handler.send_response(302)
        handler.send_header("Location", target)
        handler.end_headers()

    context.add_route("/file-origin/redirect", redirect)

    def policy_case(name, url, denied=False, action="", target_allowed=False):
        required = ()
        if denied:
            blocked = readable if action in ("fetch", "xhr", "srcdoc") else target
            required = ("console.error: Not allowed to load local resource: " +
                        blocked,)
        return Case(name, url, required_output=required,
                    forbidden_output=() if target_allowed else (TARGET_MARKER,))

    for action in ("fetch", "xhr", "frame", "srcdoc"):
        for local in (False, True):
            options = {"action": action, "allowed": str(local).lower(),
                       "file": target, "readable": readable}
            url = context.file(PAGE) if local else context.http(PAGE)
            yield policy_case(
                "%s-%s" % ("local" if local else "http", action),
                url + "?" + urlencode(options), denied=not local,
                action=action, target_allowed=local and action == "frame")

    for action in ("assign", "replace", "href", "anchor", "form"):
        url = context.http(PAGE) + "?" + urlencode({
            "action": action, "allowed": "false", "file": target})
        yield policy_case("http-" + action, url, denied=True, action=action)

    for action in ("fetch", "xhr", "frame", "replace"):
        options = {"action": action, "allowed": False,
                   "file": target, "readable": readable}
        source = ("<script>window.productSecurityOptions=" +
                  json.dumps(options) + ";</script>" + context.read(PAGE))
        url = "data:text/html;base64," + base64.b64encode(source.encode()).decode()
        yield policy_case("data-" + action, url, denied=True, action=action)

    yield policy_case("local-opaque-child",
                      context.file(PAGE) + "?action=opaque-child")
    yield Case("native-local-load", target + "?allowed=true",
               required_output=(TARGET_MARKER,))
    for action in ("assign", "replace"):
        yield Case("local-" + action, context.file(PAGE) + "?" + urlencode({
            "action": action, "allowed": "true", "file": target + "?allowed=true",
        }), required_output=(TARGET_MARKER,))
    yield policy_case("http-file-redirect",
                      context.http(PAGE) + "?" + urlencode({
                          "action": "frame", "allowed": "false",
                          "file": context.http("/file-origin/redirect"),
                      }))


SUITE = Suite("file-origin", "Local-file privileges and navigation boundaries",
              cases)
