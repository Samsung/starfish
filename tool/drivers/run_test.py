#!/usr/bin/env python
import os

def print_result(result):
    from basics.utils import PColors
    (pass_cnt, fail_cnt) = result
    summary = "Total: " + str(pass_cnt + fail_cnt)
    summary += ", Pass: " + str(pass_cnt)
    if fail_cnt > 0:
        summary += ", Fail:" + str(fail_cnt)
    print PColors.yellow(summary + "\n")

# W3C DOM Conformace Test Suites
def run_dom_conformance_test(list, font_dep):
    import basics.starfish_basic_test as basictest
    from tests.dom_conformance_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    print_result(result)

# Vendor Tests
def run_vendor_basic_test(list, font_dep):
    import basics.starfish_basic_test as basictest
    from tests.vendor_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    print_result(result)

def run_vendor_pixel_test(list, font_dep):
    import basics.starfish_pixel_test as pixeltest
    from tests.vendor_test import exp_img_namer
    result = pixeltest.run_parallel(list, ahem_font=(not font_dep), expected_namer=exp_img_namer)
    print_result(result)

# Web Platform Tests
def run_web_platform_test(list, font_dep):
    import basics.starfish_basic_test as basictest
    from tests.wpt_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    print_result(result)

# CSSWG Tests
def run_csswg_test(list, font_dep):
    import basics.starfish_pixel_test as pixeltest
    from tests.csswg_test import get_exp_img_namer, result_handler
    result = pixeltest.run_parallel(list, ahem_font=(not font_dep), expected_namer=get_exp_img_namer(font_dep), result_handler=result_handler)
    print_result(result)

# Internal Tests
def run_default_basic_test(list, font_dep):
    import basics.starfish_basic_test as basictest
    result = basictest.run_parallel(list, regression=font_dep)
    print_result(result)


tests = {}
tests["dom_conformance"] = run_dom_conformance_test
tests["web_platform"] = run_web_platform_test
tests["vendor_basic"] = run_vendor_basic_test
tests["vendor_pixel"] = run_vendor_pixel_test
tests["csswg"] = run_csswg_test
tests["internal"] = run_default_basic_test


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("test_kind")
    parser.add_argument("list_file")
    parser.add_argument("--font-dep", dest="font_dep", action="store_true")
    args = parser.parse_args()

    if not os.path.isfile(args.list_file):
        print "Cannot open " + args.list_file
        sys.exit()

    try:
        tests[args.test_kind](args.list_file, font_dep=args.font_dep)
    except KeyError:
        print "No such test named '" + args.test_title + "'"
