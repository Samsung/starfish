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


import os
import subprocess
import sys
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
    run_test(["basic", "tool/reftest/cairo/internal.res", "common", "-p8"])
    run_test(["basic", "tool/reftest/cairo/internal_manual.res", "common", "--font-dep", "-p8"])
    run_test(["csswg", "tool/pixel_test/svg.res", "cairo", "-p8"])
    run_test(["basic", "tool/reftest/cairo/internal_obsolete.res", "common", "-p8"])
    # Tests whose documents load as file:// but fetch subresources over http.
    # ROOT is the resource-only directory so test documents are not exposed via http.
    from http_server import popen_server
    ROOT = "test/cairo/internal-test/served-resources"
    with popen_server(ROOT, working_directory, "localhost", port=11011, silent=True):
        run_test(["basic",
                  "tool/reftest/cairo/internal_with_remote_resource.res",
                  "common", "-p8"])


def dom_conformance_test():
    run_test(["dom_conformance", "tool/reftest/cairo/dom_conformance_test.res", "common"])
    run_test(["dom_conformance", "tool/reftest/cairo/webkit_dom_conformance_test.res", "common"])
    run_test(["dom_conformance", "tool/reftest/cairo/blink_dom_conformance_test.res", "common"])
    run_test(["dom_conformance", "tool/reftest/cairo/gecko_dom_conformance_test.res", "common"])


def vendor_test_blink():
    run_test(["vendor_basic", "tool/reftest/cairo/blink_fast_dom.res", "common"])
    run_test(["vendor_basic", "tool/reftest/cairo/blink_fast_html.res", "common"])
    run_test(["vendor_pixel", "tool/reftest/cairo/blink_fast_css.res", "cairo"])
    run_test(["vendor_pixel", "tool/reftest/cairo/blink_fast_css_manual.res", "cairo", "--font-dep"])
    run_test(["vendor_pixel", "tool/reftest/cairo/blink_fast_etc.res", "cairo"])
    run_test(["vendor_pixel", "tool/reftest/cairo/blink_fast_etc_manual.res", "cairo", "--font-dep"])
    run_test(["vendor_pixel", "tool/reftest/cairo/blink_fast_table.res", "cairo"])
    run_test(["vendor_pixel", "tool/reftest/cairo/blink_css3.res", "cairo"])
    run_test(["vendor_pixel", "tool/reftest/cairo/blink_svg.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/blink_svg_basic.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/blink_fast_canvas_basic.res", "cairo"])
#    run_test(["vendor_pixel", "tool/reftest/cairo/tool/blink_fast_canvas.res", "cairo"])


def vendor_test_gecko():
    run_test(["vendor_pixel", "tool/reftest/cairo/gecko_layout.res", "cairo"])
    run_test(["vendor_pixel", "tool/reftest/cairo/gecko_layout_manual.res", "cairo", "--font-dep"])


def vendor_test_webkit():
    run_test(["vendor_basic", "tool/reftest/cairo/webkit_fast_dom.res", "common"])
    run_test(["vendor_basic", "tool/reftest/cairo/webkit_fast_html.res", "common"])
    run_test(["vendor_pixel", "tool/reftest/cairo/webkit_fast_css.res", "cairo"])
    run_test(["vendor_pixel", "tool/reftest/cairo/webkit_fast_css_manual.res", "cairo", "--font-dep"])
    run_test(["vendor_pixel", "tool/reftest/cairo/webkit_fast_etc.res", "cairo"])
    run_test(["vendor_pixel", "tool/reftest/cairo/webkit_fast_etc_manual.res", "cairo", "--font-dep"])


# Unlike the other run_test() call sites in this file, the Khronos WebGL
# conformance suite is not capped by -p and so runs at full
# multiprocessing.cpu_count() parallelism (e.g. 56 on this CI host). Each
# worker is a full Starfish process driving its own GL context; on this
# host's software Mesa (llvmpipe) driver, dozens of those running at once
# oversubscribe the CPU (llvmpipe itself spawns a rasterizer thread pool
# per context) badly enough that individual tests that pass fine in
# isolation start hanging/timing out under full-width parallel load. Cap
# this suite specifically -- the other vendor suites (blink/gecko/webkit)
# are plain DOM/CSS tests with no GL driver involved and don't need this.
KHRONOS_WEBGL_JOBS = 4

# Per-test timeout (seconds) for the Khronos WebGL suites only. The basic
# driver's default watchdog (DEFAULT_NATIVE_TIMEOUT_SEC, 180s) is meant to
# catch genuine hangs, never slow-but-passing tests -- but this suite has
# legitimately slow tests: multisample-corruption.html takes ~160s on a fast
# 32-core dev machine (2048x2048 readbacks verified in interpreted JS, x25
# iterations), leaving no margin for a slower or loaded CI host. Give the
# suite enough headroom that only real hangs trip the watchdog; the other
# (DOM/CSS) suites keep the tighter default.
KHRONOS_WEBGL_TIMEOUT_SEC = 1200

