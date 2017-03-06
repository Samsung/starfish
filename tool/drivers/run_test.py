#!/usr/bin/env python
import os
import sys
import argparse

def handle_result(result):
    from basics.utils import PColors
    (pass_cnt, fail_cnt) = result
    summary = "Total: " + str(pass_cnt + fail_cnt)
    summary += ", Pass: " + str(pass_cnt)
    if fail_cnt > 0:
        summary += ", Fail:" + str(fail_cnt)
    print PColors.yellow(summary + "\n")
    return (fail_cnt == 0)

# W3C DOM Conformace Test Suites
def run_dom_conformance_test(list, font_dep):
    import basics.starfish_basic_test as basictest
    from tests.dom_conformance_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    return handle_result(result)

# Vendor Tests
def run_vendor_basic_test(list, font_dep):
    import basics.starfish_basic_test as basictest
    from tests.vendor_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    return handle_result(result)

def run_vendor_pixel_test(list, font_dep):
    import basics.starfish_pixel_test as pixeltest
    from tests.vendor_test import exp_img_namer
    result = pixeltest.run_parallel(list, ahem_font=(not font_dep), expected_namer=exp_img_namer)
    return handle_result(result)

# Web Platform Tests
def run_web_platform_test(list, font_dep):
    import basics.starfish_basic_test as basictest
    from tests.wpt_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    return handle_result(result)

# CSSWG Tests
def run_csswg_test(list, font_dep):
    import basics.starfish_pixel_test as pixeltest
    from tests.csswg_test import get_exp_img_namer, tc_handler
    result = pixeltest.run_parallel(list, tc_handler=tc_handler, ahem_font=(not font_dep), expected_namer=get_exp_img_namer(font_dep))
    return handle_result(result)

# Internal Tests
def run_default_basic_test(list, font_dep):
    import basics.starfish_basic_test as basictest
    result = basictest.run_parallel(list, regression=font_dep)
    return handle_result(result)

def run_default_pixel_test(list, font_dep):
    import basics.starfish_pixel_test as pixeltest
    result = pixeltest.run_parallel(list, ahem_font=(not font_dep))
    return handle_result(result)


tests = {}
tests["dom_conformance"] = run_dom_conformance_test
tests["web_platform"] = run_web_platform_test
tests["vendor_basic"] = run_vendor_basic_test
tests["vendor_pixel"] = run_vendor_pixel_test
tests["csswg"] = run_csswg_test
tests["internal"] = run_default_basic_test
tests["pixel"] = run_default_pixel_test


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("test_kind")
    parser.add_argument("list_file")
    parser.add_argument("--font-dep", dest="font_dep", action="store_true")
    args = parser.parse_args()

    if not os.path.isfile(args.list_file):
        print "Cannot open " + args.list_file
        sys.exit(1)

    from datetime import datetime
    start_time = datetime.now()
    try:
        result = tests[args.test_kind](args.list_file, font_dep=args.font_dep)
        elapsed_time = int((datetime.now() - start_time).total_seconds() * 1000)
        print "Elapsed time " + str(elapsed_time) + " ms"
        sys.exit(0 if result else 1)
    except KeyError:
        print "No such test named '" + args.test_kind + "'"
