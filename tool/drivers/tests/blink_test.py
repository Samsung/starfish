#!/usr/bin/env python
# Blink web tests served out of the third_party/blink submodule (see
# docs/blink_web_tests.md). A test with an -expected(-mismatch) page next to
# it is a reftest (tests/vendor_reftest.py). Every other listed test prints
# a text dump through tool/webkit/inject_report.js and passes when the dump
# carries the same result words, in the same order, as its upstream
# -expected.txt; a testharness.js test without one is expected to pass every
# subtest, which Blink's testharnessreport.js prints as no result lines.
import os
import re
from urllib.parse import urlparse

import basics.starfish_basic_test as basictest
from basics.starfish_basic_test import record_result
from basics.utils import Strings
from tests import vendor_reftest

WEB_TESTS_DIR = "third_party/blink/third_party/blink/web_tests"
END_MARK = "wptTestEnd() called"
RE_KEYWORDS = re.compile(r"PASS|FAIL|TIMEOUT|NOTRUN|PRECONDITION_FAILED|Harness Error")


def local_path(url):
    return os.path.join(WEB_TESTS_DIR, urlparse(url).path.lstrip("/"))


def references(url):
    """[(relation, ref_url)] for a reftest, [] for any other test."""
    return vendor_reftest.expected_references(url, WEB_TESTS_DIR)


def tc_handler(tc_file, output, err, show_progress=True, **kwargs):
    expected_path = os.path.splitext(local_path(tc_file))[0] + "-expected.txt"
    expected = []
    if os.path.isfile(expected_path):
        with open(expected_path, errors="replace") as fp:
            expected = RE_KEYWORDS.findall(fp.read())
    # A crash or timeout never reaches the end mark.
    is_pass = (END_MARK in output and
               RE_KEYWORDS.findall(output.split(END_MARK)[0]) == expected)
    record_result(tc_file, is_pass)
    if show_progress:
        print((Strings.PASS_SIGN if is_pass else Strings.FAIL_SIGN) + tc_file)
        if not is_pass:
            print(output)
    return is_pass


def case_runner(tc):
    url = tc[1]
    refs = references(url)
    if not refs:
        return basictest.case_runner(tc)
    return vendor_reftest.report(url, *vendor_reftest.run(url, refs))
