#!/usr/bin/env python
import os
import re
import sys
from urlparse import urlparse
from basics.utils import Strings

RE_KEYWORDS = re.compile(r"PASS|FAIL")

def wpt_tc_handler(tc_file, output, show_progress=True):
    is_pass = False
    result = ""
    tc_expected_file = wpt_exp_namer(tc_file) + ".txt"
    if not os.path.isfile(tc_expected_file):
        result = Strings.FAIL_SIGN + tc_file + ": Expected file does not exist."
    else:
        expected_out = None
        starfish_out = None
        with open(tc_expected_file) as fp:
            expected_out = RE_KEYWORDS.findall(fp.read())
        starfish_out = RE_KEYWORDS.findall(output)
        if expected_out == starfish_out:
            result = Strings.PASS_SIGN
            is_pass = True
        else:
            result = Strings.FAIL_SIGN
        result += tc_file
    if show_progress:
        print result
    return is_pass

def wpt_http_exp_namer(tc_file):
    # Remote test
    # Ex) http://52.79.162.207/some/directory/tc_some_name.html
    #  => test_new/web_platform_test/some/directory/tc_some_name_expected.png
    base_name = urlparse(os.path.splitext(tc_file)[0] + "_expected").path
    root_path = "test_new/web_platform_test"
    return root_path + base_name

def wpt_exp_namer(tc_file):
    if tc_file.startswith("http"):
        return wpt_http_exp_namer(tc_file)
    return os.path.splitext(tc_file)[0] + "_expected"

def wpt_exp_png_namer(tc_file, backend):
    return wpt_exp_namer(tc_file) + "." + backend + ".png"
