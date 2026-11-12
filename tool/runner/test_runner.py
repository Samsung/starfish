#!/usr/bin/env python3

# Copyright (c) 2019-present Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import fnmatch
import os
import subprocess
import sys
import tempfile
import platform

from argparse import ArgumentParser
from difflib import unified_diff
from os.path import join, relpath, splitext

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)                   # same-dir (execution_worker, http_server)
sys.path.insert(0, os.path.dirname(_HERE))  # tool/ for drivers.basics, repo_paths
sys.path.insert(0, os.path.join(os.path.dirname(_HERE), "wpt", "scripts"))

from repo_paths import REPO_ROOT
from drivers.basics.constants import ENVOPTS, ERRORCODE
from execution_worker import WorkerRunner

script_path = "./tool/drivers/run_test.py"
working_directory = REPO_ROOT
ran_test_count = 0

def file_len(fname):
    lines = []
    with open(fname) as f:
        lines = f.readlines()
    cnt = 0
    for l in lines:
        if not l.startswith("#"):
            cnt = cnt + 1

    return cnt

def print_table(key, value):
    PADDING = 30
    print(f"{key.ljust(PADDING)} : {value}")

def run_test(argv_input, env=None):
    name = ""
    for x in argv_input:
        if ".res" in x:
            name = x
            break
    print_table("Running test", name)

    argv = [script_path]
    argv.extend(argv_input)
    global ran_test_count
    ran_test_count = ran_test_count + file_len(argv_input[1])

    return_code = subprocess.call(argv, cwd=working_directory, env=env)

    if return_code == 0:
        print(("test " + name + " runs successfully"))
    else:
        print(("test " + name + " is failed"))
        sys.exit(ERRORCODE.TEST_FAILED)


def internal_test():
    # The image-compared tests are the tests/pixel submodule.
    if not os.path.isdir(join(working_directory, "tests", "pixel", "css")):
        print("no tests/pixel checkout -- run: git submodule update --init tests/pixel")
        sys.exit(ERRORCODE.TEST_STOPPED)
    run_test(["basic", "tool/reftest/cairo/internal.res", "common", "-p8"])
    run_test(["basic", "tool/reftest/cairo/internal_manual.res", "common", "--font-dep", "-p8"])
    run_test(["csswg", "tool/pixel_test/svg.res", "cairo", "-p8"])
    run_test(["basic", "tool/reftest/cairo/internal_obsolete.res", "common", "-p8"])
    _internal_security_test()
    # Tests whose documents load as file:// but fetch subresources over http.
    # ROOT is the resource-only directory so test documents are not exposed via http.
    from http_server import popen_server
    ROOT = "tests/fixtures/served-resources"
    with popen_server(ROOT, working_directory, "localhost", port=11011, silent=True):
        run_test(["basic",
                  "tool/reftest/cairo/internal_with_remote_resource.res",
                  "common", "-p8"])


def _internal_security_test():
    # Discover registered cases, using the assertion driver and native
    # testEnd() verdict for cases that support internal_test.
    from security_cases import (
        collect_cases, configure_features, discover_suites, server,
    )
    suites = discover_suites()
    with server(configure_features(suites)) as context:
        cases = collect_cases(suites, context, internal_only=True)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".res") as manifest:
            for case in cases:
                manifest.write(case.url + "\n")
            manifest.flush()
            run_test(["basic", manifest.name, "common", "-p8"])


# Khronos WebGL conformance suites (khronos_test) -- see docs/khronos_webgl.md.
# The tests are served unmodified out of the third_party/webgl submodule,
# with the submodule root as the server root, so the lists hold the full
# http://localhost:11010/<suite dir>/... URL of each test (like the WPT lists)
# and a line is runnable as-is once the server is up.
# tool/khronos/inject_report.js, prepended to the two harness scripts by
# http_server.py, turns the harness's own reporting hook into the PASS/FAIL
# lines the basic driver counts and ends the shell with testEnd().
KHRONOS_WEBGL_ROOT = "third_party/webgl"
KHRONOS_ADDRESS = "localhost"
KHRONOS_PORT = 11010  # baked into the lists; re-prefix them if this changes
KHRONOS_INJECT_SCRIPT = "tool/khronos/inject_report.js"
# Every test page loads exactly one of these two harnesses.
KHRONOS_INJECT_INTO = ("/js-test-pre.js", "/more/unit.js")

