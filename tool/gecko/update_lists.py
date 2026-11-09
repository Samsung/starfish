#!/usr/bin/env python3
"""(Re)writes the gecko_test list from the third_party/gecko checkout.

The tests are those of every directory tool/gecko/fetch.sh checks out: the
== / != tests of a directory with a reftest.list, and the mochitests its
mochitest*.toml manifests list (not the chrome ones, which need a
privileged window). How the list is merged with its current
state: tool/common/vendor_lists.py.
"""
import glob
import os
import re
import subprocess
import sys

TOOL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOL)
sys.path.insert(0, os.path.join(TOOL, "drivers"))
from common.vendor_lists import REPO_ROOT, main  # noqa: E402
from tests.gecko_test import GECKO_DIR, reftest_list  # noqa: E402

CHECKOUT = os.path.join(REPO_ROOT, GECKO_DIR)
LIST = os.path.join(REPO_ROOT, "tool", "reftest", "cairo", "gecko.res")
URL_PREFIX = "http://localhost:11014/"  # test_runner.py gecko_test()
# A test is a section of a mochitest*.toml manifest: ["test_name.html"]
RE_MOCHITEST = re.compile(r'^\["([^"/]+\.x?html?)"\]', re.M)


def find_tests():
    served = subprocess.run(["git", "-C", CHECKOUT, "sparse-checkout", "list"],
                            capture_output=True, text=True, check=True).stdout.split()
    for top in sorted(served):
        tests = [test for _, test, _ in reftest_list(top)]
        for toml in sorted(glob.glob(os.path.join(CHECKOUT, top, "mochitest*.toml"))):
            with open(toml) as fp:
                tests += sorted(RE_MOCHITEST.findall(fp.read()))
        for test in tests:
            yield URL_PREFIX + top + "/" + test


if __name__ == "__main__":
    main(__doc__.splitlines()[0], LIST, CHECKOUT, find_tests)
