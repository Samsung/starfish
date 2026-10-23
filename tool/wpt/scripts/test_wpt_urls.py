"""WPT protocol selection and HTTPS reference regression tests."""

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from wpt_audit import parse_res, remap_candidate
from wpt_manifest_inventory import enumerate_tests, manifest_inventory
from wpt_reftest import resolve_references, run_reftest, url_to_test_path
from wpt_server import _http_ok
from wpt_status import build_tree, render_node
from wpt_urls import (ORIGINS, server_protocol, test_url as wpt_test_url,
                      with_test_protocol)


class ProtocolTests(unittest.TestCase):
    def test_curated_lists_use_required_protocol(self):
        for folder in ("testharness_lists", "reftest_lists", "crashtest_lists"):
            for path in (Path(__file__).resolve().parents[1] / folder).glob("*.res"):
                for line in path.read_text().splitlines():
                    urls = [word for word in line.split()
                            if word.startswith(("http://", "https://"))]
                    if urls:
                        with self.subTest(path=path.name, url=urls[0]):
                            self.assertEqual(with_test_protocol(urls[0]), urls[0])

    def test_wpt_flags_and_generated_variants(self):
        cases = (("dir/ordinary.any.worker.html", "http"),
                 ("https/ordinary.html", "http"),
                 ("dir/test.https.any.html?q=1#fragment", "https"),
                 ("dir/test.any.serviceworker.html", "https"),
                 ("dir/test.any.serviceworker-module.html", "https"),
                 ("dir/test.html?wpt_flags=https", "https"),
                 ("dir/test.html?wpt_flags=serviceworker", "https"),
                 ("dir/test.https.h2.html", "h2"),
                 ("dir/test.https.html?wpt_flags=h2", "h2"),
                 ("dir/test.html?wpt_flags=https&wpt_flags=h2", "h2"))
        for path, protocol in cases:
            with self.subTest(path=path):
                self.assertEqual(server_protocol(path), protocol)
                self.assertEqual(wpt_test_url(path, "/base/"),
                                 ORIGINS[protocol] + "/base/" + path)

    def test_inventory_and_list_generation_use_same_protocol(self):
        manifest = {"url_base": "/base/", "items": {"testharness": {"dir": {
            "a.any.js": ["hash", ["dir/a.any.html", {}],
                         ["dir/a.any.serviceworker.html", {}]],
            "b.https.html": ["hash", [None, {}]],
            "c.h2.html": ["hash", [None, {}]]}}}}
        expected = {ORIGINS["http"] + "/base/dir/a.any.html",
                    ORIGINS["https"] + "/base/dir/a.any.serviceworker.html",
                    ORIGINS["https"] + "/base/dir/b.https.html",
                    ORIGINS["h2"] + "/base/dir/c.h2.html"}
        self.assertEqual(manifest_inventory(manifest, ("testharness",)),
                         {("testharness", url) for url in expected})
        lists, missing = enumerate_tests(manifest, ["dir"])
        self.assertEqual(set(lists["dir"]), expected)
        self.assertEqual(missing, [])

    def test_upgrade_preserves_host_query_and_explicit_secure_origin(self):
        cases = {
            "http://www.web-platform.test:8001/a.https.html?q=1#f":
                "https://www.web-platform.test:8444/a.https.html?q=1#f",
            "http://web-platform.test:8000/a.any.serviceworker.html":
                "https://web-platform.test:8443/a.any.serviceworker.html",
            "https://web-platform.test:8443/a.h2.html":
                "https://web-platform.test:9000/a.h2.html",
            "https://web-platform.test:8444/a.https.html":
                "https://web-platform.test:8444/a.https.html",
            "http://web-platform.test:8000/plain.html":
                "http://web-platform.test:8000/plain.html",
            "http://example.org:8000/a.https.html":
                "http://example.org:8000/a.https.html"}
        for old, new in cases.items():
            with self.subTest(url=old):
                self.assertEqual(with_test_protocol(old), new)

    def test_audit_normalizes_legacy_urls_and_renamed_candidates(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory, "legacy.res")
            path.write_text("http://web-platform.test:8000/a.https.html\n"
                            "# http://web-platform.test:8000/b.https.html\n")
            self.assertEqual(list(parse_res(path, True)), [
                (ORIGINS["https"] + "/a.https.html", False),
                (ORIGINS["https"] + "/b.https.html", True)])
        url = "http://web-platform.test:8000/old/a.html?wpt_flags=https"
        self.assertEqual(remap_candidate(url, {"a.html": ["new/a.html"]}),
                         ORIGINS["https"] + "/new/a.html?wpt_flags=https")

    def test_https_health_probe_requires_successful_tls_response(self):
        with patch("wpt_server.HTTPSConnection") as connection:
            connection.return_value.getresponse.return_value.status = 200
            self.assertTrue(_http_ok(https=True))
            connection.assert_called_once()
            self.assertEqual(connection.call_args.args[:2], ("web-platform.test", 8443))
            connection.return_value.getresponse.return_value.status = 500
            self.assertFalse(_http_ok(https=True))
            connection.return_value.request.side_effect = OSError("TLS failed")
            self.assertFalse(_http_ok(https=True))
            self.assertEqual(connection.return_value.close.call_count, 3)

    def test_status_tree_uses_path_for_both_origins(self):
        results = [{"url": ORIGINS[protocol] + "/dir/test.html?q=" + protocol,
                    "ok": True, "reason": "OK", "status": "OK", "subtests": []}
                   for protocol in ("http", "https")]
        tree = build_tree(results)
        self.assertEqual(set(tree["dirs"]), {"dir"})
        self.assertEqual(len(tree["dirs"]["dir"]["tests"]), 2)
        parts = []
        render_node("dir", tree["dirs"]["dir"], parts)
        self.assertIn("test.html?q=https", "".join(parts))


