#!/usr/bin/env python
import signal
import sys
import types
import multiprocessing
import os
import time
from abc import ABC, abstractclassmethod
from basics.constants import ENVOPTS

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


def _run_indexed_case(args):
    case_runner, tc = args
    return tc[0], case_runner(tc)


def _collect_progress(pool, case_runner, tcs, interval, timeout=0xfff):
    results = [None] * len(tcs)
    completed = passed = failed = 0
    started = time.monotonic()
    deadline = started + timeout

    def report():
        elapsed = time.monotonic() - started
        percent = 100 * completed / len(tcs) if tcs else 100
        print(f"Progress: {completed}/{len(tcs)} ({percent:.1f}%) "
              f"pass={passed} fail={failed} elapsed={elapsed:.0f}s",
              flush=True)

    report()
    next_report = started + interval
    iterator = pool.imap_unordered(
        _run_indexed_case, ((case_runner, tc) for tc in tcs), chunksize=1)
    while completed < len(tcs):
        now = time.monotonic()
        if now >= deadline:
            raise multiprocessing.TimeoutError
        try:
            index, result = iterator.next(
                timeout=max(0, min(next_report, deadline) - now))
        except multiprocessing.TimeoutError:
            if time.monotonic() >= deadline:
                raise
        else:
            # Custom result handlers and result files expect input order.
            results[index] = result
            completed += 1
            passed += result is True
            failed += result is False
        if completed == len(tcs) or time.monotonic() >= next_report:
            report()
            next_report = time.monotonic() + interval
    return results


def run_test_pool(case_runner, in_path, nproc,
                  out_path=None, result_handler=None, progress_interval=0):
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

    with multiprocessing.Pool(nproc, init_worker) as p:
        try:
            if progress_interval > 0:
                itr = _collect_progress(p, case_runner, tcs, progress_interval)
            else:
                itr = p.map_async(case_runner, tcs, chunksize=1).get(0xfff)
        except KeyboardInterrupt:
            print("Terminate (KeyboardInterrupt)")
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