# Each worker drives a GL context, and software Mesa (llvmpipe) has its
# own rasterizer threads. Avoid full host cpu_count() parallelism, which
# previously made passing tests time out through CPU oversubscription.
# Default to eight tests so short tests can progress alongside long ones.
# CI sets --khronos-jobs and LP_NUM_THREADS for each runner pool separately.
KHRONOS_WEBGL_JOBS = 8

# Per-test timeout (seconds) for the Khronos WebGL suites only. The basic
# driver's default watchdog (DEFAULT_NATIVE_TIMEOUT_SEC, 180s) is meant to
# catch genuine hangs, never slow-but-passing tests -- but this suite has
# legitimately slow tests: multisample-corruption.html takes ~160s on a fast
# 32-core dev machine (2048x2048 readbacks verified in interpreted JS, x25
# iterations), leaving no margin for a slower or loaded CI host. Give the
# suite enough headroom that only real hangs trip the watchdog; the other
# (DOM/CSS) suites keep the tighter default.
KHRONOS_WEBGL_TIMEOUT_SEC = 1200
# The dEQP texture-filtering combinations can approach 1200s on llvmpipe.
# Allow margin for shared-runner contention while still detecting hangs.
KHRONOS_DEQP_TIMEOUT_SEC = 1800

def run_vendor_test_khronos(name, timeout=KHRONOS_WEBGL_TIMEOUT_SEC):
    from http_server import popen_server

    env = dict(os.environ)
    if not env.get(ENVOPTS.TIMEOUT):
        env[ENVOPTS.TIMEOUT] = str(timeout)

    with popen_server(KHRONOS_WEBGL_ROOT, working_directory, KHRONOS_ADDRESS,
                      port=KHRONOS_PORT, silent=True,
                      inject_script=KHRONOS_INJECT_SCRIPT,
                      inject_into=KHRONOS_INJECT_INTO):
        run_test(["basic", name, "common", f"-p{KHRONOS_WEBGL_JOBS}",
                  "--progress-interval=30"], env)


def vendor_test_khronos():
    run_vendor_test_khronos("tool/reftest/cairo/khronos_webgl.res")


def vendor_test_khronos2():
    run_vendor_test_khronos("tool/reftest/cairo/khronos_webgl2.res")


def vendor_test_khronossdk():
    run_vendor_test_khronos("tool/reftest/cairo/khronos_webglsdk.res")


def khronos_smoke_test():
    run_vendor_test_khronos("tool/reftest/cairo/khronos_webgl_smoke.res")


def _khronos_partition(deqp):
    # Keep the curated lists authoritative, including their known failures.
    # A temporary list retains comment markers so -f has the same meaning.
    with tempfile.TemporaryDirectory(prefix="starfish-khronos-") as directory:
        for name in ("khronos_webgl.res", "khronos_webgl2.res",
                     "khronos_webglsdk.res"):
            with open(join(working_directory, "tool/reftest/cairo", name)) as source:
                lines = [line for line in source
                         if line.lstrip("# ").startswith("http")
                         and ("/deqp/" in line.lstrip("# ").split()[0]) == deqp]
            if not lines:
                continue
            path = join(directory, name)
            with open(path, "w") as output:
                output.writelines(lines)
            run_vendor_test_khronos(
                path, KHRONOS_DEQP_TIMEOUT_SEC if deqp else KHRONOS_WEBGL_TIMEOUT_SEC)


def khronos_conformance_test():
    _khronos_partition(False)


def khronos_deqp_test():
    _khronos_partition(True)


# CI runs this in its own job, which restores the third_party/webgl submodule
# the way the WPT jobs restore third_party/wpt. It is also slow enough to be
# worth isolating locally.
def khronos_test():
    vendor_test_khronos()
    vendor_test_khronos2()
    vendor_test_khronossdk()


