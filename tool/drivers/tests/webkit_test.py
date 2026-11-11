#!/usr/bin/env python
# WebKit LayoutTests served out of the third_party/webkit submodule (see
# docs/webkit_layouttests.md). A test with an -expected(-mismatch) page next
# to it is a reftest (tests/vendor_reftest.py). Every other listed test gets
# tool/webkit/inject_report.js to print its text dump and call
# wptTestEnd(), and passes when the dump carries the same result words, in
# the same order, as its upstream -expected.txt.
import os
import re
from urllib.parse import urlparse

import basics.starfish_basic_test as basictest
from basics.starfish_basic_test import record_result
from basics.utils import Strings
from tests import vendor_reftest

LAYOUT_TESTS_DIR = "third_party/webkit/LayoutTests"
END_MARK = "wptTestEnd() called"
# js-test pages print PASS/FAIL lines; the DOM conformance suites print a
# Status of Success, failure or Skipped.
RE_KEYWORDS = re.compile(r"PASS|FAIL|Success|failure|Skipped")


def expected_file(url):
    test = urlparse(url).path.lstrip("/")
    return os.path.join(LAYOUT_TESTS_DIR, os.path.splitext(test)[0] + "-expected.txt")


def tc_handler(tc_file, output, err, show_progress=True, **kwargs):
    expected_path = expected_file(tc_file)
    if os.path.isfile(expected_path):
        with open(expected_path, errors="replace") as fp:
            expected = RE_KEYWORDS.findall(fp.read())
        # A crash or timeout never reaches the end mark.
        is_pass = (END_MARK in output and
                   RE_KEYWORDS.findall(output.split(END_MARK)[0]) == expected)
    else:
        is_pass = False
        output = "missing " + expected_path
    record_result(tc_file, is_pass)
    if show_progress:
        print((Strings.PASS_SIGN if is_pass else Strings.FAIL_SIGN) + tc_file)
        if not is_pass:
            print(output)
    return is_pass


def references(url):
    """[(relation, ref_url)] for a reftest, [] for any other test."""
    return vendor_reftest.expected_references(url, LAYOUT_TESTS_DIR)


def device_scale_args(url):
    """WebKitTestRunner renders a test whose path holds /hidpi-3x- or /hidpi-
    at device scale 3 or 2 (overrideDeviceScaleFactorForTest in
    Tools/TestRunnerShared/TestFeatures.cpp); its reference assumes that
    scale."""
    path = urlparse(url).path
    if "/hidpi-3x-" in path:
        return ["--device-pixel-ratio=3"]
    if "/hidpi-" in path:
        return ["--device-pixel-ratio=2"]
    return []


def case_runner(tc):
    url = tc[1]
    refs = references(url)
    if not refs:
        return basictest.case_runner(tc)
    return vendor_reftest.report(
        url, *vendor_reftest.run(url, refs, extra_args=device_scale_args(url)))
