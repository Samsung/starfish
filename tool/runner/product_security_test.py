#!/usr/bin/env python3
"""Run registered browser security groups against a product build."""

import argparse
import concurrent.futures
import functools
import json
import os
from pathlib import Path
import subprocess
import tempfile

from security_cases import (
    ROOT, RESULT_PREFIX, collect_cases, configure_features, discover_suites,
    server,
)


def check_output(case, output, returncode):
    if returncode:
        raise AssertionError("browser exited %d" % returncode)
    for text in case.forbidden_output:
        if text in output:
            raise AssertionError("forbidden output: " + text)
    for text in case.required_output:
        if text not in output:
            raise AssertionError("missing required output: " + text)
    reports = []
    for line in output.splitlines():
        if RESULT_PREFIX in line:
            reports.append(json.JSONDecoder().raw_decode(
                line.split(RESULT_PREFIX, 1)[1])[0])
    if len(reports) != 1:
        raise AssertionError("expected exactly one completion report")
    report = reports[0]
    if not isinstance(report, dict):
        raise AssertionError("completion report must be an object")
    if report.get("passed") is not True:
        raise AssertionError(json.dumps(report, ensure_ascii=False))
    if report.get("testHooks") is not False:
        raise AssertionError("native test bindings enabled or not reported")
    if case.validate:
        case.validate(output, report)


def run_case(browser, case, timeout):
    timeout = case.timeout or timeout
    environment = os.environ.copy()
    environment.setdefault("LP_NUM_THREADS", "2")
    output = ""
    try:
        with tempfile.TemporaryDirectory(prefix="starfish-security-") as storage:
            result = subprocess.run(
                [str(browser), case.url, "--hide-window", "--disable-console",
                 "--timeout=%d" % timeout, "--storage-dir=" + storage],
                cwd=ROOT, env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True, errors="replace",
                timeout=timeout + 10,
            )
        output = result.stdout
        check_output(case, output, result.returncode)
        return case.name, True, ""
    except (AssertionError, subprocess.TimeoutExpired, ValueError, OSError) as error:
        return case.name, False, str(error) + "\n" + output[-5000:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--browser", type=Path)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=3,
                        help="default per-case seconds; cases may override it")
    parser.add_argument("--group", action="append", default=[],
                        help="run a registered group (repeatable; default: all)")
    parser.add_argument("--case", action="append", default=[], dest="patterns",
                        help="select group/case IDs by glob (repeatable)")
    parser.add_argument("--feature", action="append", default=[],
                        help="group feature NAME=VALUE (repeatable)")
    parser.add_argument("--list", action="store_true",
                        help="list matching cases without launching a browser")
    arguments = parser.parse_args()
    if arguments.jobs < 1 or arguments.timeout < 3:
        parser.error("--jobs must be positive and --timeout at least 3")
    if not arguments.list and not arguments.browser:
        parser.error("--browser is required unless --list is used")
    browser = arguments.browser.resolve() if arguments.browser else None
    if browser and not browser.is_file():
        parser.error("browser does not exist: %s" % browser)
    try:
        suites = discover_suites()
        features = configure_features(suites, arguments.feature)
        with server(features) as context:
            selected = collect_cases(suites, context, arguments.group,
                                     arguments.patterns)
            if arguments.list:
                for case in selected:
                    print(case.name)
                return 0
            run = functools.partial(run_case, browser, timeout=arguments.timeout)
            with concurrent.futures.ThreadPoolExecutor(
                    max_workers=arguments.jobs) as pool:
                results = list(pool.map(run, selected))
    except ValueError as error:
        parser.error(str(error))
    for name, passed, detail in results:
        print("%s %s" % ("PASS" if passed else "FAIL", name))
        if detail:
            print(detail)
    failed = sum(not passed for _, passed, _ in results)
    print("%d passed, %d failed" % (len(results) - failed, failed))
    return bool(failed)


if __name__ == "__main__":
    raise SystemExit(main())