# Browser-vendor test suites, served unmodified out of a sparse submodule
# checkout (each made by tool/<vendor>/fetch.sh) with the vendor's test root
# as the server root, so each list holds http://localhost:<port>/<test> URLs
# like the Khronos lists. A bridge prepended to the vendor's harness scripts
# stands in for the vendor's own test shell and prints each page's result,
# which tool/drivers/tests/<vendor>_test.py judges; the lists also hold
# reftests, which those drivers render and compare.
# A finished test takes a second or two; the timeout only bounds hangs.
VENDOR_TIMEOUT_SEC = 30


def _vendor_test(kind, root, port, inject_script, inject_into, alias=(), reftests=False):
    from http_server import popen_server

    if not os.path.isdir(join(working_directory, root)):
        print("no %s checkout at %s -- run: tool/%s/fetch.sh" % (kind, root, kind))
        sys.exit(ERRORCODE.TEST_STOPPED)
    if reftests:
        from wpt_reftest import ensure_imgdiff
        ensure_imgdiff()
    env = dict(os.environ)
    if not env.get(ENVOPTS.TIMEOUT):
        env[ENVOPTS.TIMEOUT] = str(VENDOR_TIMEOUT_SEC)
    with popen_server(root, working_directory, "localhost", port=port, silent=True,
                      inject_script=inject_script, inject_into=inject_into,
                      alias=alias):
        run_test([kind, "tool/reftest/cairo/%s.res" % kind, "common", "-p8"], env)


# WebKit LayoutTests -- see docs/webkit_layouttests.md. The ports are baked
# into the lists (tool/<vendor>/update_lists.py).
def webkit_test():
    # Keep in sync with HARNESSES in tool/webkit/update_lists.py.
    _vendor_test("webkit", "third_party/webkit/LayoutTests", 11012,
                 "tool/webkit/inject_report.js",
                 ("/js-test-pre.js", "/js-test.js", "/selfhtml.js"),
                 reftests=True)


# Blink web tests -- see docs/blink_web_tests.md. Blink kept WebKit's
# testRunner API, so WebKit's bridge serves both.
def blink_test():
    _vendor_test("blink", "third_party/blink/third_party/blink/web_tests", 11013,
                 "tool/webkit/inject_report.js",
                 ("/resources/js-test.js", "/resources/testharnessreport.js"),
                 reftests=True)


# Gecko mochitests and reftests -- see docs/gecko_tests.md.
def gecko_test():
    _vendor_test("gecko", "third_party/gecko", 11014,
                 "tool/gecko/inject_report.js", ("/tests/SimpleTest/SimpleTest.js",),
                 alias=("/tests/SimpleTest/=testing/mochitest/tests/SimpleTest",),
                 reftests=True)


# WPT (testharness) via on-demand `wpt serve` -- see docs/wpt.md.
# Runs the active (expected-pass) lists under tool/wpt/testharness_lists/; any
# active test failing is treated as a regression. Failing tests are kept as
# `# [auto-fail:REASON]` comments (refresh with wpt_runner.py + wpt_annotate.py).
#
# wpt_serve_testharness runs every list; each wpt_serve_<module> suite runs
# one group of lists so a module can be checked in isolation.
_WPT_TESTHARNESS_LISTS_DIR = os.path.join(working_directory, "tool/wpt/testharness_lists")


def _wpt_collect(patterns, exclude):
    import glob
    import wpt_runner

    if patterns:
        targets = []
        for pat in patterns:
            targets.extend(sorted(glob.glob(os.path.join(_WPT_TESTHARNESS_LISTS_DIR, pat))))
    else:
        targets = sorted(glob.glob(os.path.join(_WPT_TESTHARNESS_LISTS_DIR, "*.res")))
    if exclude:
        targets = [t for t in targets
                   if not any(fnmatch.fnmatch(os.path.basename(t), e) for e in exclude)]

    items = []
    for t in targets:
        items.extend(wpt_runner.collect(t, force=False))
    return items


def _wpt_serve_run(*patterns, jobs=8, timeout=20, daemons=(), exclude=()):
    _wpt_serve_run_groups([(patterns, exclude, jobs, timeout)], daemons=daemons)


