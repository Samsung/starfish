#!/usr/bin/env python
import os
from basics.utils import Strings

# TODO Make it one simple rule to generate expected image name
def font_dep_exp_img_namer(tc_file):
    if "_converted/" in tc_file:
        # Support legacy expected file names temporarily
        file = os.path.splitext(tc_file)[0] + "-expected.png"
        return file.replace("_converted", "_result/font_dependent/x64", 1)
    else:
        return os.path.splitext(tc_file)[0] + "_expected.png"

def font_indep_exp_img_namer(tc_file):
    file = os.path.splitext(tc_file)[0] + "_expected.png"
    return file.replace("_converted", "_result/font_independent", 1)

def get_exp_img_namer(font_dep):
    if font_dep:
        return font_dep_exp_img_namer
    return font_indep_exp_img_namer

def tc_handler(tc_file, diff_result, show_progress=True):
    is_passed = False
    if "passed" in diff_result:
        is_passed = True
        if show_progress:
            print Strings.PASS_SIGN + tc_file + " " + diff_result
    else:
        if show_progress:
            print Strings.FAIL_SIGN + tc_file + " " + diff_result
    return is_passed
