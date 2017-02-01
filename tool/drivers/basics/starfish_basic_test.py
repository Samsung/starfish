#!/usr/bin/env python
import os
import re
import subprocess
import utils

try:
  FNULL
except NameError:
  FNULL = open(os.devnull, 'w')
__opts = None
WIDTH_OPT_PREFIX = "--width="
HEIGHT_OPT_PREFIX = "--height="
REGRESSION_OPT = "--regression-test"
NON_REGRESSION_OPT = ""
DEFAULT_WIDTH_OPT = WIDTH_OPT_PREFIX + "800"
DEFAULT_HEIGHT_OPT = HEIGHT_OPT_PREFIX + "600"
DEFAULT_REGRESSION_OPT = NON_REGRESSION_OPT

RE_PASS = re.compile(r"PASS")
RE_FAIL = re.compile(r"FAIL")

class __BasicTestOpts():
    def __init__(self):
        self.width = DEFAULT_WIDTH_OPT
        self.height = DEFAULT_HEIGHT_OPT
        self.regression = DEFAULT_REGRESSION_OPT
        self.show_progress = True
        self.tc_handler = default_tc_handler

    def set_width(self, v):
        if utils.isInt(v):
            self.width = WIDTH_OPT_PREFIX + str(v)

    def set_height(self, v):
        if utils.isInt(v):
            self.height = HEIGHT_OPT_PREFIX + str(v)

    def set_regression(self, v):      
        if utils.isBool(v):
            self.regression = REGRESSION_OPT if v else NON_REGRESSION_OPT

    def set_show_progress(self, v):
        if utils.isBool(v):
            self.show_progress = v

    def set_tc_handler(self, v):
        if utils.isFunction(v):
            self.tc_handler = v


def case_runner(tc):
    tc_idx, tc_file = tc
    # Assure TC exist
    if not (tc_file.startswith("http") or os.path.isfile(tc_file)):
        print "ERROR : TC file does not exist - " + tc_file
        return __opts.tc_handler(tc_file, "FAIL", __opts.show_progress)

    # Run starfish
    starfish_command = ["./StarFish", tc_file, "--hide-window", __opts.width, __opts.height, __opts.regression]
    try:
        result = subprocess.check_output(starfish_command, stderr=FNULL)
    except subprocess.CalledProcessError:
        print "ERROR : Crash - " + tc_file
        return __opts.tc_handler(tc_file, "FAIL", __opts.show_progress)
    return __opts.tc_handler(tc_file, result, __opts.show_progress)


def run_parallel(list_file, width=None, height=None, regression=None,
                 show_progress=None, tc_handler=None, result_handler=None):
    import parallel
    global __opts
    if __opts is None:
        __opts = __BasicTestOpts()
    __opts.set_width(width)
    __opts.set_height(height)
    __opts.set_regression(regression)
    __opts.set_show_progress(show_progress)
    __opts.set_tc_handler(tc_handler)

    return parallel.run_test_pool(case_runner, list_file, result_handler=result_handler)


def default_tc_handler(tc_file, output, show_progress=True):
    word_pass = len(RE_PASS.findall(output))
    word_fail = len(RE_FAIL.findall(output))
    if word_pass != 0 and word_fail == 0:
        if show_progress:
            print utils.Strings.PASS_SIGN + tc_file
        return True
    else:
        if show_progress:
            print utils.Strings.FAIL_SIGN + tc_file
        return False


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('list_file')
    args = parser.parse_args()

    run_parallel(args.list_file)