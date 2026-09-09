"""Inventory contract tests; never start WPT, Starfish, or worker daemons."""

import unittest

from wpt_manifest_inventory import manifest_inventory
from wpt_scope import compare_inventory


class InventoryTests(unittest.TestCase):
    def test_full_manifest_preserves_variants_and_types(self):
        manifest = {"url_base": "/base/", "items": {
            "testharness": {"a.html": ["hash", [None, {}]],
                            "other": {"b.any.js": ["hash",
                                ["other/b.any.html?q=1", {}],
                                ["other/b.any.worker.html", {}]]}},
            "reftest": {"a.html": ["hash", [None, [["ref.html", "=="]], {}]]}}}
        origin = "http://web-platform.test:8000/base/"
        self.assertEqual(manifest_inventory(manifest, ("testharness", "reftest")), {
            ("testharness", origin + "a.html"),
            ("testharness", origin + "other/b.any.html?q=1"),
            ("testharness", origin + "other/b.any.worker.html"),
            ("reftest", origin + "a.html")})

    def test_missing_manifest_branch_is_not_empty_success(self):
        with self.assertRaises(KeyError):
            manifest_inventory({"url_base": "/", "items": {}}, ("reftest",))

    def test_ci_duplicates_keep_provenance_and_workers(self):
        key = ("testharness", "http://example/workers/a.html")
        result = compare_inventory([key, key], {"testharness": [
            ("worker.res", key[1]), ("other.res", key[1]),
            ("worker.res", key[1])]})
        self.assertEqual(result["covered"], {key})
        self.assertEqual(result["ci_sources"][key], {"worker.res", "other.res"})
        self.assertEqual(result["missing"], {})

    def test_mismatches_are_not_added_to_candidates(self):
        candidates = {("testharness", "https://example/a.html?q=1"),
                      ("reftest", "http://example/ref.html")}
        urls = ["http://example/a.html?q=1", "http://example/a.html?q=2",
                "http://example/ref.html", "http://example/gone.html"]
        result = compare_inventory(candidates, {"testharness": [
            ("dom.res", url) for url in urls]})
        self.assertEqual(result["missing"], dict(zip(
            (("testharness", url) for url in urls),
            ("origin_mismatch", "absent", "type_mismatch", "absent"))))
        self.assertEqual(result["covered"], set())
        self.assertEqual(result["additional"], candidates)


if __name__ == "__main__":
    unittest.main()