def run_vendor_test_khronos(root, name):
    from http_server import popen_server

    ROOT = root
    DIR = working_directory
    ADDRESS = "localhost"
    PORT = 11010

    env = dict(os.environ)

    if not env.get(ENVOPTS.REPLACE_STR):
        env[ENVOPTS.REPLACE_STR] = f"{ROOT}/\\http://{ADDRESS}:{PORT}/"

    if not env.get(ENVOPTS.TIMEOUT):
        env[ENVOPTS.TIMEOUT] = str(KHRONOS_WEBGL_TIMEOUT_SEC)

    with popen_server(ROOT, DIR, ADDRESS, port=PORT, silent=True):
        run_test(["basic", name, "common", f"-p{KHRONOS_WEBGL_JOBS}"], env)


def vendor_test_khronos():
    run_vendor_test_khronos("test/cairo/reftest/vendor/khronos/webgl/1.0.3",
                            "tool/reftest/cairo/khronos_webgl.res")


def vendor_test_khronos2():
    run_vendor_test_khronos("test/cairo/reftest/vendor/khronos/webgl/2.0.0",
                            "tool/reftest/cairo/khronos_webgl2.res")


def vendor_test_khronossdk():
    run_vendor_test_khronos("test/cairo/reftest/vendor/khronos/webgl/sdk",
                            "tool/reftest/cairo/khronos_webglsdk.res")


def vendor_test():
    vendor_test_blink()
    vendor_test_gecko()
    vendor_test_webkit()
    vendor_test_khronos()
    vendor_test_khronos2()
    vendor_test_khronossdk()


def wpt_css_css21():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css21_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css21_dev_manual.res", "cairo", "--font-dep"])


def wpt_css_backgrounds():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_manual.res", "cairo", "--font-dep"])


def wpt_css_color():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-color-3_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-color-3_dev_manual.res", "cairo", "--font-dep"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-color-4_dev_pixel.res", "cairo"])


def wpt_css_flexbox():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-flexbox-1_dev_pixel.res", "cairo"])


def wpt_css_transforms():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-transforms-1_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-transforms-1_dev_manual.res", "cairo", "--font-dep"])


def wpt_css_variables():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-variables-1_dev_pixel.res", "cairo"])


def wpt_mediaqueries():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_mediaqueries-3_dev_pixel.res", "cairo"])


def wpt_selectors():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_selectors-3_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_selectors-4_dev_pixel.res", "cairo"])


def wpt_css_all():
    wpt_css_css21()
    wpt_css_backgrounds()
    wpt_css_color()
    wpt_css_flexbox()
    wpt_css_transforms()
    wpt_css_variables()
    wpt_mediaqueries()
    wpt_selectors()


def wpt_all():
    wpt_css_all()


# WPT (testharness) via on-demand `wpt serve` -- see docs/wpt.md.
# Runs the active (expected-pass) lists under tool/wpt/testharness_lists/; any
# active test failing is treated as a regression. Failing tests are kept as
# `# [auto-fail:REASON]` comments (refresh with wpt_runner.py + wpt_annotate.py).
#
# wpt_serve_testharness runs every list; each wpt_serve_<module> suite runs
# one group of lists so a module can be checked in isolation.
_WPT_TESTHARNESS_LISTS_DIR = os.path.join(working_directory, "tool/wpt/testharness_lists")


def _wpt_serve_run(*patterns, jobs=8, timeout=20, daemons=(), exclude=()):
    import glob
    import shutil
    import tempfile
    import wpt_runner
    from wpt_server import wpt_serve, DEFAULT_WPT_ROOT, WptServerError

    if patterns:
        targets = []
        for pat in patterns:
            targets.extend(sorted(glob.glob(os.path.join(_WPT_TESTHARNESS_LISTS_DIR, pat))))
    else:
        targets = sorted(glob.glob(os.path.join(_WPT_TESTHARNESS_LISTS_DIR, "*.res")))
    if exclude:
        targets = [t for t in targets
                   if os.path.basename(t) not in exclude]

    items = []
    for t in targets:
        items.extend(wpt_runner.collect(t, force=False))
    label = ", ".join(patterns) if patterns else "all"
    print_table("Running WPT (on-demand)", "%d tests [%s]" % (len(items), label))
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
                    # verbose=True: this suite gates CI, so a crash here means a
                    # crash on the CI machine -- surface the captured backtrace
                    # in the (only) log we get, the CI job's own live stdout.
                    npass, reasons, per_list = wpt_runner.run_all(
                        items, jobs, timeout, None, verbose=True,
                        storage_dir=shared_storage_dir)
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
    ran_test_count += len(items)
    if len(per_list) > 1:
        for name in sorted(per_list):
            pn, tn = per_list[name]
            print("  %-44s %d/%d" % (name, pn, tn))
    print("WPT pass %d/%d" % (npass, len(items)))
    if npass != len(items):
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
    _wpt_serve_run(exclude=("worker.res", "serviceworker.res"))


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
    from wpt_server import wpt_serve, DEFAULT_WPT_ROOT, WptServerError

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
        with wpt_serve(DEFAULT_WPT_ROOT, verbose=True):
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


def bidi_test():
    run_test(["bidi", "tool/reftest/cairo/bidi.res", "cairo", "--font-dep"])


def reftest_all():
    dom_conformance_test()
    vendor_test()
    bidi_test()
    wpt_all()


def test_all():
    internal_test()
    reftest_all()


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
                           "run_test", "run_vendor_test_khronos", "_wpt_serve_run",
                           "_wpt_manifest_run"]:
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
    args = parser.parse_args()

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

