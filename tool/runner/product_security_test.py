#!/usr/bin/env python3
"""Exercise file-origin and WebGL boundaries without native test bindings."""

import argparse
import base64
import concurrent.futures
import contextlib
import functools
import http.server
import json
import os
from pathlib import Path
import subprocess
import threading
import tempfile
from urllib.parse import urlencode

ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / "tests"
PAGE = TESTS / "assertion/security/file-origin-policy.html"
TARGET = TESTS / "fixtures/security/file-origin-target.html"
READABLE = TESTS / "fixtures/security/file-origin-readable.txt"
MARKER = "PRODUCT_SECURITY:"


class Handler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        if self.path == "/redirect-to-file":
            self.send_response(302)
            self.send_header("Location", TARGET.as_uri())
            self.end_headers()
            return
        super().do_GET()


@contextlib.contextmanager
def server():
    handler = functools.partial(Handler, directory=str(TESTS))
    with http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler) as httpd:
        thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        thread.start()
        try:
            yield "http://127.0.0.1:%d" % httpd.server_port
        finally:
            httpd.shutdown()
            thread.join()


def cases(base):
    for action in ("fetch", "xhr", "frame", "srcdoc"):
        for local in (False, True):
            options = {
                "action": action,
                "allowed": "true" if local else "false",
                "file": TARGET.as_uri(),
                "readable": READABLE.as_uri(),
            }
            url = (PAGE.as_uri() if local else
                   base + "/assertion/security/file-origin-policy.html")
            yield ("%s-%s" % ("local" if local else "http", action),
                   url + "?" + urlencode(options), False,
                   local and action == "frame")

    for action in ("assign", "replace", "href", "anchor", "form"):
        options = {
            "action": action, "allowed": "false",
            "file": TARGET.as_uri(),
        }
        yield ("http-" + action,
               base + "/assertion/security/file-origin-policy.html?" +
               urlencode(options), False, False)

    for action in ("fetch", "xhr", "frame", "replace"):
        options = {
            "action": action, "allowed": False,
            "file": TARGET.as_uri(), "readable": READABLE.as_uri(),
        }
        source = ("<script>window.productSecurityOptions=" +
                  json.dumps(options) + ";</script>" + PAGE.read_text())
        data_url = ("data:text/html;base64," +
                    base64.b64encode(source.encode()).decode())
        yield ("data-" + action, data_url, False, False)

    yield ("local-opaque-child",
           PAGE.as_uri() + "?action=opaque-child", False, False)
    yield ("native-local-load", TARGET.as_uri(), True, True)
    for action in ("assign", "replace"):
        yield ("local-" + action,
               PAGE.as_uri() + "?" + urlencode({
                   "action": action, "allowed": "true",
                   "file": TARGET.as_uri(),
               }), True, True)
    # A network redirect must not turn an HTTP initiator into a local app.
    yield ("http-file-redirect",
           base + "/assertion/security/file-origin-policy.html?" +
           urlencode({"action": "frame", "allowed": "false",
                      "file": base + "/redirect-to-file"}), False, False)


def run_case(browser, case, timeout, allow_test_hooks):
    name, url, target_expected, target_allowed = case
    environment = os.environ.copy()
    environment.setdefault("LP_NUM_THREADS", "2")
    try:
        with tempfile.TemporaryDirectory(
                prefix="starfish-product-security-") as storage:
            result = subprocess.run(
                [str(browser), url, "--hide-window", "--disable-console",
                 "--timeout=%d" % timeout, "--storage-dir=" + storage],
                cwd=ROOT, env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True, errors="replace",
                timeout=timeout + 10,
            )
        output = result.stdout
        if result.returncode:
            raise AssertionError("browser exited %d" % result.returncode)
        target_loaded = "FILE_ORIGIN_TARGET_LOADED" in output
        if target_loaded and not target_allowed:
            raise AssertionError("untrusted content executed a local file")
        if target_expected:
            if not target_loaded:
                raise AssertionError("trusted local navigation did not complete")
        else:
            reports = []
            for line in output.splitlines():
                if MARKER in line:
                    reports.append(json.JSONDecoder().raw_decode(
                        line.split(MARKER, 1)[1])[0])
            if len(reports) != 1:
                raise AssertionError("expected exactly one completion report")
            report = reports[0]
            if not report["passed"]:
                raise AssertionError(json.dumps(report, ensure_ascii=False))
            if report["testHooks"] and not allow_test_hooks:
                raise AssertionError("native test bindings are enabled")
        return name, True, ""
    except (AssertionError, subprocess.TimeoutExpired, ValueError) as error:
        output = locals().get("output", "")
        return name, False, str(error) + "\n" + output[-5000:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--browser", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=3)
    parser.add_argument("--webgl", choices=("on", "off"), default="on")
    parser.add_argument(
        "--allow-test-hooks", action="store_true",
        help="permit a test build only for before/after comparison")
    arguments = parser.parse_args()
    browser = arguments.browser.resolve()
    if not browser.is_file():
        parser.error("browser does not exist: %s" % browser)
    if arguments.jobs < 1 or arguments.timeout < 3:
        parser.error("--jobs must be positive and --timeout at least 3")

    with server() as base:
        selected = list(cases(base))
        if arguments.webgl == "on":
            selected.append((
                "webgl-buffers",
                base + "/assertion/canvas/webgl-buffer-input-validation.html",
                False, False))
        else:
            source = """<script>
var passed = !document.createElement('canvas').getContext('webgl') &&
             !document.createElement('canvas').getContext('webgl2');
console.log('PRODUCT_SECURITY:' + JSON.stringify({
  passed: passed, testHooks: typeof testEnd === 'function'
}));
</script>"""
            data_url = ("data:text/html;base64," +
                        base64.b64encode(source.encode()).decode())
            selected.append(("webgl-disabled", data_url, False, False))
        run = functools.partial(
            run_case, browser, timeout=arguments.timeout,
            allow_test_hooks=arguments.allow_test_hooks)
        with concurrent.futures.ThreadPoolExecutor(
                max_workers=arguments.jobs) as pool:
            results = list(pool.map(run, selected))
    for name, passed, detail in results:
        print("%s %s" % ("PASS" if passed else "FAIL", name))
        if detail:
            print(detail)
    failed = sum(not passed for _, passed, _ in results)
    print("%d passed, %d failed" % (len(results) - failed, failed))
    return bool(failed)


if __name__ == "__main__":
    raise SystemExit(main())