class ReferenceTests(unittest.TestCase):
    def setUp(self):
        self.manifest = {"url_base": "/base/", "items": {"reftest": {
            "dir": {"test.https.html": ["hash",
                ["dir/test.https.html?q=1", [["/dir/ref.html", "=="],
                    ["about:blank", "!="],
                    ["https://other.test/ref.html", "!="],
                    ["//other.test/ref.html", "!="]], {}],
                ["dir/test.https.html?q=2", [["/dir/other.html", "=="]], {}]]}}}}

    def test_reference_scheme_port_and_variant_are_preserved(self):
        for origin in (ORIGINS["https"], ORIGINS["h2"], "https://www.web-platform.test:8444"):
            with self.subTest(origin=origin):
                url = origin + "/base/dir/test.https.html?q=1#fragment"
                path = url_to_test_path(url, "/base/")
                self.assertEqual(path, "dir/test.https.html?q=1")
                self.assertEqual(resolve_references(self.manifest, path, origin), [
                    (origin + "/base/dir/ref.html", "==", None),
                    ("about:blank", "!=", None),
                    ("https://other.test/ref.html", "!=", None),
                    ("https://other.test/ref.html", "!=", None)])

    def test_run_reftest_passes_https_origin_to_reference_capture(self):
        self.manifest["items"]["reftest"]["dir"]["test.https.html"] = [
            "hash", [None, [["/dir/ref.html", "=="]], {}]]
        url = ORIGINS["https"] + "/base/dir/test.https.html"
        with patch("wpt_reftest._screenshot", return_value=(True, None, None)) as capture, \
                patch("wpt_reftest._images_differ", return_value=False):
            self.assertTrue(run_reftest(url, manifest=self.manifest)[0])
        self.assertEqual([call.args[0] for call in capture.call_args_list],
                         [url, ORIGINS["https"] + "/base/dir/ref.html"])


if __name__ == "__main__":
    unittest.main()
