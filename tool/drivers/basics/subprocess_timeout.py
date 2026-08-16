#!/usr/bin/env python
"""Shared helper: run a test subprocess and enforce a hard wall-clock timeout.

Used by both starfish_basic_test.py and starfish_pixel_test.py's case_runner
so a single hung Starfish invocation (e.g. under host resource contention)
cannot block its multiprocessing.Pool worker -- and therefore the entire
parallel batch -- forever. See constants.resolve_tc_timeout() for how the
timeout value itself is chosen.
"""
import os
import signal
import subprocess
import time
from subprocess import Popen, PIPE


def run_subprocess_with_timeout(command, timeout=None, stdin=None):
    """Run `command`, waiting up to `timeout` seconds (None = wait forever,
    the pre-existing behavior).

    `stdin`, when not None, is sent as communicate() input (and a stdin pipe
    is opened for the child); this mirrors each call site's prior Popen
    setup exactly.

    On timeout, SIGKILLs the whole process group (Starfish can spawn a
    child tree) and raises TimeoutError.
    """
    start_time = time.time()

    # Own a process group only when a timeout is set: that is the only path
    # that may need to SIGKILL the whole tree the child spawns. Without a
    # timeout the call behaves exactly as a plain Popen (no session change).
    process = Popen(command, stdin=(PIPE if stdin is not None else None),
                    stdout=PIPE, stderr=PIPE, start_new_session=bool(timeout))
    try:
        stdout, stderr = process.communicate(stdin, timeout=timeout)
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