def _wpt_serve_run_groups(groups, daemons=()):
    """Run (patterns, exclude, jobs, timeout) groups under one `wpt serve`.

    Each group keeps its own concurrency and per-test timeout; pass/fail is
    aggregated, and the run exits with TEST_FAILED if any group regressed.
    """
    import shutil
    import tempfile
    from collections import Counter
    import wpt_runner
    from wpt_server import wpt_serve, DEFAULT_WPT_ROOT, WptServerError

    batches = []
    for patterns, exclude, jobs, timeout in groups:
        items = _wpt_collect(patterns, exclude)
        label = ", ".join(patterns) if patterns else "all"
        if exclude:
            label += " except " + ", ".join(exclude)
        print_table("Running WPT (on-demand)",
                    "%d tests [%s] jobs=%d timeout=%ds" % (len(items), label, jobs, timeout))
        batches.append((items, jobs, timeout))
    total = sum(len(items) for items, _, _ in batches)
    npass = 0
    reasons = Counter()
    per_list = {}
    runners = [WorkerRunner(name) for name in daemons]
    # daemons (SharedWorker/ServiceWorker) are one long-lived process shared
    # by every job in this run, so every client Starfish invocation must
    # agree with it on where its worker IPC socket lives (WorkerIPCAddress
    # derives that path from the storage dir) -- give the daemon and all
    # jobs THIS one shared directory instead of wpt_runner's usual per-
    # invocation isolated_storage_dir(). Otherwise every test that actually
    # round-trips through the daemon (as opposed to just touching surface
    # constructor properties) times out or gets a non-zero harness status,
    # 100% reproducibly -- confirmed live on WPT's
    # workers/constructors/SharedWorker/{empty-name,name,port-onmessage,
    # unexpected-global-properties}.html. See wpt_runner._storage_dir_scope.
    shared_storage_dir = tempfile.mkdtemp(prefix="starfish-storage-") if runners else None
    try:
        try:
            with wpt_serve(DEFAULT_WPT_ROOT, verbose=True):
                try:
                    for r in runners:
                        r.run(shared_storage_dir)
                    for items, jobs, timeout in batches:
                        if not items:
                            continue
                        # verbose=True: this suite gates CI, so a crash here
                        # means a crash on the CI machine -- surface the
                        # captured backtrace in the (only) log we get, the CI
                        # job's own live stdout.
                        n, r, pl = wpt_runner.run_all(
                            items, jobs, timeout, None, verbose=True,
                            storage_dir=shared_storage_dir)
                        npass += n
                        reasons.update(r)
                        for name, (pn, tn) in pl.items():
                            acc = per_list.setdefault(name, [0, 0])
                            acc[0] += pn
                            acc[1] += tn
                finally:
                    for r in runners:
                        r.terminate()
        except WptServerError as e:
            print("wpt serve failed: %s" % e)
            print("hosts not set? run: "
                  "python3 third_party/wpt/wpt make-hosts-file | sudo tee -a /etc/hosts")
            sys.exit(ERRORCODE.TEST_STOPPED)
    finally:
        if shared_storage_dir:
            shutil.rmtree(shared_storage_dir, ignore_errors=True)

    global ran_test_count
    ran_test_count += total
    if len(per_list) > 1:
        for name in sorted(per_list):
            pn, tn = per_list[name]
            print("  %-44s %d/%d" % (name, pn, tn))
    print("WPT pass %d/%d" % (npass, total))
    if npass != total:
        for reason, n in reasons.most_common():
            print("  %5d  %s" % (n, reason))
        sys.exit(ERRORCODE.TEST_FAILED)


def wpt_serve_testharness_css():
    _wpt_serve_run("css_*.res")


def wpt_serve_testharness_dom():
    _wpt_serve_run("dom_*.res")


def wpt_serve_testharness_canvas():
    _wpt_serve_run("2dcontext.res")


def wpt_serve_testharness_html():
    _wpt_serve_run("html_*.res")


def wpt_serve_testharness_xhr():
    _wpt_serve_run("xhr_*.res")


def wpt_serve_testharness_fetch():
    _wpt_serve_run("fetch_*.res")


def wpt_serve_testharness_worker():
    _wpt_serve_run("worker.res", daemons=("Starfish-sharedworker",))


def wpt_serve_testharness_serviceworker():
    _wpt_serve_run("serviceworker.res", daemons=("Starfish-serviceworker",))


def wpt_serve_testharness_idb():
    _wpt_serve_run("indexeddb.res")


