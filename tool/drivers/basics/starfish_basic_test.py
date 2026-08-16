#!/usr/bin/env python3
import os
import re
from . import utils
import fcntl
from basics.constants import ENVOPTS, resolve_tc_timeout
from basics.subprocess_timeout import run_subprocess_with_timeout

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
TEST_RESULT_PASS_FILE = TEST_RESULT_FAIL_FILE = None

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
        if utils.is_int(v):
            self.width = WIDTH_OPT_PREFIX + str(v)

    def set_height(self, v):
        if utils.is_int(v):
            self.height = HEIGHT_OPT_PREFIX + str(v)

    def set_regression(self, v):
        if utils.is_bool(v):
            self.regression = REGRESSION_OPT if v else NON_REGRESSION_OPT

    def set_show_progress(self, v):
        if utils.is_bool(v):
            self.show_progress = v

    def set_tc_handler(self, v):
        if utils.is_function(v):
            self.tc_handler = v


def open_subprocess(command, timeout=None):
    # Delegates to the shared helper (also used by starfish_pixel_test.py) so
    # both test styles get identical hang protection: SIGKILL the whole
    # process group and raise TimeoutError if `command` outlives `timeout`.
    return run_subprocess_with_timeout(command, timeout)


def case_runner(tc):
    tc_idx, tc_file = tc
    # Assure TC exist
    if not (tc_file.startswith("http") or os.path.isfile(tc_file)):
        print("ERROR : TC file does not exist - " + tc_file)
        return __opts.tc_handler(tc_file, "FAIL", __opts.show_progress)

    # Fails safe: resolve_tc_timeout() returns a default (currently 60s) when
    # TC_TIMEOUT isn't set at all, rather than "wait forever". Every CI job
    # today invokes test_runner.py without --timeout, so without this default
    # a single hung Starfish process previously blocked its whole parallel
    # batch (and thus the CI job) until the outer 60-minute job timeout fired
    # with zero diagnostic output.
    timeout = resolve_tc_timeout()

    # Run starfish
    starfish_command = ["./Starfish", tc_file, "--hide-window", __opts.width, __opts.height, __opts.regression, "--disable-console"]
    # Bind before the try so the except handler stays safe even when
    # open_subprocess raises before returning (e.g. Popen fails to launch).
    starfish_output = starfish_err = b""
    try:
        starfish_output, starfish_err, elapsed_time = open_subprocess(starfish_command, timeout)
        starfish_output = str(starfish_output, 'utf-8')
        starfish_err = str(starfish_err, 'utf-8')
        if "[STARFISH_TEST] Got signal" in starfish_output :
            raise Exception("Starfish Got signal")
    except TimeoutError:
        print(f"ERROR : Timeout ({timeout} sec.) - {tc_file}")
        return __opts.tc_handler(tc_file, "FAIL", __opts.show_progress)
    except:
        print("ERROR : Crash - " + tc_file)
        print("stdout=>")
        print(starfish_output)
        print("stderr=>")
        print(starfish_err)
        return __opts.tc_handler(tc_file, "FAIL", __opts.show_progress)
    return __opts.tc_handler(tc_file, starfish_output, starfish_err, __opts.show_progress, elapsed_time=elapsed_time)


def run_parallel(list_file, nproc=None, width=None, height=None, regression=None,
                 show_progress=None, tc_handler=None, result_handler=None):
    from . import parallel
    global __opts
    if __opts is None:
        __opts = __BasicTestOpts()
    __opts.set_width(width)
    __opts.set_height(height)
    __opts.set_regression(regression)
    __opts.set_show_progress(show_progress)
    __opts.set_tc_handler(tc_handler)

    return parallel.run_test_pool(case_runner, list_file, nproc,
                                  result_handler=result_handler)

if os.environ.get(ENVOPTS.TEST_RESULT_FILE):
    test_result_file = os.environ.get(ENVOPTS.TEST_RESULT_FILE)
    file_name, file_extension = os.path.splitext(test_result_file)
    TEST_RESULT_PASS_FILE = test_result_file
    TEST_RESULT_FAIL_FILE = file_name + "_fail" + file_extension
    with open(TEST_RESULT_PASS_FILE, 'w'):
        pass
    with open(TEST_RESULT_FAIL_FILE, 'w'):
        pass

def time_string(raw):
    if raw is None:
        return ""
    m, s = divmod(int(raw), 60)
    h, m = divmod(m, 60)
    return " ".join(f"{t}{u}" for t, u in zip([h, m, s], ["h", "m", "s"]) if t > 0 or u == "s")

def default_tc_handler(tc_file, output, err, show_progress=True, **kwargs):
    elapsed_time = kwargs.get('elapsed_time', None)

    word_pass = len(RE_PASS.findall(output))
    word_fail = len(RE_FAIL.findall(output))
    if word_pass != 0 and word_fail == 0:
        if show_progress:
            print(f"{utils.Strings.PASS_SIGN}{tc_file} {time_string(elapsed_time)}")
        if TEST_RESULT_PASS_FILE:
            with open(TEST_RESULT_PASS_FILE, 'a') as file:
                fcntl.flock(file, fcntl.LOCK_EX)
                file.write(tc_file + '\n')
                fcntl.flock(file, fcntl.LOCK_UN)
        return True
    else:
        if show_progress:
            print(utils.Strings.FAIL_SIGN + tc_file)
            print("starfish output  =>")
            print(output)
        if TEST_RESULT_FAIL_FILE:
            with open(TEST_RESULT_FAIL_FILE, 'a') as file:
                fcntl.flock(file, fcntl.LOCK_EX)
                file.write(tc_file + '\n')
                fcntl.flock(file, fcntl.LOCK_UN)
        return False


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('list_file')
    args = parser.parse_args()

    run_parallel(args.list_file)
