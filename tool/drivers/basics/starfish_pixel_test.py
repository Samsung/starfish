#!/usr/bin/env python
import os
import subprocess
import utils
from urlparse import urlparse
from shutil import copyfile

try:
  FNULL
except NameError:
  FNULL = open(os.devnull, "w")

__opts = None
ERRSTR = " diff: 100.0% failed"
WIDTH_OPT_PREFIX = "--width="
HEIGHT_OPT_PREFIX = "--height="
SCREENSHOT_OPT_PREFIX = "--screen-shot="
AHEM_OPT = "--pixel-test"
NON_AHEM_OPT = "--regression-test"
HIDE_WINDOW_OPT = "--hide-window"
DEFAULT_WIDTH_OPT = WIDTH_OPT_PREFIX + "800"
DEFAULT_HEIGHT_OPT = HEIGHT_OPT_PREFIX + "600"
DEFAULT_FONT_OPT = AHEM_OPT
REMOTE_EXP_DIR = "test/remote-test"
OUT_DIR = "out"

class __PixelTestOpts():
    def __init__(self):
        self.width = DEFAULT_WIDTH_OPT
        self.height = DEFAULT_HEIGHT_OPT
        self.font_opt = DEFAULT_FONT_OPT
        self.show_progress = True
        self.expected_namer = default_expected_namer
        self.tc_handler = default_tc_handler

    def set_width(self, v):
        if utils.is_int(v):
            self.width = WIDTH_OPT_PREFIX + str(v)

    def set_height(self, v):
        if utils.is_int(v):
            self.height = HEIGHT_OPT_PREFIX + str(v)

    def set_ahem_font(self, v):      
        if utils.is_bool(v):
            self.font_opt = AHEM_OPT if v else NON_AHEM_OPT

    def set_show_progress(self, v):
        if utils.is_bool(v):
            self.show_progress = v

    def set_expected_namer(self, v):
        if utils.is_function(v):
            self.expected_namer = v

    def set_tc_handler(self, v):
        if utils.is_function(v):
            self.tc_handler = v

def case_runner(tc):
    tc_idx, tc_file = tc
    tc_expected_png = __opts.expected_namer(tc_file)
    tc_result_png = str(tc_idx) + "__starfish_result.png"

    # Assure TC exist
    if not (tc_file.startswith("http") or os.path.isfile(tc_file)):
        print "ERROR : TC file does not exist - " + tc_file
        return __opts.tc_handler(tc_file, ERRSTR, __opts.show_progress)

    # Assure expected image
    if not os.path.isfile(tc_expected_png):
        print "ERROR : Expected file does not exist - " + tc_expected_png
        return __opts.tc_handler(tc_file, ERRSTR, __opts.show_progress)

    # Create screen-shot image using Starfish
    starfish_command = ["./StarFish", tc_file, HIDE_WINDOW_OPT,
                        __opts.font_opt, __opts.width, __opts.height,
                        SCREENSHOT_OPT_PREFIX + tc_result_png]
    try:
        subprocess.call(starfish_command, stdout=FNULL, stderr=subprocess.STDOUT)
        if not os.path.isfile(tc_result_png):
            print "ERROR : Starfish error - " + tc_file
            return __opts.tc_handler(tc_file, ERRSTR, __opts.show_progress)

        # Diff
        diff_command = ["tool/imgdiff/imgdiff", tc_result_png, tc_expected_png]
        diff_result = subprocess.check_output(diff_command).decode("UTF-8").strip()
        success = __opts.tc_handler(tc_file, diff_result, __opts.show_progress)
        if not success:
            # When tc failed, give 3 images to user
            # _1 Screen-shot from Starfish
            # _2 Expected image
            # _3 Diff image
            base_path = os.path.join(OUT_DIR, tc_file)
            image_1 = base_path + ".png"
            image_2 = base_path + "_expected.png"
            image_3 = base_path + "_diff.png"
            dir_name = os.path.dirname(image_1)
            if not os.path.exists(dir_name):
                os.makedirs(dir_name)
            copyfile(tc_result_png, image_1)
            copyfile(tc_expected_png, image_2)
            gen_cmd = ["test/tool/image_diff", "--diff",
                                image_1, image_2, image_3]
            subprocess.call(gen_cmd, stdout=FNULL, stderr=subprocess.STDOUT)
            print utils.PColors.red("Check images: " + base_path + "*.png")

        os.remove(tc_result_png)
        return success

    except subprocess.CalledProcessError:
        return __opts.tc_handler(tc_file, ERRSTR, __opts.show_progress)
    except OSError, e:
        if e.errno != 17:
            raise



def run_parallel(list_file, nproc=None, width=None, height=None,
                 ahem_font=None, show_progress=None,
                 expected_namer=None, tc_handler=None, result_handler=None):
    import parallel
    global __opts
    if __opts is None:
        __opts = __PixelTestOpts()
    __opts.set_width(width)
    __opts.set_height(height)
    __opts.set_ahem_font(ahem_font)
    __opts.set_show_progress(show_progress)
    __opts.set_expected_namer(expected_namer)
    __opts.set_tc_handler(tc_handler)

    return parallel.run_test_pool(case_runner, list_file, nproc,
                                  result_handler=result_handler)


def default_tc_handler(tc_file, diff_result, show_progress=True):
    is_passed = False
    result = ""
    if "passed" in diff_result:
        result = utils.Strings.PASS_SIGN
        is_passed = True
    else:
        result = utils.Strings.FAIL_SIGN
    result += tc_file + " " + diff_result
    if show_progress:
        print result
    return is_passed

def default_http_expected_namer(tc_file):
    # Remote test
    # Ex) http://52.79.162.207/some/directory/tc_some_name.html
    #  => test/remote-test/some/directory/tc_some_name_expected.png
    expected = os.path.splitext(tc_file)[0] + "_expected.png"
    path = urlparse(expected).path
    return REMOTE_EXP_DIR + path

def default_expected_namer(tc_file):
    if tc_file.startswith("http"):
        return default_http_expected_namer(tc_file)
    return os.path.splitext(tc_file)[0] + "_expected.png"

# standalone version
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("list_path")
    parser.add_argument("--width", dest="width", action="store")
    parser.add_argument("--height", dest="height", action="store")
    parser.add_argument("--ahem_font", dest="ahem_font", action="store_true")
    args = parser.parse_args()

    run_parallel(args.list_path, args.width, args.height, args.ahem_font)