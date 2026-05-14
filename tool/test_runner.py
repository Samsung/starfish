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
from drivers.basics.constants import ENVOPTS, ERRORCODE
from execution_worker import WorkerRunner

script_path = "./tool/drivers/run_test.py"
working_directory = os.path.dirname(os.path.abspath(__file__)) + "/../"
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


def run_vendor_test_khronos(root, name):
    from http_server import popen_server

    ROOT = root
    DIR = working_directory
    ADDRESS = "localhost"
    PORT = 11010

    env = dict(os.environ)

    if not env.get(ENVOPTS.REPLACE_STR):
        env[ENVOPTS.REPLACE_STR] = f"{ROOT}/\\http://{ADDRESS}:{PORT}/"

    with popen_server(ROOT, DIR, ADDRESS, port=PORT, silent=True):
        run_test(["basic", name, "common"], env)


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
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_css21_dev_basic.res", "basic"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css21_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css21_dev_manual.res", "cairo", "--font-dep"])


def wpt_css_backgrounds():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_basic.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_manual.res", "cairo", "--font-dep"])


def wpt_css_color():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_css-color-3_dev_basic.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-color-3_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-color-3_dev_manual.res", "cairo", "--font-dep"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_css-color-4_dev_basic.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-color-4_dev_pixel.res", "cairo"])


def wpt_css_flexbox():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_css-flexbox-1_dev_basic.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-flexbox-1_dev_pixel.res", "cairo"])


def wpt_cssom_view():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_cssom_view.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_cssom-view-1_dev_basic.res", "cairo"])


def wpt_css_transforms():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_css-transforms-1_dev_basic.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-transforms-1_dev_pixel.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-transforms-1_dev_manual.res", "cairo", "--font-dep"])


def wpt_css_variables():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_css-variables-1_dev_basic.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_css-variables-1_dev_pixel.res", "cairo"])


def wpt_mediaqueries():
    run_test(["csswg", "tool/reftest/cairo/wpt/css_mediaqueries-3_dev_pixel.res", "cairo"])


def wpt_selectors():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_selectors-3_dev_basic.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_selectors-3_dev_pixel.res", "cairo"])
#    run_test(["multi_basic", "tool/reftest/cairo/wpt/css_selectors-4_dev_basic.res", "cairo"])
    run_test(["csswg", "tool/reftest/cairo/wpt/css_selectors-4_dev_pixel.res", "cairo"])


def wpt_css_all():
    wpt_css_css21()
    wpt_css_backgrounds()
    wpt_css_color()
    wpt_css_flexbox()
    wpt_cssom_view()
    wpt_css_transforms()
    wpt_css_variables()
    wpt_mediaqueries()
    wpt_selectors()


def wpt_others():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/html_basic.res", "cairo"])
    run_test(["csswg_with_remote", "tool/reftest/cairo/wpt/html_pixel.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/dom_basic.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/dom_parsing_basic.res", "cairo"])
#    run_test(["multi_basic", "tool/reftest/cairo/wpt/dom_xpath_basic.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/page_visibility_basic.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/x-frame-options.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/csp.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/webstorage.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/cors.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/cookies.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/fileAPI.res", "cairo"])


def wpt_canvas():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/2dcontext.res", "cairo"])

def wpt_websocket():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/websocket.res", "cairo"])

def wpt_xhr():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/xhr_basic.res", "cairo", "-p8"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/xhr_single_thread.res", "cairo", "-p1"])

def wpt_pwa():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/fetch_basic.res", "cairo"])

def wpt_webrtc():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/webrtc.res", "cairo", "-p1"])

def wpt_intersection_observer():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/intersection-observer.res", "cairo"])

def wpt_idb():
    run_test(["multi_basic", "tool/reftest/cairo/wpt/indexeddb.res", "cairo"])

def wpt_worker():
    runner = WorkerRunner('Starfish-sharedworker')
    runner.run()
    run_test(["multi_basic", "tool/reftest/cairo/wpt/worker.res", "cairo"])
    run_test(["multi_basic", "tool/reftest/cairo/wpt/serviceworker.res", "cairo"])
    runner.terminate()

def wpt_all():
    wpt_css_all()
    wpt_pwa()
    # wpt_webrtc() // Disable in CI
    wpt_canvas()
    wpt_others()
    wpt_websocket()
    wpt_xhr()
    wpt_intersection_observer()
    wpt_idb()

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
    print("run ./tool/test_runner.py")
    print("on repository root directory")
    print("if you want to run specific test suite,")
    print("run ./tool/test_runner.py <test_name> <test_name> ...")
    print("this is list of test suites")

    test_functions = []
    for key, value in list(locals().items()):
        if callable(value) and value.__module__ == __name__:
            if key not in ["file_len", "print_columns", "print_table",
                           "run_test", "run_vendor_test_khronos"]:
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

