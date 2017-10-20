#!/usr/bin/env python
import os
from basics.utils import Strings
from basics.starfish_pixel_test import default_http_expected_namer

# TODO Make it one simple rule to generate expected image name
def font_dep_exp_img_namer(tc_file, backend):
    if tc_file.startswith("http"):
        return default_http_expected_namer(tc_file, backend)
    if "_converted/" in tc_file:
        # Support legacy expected file names temporarily
        file = os.path.splitext(tc_file)[0] + "-expected.png"
        return file.replace("_converted", "_result/font_dependent/x64", 1)
    else:
        return os.path.splitext(tc_file)[0] + "_expected.png"

def font_indep_exp_img_namer(tc_file, backend):
    if tc_file.startswith("http"):
        return default_http_expected_namer(tc_file, backend)
    file = os.path.splitext(tc_file)[0] + "_expected.png"
    return file.replace("_converted", "_result/font_independent", 1)

def get_exp_img_namer(font_dep, backend):
    if font_dep:
        return font_dep_exp_img_namer
    return font_indep_exp_img_namer
