#!/usr/bin/env python
# Reftests of the browser-vendor suites (webkit_test, blink_test,
# gecko_test): the test page and each of its references are rendered and
# compared the way wpt_serve_reftest compares a WPT reftest, with the same
# screenshot and imgdiff helpers (tool/wpt/scripts/wpt_reftest.py). Only how
# a test finds its references differs per vendor, so the drivers pass those
# in.
import os
import sys
import tempfile
from urllib.parse import urlparse

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "..", "wpt", "scripts"))
from wpt_reftest import _images_differ, _screenshot  # noqa: E402

from basics.constants import ENVOPTS
from basics.starfish_basic_test import record_result
from basics.utils import Strings

DEFAULT_TIMEOUT_SEC = 30
REF_EXTENSIONS = (".html", ".htm", ".xhtml", ".xht", ".svg")


def expected_references(url, root):
    """WebKit and Blink name a reftest's references after it: page-expected.html
    must match, page-expected-mismatch.html must not (any of REF_EXTENSIONS).
    root is the served directory the URL path is relative to. Returns
    [(relation, ref_url)], empty for a test that is no reftest."""
    stem = os.path.splitext(url)[0]
    local_stem = os.path.join(root, os.path.splitext(urlparse(url).path)[0].lstrip("/"))
    return [(relation, stem + suffix + ext)
            for suffix, relation in (("-expected", "=="), ("-expected-mismatch", "!="))
            for ext in REF_EXTENSIONS
            if os.path.isfile(local_stem + suffix + ext)]


def run(url, references, width=800, height=600):
    """references: [(relation, ref_url)], relation "==" or "!=". Every one
    must hold. Returns (ok, reason)."""
    timeout = float(os.environ.get(ENVOPTS.TIMEOUT) or DEFAULT_TIMEOUT_SEC)
    with tempfile.TemporaryDirectory(prefix="vendor-reftest-") as tmp:
        test_png = os.path.join(tmp, "test.png")
        ok, reason, _ = _screenshot(url, test_png, timeout, width, height)
        if not ok:
            return False, reason
        for i, (relation, ref_url) in enumerate(references):
            ref_png = os.path.join(tmp, "ref%d.png" % i)
            ok, reason, _ = _screenshot(ref_url, ref_png, timeout, width, height)
            if not ok:
                return False, "REF_" + reason
            try:
                differ = _images_differ(test_png, ref_png, timeout)
            except Exception as e:
                return False, "IMGDIFF_ERROR: %s" % e
            if differ == (relation == "=="):
                return False, "IMG_MISMATCH" if differ else "IMG_UNEXPECTED_MATCH"
    return True, "OK"


def report(url, ok, reason, show_progress=True):
    record_result(url, ok)
    if show_progress:
        print((Strings.PASS_SIGN if ok else Strings.FAIL_SIGN) + url +
              ("" if ok else " " + reason))
    return ok
