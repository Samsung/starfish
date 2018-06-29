#!/usr/bin/env python
import os
import re
import subprocess
from shutil import copyfile
import sys
from subprocess import Popen, PIPE
from threading import Timer
from urlparse import urlparse

sys.path.append(os.path.join(os.path.dirname(__file__), "../"))
from basics.utils import Strings, PColors

try:
  FNULL
except NameError:
  FNULL = open(os.devnull, "w")

RE_KEYWORDS = re.compile(r"PASS|FAIL")
RE_RTERROR = re.compile("STARFISH_RTERROR.*")
RE_RTPASS = re.compile("STARFISH_RTPASS$")
RE_RTCAPTURED = re.compile("STARFISH_RTCAPTURED.*")
TIMEOUT_SEC = 5

def _detectError(outs):
    list = RE_RTERROR.findall(outs)
    if len(list):
        return list[0][17:]
    return None

def _isPass(outs):
    list = RE_RTPASS.findall(outs)
    return len(list) > 0

def _validateFile(file):
    return file.startswith("http") or os.path.isfile(file)

def _timeout(proc, tc_file):
    proc.kill()
    print("Timeout(" + str(TIMEOUT_SEC) + "s): " + tc_file)

def _run_starfish_reftest(tc_file):
    starfish_command = ["./StarFish", tc_file,
                        "--ref-test", "--width=800", "--height=600"]
    outs = ""
    errs = ""
    try:
        p = Popen(starfish_command, stdin=PIPE, stdout=PIPE, stderr=PIPE)
        timer = Timer(TIMEOUT_SEC, _timeout, args=[p, tc_file])
        timer.start()
        outs, errs = p.communicate("")
    except subprocess.CalledProcessError:
        return (False, "CalledProcessError")
    finally:
        timer.cancel()
    return (True, outs)

def _gen_diff(outs):
    list = RE_RTCAPTURED.findall(outs)
    if len(list) != 2:
        return
    diff_cmd = ["test/tool/image_diff", "--diff",
                list[0][20:], list[1][20:], "diff.png"]
    subprocess.call(diff_cmd, stdout=FNULL, stderr=subprocess.STDOUT)
    print "Check 'diff.png'"

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
        print(result)
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

def wpt_reftest_case_runner(tc, gen_diff=False):
    tc_idx, tc_file = tc

    # Assure TC exist
    if not _validateFile(tc_file):
        print(Strings.FAIL_SIGN + tc_file)
        print("ERROR : TC file does not exist - " + tc_file)
        return False

    # Run StarFish
    result, outs = _run_starfish_reftest(tc_file)
    if not result:
        print(Strings.FAIL_SIGN + tc_file + " TC_CRASH")
        print("ERROR : Starfish error while running " + tc_file)
        print(outs)
        return False

    err = _detectError(outs)
    if err is not None:
        print(Strings.FAIL_SIGN + tc_file + " " + err)
        return False

    if not _isPass(outs):
        print(Strings.FAIL_SIGN + tc_file)
        if gen_diff:
            _gen_diff(outs)
        return False

    print(Strings.PASS_SIGN + tc_file)
    return True

# standalone version
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("tc_file")
    args = parser.parse_args()

    wpt_reftest_case_runner((0, args.tc_file), gen_diff=True)

