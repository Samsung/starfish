#!/usr/bin/env python3

"""Run one Chromium inspector-protocol test.

The test is JavaScript, so it runs under Node. This module starts a browser,
hands its browser-level CDP endpoint to Node, and compares the printed log
with the test's -expected.txt. The comparison is exact.
"""

import pathlib
import re
import subprocess

from . import client, launcher

# The Chromium tests are JavaScript, so they run under Node. node/main.mjs
# is the only part of this suite that is not Python.
NODE_MAIN = pathlib.Path(__file__).resolve().parent / "node" / "main.mjs"


def entries(suite_dir):
    """Every test in the suite, as paths relative to the suite root.

    A .js file under resources/ is a helper loaded by a test, not a test.
    A test without an -expected.txt has nothing to compare against.
    """
    found = []
    for path in sorted(suite_dir.rglob("*.js")):
        relative = path.relative_to(suite_dir)
        if "resources" in relative.parts:
            continue
        expected = path.with_name(path.stem + "-expected.txt")
        if expected.exists():
            found.append(relative.as_posix())
    return found


def label(entry):
    return entry


def run(starfish, suite_dir, entry, timeout):
    """Run one test and raise AssertionError when the output differs."""
    if not NODE_MAIN.is_file():
        raise RuntimeError("node/main.mjs is missing at %s" % NODE_MAIN)
    test_path = suite_dir / entry
    expected = test_path.with_name(test_path.stem + "-expected.txt").read_text()

    with launcher.running(starfish, timeout) as endpoint:
        # The suite starts from a browser-level session and creates its own
        # target, so it needs the browser endpoint, not the page one.
        version = client.get_json(endpoint, "/json/version")
        browser_url = version.get("webSocketDebuggerUrl")
        if not browser_url:
            raise RuntimeError("/json/version has no webSocketDebuggerUrl")
        try:
            result = subprocess.run(
                ["node", str(NODE_MAIN), browser_url, str(suite_dir),
                 str(test_path), str(int(timeout))],
                capture_output=True, text=True, timeout=timeout * 3 + 30)
        except subprocess.TimeoutExpired:
            # TimeoutExpired is not an OSError, so without this the whole
            # run would abort on one wedged test instead of recording it.
            raise RuntimeError("node did not exit within %d seconds"
                               % (timeout * 3 + 30))

    if result.returncode != 0:
        first_line = next(iter(result.stderr.strip().splitlines()), "")
        raise RuntimeError("node exited %d: %s"
                           % (result.returncode, first_line))
    if result.stdout.strip() != expected.strip():
        raise AssertionError(_difference(result.stdout, expected))


def reason(error):
    """Turn a raw failure into a (category, detail) pair.

    A list of 900 raw diffs is unreadable, so each failure is grouped. Every
    marker below is matched against the observed side only, never against the
    expected output, or a test that merely prints "-32601" would be filed as
    a missing method. Each category names what was seen, not why.
    """
    text = str(error)
    seen = _observed(text)
    if "HARNESS TIMEOUT" in seen:
        # main.mjs prints this when it runs out of time. What the test was
        # waiting for is not recorded, so the name does not claim to know.
        category = "timeout"
    elif "-32601" in seen:
        category = "not-implemented"
    elif "-32602" in seen:
        category = "bad-params"
    elif "Could not connect to ws" in seen or "WebSocket error:" in seen:
        category = "no-connection"
    elif ("node exited" in seen or "node did not exit" in seen
          or "did not evaluate" in seen):
        category = "harness-error"
    elif "Error while executing test script:" in seen:
        # main.mjs prints this when the test function throws. The test never
        # finished, so its output says nothing about Starfish yet.
        category = "script-error"
    else:
        category = "output-differs"
    return category, " ".join(text.split())


def _observed(text):
    """What the run saw, out of a failure that may be a diff.

    A diff carries the test's expected output too, and matching a marker
    there would name the category after what Starfish was supposed to do.
    Anything that is not a diff was raised by this module, so all of it is
    what the run saw.
    """
    match = _DIFFERENCE.search(text)
    return match.group(4) if match else text


_DIFFERENCE = re.compile(r"expected (['\"])(.*?)\1, got (['\"])(.*)$",
                         re.DOTALL)


def _difference(got, expected):
    """The first line that differs, which is what a reader needs."""
    got_lines = got.strip().splitlines()
    expected_lines = expected.strip().splitlines()
    for index in range(max(len(got_lines), len(expected_lines))):
        mine = got_lines[index] if index < len(got_lines) else "(nothing)"
        theirs = (expected_lines[index]
                  if index < len(expected_lines) else "(nothing)")
        if mine != theirs:
            return "line %d: expected %r, got %r" % (index + 1, theirs[:120],
                                                     mine[:120])
    return "output differs only in trailing whitespace"
