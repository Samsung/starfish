#!/usr/bin/env python3
"""(Re)writes the webkit_test list from the third_party/webkit checkout.

The tests are those of every LayoutTests directory tool/webkit/fetch.sh
checks out that Starfish can judge: a reftest (an -expected or
-expected-mismatch page next to it), or a page that loads one of the harness
scripts tool/webkit/inject_report.js is injected into and has a text
-expected.txt (not a render tree dump). How the list is merged with its
current state: tool/common/vendor_lists.py.
"""
import os
import sys

TOOL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOL)
sys.path.insert(0, os.path.join(TOOL, "drivers"))
from common.vendor_lists import REPO_ROOT, main, served_pages  # noqa: E402
from tests.webkit_test import LAYOUT_TESTS_DIR, references  # noqa: E402

CHECKOUT = os.path.join(REPO_ROOT, "third_party", "webkit")
LAYOUT_TESTS = os.path.join(REPO_ROOT, LAYOUT_TESTS_DIR)
LIST = os.path.join(REPO_ROOT, "tool", "reftest", "cairo", "webkit.res")
URL_PREFIX = "http://localhost:11012/"  # test_runner.py webkit_test()
# Keep in sync with the scripts webkit_test() in tool/runner/test_runner.py
# injects into.
HARNESSES = ("js-test-pre.js", "js-test.js", "selfhtml.js")


def is_test(page, url):
    if references(url):
        return True
    expected = os.path.splitext(page)[0] + "-expected.txt"
    if not os.path.isfile(expected):
        return False
    with open(expected, errors="replace") as fp:
        if fp.read(9) == "layer at ":
            return False
    with open(page, errors="replace") as fp:
        text = fp.read()
    return any(h in text for h in HARNESSES)


def find_tests():
    for page in served_pages(CHECKOUT):
        url = URL_PREFIX + os.path.relpath(page, LAYOUT_TESTS)
        if is_test(page, url):
            yield url


if __name__ == "__main__":
    main(__doc__.splitlines()[0], LIST, LAYOUT_TESTS, find_tests)
