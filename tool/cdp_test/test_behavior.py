#!/usr/bin/env python3

import pathlib
import sys
import unittest

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR.parent))

from driver import behavior, launcher


class BehaviorReasonTest(unittest.TestCase):
    def test_output_difference_does_not_store_the_repository_path(self):
        local_url = (
            launcher.REPOSITORY_DIR
            / "test/cdp/inspector-protocol/resources/test-page.html"
        ).as_uri()
        error = AssertionError(
            "line 4: expected 'current location: "
            "http://127.0.0.1:8000/inspector-protocol/resources/test-page.html', "
            "got 'current location: %s'" % local_url
        )

        category, detail = behavior.reason(error)

        self.assertEqual("output-differs", category)
        self.assertNotIn(str(launcher.REPOSITORY_DIR), detail)
        self.assertIn("file://<repo>/test/cdp/inspector-protocol/", detail)

    def test_harness_error_does_not_store_the_repository_path(self):
        script = (
            launcher.REPOSITORY_DIR
            / "test/cdp/inspector-protocol/example/example.js"
        )

        category, detail = behavior.reason(
            RuntimeError("node exited 1: %s:23" % script)
        )

        self.assertEqual("harness-error", category)
        self.assertNotIn(str(launcher.REPOSITORY_DIR), detail)
        self.assertEqual(
            "node exited 1: <repo>/test/cdp/inspector-protocol/"
            "example/example.js:23",
            detail,
        )

if __name__ == "__main__":
    unittest.main()