def wpt_serve_testharness_websocket():
    _wpt_serve_run("websocket.res")


def wpt_serve_testharness_webrtc():
    _wpt_serve_run("webrtc.res")


# Offline audio renders can consume substantial memory per case; keep
# concurrent renderers low so the timeout remains meaningful on CI hosts.
_WPT_WEBAUDIO_GROUP = (("webaudio_*.res",), (), 2, 60)


def wpt_serve_testharness_webaudio():
    _wpt_serve_run_groups([_WPT_WEBAUDIO_GROUP])


def wpt_serve_testharness_intersection_observer():
    _wpt_serve_run("intersection-observer.res")


def wpt_serve_testharness_svg():
    _wpt_serve_run("svg_*.res")


def wpt_serve_testharness_custom_elements():
    _wpt_serve_run("custom-elements.res")


def wpt_serve_testharness_fullscreen():
    _wpt_serve_run("fullscreen.res")


def wpt_serve_testharness_others():
    _wpt_serve_run("battery_status.res", "cookies.res", "cors.res", "csp.res",
                   "fileAPI.res", "frame-ancestors.res", "page_visibility_basic.res",
                   "webstorage.res", "x-frame-options.res")


def wpt_serve_testharness():
    # worker.res/serviceworker.res need the SharedWorker/ServiceWorker daemon
    # peers, which this aggregate's CI job (x64_test.yml) does not build. They
    # are gated separately by the daemon-equipped wpt_serve_testharness_worker/
    # _serviceworker suites, so exclude them here and skip daemon startup.
    # Web Audio lists run in the same `wpt serve` session, but with the
    # dedicated wpt_serve_testharness_webaudio concurrency and timeout.
    _wpt_serve_run_groups([
        ((), ("worker.res", "serviceworker.res", "webaudio_*.res"), 8, 20),
        _WPT_WEBAUDIO_GROUP,
    ])


# WPT reftest / crashtest via on-demand `wpt serve` -- see docs/wpt.md.
# Runs the active (expected-pass) lists under tool/wpt/reftest_lists/ and
# tool/wpt/crashtest_lists/; any active test failing is treated as a
# regression, same CI-gate contract as wpt_serve_*. These lists have no
# legacy corpus to carry forward (generated straight from MANIFEST.json by
# wpt_manifest_lists.py, unlike wpt_serve_*'s wpt_audit.py source), but are
# baselined and annotated the same way (wpt_runner.py + wpt_annotate.py) so
# they also gate at ~100%. Re-run after an engine fix or pin bump to refresh
# which tests gate.
_WPT_REFTEST_LISTS_DIR = os.path.join(working_directory, "tool/wpt/reftest_lists")
_WPT_CRASHTEST_LISTS_DIR = os.path.join(working_directory, "tool/wpt/crashtest_lists")


def _wpt_manifest_run(list_dir, mode, jobs=8, timeout=20):
    import wpt_runner
    from wpt_server import wpt_serve, inject_script_for, DEFAULT_WPT_ROOT, WptServerError

    if not os.path.isdir(list_dir):
        print("no lists at %s -- run: python3 tool/wpt/scripts/wpt_manifest_lists.py "
              "--mode %s --out-dir %s" % (list_dir, mode, list_dir))
        sys.exit(ERRORCODE.TEST_STOPPED)

    items = wpt_runner.collect(list_dir, force=False)
    print_table("Running WPT %s (on-demand)" % mode, "%d tests" % len(items))
    manifest = None
    if mode == "reftest":
        from wpt_reftest import ensure_imgdiff
        from wpt_status import ensure_manifest
        ensure_imgdiff()
        manifest_path = os.path.join(DEFAULT_WPT_ROOT, "MANIFEST.json")
        ensure_manifest(DEFAULT_WPT_ROOT, manifest_path)
        manifest = wpt_runner.load_manifest(DEFAULT_WPT_ROOT)
    try:
        with wpt_serve(DEFAULT_WPT_ROOT, inject_script=inject_script_for(mode),
                       verbose=True):
            # verbose=True: this suite gates CI, so a crash here means a
            # crash on the CI machine -- surface the captured backtrace
            # in the (only) log we get, the CI job's own live stdout.
            npass, reasons, per_list = wpt_runner.run_all(
                items, jobs, timeout, None, mode=mode, manifest=manifest,
                verbose=True)
    except WptServerError as e:
        print("wpt serve failed: %s" % e)
        print("hosts not set? run: "
              "python3 third_party/wpt/wpt make-hosts-file | sudo tee -a /etc/hosts")
        sys.exit(ERRORCODE.TEST_STOPPED)

    global ran_test_count
    ran_test_count += len(items)
    if len(per_list) > 1:
        for name in sorted(per_list):
            pn, tn = per_list[name]
            print("  %-44s %d/%d" % (name, pn, tn))
    print("WPT %s pass %d/%d" % (mode, npass, len(items)))
    if npass != len(items):
        for reason, n in reasons.most_common():
            print("  %5d  %s" % (n, reason))
        sys.exit(ERRORCODE.TEST_FAILED)


