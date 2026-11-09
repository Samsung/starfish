#!/usr/bin/env python3
"""Progress must report live completions without changing result order."""

import contextlib
import io
import multiprocessing
from pathlib import Path
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from basics import parallel


def delayed_case(tc):
    index, _ = tc
    time.sleep(0.12 if index == 0 else 0.02)
    return index != 1


def failing_case(tc):
    raise ValueError("worker failure")


def return_results(tcs, results):
    return results


class ParallelProgressTest(unittest.TestCase):
    def test_live_progress_keeps_results_in_input_order(self):
        output = io.StringIO()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory, "cases.res")
            path.write_text("slow.html\nfail.html\nfast.html\n")
            with contextlib.redirect_stdout(output):
                results = parallel.run_test_pool(
                    delayed_case, str(path), 2,
                    result_handler=return_results, progress_interval=0.03)
        self.assertEqual(results, [True, False, True])
        log = output.getvalue()
        self.assertIn("Progress: 0/3", log)
        # The slow first item must not hide later completions.
        self.assertIn("Progress: 2/3", log)
        self.assertIn("Progress: 3/3 (100.0%) pass=2 fail=1", log)
        self.assertIn("elapsed=", log)

    def test_reports_while_no_test_has_finished(self):
        with multiprocessing.Pool(1) as pool:
            with contextlib.redirect_stdout(io.StringIO()) as output:
                results = parallel._collect_progress(
                    pool, delayed_case, [(0, "test")], 0.02)
        self.assertEqual(results, [True])
        self.assertGreaterEqual(output.getvalue().count("Progress: 0/1"), 2)

    def test_disabled_progress_keeps_the_existing_result_path(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory, "cases.res")
            path.write_text("slow.html\nfail.html\nfast.html\n")
            with contextlib.redirect_stdout(io.StringIO()) as output:
                result = parallel.run_test_pool(delayed_case, str(path), 2)
        self.assertEqual(result, (2, 1))
        self.assertNotIn("Progress:", output.getvalue())

    def test_worker_exception_is_propagated(self):
        with multiprocessing.Pool(1) as pool:
            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaisesRegex(ValueError, "worker failure"):
                    parallel._collect_progress(
                        pool, failing_case, [(0, "test")], 0.01)

    def test_deadline_is_preserved(self):
        with multiprocessing.Pool(1) as pool:
            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(multiprocessing.TimeoutError):
                    parallel._collect_progress(
                        pool, delayed_case, [(0, "test")], 0.01, timeout=0.03)

    def test_empty_suite(self):
        with multiprocessing.Pool(1) as pool:
            with contextlib.redirect_stdout(io.StringIO()) as output:
                results = parallel._collect_progress(pool, delayed_case, [], 0.01)
        self.assertEqual(results, [])
        self.assertIn("0/0 (100.0%)", output.getvalue())


if __name__ == "__main__":
    unittest.main()
