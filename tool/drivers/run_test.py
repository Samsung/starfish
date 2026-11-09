#!/usr/bin/env python3
import os
import sys
import argparse

nproc = None
progress_interval = 0

def default_result_summarizer(result):
    from basics.utils import PColors
    (pass_cnt, fail_cnt) = result
    summary = "Total: " + str(pass_cnt + fail_cnt)
    summary += ", Pass: " + str(pass_cnt)
    if fail_cnt > 0:
        summary += ", Fail:" + str(fail_cnt)
    print(PColors.yellow(summary + "\n"))
    return (fail_cnt == 0)

# WebKit LayoutTests, Blink web tests and Gecko tests, served from
# third_party/webkit, blink and gecko: text tests plus reftests
def run_webkit_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    from tests.webkit_test import case_runner, tc_handler
    result = basictest.run_parallel(list, nproc, tc_handler=tc_handler, runner=case_runner)
    return default_result_summarizer(result)

def run_blink_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    from tests.blink_test import case_runner, tc_handler
    result = basictest.run_parallel(list, nproc, tc_handler=tc_handler, runner=case_runner)
    return default_result_summarizer(result)

def run_gecko_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    from tests.gecko_test import case_runner, tc_handler
    result = basictest.run_parallel(list, nproc, tc_handler=tc_handler, runner=case_runner)
    return default_result_summarizer(result)

# CSSWG Test
def run_csswg_test(list, backend, font_dep, out_file=None):
    import basics.starfish_pixel_test as pixeltest
    from tests.csswg_test import get_exp_img_namer
    result = pixeltest.run_parallel(list, backend, nproc, ahem_font=(not font_dep),
                                    expected_namer=get_exp_img_namer(font_dep, backend))
    return default_result_summarizer(result)

# Default test style (text based)
# > Internal Test
def run_default_basic_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    result = basictest.run_parallel(list, nproc, progress_interval=progress_interval)
    return default_result_summarizer(result)

tests = {}
# Named tests
tests["webkit"] = run_webkit_test
tests["blink"] = run_blink_test
tests["gecko"] = run_gecko_test
tests["csswg"] = run_csswg_test
# General tests
tests["basic"] = run_default_basic_test

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("test_kind")
    parser.add_argument("list_file")
    parser.add_argument("backend")
    parser.add_argument("--font-dep", dest="font_dep", action="store_true")
    parser.add_argument("--proc", "-p", dest="proc", type=int)
    parser.add_argument("--out-file", dest="out_file")
    parser.add_argument("--progress-interval", type=float, default=0,
                        help="Basic-test progress interval in seconds (0 disables)")
    args = parser.parse_args()
    if args.progress_interval < 0:
        parser.error("--progress-interval must be nonnegative")
    progress_interval = args.progress_interval

    nproc = args.proc
    if not os.path.isfile(args.list_file):
        print("Cannot open " + args.list_file)
        sys.exit(1)

    from datetime import datetime
    start_time = datetime.now()
    try:
        result = tests[args.test_kind](args.list_file, args.backend, font_dep=args.font_dep, out_file=args.out_file)
        elapsed_time = int((datetime.now() - start_time).total_seconds() * 1000)
        print("Elapsed time " + str(elapsed_time) + " ms")
        sys.exit(0 if result else 1)
    except KeyError:
        print("No such test named '" + args.test_kind + "'")
