#!/usr/bin/env python
import signal
import sys
import types
import multiprocessing

def init_worker():
    signal.signal(signal.SIGINT, signal.SIG_IGN)


def run_test_pool(case_runner, in_path, nproc,
                  out_path=None, result_handler=None):
    if type(result_handler) is not types.FunctionType:
        result_handler = default_result_handler

    tcs = []
    idx = 0
    # TODO : Consider too long tc list
    try:
        with open(in_path) as fp:
            for line in fp:
                line = line.strip()
                if len(line) == 0:
                    continue
                splited = line.split();
                content = splited[0]

                if (len(content) > 0) and content[0] != '#':
                    if len(splited) >= 2:
                        tcs.append((idx, [content, splited[1]]))
                    else:
                        tcs.append((idx, content))
                    idx = idx + 1
    except IOError:
        print "No such file " + in_path
        sys.exit(1)

    # Note : http://xcodest.me/interrupt-the-python-multiprocessing-pool-in-graceful-way.html
    max_nproc = multiprocessing.cpu_count();
    if type(nproc) is types.IntType:
        nproc = max(1, min(nproc, max_nproc))
    else:
        nproc = max_nproc
    print "Running " + str(nproc) + " jobs in parallel"

    p = multiprocessing.Pool(nproc, init_worker)
    try:
        itr = p.map_async(case_runner, tcs, chunksize=1).get(0xfff)

    except KeyboardInterrupt:
        print "Terminate (KeyboardInterrupt)"
        p.terminate()
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
