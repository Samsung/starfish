#!/usr/bin/env python
import signal

def init_worker():
    signal.signal(signal.SIGINT, signal.SIG_IGN)


def run_test_pool(case_runner, in_path, out_path=None, result_handler=None):
    import sys
    import types
    import multiprocessing

    if type(result_handler) is not types.FunctionType:
        result_handler = default_result_handler

    tcs = []
    idx = 0
    # TODO : Consider too long tc list
    try:
        with open(in_path) as fp:
            for line in fp:
                content = line.strip()
                if (len(content) > 0) and content[0] != '#':
                    tcs.append((idx, content))
                    idx = idx + 1
    except IOError:
        print "No such file " + in_path
        sys.exit(1)

    # Note : http://xcodest.me/interrupt-the-python-multiprocessing-pool-in-graceful-way.html
    nproc = multiprocessing.cpu_count()
    p = multiprocessing.Pool(nproc, init_worker)
    try:
        itr = p.map_async(case_runner, tcs, chunksize=1).get(0xfff)

    except KeyboardInterrupt:
        print "Terminate (KeyboardInterrupt)"
        p.terminate()
        sys.exit(1)

    if out_path is not None:
        with open(out_path, 'a') as fp:
            for line in itr:
                fp.write(line + "\n")

    return result_handler(tcs, itr)


def default_result_handler(tc_itr, result_itr):
    ntc = 0
    npass = 0
    for result in result_itr:
        ntc += 1
        npass += (1 if result is True else 0)
    return (npass, ntc - npass)
