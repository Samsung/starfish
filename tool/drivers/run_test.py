#!/usr/bin/env python
import os
import sys
import argparse

nproc = None

def default_result_summarizer(result):
    from basics.utils import PColors
    (pass_cnt, fail_cnt) = result
    summary = "Total: " + str(pass_cnt + fail_cnt)
    summary += ", Pass: " + str(pass_cnt)
    if fail_cnt > 0:
        summary += ", Fail:" + str(fail_cnt)
    print PColors.yellow(summary + "\n")
    return (fail_cnt == 0)

# W3C DOM Conformace Test Suites
def run_dom_conformance_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    from tests.dom_conformance_test import tc_handler
    result = basictest.run_parallel(list, nproc, tc_handler=tc_handler)
    return default_result_summarizer(result)

# Vendor Test (text based)
def run_vendor_basic_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    from tests.vendor_test import tc_handler
    result = basictest.run_parallel(list, nproc, tc_handler=tc_handler)
    return default_result_summarizer(result)

# Vendor Test (pixel based)
def run_vendor_pixel_test(list, backend, font_dep, out_file=None):
    import basics.starfish_pixel_test as pixeltest
    from tests.vendor_test import exp_img_namer
    result = pixeltest.run_parallel(list, backend, nproc, ahem_font=(not font_dep),
                                    expected_namer=exp_img_namer)
    return default_result_summarizer(result)

# CSSWG Test
def run_csswg_test(list, backend, font_dep, out_file=None):
    import basics.starfish_pixel_test as pixeltest
    from tests.csswg_test import get_exp_img_namer
    result = pixeltest.run_parallel(list, backend, nproc, ahem_font=(not font_dep),
                                    expected_namer=get_exp_img_namer(font_dep, backend))
    return default_result_summarizer(result)

# Bidi Test
def run_bidi_test(list, backend, font_dep, out_file=None):
    import basics.starfish_pixel_test as pixeltest
    result = pixeltest.run_parallel(list, backend, nproc, ahem_font=(not font_dep),
                                    width=900, height=900)
    return default_result_summarizer(result)

# Default test style (text based)
# > Internal Test
def run_default_basic_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    result = basictest.run_parallel(list, nproc, regression=font_dep)
    return default_result_summarizer(result)

# Default test style (pixel based)
def run_default_pixel_test(list, backend, font_dep, out_file=None):
    import basics.starfish_pixel_test as pixeltest
    result = pixeltest.run_parallel(list, backend, nproc, ahem_font=(not font_dep))
    return default_result_summarizer(result)

# Multi result style
# > Web Platform Test
# > React Test
def run_multi_results_basic_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    from tests.multi_results_test import tc_handler, result_handler, result_summarizer
    result = basictest.run_parallel(list, nproc, tc_handler=tc_handler, result_handler=result_handler)
    return result_summarizer(result)

# WPT basic test
def run_wpt_basic_test(list, backend, font_dep, out_file=None):
    import basics.starfish_basic_test as basictest
    from tests.wpt_test import wpt_tc_handler
    result = basictest.run_parallel(list, nproc, tc_handler=wpt_tc_handler)
    return default_result_summarizer(result)

# WPT reference test
def run_wpt_reference_test(list, backend, font_dep, out_file=None):
    import basics.parallel as parallel
    from tests.wpt_test import wpt_reftest_case_runner
    result = parallel.run_test_pool(wpt_reftest_case_runner, list, nproc, out_path=out_file)
    return default_result_summarizer(result)

tests = {}
# Named tests
tests["dom_conformance"] = run_dom_conformance_test
tests["vendor_basic"] = run_vendor_basic_test
tests["vendor_pixel"] = run_vendor_pixel_test
tests["csswg"] = run_csswg_test
tests["bidi"] = run_bidi_test
tests["wpt_basic"] = run_wpt_basic_test
tests["wpt_ref"] = run_wpt_reference_test
# General tests
tests["basic"] = run_default_basic_test
tests["pixel"] = run_default_pixel_test
tests["multi_basic"] = run_multi_results_basic_test

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("test_kind")
    parser.add_argument("list_file")
    parser.add_argument("backend")
    parser.add_argument("--font-dep", dest="font_dep", action="store_true")
    parser.add_argument("--proc", "-p", dest="proc", type=int)
    parser.add_argument("--out-file", dest="out_file")
    args = parser.parse_args()

    nproc = args.proc
    if not os.path.isfile(args.list_file):
        print "Cannot open " + args.list_file
        sys.exit(1)

    from datetime import datetime
    start_time = datetime.now()
    try:
        result = tests[args.test_kind](args.list_file, args.backend, font_dep=args.font_dep, out_file=args.out_file)
        elapsed_time = int((datetime.now() - start_time).total_seconds() * 1000)
        print "Elapsed time " + str(elapsed_time) + " ms"
        sys.exit(0 if result else 1)
    except KeyError:
        print "No such test named '" + args.test_kind + "'"
