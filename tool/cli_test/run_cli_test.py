#!/usr/bin/env python3

import argparse
import json
import os
from pathlib import Path
import sys
import unittest

# tool/, for common and repo_paths.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from common import storage  # noqa: E402

from driver import STORAGE_GROUP  # noqa: E402

REPOSITORY_DIR = Path(__file__).resolve().parents[2]
DEFAULT_BINARY = (
    REPOSITORY_DIR / "out" / "headless" / "bin" /
    "lightweight-web-engine-cli"
)
CLI_TEST_DIR = Path(__file__).resolve().parent
TEST_LIST = CLI_TEST_DIR / "testlist.json"
TEST_CASE_DIR = REPOSITORY_DIR / "test" / "cli"
CLI_BINARY_ENVIRONMENT_VARIABLE = "LWE_CLI_TEST_BINARY"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
    parser.add_argument(
        "--all", action="store_true",
        help="run every discovered test, not only the ones %s enables"
        % TEST_LIST.name)
    arguments = parser.parse_args()

    binary = arguments.binary.resolve()
    if not binary.is_file():
        parser.error("CLI binary is missing at %s" % binary)

    if not TEST_CASE_DIR.is_dir():
        parser.error(
            "CLI test cases are missing at %s; check out the test submodule"
            % TEST_CASE_DIR
        )

    # A run stopped with Ctrl-C never reaches tearDown, and the daemon it left
    # has no idle timeout, so it would keep running with its engine. Clear
    # those before starting, and drop this run's own storage on the way out.
    strays = storage.sweep_strays()
    if strays:
        print("killed %d process(es) left by an earlier run" % strays)
    stale = storage.sweep_homes(STORAGE_GROUP)
    if stale:
        print("removed the storage of %d earlier run(s)" % stale)
    storage.register_cleanup(STORAGE_GROUP)

    os.environ[CLI_BINARY_ENVIRONMENT_VARIABLE] = str(binary)
    # test_*.py under TEST_CASE_DIR import the driver package below, so both
    # directories need to be on sys.path before discovery imports them.
    sys.path.insert(0, str(CLI_TEST_DIR))
    sys.path.insert(0, str(TEST_CASE_DIR))
    discovered = unittest.defaultTestLoader.discover(
        str(TEST_CASE_DIR),
        pattern="test_*.py",
    )
    tests = list(flatten(discovered))
    if not arguments.all:
        tests = enabled_tests(tests, parser)
    result = unittest.TextTestRunner(verbosity=2).run(
        unittest.TestSuite(tests))
    return 0 if result.wasSuccessful() else 1


def flatten(suite):
    for test in suite:
        if isinstance(test, unittest.TestSuite):
            yield from flatten(test)
        else:
            yield test


def enabled_tests(tests, parser):
    """Keep only the tests that TEST_LIST names.

    The test submodule can contain tests for features that are not merged
    yet. A test runs only after the change that makes it pass adds it to
    the list.
    """
    enabled = set(json.loads(TEST_LIST.read_text())["tests"])
    known = {test.id() for test in tests}
    unknown = sorted(enabled - known)
    if unknown:
        parser.error(
            "%s names %d tests that do not exist, for example %s"
            % (TEST_LIST.name, len(unknown), ", ".join(unknown[:3]))
        )

    # A test file that fails to import becomes one _FailedTest. Its ID is
    # never in the list, so keep it. Otherwise the broken file would be
    # skipped without a report.
    selected = [test for test in tests
                if test.id() in enabled
                or isinstance(test, unittest.loader._FailedTest)]
    disabled = len(tests) - len(selected)
    if disabled:
        print("skipped %d test(s) that %s does not enable; pass --all to "
              "run them" % (disabled, TEST_LIST.name))
    return selected


if __name__ == "__main__":
    raise SystemExit(main())
