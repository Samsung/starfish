#!/usr/bin/env python
import os
import sys
from urlparse import urlparse

def wpt_http_exp_png_namer(tc_file, backend):
    # Remote test
    # Ex) http://52.79.162.207/some/directory/tc_some_name.html
    #  => test_new/web_platform_test/some/directory/tc_some_name_expected.png
    base_name = urlparse(os.path.splitext(tc_file)[0] + "_expected." + backend + ".png").path
    root_path = "test_new/web_platform_test"
    return root_path + base_name

def wpt_exp_png_namer(tc_file, backend):
    if tc_file.startswith("http"):
        return wpt_http_exp_png_namer(tc_file, backend)
    return os.path.splitext(tc_file)[0] + "_expected." + backend + ".png"
