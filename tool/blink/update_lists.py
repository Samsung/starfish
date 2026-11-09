#!/usr/bin/env python3
"""(Re)writes the blink_test list from the third_party/blink checkout.

The tests are those of every web test directory tool/blink/fetch.sh checks
out that Starfish can judge: a reftest (an -expected or -expected-mismatch
page next to it), a testharness.js test, or a js-test.js test with a text
-expected.txt (not a render tree dump). Pixel tests and pages that call
testRunner without a harness are not listed. How the list is merged with its
current state: tool/common/vendor_lists.py.
"""
import os
import sys

TOOL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOL)
sys.path.insert(0, os.path.join(TOOL, "drivers"))
from common.vendor_lists import REPO_ROOT, main, served_pages  # noqa: E402
from tests.blink_test import WEB_TESTS_DIR, references  # noqa: E402

CHECKOUT = os.path.join(REPO_ROOT, "third_party", "blink")
WEB_TESTS = os.path.join(REPO_ROOT, WEB_TESTS_DIR)
LIST = os.path.join(REPO_ROOT, "tool", "reftest", "cairo", "blink.res")
URL_PREFIX = "http://localhost:11013/"  # test_runner.py blink_test()
# Checked out for the CDP suite (tool/cdp_test), which runs these itself.
CDP_TESTS = os.path.join(WEB_TESTS, "http", "tests", "inspector-protocol")


def is_test(page, url):
    if "-manual." in os.path.basename(page):
        return False
    if references(url):
        return True
    with open(page, errors="replace") as fp:
        text = fp.read()
    if "testharness.js" in text:
        return True
    expected = os.path.splitext(page)[0] + "-expected.txt"
    if "js-test.js" not in text or not os.path.isfile(expected):
        return False
    with open(expected, errors="replace") as fp:
        return fp.read(9) != "layer at "


def find_tests():
    for page in served_pages(CHECKOUT):
        if page.startswith(CDP_TESTS + os.sep):
            continue
        url = URL_PREFIX + os.path.relpath(page, WEB_TESTS)
        if is_test(page, url):
            yield url


if __name__ == "__main__":
    main(__doc__.splitlines()[0], LIST, WEB_TESTS, find_tests)