def wpt_serve_reftest():
    _wpt_manifest_run(_WPT_REFTEST_LISTS_DIR, "reftest")


def wpt_serve_crashtest():
    _wpt_manifest_run(_WPT_CRASHTEST_LISTS_DIR, "crashtest")


def test_all():
    internal_test()
    wpt_serve_reftest()
    khronos_test()
    webkit_test()
    blink_test()
    gecko_test()


def print_columns(iterable, num_columns):
    max_length = max(len(str(item)) for item in iterable)
    num_rows = -(-len(iterable) // num_columns)

    for i in range(num_rows):
        for j in range(i, len(iterable), num_rows):
            print(str(iterable[j]).ljust(max_length), end=" ")
        print()


if __name__ == "__main__":
    print("Usage----------------------------")
    print("run ./tool/runner/test_runner.py")
    print("on repository root directory")
    print("if you want to run specific test suite,")
    print("run ./tool/runner/test_runner.py <test_name> <test_name> ...")
    print("this is list of test suites")

    test_functions = []
    for key, value in list(locals().items()):
        if callable(value) and value.__module__ == __name__:
            if key not in ["file_len", "print_columns", "print_table",
                           "run_test", "run_vendor_test_khronos",
                           "_khronos_partition", "_wpt_serve_run",
                           "_wpt_serve_run_groups", "_wpt_collect",
                           "_wpt_manifest_run", "_vendor_test"]:
                test_functions.append(key)
    print_columns(sorted(test_functions), 4)

    parser = ArgumentParser()
    parser.add_argument(
        "test_suite_names", nargs="*", help="Names of test suite to run"
    )
    parser.add_argument(
        "-t", "--timeout",
        type=int,
        default=0,
        help="Set timeout in seconds to individual tests",
    )
    parser.add_argument(
        "-f", "--force", action="store_true", help="Force commented tests to run"
    )
    parser.add_argument(
        "--out-pass-list",
        action="store_true",
        help="Create a file that records passed tests",
    )
    parser.add_argument(
        "--out-pass-list-filename",
        nargs="?",
        default="test_result.txt",
        help="Set a file name to record passed tests",
    )
    parser.add_argument(
        "--khronos-jobs", type=int, default=KHRONOS_WEBGL_JOBS,
        help="Number of parallel Khronos WebGL tests (default: 8)",
    )
    args = parser.parse_args()
    if args.khronos_jobs < 1:
        parser.error("--khronos-jobs must be positive")
    KHRONOS_WEBGL_JOBS = args.khronos_jobs

    if args.timeout > 0:
        os.environ[ENVOPTS.TIMEOUT] = str(args.timeout)

    if args.force == True:
        os.environ[ENVOPTS.FORCE_ENABLE] = str(True)

    if args.out_pass_list == True:
        os.environ[ENVOPTS.TEST_RESULT_FILE] = args.out_pass_list_filename

    if args.test_suite_names:
        for test_suite_name in args.test_suite_names:
            if test_suite_name in locals() and callable(locals()[test_suite_name]):
                locals()[test_suite_name]()
            else:
                print(f"There is no test named '{test_suite_name}'.")
                sys.exit(ERRORCODE.TEST_STOPPED)
    else: # test all
        print("running every tests!")
        test_all()

    print((str(ran_test_count) + " test cases rans successfully"))
