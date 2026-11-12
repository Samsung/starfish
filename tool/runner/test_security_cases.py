#!/usr/bin/env python3
"""Contracts for discovering and running future browser security groups."""

import importlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
from urllib.request import Request, urlopen

sys.path.insert(0, str(Path(__file__).resolve().parent))
import security_cases
from security_cases import (
    Case, Context, Suite, collect_cases, configure_features, discover_suites,
    server,
)
from product_security_test import check_output


def output(**values):
    report = {"passed": True, "testHooks": False}
    report.update(values)
    return "PRODUCT_SECURITY:" + json.dumps(report)


class RegistrationTest(unittest.TestCase):
    def test_new_group_needs_no_runner_or_ci_registration(self):
        source = """from security_cases import Case, Suite
def cases(context):
    def reply(handler):
        handler.send_response(200)
        handler.end_headers()
        handler.wfile.write(handler.command.encode())
    context.add_route("/future/probe", reply, methods=("GET", "POST"))
    yield Case("probe", context.http("/future/probe"), timeout=7)
SUITE = Suite("future", "A new area", cases, features={"future-mode": "strict"})
"""
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "future_test_group.py").write_text(source)
            with mock.patch.object(security_cases, "__path__", [directory]):
                importlib.invalidate_caches()
                try:
                    suites = discover_suites()
                    features = configure_features(suites, ["future-mode=relaxed"])
                    with server(features) as context:
                        cases = collect_cases(suites, context)
                        self.assertEqual(["future/probe"], [c.name for c in cases])
                        self.assertEqual(7, cases[0].timeout)
                        self.assertEqual("relaxed", context.features["future-mode"])
                        with urlopen(Request(cases[0].url, method="POST")) as reply:
                            self.assertEqual(b"POST", reply.read())
                finally:
                    sys.modules.pop("security_cases.future_test_group", None)

    def test_group_and_case_filters_preserve_existing_coverage(self):
        suites = discover_suites()
        context = Context("http://127.0.0.1:1", configure_features(suites))
        cases = collect_cases(suites, context, groups=["file-origin"],
                              patterns=["*xhr*"])
        self.assertEqual(3, len(cases))
        self.assertTrue(all(c.name.endswith("-xhr") for c in cases))
        context = Context("http://127.0.0.1:1", configure_features(suites))
        self.assertEqual(22, len(collect_cases(suites, context, internal_only=True)))

    def test_invalid_selection_and_duplicate_ids_fail(self):
        suite = Suite("example", "example",
                      lambda context: [Case("one", "about:blank")] * 2)
        suites = {"example": suite}
        with self.assertRaisesRegex(ValueError, "duplicate security case"):
            collect_cases(suites, Context(""))
        with self.assertRaisesRegex(ValueError, "unknown security group"):
            collect_cases(suites, Context(""), groups=["typo"])
        single = {"example": Suite("example", "example",
                                  lambda context: [Case("one", "about:blank")])}
        with self.assertRaisesRegex(ValueError, "matched nothing"):
            collect_cases(single, Context(""), patterns=["*typo*"])
        with self.assertRaisesRegex(ValueError, "feature setting"):
            configure_features(single, ["typo=on"])

    def test_group_fixtures_are_closed_with_the_server(self):
        cleanup = []
        with server({}) as context:
            context.fixtures.callback(cleanup.append, "closed")
            context.add_route("/same", lambda handler: None)
            with self.assertRaisesRegex(ValueError, "duplicate"):
                context.add_route("/same", lambda handler: None)
        self.assertEqual(["closed"], cleanup)


class ResultContractTest(unittest.TestCase):
    def test_case_rules_and_custom_validators_are_not_group_specific(self):
        validated = []
        case = Case("future/check", "about:blank", required_output=("authorized",),
                    forbidden_output=("LEAK",),
                    validate=lambda text, report: validated.append(report))
        check_output(case, "authorized\n" + output(), 0)
        self.assertEqual(1, len(validated))
        for text in ("authorized LEAK\n" + output(), output()):
            with self.assertRaises(AssertionError):
                check_output(case, text, 0)

    def test_product_report_is_mandatory_and_strict(self):
        case = Case("future/check", "about:blank")
        for text in ("", output() + "\n" + output(),
                     output(passed=False), output(testHooks=True),
                     "PRODUCT_SECURITY:[]",
                     "PRODUCT_SECURITY:" + json.dumps({"passed": True})):
            with self.subTest(text=text), self.assertRaises(AssertionError):
                check_output(case, text, 0)
        with self.assertRaisesRegex(AssertionError, "browser exited"):
            check_output(case, output(), -11)


if __name__ == "__main__":
    unittest.main()
