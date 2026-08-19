#!/usr/bin/env python3
import os
import re
import subprocess
from . import utils
from subprocess import Popen, PIPE
import time
import fcntl
import signal
import shutil
import tempfile
from basics.constants import ENVOPTS

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
TIMEOUT_OPT_PREFIX = "--timeout="
# Starfish's own AppLoop can end up idle forever if a test's JS never reaches
# the native testEnd()/wptTestEnd() binding that is the only thing able to
# call AppLoop::stop() on this driver path (see CI_UPGRADE.md:
# vec_009_to_016.html -- gdb-confirmed genuinely idle glib main loop, parked
# in poll() inside g_main_loop_run(), not a JS busy-loop). This is a native
# engine watchdog, independent of the optional TC_TIMEOUT-driven python-side
# subprocess kill below, so the process always exits on its own (clean exit,
# whatever PASS/FAIL text was already printed stays correct) instead of
# hanging the whole test run. Generous on purpose -- this must only ever
# catch genuine hangs, not slow-but-passing tests.
DEFAULT_NATIVE_TIMEOUT_SEC = 180

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
    start_time = time.time()

    # Own a process group only when a timeout is set: that is the only path that
    # may need to SIGKILL the whole tree Starfish spawns. Without a timeout the
    # call behaves exactly as before (no session change), so the common,
    # no-timeout path used by almost every test is untouched.
    process = Popen(command, stdout=PIPE, stderr=PIPE,
                    start_new_session=bool(timeout))
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        # SIGKILL the entire process group, then reap to release the pipes.
        # The second communicate() returns immediately since the tree is dead.
        # ProcessLookupError: the tree already exited in the timeout race; the
        # pipes are then already closed, so just reap and report the timeout.
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.communicate()
        raise TimeoutError

    return stdout, stderr, time.time() - start_time


def case_runner(tc):
    tc_idx, tc_file = tc
    # Assure TC exist
    if not (tc_file.startswith("http") or os.path.isfile(tc_file)):
        print("ERROR : TC file does not exist - " + tc_file)
        return __opts.tc_handler(tc_file, "FAIL", "", __opts.show_progress)

    timeout = None
    if os.environ.get(ENVOPTS.TIMEOUT):
        timeout = float(os.environ.get(ENVOPTS.TIMEOUT))

    # Native watchdog seconds for AppLoopGlib (see DEFAULT_NATIVE_TIMEOUT_SEC
    # above). Reuse TC_TIMEOUT when it's set so the native watchdog and the
    # python-side kill (if any) agree; otherwise fall back to the generous
    # default so this is armed even when TC_TIMEOUT isn't configured at all.
    native_timeout = int(timeout) if timeout else DEFAULT_NATIVE_TIMEOUT_SEC

    # Give every test its own throwaway localStorage/cookies/HTTP-cache dir
    # instead of Starfish's default $HOME/Starfish-storage. That default is
    # shared by every Starfish process on the machine -- including a real
    # user's own browsing profile -- so without this, a test that seeds its
    # state from localStorage (e.g. a TodoMVC app restoring saved todos) can
    # accumulate state across repeated runs and start failing permanently
    # (root-caused live: vue-3-2-todo-dist/index.html's `r.length == 2`
    # assertion kept failing because 12+ stale todos had piled up in
    # $HOME/Starfish-storage/localStorage.txt from earlier runs -- the app
    # code was fine, the shared profile wasn't). A fresh directory per test
    # also means CI runs (especially on persistent self-hosted runners,
    # which don't get a clean $HOME every job) can't cross-contaminate.
    storage_dir = tempfile.mkdtemp(prefix="starfish-storage-")
    try:
        # Run starfish
        starfish_command = ["./Starfish", tc_file, "--hide-window", __opts.width, __opts.height, __opts.regression, "--disable-console",
                            TIMEOUT_OPT_PREFIX + str(native_timeout), "--storage-dir=" + storage_dir]
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
            return __opts.tc_handler(tc_file, "FAIL", "", __opts.show_progress)
        except:
            print("ERROR : Crash - " + tc_file)
            print("stdout=>")
            print(starfish_output)
            print("stderr=>")
            print(starfish_err)
            return __opts.tc_handler(tc_file, "FAIL", "", __opts.show_progress)
        return __opts.tc_handler(tc_file, starfish_output, starfish_err, __opts.show_progress, elapsed_time=elapsed_time)
    finally:
        shutil.rmtree(storage_dir, ignore_errors=True)


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
            if err:
                print("starfish stderr  =>")
                print(err)
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
