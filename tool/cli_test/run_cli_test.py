#!/usr/bin/env python3

import argparse
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
TEST_CASE_DIR = REPOSITORY_DIR / "test" / "cli"
CLI_BINARY_ENVIRONMENT_VARIABLE = "LWE_CLI_TEST_BINARY"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
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
    suite = unittest.defaultTestLoader.discover(
        str(TEST_CASE_DIR),
        pattern="test_*.py",
    )
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
