#!/usr/bin/env python
import os
from functools import partial


def print_result(result):
    from basics.utils import PColors
    (pass_cnt, fail_cnt) = result
    summary = "Total: " + str(pass_cnt + fail_cnt)
    summary += ", Pass: " + str(pass_cnt)
    summary += ", Fail:" + str(fail_cnt) + "\n"
    # if fail_cnt > 0:
    #     summary = "FAIL " + summary
    # else:
    #     summary = "PASS " + summary
    print PColors.yellow(summary)

# W3C DOM Conformace Test Suites
def run_dom_conformance_test(list):
    import basics.starfish_basic_test as basictest
    from tests.dom_conformance_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    print_result(result)

# Vendor Tests
def run_vendor_test(list):
    import basics.starfish_basic_test as basictest
    from tests.vendor_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    print_result(result)

def run_vendor_pixel_test(normal_list, manual_list):
    import basics.starfish_pixel_test as pixeltest
    from tests.vendor_test import exp_img_namer
    normal = pixeltest.run_parallel(normal_list, expected_namer=exp_img_namer)
    manual = pixeltest.run_parallel(manual_list, ahem_font=False, expected_namer=exp_img_namer)
    print_result((normal[0] + manual[0], normal[1] + manual[1]))

# Web Platform Tests
def run_web_platform_test(list):
    import basics.starfish_basic_test as basictest
    from tests.wpt_test import tc_handler
    result = basictest.run_parallel(list, tc_handler=tc_handler)
    print_result(result)

# CSSWG Tests
def run_csswg_test(list):
    import basics.starfish_pixel_test as pixeltest
    from tests.csswg_test import fd_exp_img_namer, result_handler
    result = pixeltest.run_parallel(list, expected_namer=fd_exp_img_namer, result_handler=result_handler)
    print_result(result)

# Internal Tests
def run_internal_test(normal_list, manual_list=None):
    import basics.starfish_basic_test as basictest
    normal = basictest.run_parallel(normal_list)
    manual = (0, 0)
    if manual_list is not None:
        manual = basictest.run_parallel(manual_list, regression=True)
    print_result((normal[0] + manual[0], normal[1] + manual[1]))


tests = {}

# Binding W3C DOM Conformace Test Suites
tests["regression_test_dom_conformance_test"] = partial(run_dom_conformance_test, "tool/reftest/dom_conformance_test.res")
tests["regression_test_webkit_dom_conformance_test"] = partial(run_dom_conformance_test, "tool/reftest/webkit_dom_conformance_test.res")
tests["regression_test_blink_dom_conformance_test"] = partial(run_dom_conformance_test, "tool/reftest/blink_dom_conformance_test.res")
tests["regression_test_gecko_dom_conformance_test"] = partial(run_dom_conformance_test, "tool/reftest/gecko_dom_conformance_test.res")

# Binding Vendor Tests (Text)
tests["regression_test_webkit_fast_dom"] = partial(run_vendor_test, "tool/reftest/webkit_fast_dom.res")
tests["regression_test_webkit_fast_html"] = partial(run_vendor_test, "tool/reftest/webkit_fast_html.res")
tests["regression_test_blink_fast_dom"] = partial(run_vendor_test, "tool/reftest/blink_fast_dom.res")
tests["regression_test_blink_fast_html"] = partial(run_vendor_test, "tool/reftest/blink_fast_html.res")

# Binding Vendor Tests (Pixel)
tests["regression_test_webkit_fast_css"] = partial(run_vendor_pixel_test, "tool/reftest/webkit_fast_css.res", "tool/reftest/webkit_fast_css_manual.res")
tests["regression_test_webkit_fast_etc"] = partial(run_vendor_pixel_test, "tool/reftest/webkit_fast_etc.res", "tool/reftest/webkit_fast_etc_manual.res")
tests["regression_test_blink_fast_css"] = partial(run_vendor_pixel_test, "tool/reftest/blink_fast_css.res", "tool/reftest/blink_fast_css_manual.res")
tests["regression_test_blink_fast_etc"] = partial(run_vendor_pixel_test, "tool/reftest/blink_fast_etc.res", "tool/reftest/blink_fast_etc_manual.res")

# Binding Web Platform Tests
tests["regression_test_wpt_dom"] = partial(run_web_platform_test, "tool/reftest/wpt_dom.res")
tests["regression_test_wpt_dom_events"] = partial(run_web_platform_test, "tool/reftest/wpt_dom_events.res")
tests["regression_test_wpt_html"] = partial(run_web_platform_test, "tool/reftest/wpt_html.res")
tests["regression_test_wpt_page_visibility"] = partial(run_web_platform_test, "tool/reftest/wpt_page_visibility.res")
tests["regression_test_wpt_progress_events"] = partial(run_web_platform_test, "tool/reftest/wpt_progress_events.res")
tests["regression_test_wpt_xhr"] = partial(run_web_platform_test, "tool/reftest/wpt_xhr.res")

# Binding CSSWG Tests
tests["pixel_test_css1"] = partial(run_csswg_test, "tool/reftest/tclist/wpt_css1.res")
tests["pixel_test_css21"] = partial(run_csswg_test, "tool/reftest/tclist/wpt_css21.res")
tests["pixel_test_css3_backgrounds"] = partial(run_csswg_test, "tool/reftest/tclist/wpt_css3_backgrounds.res")
tests["pixel_test_css3_color"] = partial(run_csswg_test, "tool/reftest/tclist/wpt_css3_color.res")
tests["pixel_test_css3_transforms"] = partial(run_csswg_test, "tool/reftest/tclist/wpt_css3_transforms.res")
tests["pixel_test_css_rtl"] = partial(run_csswg_test, "tool/reftest/tclist/wpt_rtl_css.res")

# Binding Internal Tests
tests["internal_test_fast"] = partial(run_internal_test, "tool/reftest/internal_fast.res", "tool/reftest/internal_manual.res")
tests["internal_test"] = partial(run_internal_test, "tool/reftest/internal.res", "tool/reftest/internal_manual.res")
tests["internal_test_part1"] = partial(run_internal_test, "tool/reftest/internal_part1.res")
tests["internal_test_part2"] = partial(run_internal_test, "tool/reftest/internal_part2.res")
tests["internal_test_part3"] = partial(run_internal_test, "tool/reftest/internal_part3.res")
tests["internal_test_part4"] = partial(run_internal_test, "tool/reftest/internal_part4.res")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('test_title')
    args = parser.parse_args()
    try:
        tests[args.test_title]()
    except KeyError:
        print 'No such test "' + args.test_title + '"'
