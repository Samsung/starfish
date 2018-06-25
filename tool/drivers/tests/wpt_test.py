#!/usr/bin/env python
import os
import re
import subprocess
from shutil import copyfile
import sys
from basics.utils import Strings, PColors
from basics.starfish_pixel_test import pixel_diff, default_tc_handler
from subprocess import Popen, PIPE
from urlparse import urlparse

RE_KEYWORDS = re.compile("STARFISH_REFTEST_REF.*[htm|html|svg|xht]$")

try:
  FNULL
except NameError:
  FNULL = open(os.devnull, "w")

def _extractRefPath(starfish_output):
    list = RE_KEYWORDS.findall(starfish_output)
    if len(list) > 0:
        return list[0][21:]
    return ""

def _validateFile(file):
    return file.startswith("http") or os.path.isfile(file)

def _failed(tc_file, msg):
    print Strings.FAIL_SIGN + tc_file
    print msg

def _succeed(tc_file, msg):
    print Strings.PASS_SIGN + tc_file
    print msg

def _capture_starfish(tc_file, png_name, extra_options=[]):
    starfish_command = ["./StarFish", tc_file, "--hide-window",
                        "--regression-test", "--width=800", "--height=600",
                        "--screen-shot=" + png_name] + extra_options
    starfish_output = ""
    starfish_err = ""
    try:
        p = Popen(starfish_command, stdin=PIPE, stdout=PIPE, stderr=PIPE)
        starfish_output, starfish_err = p.communicate("")
        if not os.path.isfile(png_name):
            return (False, starfish_output + starfish_err)
        else:
            return (True, starfish_output)
    except subprocess.CalledProcessError:
        return (False, "CalledProcessError")
    except OSError, e:
        if e.errno != 17:
            return (False, "OSError")


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

def wpt_reftest_case_runner(tc):
    tc_idx, tc_file = tc
    tc_result_png = str(tc_idx) + "__starfish_result.png"
    tc_ref_png = str(tc_idx) + "__starfish_result.ref.png"

    # Assure TC exist
    if not _validateFile(tc_file):
        print Strings.FAIL_SIGN + tc_file
        print "ERROR : TC file does not exist - " + tc_file
        return False

    # Capture TC in StarFish
    result, outs = _capture_starfish(tc_file, tc_result_png, extra_options=["--ref-test"])
    if not result:
        print Strings.FAIL_SIGN + tc_file
        print "ERROR : Starfish error while running " + tc_file
        print outs
        return False

    # Assure TC reference
    ref_file = _extractRefPath(outs)
    if not len(ref_file) or not _validateFile(ref_file):
        print Strings.FAIL_SIGN + tc_file
        print "Invalid reference file"
        print ": Document does not have reference informations or a reference file does not exist"
        return False

    # Capture TC reference in StarFish
    result, outs = _capture_starfish(ref_file, tc_ref_png)
    if not result:
        print Strings.FAIL_SIGN + tc_file
        print "ERROR : Starfish error while running " + ref_file
        print outs
        return False

    # Image diff
    return pixel_diff(tc_file, tc_result_png, tc_ref_png, default_tc_handler)

