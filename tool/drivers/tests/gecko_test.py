#!/usr/bin/env python
# Gecko tests served out of the third_party/gecko submodule (see
# docs/gecko_tests.md). A test under layout/reftests/ is a reftest whose
# references its directory's reftest.list names (tests/vendor_reftest.py),
# rendered at the 800x1000 size Gecko's reftest harness uses. Every other
# listed test is a mochitest: SimpleTest.js prints one TEST-* line per check
# through tool/gecko/inject_report.js, and the test passes when it finished,
# passed a check, and printed no TEST-UNEXPECTED-* line.
import os
import posixpath
from urllib.parse import urlparse

import basics.starfish_basic_test as basictest
from basics.starfish_basic_test import record_result
from basics.utils import Strings
from tests import vendor_reftest

GECKO_DIR = "third_party/gecko"
REFTEST_PREFIX = "/layout/reftests/"
END_MARK = "wptTestEnd() called"


def reftest_list(directory):
    """Yields (relation, test, ref) for each == / != line of a reftest.list,
    names relative to its directory. Gecko's own annotations (fails,
    fuzzy(), skip-if(), pref(), ...) are its expectations, not ours, and are
    ignored; ours are the list's # lines."""
    path = os.path.join(GECKO_DIR, directory.lstrip("/"), "reftest.list")
    if not os.path.isfile(path):
        return
    with open(path) as fp:
        for line in fp:
            tokens = line.split("#", 1)[0].split()
            for i, token in enumerate(tokens[:-2]):
                if token in ("==", "!="):
                    yield token, tokens[i + 1], tokens[i + 2]
                    break


def references(url):
    """[(relation, ref_url)] for a reftest, [] for a mochitest."""
    parts = urlparse(url)
    if not parts.path.startswith(REFTEST_PREFIX):
        return []
    directory, name = posixpath.split(parts.path)
    if parts.query:  # one page tested several ways: page.html?variant
        name += "?" + parts.query
    base = "%s://%s%s/" % (parts.scheme, parts.netloc, directory)
    return [(relation, base + ref) for relation, test, ref in reftest_list(directory)
            if test == name]


def tc_handler(tc_file, output, err, show_progress=True, **kwargs):
    # A crash or timeout never reaches the end mark.
    log = output.split(END_MARK)[0]
    is_pass = (END_MARK in output and "TEST-PASS" in log and
               "TEST-UNEXPECTED-" not in log)
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
    return vendor_reftest.report(url, *vendor_reftest.run(url, refs, height=1000))
