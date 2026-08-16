#!/usr/bin/env python
import math
import signal
import sys
import types
import multiprocessing
import os
from abc import ABC, abstractclassmethod
from basics.constants import ENVOPTS, resolve_tc_timeout

class StringEditor(ABC):
    @abstractclassmethod
    def run(self, input_string):
        pass

class StringReplacer(StringEditor):
    def __init__(self, old_value, new_value):
        self.old_value = old_value
        self.new_value = new_value
    def run(self, input_string):
        return input_string.replace(self.old_value, self.new_value)

def init_worker():
    signal.signal(signal.SIGINT, signal.SIG_IGN)


def run_test_pool(case_runner, in_path, nproc,
                  out_path=None, result_handler=None):
    if type(result_handler) is not types.FunctionType:
        result_handler = default_result_handler

    tcs = []
    idx = 0
    editers = []
    force = True if os.environ.get(ENVOPTS.FORCE_ENABLE) else False

    for value in [v for k, v in os.environ.items() if k.startswith(ENVOPTS.REPLACE_STR)]:
        tokens = value.split("\\")
        editers += [StringReplacer(tokens[0], tokens[1])] if len(tokens) > 1 else []

    # TODO : Consider too long tc list
    try:
        with open(in_path) as fp:
            for line in fp:
                line = line.strip()

                if len(line) == 0:
                    continue

                if line[0] == "#":
                    if force == True:
                        line = line[1:].strip()
                    else:
                        continue

                for editor in editers:
                    line = editor.run(line)

                params = line.split()
                content = params[0]

                if len(content) > 0:
                    # If the 2nd param starts with '#', it's considered a comment.
                    if len(params) > 1 and (not params[1].startswith("#")):
                        tcs.append((idx, [content, params[1]]))
                    else:
                        tcs.append((idx, content))
                    idx = idx + 1
    except IOError:
        print("No such file " + in_path)
        sys.exit(1)

    # Note : http://xcodest.me/interrupt-the-python-multiprocessing-pool-in-graceful-way.html
    max_nproc = multiprocessing.cpu_count();
    if type(nproc) is int:
        nproc = max(1, min(nproc, max_nproc))
    else:
        nproc = max_nproc
    print("Running " + str(nproc) + " jobs in parallel")

    # Bound how long we ever wait for the whole batch. Each individual test
    # case is already bounded by its own per-test-case timeout (see
    # resolve_tc_timeout()/case_runner) -- that is the real fix for a single
    # hung Starfish process. This is a second, independent safety net against
    # a Pool/OS-level hang (e.g. a worker that never gets scheduled).
    # It is deliberately kept *below* typical CI job timeouts (e.g. GitHub
    # Actions' 60-minute default) so that if it ever fires, the suite fails
    # fast with a clear diagnostic instead of being silently killed by the CI
    # runner with zero output -- which is exactly the bug this fixes. (The
    # previous flat 0xfff/4095s bound was *longer* than the 60-minute CI job
    # timeout, so it could structurally never fire in time -- and even if it
    # had, multiprocessing.TimeoutError wasn't caught here at all.)
    per_tc_timeout = resolve_tc_timeout()
    if per_tc_timeout is None:
        # Explicit opt-out (TC_TIMEOUT=0): keep the old, effectively-unbounded
        # wait for interactive/debug use.
        pool_get_timeout = 0xfff
    else:
        rounds = math.ceil(len(tcs) / nproc) if tcs else 1
        # 2x safety margin over the worst-case serial time this worker's
        # queue could take, plus fixed slack for process spawn/collection.
        pool_get_timeout = max(300, min(3300, int(rounds * per_tc_timeout * 2) + 300))

    p = multiprocessing.Pool(nproc, init_worker)
    try:
        itr = p.map_async(case_runner, tcs, chunksize=1).get(pool_get_timeout)

    except KeyboardInterrupt:
        print("Terminate (KeyboardInterrupt)")
        p.terminate()
        p.join()
        sys.exit(1)

    except multiprocessing.TimeoutError:
        print("ERROR : test pool did not finish within %ds (%d workers, "
              "%d test cases, %s sec/test-case) -- a worker is stuck well "
              "beyond its own per-test-case timeout (Pool/OS-level hang, "
              "not just a slow test). Terminating so the failure is visible "
              "instead of stalling until the outer CI job timeout." %
              (pool_get_timeout, nproc, len(tcs), per_tc_timeout))
        p.terminate()
        p.join()
        sys.exit(1)

    if out_path is not None:
        i = 0
        with open(out_path, "w+") as fp:
            for line in itr:
                fp.write(str(line) + " " + tcs[i][1] + "\n")
                i += 1

    return result_handler(tcs, itr)


def default_result_handler(tc_itr, result_itr):
    ntc = 0
    npass = 0
    for result in result_itr:
        ntc += 1
        npass += (1 if result is True else 0)
    return (npass, ntc - npass)
